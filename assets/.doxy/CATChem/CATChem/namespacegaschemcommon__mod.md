

# Namespace gaschemcommon\_mod



[**Namespace List**](namespaces.md) **>** [**gaschemcommon\_mod**](namespacegaschemcommon__mod.md)










































## Public Functions

| Type | Name |
| ---: | :--- |
|  character(len=32) function, public | [**int\_to\_string**](#function-int_to_string) (integer, intent(in) int\_val) <br>_Convert integer to string (utility function)_  |
|  subroutine | [**validate\_gaschem\_config**](#function-validate_gaschem_config) (class([**gaschemconfig**](namespacegaschemcommon__mod.md#none-gaschemconfig)), intent(inout) this, type([**errormanagertype**](namespaceerror__mod.md#none-errormanagertype)), intent(inout) error\_handler) <br>_Validate GasChem configuration._  |




























## Public Functions Documentation




### function int\_to\_string 

_Convert integer to string (utility function)_ 
```Fortran
character(len=32) function, public gaschemcommon_mod::int_to_string (
    integer, intent(in) int_val
) 
```




<hr>



### function validate\_gaschem\_config 

_Validate GasChem configuration._ 
```Fortran
subroutine gaschemcommon_mod::validate_gaschem_config (
    class( gaschemconfig ), intent(inout) this,
    type( errormanagertype ), intent(inout) error_handler
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `src/process/GasChem/GasChemCommon_Mod.F90`

