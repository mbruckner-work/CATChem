

# Namespace gaschemscheme\_musica\_mod



[**Namespace List**](namespaces.md) **>** [**gaschemscheme\_musica\_mod**](namespacegaschemscheme__musica__mod.md)










































## Public Functions

| Type | Name |
| ---: | :--- |
|  impure subroutine, public | [**compute\_no\_phot**](#function-compute_no_phot) (integer, intent(in) num\_layers, integer, intent(in) num\_species, type([**gaschemschememusicaconfig**](namespacegaschemcommon__mod.md#none-gaschemschememusicaconfig)), intent(in) params, real(fp), dimension(num\_layers), intent(in) airden, real(fp), dimension(num\_layers), intent(in) delp, real(fp), dimension(num\_layers), intent(in) pmid, real(fp), dimension(num\_layers), intent(in) t, real(fp), dimension(num\_layers, num\_species), intent(in) species\_conc, real(fp), dimension(num\_layers, num\_species), intent(inout) species\_tendencies, real(fp), dimension(:,:), intent(inout), optional total\_rate\_per\_species\_per\_level, real(fp), dimension(:), intent(inout), optional net\_chemical\_rate\_per\_species, integer, dimension(:), intent(in), optional diagnostic\_species\_id) <br>_Pure science computation for no\_phot scheme._  |




























## Public Functions Documentation




### function compute\_no\_phot 

_Pure science computation for no\_phot scheme._ 
```Fortran
impure subroutine, public gaschemscheme_musica_mod::compute_no_phot (
    integer, intent(in) num_layers,
    integer, intent(in) num_species,
    type( gaschemschememusicaconfig ), intent(in) params,
    real(fp), dimension(num_layers), intent(in) airden,
    real(fp), dimension(num_layers), intent(in) delp,
    real(fp), dimension(num_layers), intent(in) pmid,
    real(fp), dimension(num_layers), intent(in) t,
    real(fp), dimension(num_layers, num_species), intent(in) species_conc,
    real(fp), dimension(num_layers, num_species), intent(inout) species_tendencies,
    real(fp), dimension(:,:), intent(inout), optional total_rate_per_species_per_level,
    real(fp), dimension(:), intent(inout), optional net_chemical_rate_per_species,
    integer, dimension(:), intent(in), optional diagnostic_species_id
) 
```



This is a pure computational kernel implementing MICM gas phase chemistry solver without photolysis. NO error checking, validation, or infrastructure concerns. Host model must ensure all inputs are valid before calling.




**Parameters:**


* `num_layers` Number of vertical layers 
* `num_species` Number of chemical species 
* `params` Scheme parameters (pre-validated by host) 
* `airden` AIRDEN field [appropriate units] 
* `delp` DELP field [appropriate units] 
* `pmid` PMID field [appropriate units] 
* `t` T field [appropriate units] 
* `species_conc` Species concentrations [ppm or ug/kg] (num\_layers, num\_species) 
* `species_tendencies` Species tendency terms [mol/mol/s] (num\_layers, num\_species) 
* `total_rate_per_species_per_level` Net chemical change per species per level [molecules/cm3/s] (num\_layers, num\_species) 
* `net_chemical_rate_per_species` Net chem rate [molecules/cm3/s] (num\_species) 
* `diagnostic_species_id` Indices mapping diagnostic species to species array (optional, for per-species diagnostics) 




        

<hr>

------------------------------
The documentation for this class was generated from the following file `src/process/GasChem/schemes/GasChemScheme_MUSICA_Mod.F90`

