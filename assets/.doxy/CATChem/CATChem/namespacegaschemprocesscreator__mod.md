

# Namespace gaschemprocesscreator\_mod



[**Namespace List**](namespaces.md) **>** [**gaschemprocesscreator\_mod**](namespacegaschemprocesscreator__mod.md)










































## Public Functions

| Type | Name |
| ---: | :--- |
|  subroutine, public | [**create\_gaschem\_process**](#function-create_gaschem_process) (class([**processinterface**](namespaceprocessinterface__mod.md#none-processinterface)), intent(out), allocatable process, integer, intent(out) rc) <br>_Create a new GasChem process instance._  |
|  subroutine, public | [**get\_gaschem\_default\_config**](#function-get_gaschem_default_config) (character(len=\*), intent(out) config\_data) <br>_Get default configuration for GasChem process._  |
|  subroutine, public | [**register\_gaschem\_process**](#function-register_gaschem_process) (type([**processmanagertype**](namespaceprocessmanager__mod.md#none-processmanagertype)), intent(inout) process\_mgr, integer, intent(out) rc) <br>_Register the GasChem process with a ProcessManager._  |




























## Public Functions Documentation




### function create\_gaschem\_process 

_Create a new GasChem process instance._ 
```Fortran
subroutine, public gaschemprocesscreator_mod::create_gaschem_process (
    class( processinterface ), intent(out), allocatable process,
    integer, intent(out) rc
) 
```



This factory function creates and returns a new instance of the GasChem process. The process is not initialized - the caller must call the init() method with appropriate configuration.




**Parameters:**


* `process` Allocated process instance 
* `rc` Return code 




        

<hr>



### function get\_gaschem\_default\_config 

_Get default configuration for GasChem process._ 
```Fortran
subroutine, public gaschemprocesscreator_mod::get_gaschem_default_config (
    character(len=*), intent(out) config_data
) 
```



This function returns a default configuration string that can be used to initialize the GasChem process with reasonable defaults.




**Parameters:**


* `config_data` Default configuration string 




        

<hr>



### function register\_gaschem\_process 

_Register the GasChem process with a ProcessManager._ 
```Fortran
subroutine, public gaschemprocesscreator_mod::register_gaschem_process (
    type( processmanagertype ), intent(inout) process_mgr,
    integer, intent(out) rc
) 
```



This subroutine registers the GasChem process with a ProcessManager's factory. This is the correct way to register processes for use in applications and integration tests.




**Parameters:**


* `process_mgr` The ProcessManager to register with 
* `rc` Return code 




        

<hr>

------------------------------
The documentation for this class was generated from the following file `src/process/GasChem/GasChemProcessCreator_Mod.F90`

