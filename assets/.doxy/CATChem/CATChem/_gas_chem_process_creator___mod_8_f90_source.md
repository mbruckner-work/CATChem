

# File GasChemProcessCreator\_Mod.F90

[**File List**](files.md) **>** [**GasChem**](dir_2a332609ebf1b4c61e55d9d1fdc7835b.md) **>** [**GasChemProcessCreator\_Mod.F90**](_gas_chem_process_creator___mod_8_f90.md)

[Go to the documentation of this file](_gas_chem_process_creator___mod_8_f90.md)


```Fortran


module gaschemprocesscreator_mod

   use precision_mod, only: fp
   use error_mod, only: cc_success, cc_failure, cc_error, cc_warning, errormanagertype
   use processinterface_mod
   use processgascheminterface_mod

   implicit none
   private

   public :: create_gaschem_process
   public :: register_gaschem_process
   public :: get_gaschem_default_config

contains

   subroutine create_gaschem_process(process, rc)
      class(ProcessInterface), allocatable, intent(out) :: process
      integer, intent(out) :: rc

      type(ProcessGasChemInterface), allocatable :: GasChem_process
      integer :: alloc_stat

      rc = cc_success

      ! Allocate the process instance
      allocate(gaschem_process, stat=alloc_stat)
      if (alloc_stat /= 0) then
         rc = cc_failure
         return
      end if

      ! Move to polymorphic variable
      call move_alloc(gaschem_process, process)

   end subroutine create_gaschem_process

   subroutine register_gaschem_process(process_mgr, rc)
      use processmanager_mod, only: processmanagertype

      type(ProcessManagerType), intent(inout) :: process_mgr
      integer, intent(out) :: rc

      rc = cc_success

      call process_mgr%register_process( &
         name='GasChem', &
         category='chemistry', &
         description='Process for MICM gas phase chemical solver', &
         creator=create_gaschem_process, &
         rc=rc &
      )

   end subroutine register_gaschem_process

   subroutine get_gaschem_default_config(config_data)
      character(len=*), intent(out) :: config_data

      ! Return default YAML configuration
      config_data = &
         '# Default GasChem process configuration' // new_line('A') // &
         'process:' // new_line('A') // &
         '  name: "GasChem"' // new_line('A') // &
         '  version: "1.0.0"' // new_line('A') // &
         '  active_scheme: ""' // new_line('A') // &
         '  is_active: true' // new_line('A') // &
         '' // new_line('A') // &
         '# Scheme configuration' // new_line('A') // &
         'schemes:' // new_line('A') // &
         '  no_phot:' // new_line('A') // &
         '    description: "MICM gas phase chemistry solver without photolysis"' // new_line('A') // &
         '    algorithm_type: "explicit"' // new_line('A') // &
         '    parameters:' // new_line('A') // &
         '      scale_factor: 1.0' // new_line('A') // &
         '' // new_line('A') // &
         '# Diagnostic configuration' // new_line('A') // &
         'diagnostics:' // new_line('A') // &
         '  output_frequency: 3600.0  # seconds' // new_line('A') // &
         '  output_diagnostics: true'

   end subroutine get_gaschem_default_config

end module GasChemProcessCreator_Mod
```


