// src/process/gaschem/catchem_process_gaschem.cpp
#include "catchem_process_gaschem.hpp"
#include "catchem_constants.hpp"
#include "catchem_diagnostic_manager.hpp"
#include "catchem_logger.hpp"
#include "catchem_process_registry.hpp"
#include <Kokkos_Core.hpp>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>

namespace catchem {

    ProcessContract GasChemProcess::get_contract() const {
        return make_contract(get_name(),
                             {host_field_3d("T", "K"), host_field_3d("PMID", "Pa"), host_field_3d("QV","kg/kg"),
                              host_field_3d("AIRDEN_DRY", "kg/m3", FieldRequirement::Optional), host_concentration()});
    }

    GasChemProcess::GasChemProcess() = default;
    GasChemProcess::~GasChemProcess() = default;

    void GasChemProcess::init(std::shared_ptr<StateManager> state) {
        Logger::debug(*state, "GasChemProcess::init started");

        if (state->config_manager()) {
            std::string file = state->config_manager()->get_string("processes/gaschem/config_file", "");
            if (file.empty()) {
                file = state->config_manager()->get_string("process/gaschem/config_file", "");
            }
            if (!file.empty()) {
                this->config_file_ = file;
            }

            std::string dir = state->config_manager()->get_string("processes/gaschem/config_dir", "");
            if (dir.empty()) {
                dir = state->config_manager()->get_string("process/gaschem/config_dir", "");
            }
            if (!dir.empty()) {
                this->config_dir_ = dir;
            }
        }

        if (this->config_dir_.empty()) {
            if (!state->configuration_path().empty()) {
                std::string path = state->configuration_path();
                size_t last_slash = path.find_last_of("/\\");
                if (last_slash != std::string::npos) {
                    this->config_dir_ = path.substr(0, last_slash + 1);
                } else {
                    this->config_dir_ = "./";
                }
            } else {
                this->config_dir_ = "tests/Configs/Default/";
            }
        }

        if (this->config_file_.empty() && !this->config_dir_.empty()) {
            for (const char* candidate : {"config.yaml", "config.yml"}) {
                std::filesystem::path probe = std::filesystem::path(this->config_dir_) / candidate;
                if (std::filesystem::is_regular_file(probe)) {
                    this->config_file_ = probe.string();
                    break;
                }
            }
        }

        const std::string micm_config_path = this->config_file_.empty() ? this->config_dir_ : this->config_file_;
        Logger::info(
            *state, "GasChemProcess: resolved MICM configuration",
            {{"path", micm_config_path}, {"type", this->config_file_.empty() ? "directory (v0 CAMP)" : "file"}});

        try {
            rosenbrock_params_.relative_tolerance = 0.5e-2;
            rosenbrock_params_.h_min = 0;
            rosenbrock_params_.h_max = 60;
            rosenbrock_params_.h_start = 1.0e-5;
            rosenbrock_params_.max_number_of_steps = 10000;

            micm_instance_ = std::make_unique<musica::MICM>(
                micm_config_path, musica::RosenbrockStandardOrder, rosenbrock_params_);

            const int num_threads = Kokkos::DefaultHostExecutionSpace().concurrency();
            thread_states_.resize(num_threads);

            const size_t total_cells = static_cast<size_t>(state->column_count()) * static_cast<size_t>(state->level_count());
            cells_per_thread_ = (total_cells + num_threads - 1) / num_threads;

            const size_t v_size = micm_instance_->GetVectorSize();
            if (v_size > 1 && (cells_per_thread_ % v_size != 0)) {
                cells_per_thread_ = ((cells_per_thread_ + v_size - 1) / v_size) * v_size;
            }

            for (int t = 0; t < num_threads; ++t) {
                thread_states_[t] = std::make_unique<musica::State>(*micm_instance_, cells_per_thread_);
            }

            initialized_ = true;

            std::clog << "[INFO] GasChemProcess: initialized MICM for " << num_threads 
                      << " OpenMP threads (" << cells_per_thread_ << " cells/thread, total: " 
                      << total_cells << ")" << std::endl;

            auto variable_map = thread_states_[0]->GetVariableMap();
            for (int ispec = 0; ispec < state->species_count(); ++ispec) {
                std::string name = state->chemistry().species_list[ispec].short_name;
                std::string lowercase_name = name;
                std::transform(lowercase_name.begin(), lowercase_name.end(), lowercase_name.begin(), [](unsigned char c) { 
                    return std::tolower(c);
                });

                bool found_in_micm = (variable_map.find(lowercase_name) != variable_map.end()) || 
                                     (variable_map.find(name) != variable_map.end());

                if (!found_in_micm) {
                    Logger::warn(*state, "Active species not found in MICM solver variable map", {{"species", name}});
                }
            }
        } catch (const std::exception& e) {
            Logger::error(*state, "GasChemProcess: failed to initialize MICM", {{"error", e.what()}});
            initialized_ = false;
            throw std::runtime_error(std::string("GasChemProcess: failed to initialize MICM: ") + e.what());
        }
    }

    void GasChemProcess::run(std::shared_ptr<StateManager> state) {
        if (!initialized_) {
            Logger::warn(*state, "GasChemProcess: skipped run because solver is not initialized");
            return;
        }

        if (state->meteorology().PMID && state->meteorology().T) {
            state->derive_airden_dry();
        }

        const auto import_generation = state->current_import_generation();
        if (!state->meteorology().T || !state->meteorology().PMID || !state->meteorology().AIRDEN_DRY || !state->meteorology().QV ||
            !state->meteorology().T->is_current(import_generation) ||
            !state->meteorology().PMID->is_current(import_generation) ||
            !state->meteorology().AIRDEN_DRY->is_current(import_generation) || 
            !state->meteorology().QV->is_current(import_generation) || !state->chemistry().conc) {
            throw std::runtime_error(
                "GasChem requires current T, PMID, AIRDEN_DRY, QV, and chemistry concentration fields");
        }

        auto temp = state->meteorology().T->host_view;
        auto pmid = state->meteorology().PMID->host_view;
        auto airden_dry = state->meteorology().AIRDEN_DRY->host_view;
        auto qv = state->meteorology().QV->host_view;
        state->chemistry().conc->host_write();
        auto conc = state->chemistry().conc->host_view;

        const int nc = state->column_count();
        const int nl = state->level_count();
        const int ns = state->species_count();
        const size_t total_cells = static_cast<size_t>(nc) * static_cast<size_t>(nl);

        const double tstep = state->clock().timestep;
        if (tstep <= 0.0) {
            throw std::runtime_error("GasChemProcess: timestep must be greater than zero.");
        }

        constexpr double air_mw_kg = catchem::constants::AIR_MW * 1.0e-3;

        // Retrieve maps and sizes from thread_states_[0]
        const auto variable_map = thread_states_[0]->GetVariableMap();
        const auto rate_param_map = thread_states_[0]->GetRateParameterMap();
        const size_t n_micm_species = variable_map.size();
        const size_t n_rate_params = rate_param_map.size();
        const size_t vector_size_ = micm_instance_->GetVectorSize();

        enum SpecialSpecies : int { None = 0, O2 = 1, N2 = 2, H2O = 3 };
        struct SpeciesMapEntry {
            size_t micm_index;
            int catchem_index;
            int special;
        };

        std::vector<SpeciesMapEntry> h_mapped_species;
        h_mapped_species.reserve(ns);

        for (int ispec = 0; ispec < ns; ++ispec) {
            std::string name = state->chemistry().species_list[ispec].short_name;
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
                return std::tolower(c);
            });

            auto it = variable_map.find(name);
            if (it != variable_map.end()) {
                int spec = SpecialSpecies::None;
                if (name == "o2") spec = SpecialSpecies::O2;
                else if (name == "n2") spec = SpecialSpecies::N2;
                else if (name == "h2o") spec = SpecialSpecies::H2O;

                h_mapped_species.push_back({it->second, ispec, spec});
            }
        }

        const size_t num_mapped_species = h_mapped_species.size();
        Kokkos::View<SpeciesMapEntry*, Kokkos::HostSpace> mapped_species("mapped_species", num_mapped_species);
        for (size_t i = 0; i < num_mapped_species; ++i) {
            mapped_species(i) = h_mapped_species[i];
        }

        struct RateParamEntry {
            size_t micm_index;
            double* diag_ptr;
            bool is_loss;
        };

        std::vector<RateParamEntry> h_mapped_rates;
        h_mapped_rates.reserve(rate_param_map.size());

        for (const auto& [param_name, i_param] : rate_param_map) {
            if (param_name.rfind("PHOTO.", 0) == 0) {
                std::string label = param_name.substr(6);
                std::string diag_name = "photolysis_rate_" + label;
                double* ptr = nullptr;
                if (state->diagnostic_manager() && state->diagnostic_manager()->has_field(diag_name)) {
                    ptr = static_cast<double*>(state->diagnostic_manager()->get_host_pointer(diag_name));
                }
                h_mapped_rates.push_back({i_param, ptr, false});
            } else if (param_name.rfind("LOSS.", 0) == 0) {
                h_mapped_rates.push_back({i_param, nullptr, true});
            }
        }

        const size_t num_mapped_rates = h_mapped_rates.size();
        Kokkos::View<RateParamEntry*, Kokkos::HostSpace> mapped_rates("mapped_rates", num_mapped_rates);
        for (size_t i = 0; i < num_mapped_rates; ++i) {
            mapped_rates(i) = h_mapped_rates[i];
        }

        const int num_threads = Kokkos::DefaultHostExecutionSpace().concurrency();

        // Multi-threaded cell integration
        Kokkos::parallel_for("GasChem_ThreadParallelSolve",
            Kokkos::RangePolicy<Kokkos::DefaultHostExecutionSpace>(0, num_threads),
            KOKKOS_LAMBDA(const int thread_id) {
                const size_t start_cell = static_cast<size_t>(thread_id) * cells_per_thread_;
                const size_t end_cell = (start_cell + cells_per_thread_ < total_cells) ? 
                                         start_cell + cells_per_thread_ : total_cells;

                if (start_cell >= end_cell) return;
                const size_t n_cells_local = end_cell - start_cell;

                auto* local_state = thread_states_[thread_id].get();
                auto& local_conditions = local_state->GetConditions();
                auto& local_concs = local_state->GetOrderedConcentrations();
                auto& local_rate_params = local_state->GetOrderedRateParameters();

                auto* conditions_data = local_conditions.data();
                auto* concs_data = local_concs.data();
                auto* rate_params_data = local_rate_params.data();

                // 1. Pack cell inputs
                for (size_t i_local = 0; i_local < n_cells_local; ++i_local) {
                    const size_t global_cell = start_cell + i_local;
                    const int ilev = static_cast<int>(global_cell / nc);
                    const int icol = static_cast<int>(global_cell % nc);

                    const double t_val = temp(icol, ilev, 0);
                    const double p_val = pmid(icol, ilev, 0);
                    const double density_dry_kg = airden_dry(icol, ilev, 0);
                    const double qv_val = qv(icol, ilev, 0);

                    const double air_density_mol = density_dry_kg / air_mw_kg;
                    conditions_data[i_local].temperature_ = t_val;
                    conditions_data[i_local].pressure_ = p_val;
                    conditions_data[i_local].air_density_ = air_density_mol;

                    const size_t group_index = i_local / vector_size_;
                    const size_t row_in_group = i_local % vector_size_;

                    for (size_t s = 0; s < num_mapped_species; ++s) {
                        const auto& entry = mapped_species(s);
                        const double raw_ppmv = conc(icol, ilev, entry.catchem_index);
                        const double ppmv_val = (raw_ppmv > 0.0) ? raw_ppmv : 0.0;

                        double conc_molar = ppmv_val * 1.0e-6 * air_density_mol;
                        if (entry.special == SpecialSpecies::O2) {
                            conc_molar = 0.20946 * air_density_mol;
                        } else if (entry.special == SpecialSpecies::N2) {
                            conc_molar = 0.78084 * air_density_mol;
                        } else if (entry.special == SpecialSpecies::H2O) {
                            conc_molar = (qv_val * density_dry_kg) / 0.0180153;
                        }

                        const size_t idx = (group_index * n_micm_species + entry.micm_index) * vector_size_ + row_in_group;
                        concs_data[idx] = conc_molar;
                    }

                    for (size_t r = 0; r < num_mapped_rates; ++r) {
                        const auto& r_entry = mapped_rates(r);
                        const size_t idx = (group_index * n_rate_params + r_entry.micm_index) * vector_size_ + row_in_group;
                        if (r_entry.is_loss) {
                            rate_params_data[idx] = 1.0;
                        } else {
                            double rate_val = 0.0;
                            if (r_entry.diag_ptr) {
                                rate_val = r_entry.diag_ptr[global_cell];
                            }
                            rate_params_data[idx] = rate_val;
                        }
                    }
                }

                // 2. Solve ODEs across threads
                auto solver_result = micm_instance_->Solve(local_state, tstep);
                if (solver_result.state_ != micm::SolverState::Converged &&
                    solver_result.state_ != micm::SolverState::AcceptingUnconvergedIntegration) {
                    Kokkos::abort("GasChemProcess: MICM Solver failed to reach convergence");
                }

                // 3. Unpack output concentrations
                for (size_t i_local = 0; i_local < n_cells_local; ++i_local) {
                    const size_t global_cell = start_cell + i_local;
                    const int ilev = static_cast<int>(global_cell / nc);
                    const int icol = static_cast<int>(global_cell % nc);

                    const double air_density_mol = conditions_data[i_local].air_density_;
                    const size_t group_index = i_local / vector_size_;
                    const size_t row_in_group = i_local % vector_size_;

                    for (size_t s = 0; s < num_mapped_species; ++s) {
                        const auto& entry = mapped_species(s);
                        const size_t idx = (group_index * n_micm_species + entry.micm_index) * vector_size_ + row_in_group;
                        const double conc_molar = concs_data[idx];
                        const double raw_output = (conc_molar / air_density_mol) * 1.0e6;

                        conc(icol, ilev, entry.catchem_index) = (raw_output > 0.0) ? raw_output : 0.0;
                    }
                }
            });
        Kokkos::fence();

        if (state->chemistry().conc) {
            state->chemistry().conc->mark_host_modified();
        }
    }

    void GasChemProcess::finalize() {}

} // namespace catchem

extern "C" {
void catchem_register_gaschem_cpp() {
    catchem::ProcessRegistry::get_instance().register_process(
        std::string(catchem::ProcessNames::GasChem), []() { return std::make_shared<catchem::GasChemProcess>(); });
}
}