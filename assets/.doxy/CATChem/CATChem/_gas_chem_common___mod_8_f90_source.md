

# File GasChemCommon\_Mod.F90

[**File List**](files.md) **>** [**GasChem**](dir_2a332609ebf1b4c61e55d9d1fdc7835b.md) **>** [**GasChemCommon\_Mod.F90**](_gas_chem_common___mod_8_f90.md)

[Go to the documentation of this file](_gas_chem_common___mod_8_f90.md)


```Fortran


module gaschemcommon_mod

   use precision_mod, only: fp
   ! use precision_mod, only: fp
   use error_mod, only: cc_success, cc_failure, cc_error, cc_warning, errormanagertype, &
                        error_invalid_config, error_invalid_state, error_not_found
   use configmanager_mod, only: configmanagertype  ! ConfigManager integration
   use statemanager_mod, only: statemanagertype  ! Add StateManager integration

   implicit none
   private

   ! Export types
   public :: gaschemprocessconfig  ! New unified process config
   public :: gaschemconfig
   public :: gaschemschememusicaconfig

   ! Export utility functions
   public :: int_to_string

   type :: gaschemconfig

      ! Process settings
      character(len=32) :: scheme = 'no_phot'
      logical :: is_active = .true.
      logical :: diagnostics = .false.  ! Diagnostic switch

      ! Diagnostic species configuration
      integer :: n_diagnostic_species = 0
      character(len=32), allocatable :: diagnostic_species(:)  ! User-defined species for diagnostics
      integer, allocatable :: diagnostic_species_id(:)  ! Indices mapping diagnostic_species to species_names
      real(fp) :: dt_min = 1.0_fp     ! Minimum time step (seconds)
      real(fp) :: dt_max = 3600.0_fp  ! Maximum time step (seconds)

      ! Species configuration
      integer :: n_species = 0
      character(len=32), allocatable :: species_names(:)
      integer, allocatable :: species_indices(:)  ! Indices of GasChem species in ChemState




      ! Diagnostic configuration
      logical :: output_diagnostics = .true.
      real(fp) :: diagnostic_frequency = 3600.0_fp  ! Output frequency (seconds)

   contains
      procedure, public :: validate => validate_gaschem_config
      procedure, public :: finalize => finalize_gaschem_config
      procedure, public :: print_summary => print_gaschem_config_summary
   end type gaschemconfig

   type :: gaschemschememusicaconfig

      ! Scheme metadata
      character(len=64) :: scheme_name = 'no_phot'
      character(len=256) :: description = 'MICM gas phase chemistry solver without photolysis'
      character(len=64) :: author = 'Maggie Bruckner'
      character(len=16) :: algorithm_type = 'explicit'

      ! Process configuration
      logical :: affects_full_column = .true.  ! Full column processing

      ! Scheme parameters
      character(len=256) :: mechanism = "mechanism_config.yaml"  ! MICM yaml file for chemical mechanism

      ! Required meteorological fields
      integer :: n_required_met_fields = 4
      character(len=32) :: required_met_fields(4)

   contains
      procedure, public :: validate => validate_no_phot_config
      procedure, public :: finalize => finalize_no_phot_config
   end type gaschemschememusicaconfig

   ! no_phot scheme uses local variables only - no persistent state type needed


   type :: gaschemprocessconfig

      ! Process metadata
      character(len=64) :: process_name = 'GasChem'
      character(len=16) :: process_version = '1.0.0'
      logical :: is_active = .true.

      ! Process-specific configuration (delegate to GasChemConfig)
      type(GasChemConfig) :: GasChem_config

      ! Scheme configurations
      type(GasChemSchemeMUSICAConfig) :: no_phot_config


   contains
      procedure, public :: load_from_config => gaschem_process_load_config
      procedure, public :: load_species_from_chem_state => load_species_from_chem_state
      procedure, public :: validate => gaschem_process_validate
      procedure, public :: finalize => gaschem_process_finalize
      procedure, public :: get_active_scheme_config => get_active_scheme_config
      procedure, public :: load_no_phot_config
      procedure, public :: map_diagnostic_species_indices
   end type gaschemprocessconfig

contains

   subroutine validate_gaschem_config(this, error_handler)
      class(GasChemConfig), intent(inout) :: this
      type(ErrorManagerType), intent(inout) :: error_handler

      character(len=256) :: error_msg
      integer :: rc

      ! Validate time step bounds
      if (this%dt_min <= 0.0_fp) then
         call error_handler%report_error(error_invalid_config, &
            "Minimum time step must be positive", rc)
         return
      end if

      if (this%dt_max < this%dt_min) then
         call error_handler%report_error(error_invalid_config, &
            "Maximum time step must be >= minimum time step", rc)
         return
      end if

      ! Validate active scheme(s)
      ! Validate scheme
      if (trim(this%scheme) /= 'no_phot' .and. &
          .true.) then
         write(error_msg, '(A)') "Invalid scheme: " // trim(this%scheme)
         call error_handler%report_error(error_invalid_config, error_msg, rc)
         return
      end if

   end subroutine validate_gaschem_config

   subroutine print_gaschem_config_summary(this)
      class(GasChemConfig), intent(in) :: this

      write(*, '(A)') "=== GasChem Process Configuration ==="
      write(*, '(A,A)') "  Active scheme: ", trim(this%scheme)
      write(*, '(A,I0)') "  Number of species: ", this%n_species
      write(*, '(A,F0.1,A)') "  Minimum time step: ", this%dt_min, " s"
      write(*, '(A,F0.1,A)') "  Maximum time step: ", this%dt_max, " s"
      write(*, '(A,L1)') "  Output diagnostics: ", this%output_diagnostics
      write(*, '(A)') "============================================="

   end subroutine print_gaschem_config_summary

   subroutine finalize_gaschem_config(this)
      class(GasChemConfig), intent(inout) :: this

      ! Deallocate species names array
      if (allocated(this%species_names)) then
         deallocate(this%species_names)
      end if

      ! Deallocate species indices array
      if (allocated(this%species_indices)) then
         deallocate(this%species_indices)
      end if



      ! Deallocate diagnostic species array
      if (allocated(this%diagnostic_species)) then
         deallocate(this%diagnostic_species)
      end if

      ! Deallocate diagnostic species indices array
      if (allocated(this%diagnostic_species_id)) then
         deallocate(this%diagnostic_species_id)
      end if

   end subroutine finalize_gaschem_config

   subroutine validate_no_phot_config(this, error_handler)
      class(GasChemSchemeMUSICAConfig), intent(inout) :: this
      type(ErrorManagerType), intent(inout) :: error_handler

      ! TODO: Add scheme-specific validation

   end subroutine validate_no_phot_config

   subroutine finalize_no_phot_config(this)
      class(GasChemSchemeMUSICAConfig), intent(inout) :: this

      ! Nothing to deallocate for basic configuration

   end subroutine finalize_no_phot_config



   function int_to_string(int_val) result(str_val)
      integer, intent(in) :: int_val
      character(len=32) :: str_val

      write(str_val, '(I0)') int_val
      str_val = adjustl(str_val)

   end function int_to_string

   subroutine gaschem_process_load_config(this, config_manager, error_handler)
      class(GasChemProcessConfig), intent(inout) :: this
      type(ConfigManagerType), intent(inout) :: config_manager
      type(ErrorManagerType), intent(inout) :: error_handler

      character(len=256) :: scheme_name
      integer :: ierr, rc

      ! Process reads directly from master YAML structure: processes.GasChem
      ! ConfigManager provides generic YAML access, process handles its own configuration

      ! Load process metadata
      call config_manager%get_string("processes/GasChem/name", this%process_name, rc, "GasChem")
      if (rc /= cc_success) this%process_name = "GasChem"  ! default

      call config_manager%get_string("processes/GasChem/version", this%process_version, rc, "1.0.0")
      if (rc /= cc_success) this%process_version = "1.0.0"  ! default

      call config_manager%get_logical("processes/GasChem/activate", this%is_active, rc, .true.)
      if (rc /= cc_success) this%is_active = .true.  ! default

      ! Load process-specific configuration directly from master YAML
      call config_manager%get_string("processes/GasChem/scheme", this%GasChem_config%scheme, rc, "no_phot")
      if (rc /= cc_success) then
         call error_handler%report_error(error_invalid_config, &
            "Missing required 'scheme' in processes/GasChem configuration", rc)
         return
      end if

      ! Load diagnostic switch
      call config_manager%get_logical("processes/GasChem/diagnostics", this%GasChem_config%diagnostics, rc, .false.)
      if (rc /= cc_success) this%GasChem_config%diagnostics = .false.  ! Default

      ! Load diagnostic species list
      call config_manager%get_array("processes/GasChem/diag_species", this%GasChem_config%diagnostic_species, &
                                    rc, default_values=["All"])
      if (rc /= cc_success) then
         ! Default to all species if not specified
         allocate(this%GasChem_config%diagnostic_species(1))
         this%GasChem_config%diagnostic_species(1) = "All"
         this%GasChem_config%n_diagnostic_species = 1
      else
         ! Set the count based on the returned array size
         if (allocated(this%GasChem_config%diagnostic_species)) then
            this%GasChem_config%n_diagnostic_species = size(this%GasChem_config%diagnostic_species)
         else
            this%GasChem_config%n_diagnostic_species = 0
         end if
      end if

      ! Species configuration is loaded from ChemState in load_species_from_chem_state
      ! The species come from the master species YAML file (CATChem_species.yml)
      ! and are filtered by is_GasChem property


      ! Load scheme-specific configuration from master YAML
      scheme_name = trim(this%GasChem_config%scheme)
      select case (scheme_name)
      case ('no_phot')
         call this%load_no_phot_config(config_manager, error_handler)
      case default
         call error_handler%report_error(error_invalid_state, &
            "Unknown GasChem scheme: " // trim(scheme_name), rc)
         return
      end select

   end subroutine gaschem_process_load_config


   subroutine load_species_from_chem_state(this, chem_state, error_handler)
      use chemstate_mod, only: chemstatetype

      class(GasChemProcessConfig), intent(inout) :: this
      type(ChemStateType), pointer, intent(in) :: chem_state
      type(ErrorManagerType), intent(inout) :: error_handler

      integer :: i, rc

      if (.not. associated(chem_state)) then
         call error_handler%report_error(error_invalid_state, &
            "ChemState not associated in load_species_from_chem_state", rc)
         return
      end if

      ! by_metadata mode: Load species by type from ChemState using dynamic metadata flag mapping
      ! Dynamic mapping: is_gas -> nSpeciesGas and GasIndex
      this%GasChem_config%n_species = chem_state%nSpeciesGas

      if (this%GasChem_config%n_species <= 0) then
         call error_handler%report_error(error_invalid_state, &
            "No gas species found in ChemState", rc)
         return
      end if

      ! Check if GasIndex is allocated and has correct size
      if (.not. allocated(chem_state%GasIndex)) then
         call error_handler%report_error(error_invalid_state, &
            "GasIndex not allocated in ChemState", rc)
         return
      end if

      if (size(chem_state%GasIndex) < this%GasChem_config%n_species) then
         call error_handler%report_error(error_invalid_state, &
            "GasIndex size inconsistent with nSpeciesGas", rc)
         return
      end if

      ! Deallocate existing arrays if allocated
      if (allocated(this%GasChem_config%species_names)) then
         deallocate(this%GasChem_config%species_names)
      end if
      if (allocated(this%GasChem_config%species_indices)) then
         deallocate(this%GasChem_config%species_indices)
      end if

      ! Allocate arrays
      allocate(this%GasChem_config%species_names(this%GasChem_config%n_species))
      allocate(this%GasChem_config%species_indices(this%GasChem_config%n_species))

      ! by_metadata mode: Copy indices from metadata-specific index array using dynamic mapping
      ! Dynamic mapping: is_gas -> GasIndex
      this%GasChem_config%species_indices(1:this%GasChem_config%n_species) = &
         chem_state%GasIndex(1:this%GasChem_config%n_species)
      write(*,'(A)') "Grabbing species: "
      ! Get species names using the indices
      do i = 1, this%GasChem_config%n_species
         if (this%GasChem_config%species_indices(i) > 0 .and. &
             this%GasChem_config%species_indices(i) <= size(chem_state%SpeciesNames)) then
            this%GasChem_config%species_names(i) = &
               trim(chem_state%SpeciesNames(this%GasChem_config%species_indices(i)))
            write(*,'(A)') "Loading: ", this%GasChem_config%species_names(i)
         else
            call error_handler%report_error(error_invalid_state, &
               "Invalid species index in species index array", rc)
            return
         end if
      end do


   end subroutine load_species_from_chem_state


   subroutine load_no_phot_config(this, config_manager, error_handler)
      class(GasChemProcessConfig), intent(inout) :: this
      type(ConfigManagerType), intent(inout) :: config_manager
      type(ErrorManagerType), intent(inout) :: error_handler

      integer :: ierr, rc

      ! Load scheme parameters directly from processes/GasChem/no_phot/ in master YAML
      call config_manager%get_string("processes/GasChem/no_phot/mechanism", &
           this%no_phot_config%mechanism, rc, "mechanism_config.yaml")
      if (rc /= cc_success) this%no_phot_config%mechanism = "mechanism_config.yaml"


   end subroutine load_no_phot_config


   subroutine gaschem_process_validate(this, state_manager, error_handler)
      class(GasChemProcessConfig), intent(inout) :: this
      type(StateManagerType), intent(in) :: state_manager
      type(ErrorManagerType), intent(inout) :: error_handler

      ! Validate main config
      call this%GasChem_config%validate(error_handler)

      ! Validate scheme-specific config
      select case (trim(this%GasChem_config%scheme))
      case ('no_phot')
         call this%no_phot_config%validate(error_handler)
      end select

   end subroutine gaschem_process_validate

   subroutine gaschem_process_finalize(this)
      class(GasChemProcessConfig), intent(inout) :: this


      call this%GasChem_config%finalize()
      call this%no_phot_config%finalize()

   end subroutine gaschem_process_finalize


   function get_active_scheme_config(this) result(scheme_config)
      class(GasChemProcessConfig), intent(in) :: this
      class(*), allocatable :: scheme_config

      select case (trim(this%GasChem_config%scheme))
      case ('no_phot')
         allocate(scheme_config, source=this%no_phot_config)
      case default
         ! Return null
      end select

   end function get_active_scheme_config

   subroutine map_diagnostic_species_indices(this, error_handler)
      class(GasChemProcessConfig), intent(inout) :: this
      type(ErrorManagerType), intent(inout) :: error_handler

      integer :: i, j, rc
      character(len=256) :: error_msg
      logical :: found_species

      ! Only proceed if diagnostic species are defined
      if (this%GasChem_config%n_diagnostic_species == 0) return

      ! Handle "All" case - map all available species
      if (this%GasChem_config%n_diagnostic_species == 1 .and. &
          trim(this%GasChem_config%diagnostic_species(1)) == "All") then

         ! Deallocate and reallocate for all species
         if (allocated(this%GasChem_config%diagnostic_species_id)) deallocate(this%GasChem_config%diagnostic_species_id)
         allocate(this%GasChem_config%diagnostic_species_id(this%GasChem_config%n_species))
         if (allocated(this%GasChem_config%diagnostic_species)) deallocate(this%GasChem_config%diagnostic_species)
         allocate(this%GasChem_config%diagnostic_species(this%GasChem_config%n_species))
         this%GasChem_config%n_diagnostic_species = this%GasChem_config%n_species
         this%GasChem_config%diagnostic_species = this%GasChem_config%species_names

         ! Map all species indices (1:n_species)
         do i = 1, this%GasChem_config%n_species
            this%GasChem_config%diagnostic_species_id(i) = i
         end do

         return
      end if

      ! Allocate diagnostic species indices array
      if (allocated(this%GasChem_config%diagnostic_species_id)) deallocate(this%GasChem_config%diagnostic_species_id)
      allocate(this%GasChem_config%diagnostic_species_id(this%GasChem_config%n_diagnostic_species))

      ! Map each diagnostic species name to its index in species_names
      do i = 1, this%GasChem_config%n_diagnostic_species
         found_species = .false.

         do j = 1, this%GasChem_config%n_species
            if (trim(this%GasChem_config%diagnostic_species(i)) == trim(this%GasChem_config%species_names(j))) then
               this%GasChem_config%diagnostic_species_id(i) = j
               found_species = .true.
               exit
            end if
         end do

         if (.not. found_species) then
            write(error_msg, '(A,A,A)') "Diagnostic species '", &
                  trim(this%GasChem_config%diagnostic_species(i)), &
                  "' not found in process species list"
            call error_handler%report_error(error_not_found, error_msg, rc)
            !return !do not return and the diagnostics for this unspecified species will be zero in the output
         end if
      end do

   end subroutine map_diagnostic_species_indices

end module gaschemcommon_mod
```


