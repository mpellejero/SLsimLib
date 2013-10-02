#include "../include/InputParams.h"

#include <set>

// array of all parameters
namespace {
	const char* parameter_list[][3] = {
		// General
		{"outputfile", "output", ""},
		{"deflection_off", "0", "switches deflection off (but not kappa and gamma, ie. Born approximation)"},
		{"lensing_off", "0", "switches all lensing off"},
		{"read_redshift_planes", "0", "reads in the redshifts of the lensing planes from a file"},
		{"redshift_planes_file", "Z.txt", "an asci file with redshifts of the planes, excluding the source redshift"},
		{"z_lens", "0.42", "lens redshift"},
		{"z_source", "3.62", "source redshift"},
		
		// Cosmology
		{"Omega_matter", "0.3", "Matter energy density fraction"},
		{"Omega_lambda", "0.7", "Lambda energy density fraction"},
		{"Omega_baryon", "0", "Baryon energy density fraction"},
		{"Omega_neutrino", "0", "Neutrino energy density fraction"},
		{"hubble", "0.73", "Hubble constant H/100"},
		{"sigma_8", "0.8", "Sigma_8 constant for large scale structure"},
		
		// Main halos type
		{"main_halo_on", "1", "insert a main halo into the simulation"},
		{"main_DM_halo_type", "DummyLens", "main halo type: nolens, NFW, PseudoNFW, PowerLaw, NSIE, AnaLens, UniLens, MOKALens, DummyLens, Hernquist, Jaffe, MultiDark"},
		{"main_galaxy_halo_type", "0", "if set, a main halo galaxy profile is chosen: 0 or none, 1 or NSIE"},
		
		// Field halos
		{"field_off", "0", "turn off field halos"},
		{"field_Nplanes", "8", "number of field halo planes"},
		
		// Field halos type
		{"field_internal_profile", "NFW", "field halo type: nolens, NFW, PseudoNFW, PowerLaw, NSIE, AnaLens, UniLens, DummyLens"},
		{"field_internal_profile_galaxy", "0", "if set, a field halo galaxy profile is chosen: 0 or none, 1 or NSIE"},
		{"field_prof_internal_slope_pnfw", "2", "slope of the PseudoNFW profile"},
		{"field_prof_internal_slope_pl", "-1", "slope of the field halo PowerLaw profile"},
		
		// Field halos from a mass function
		{"field_mass_func_type", "1", "mass function type, 0: Press-Schechter, 1: Sheth-Tormen, 2: PowerLaw"},
		{"field_mass_func_alpha", "", ""},
		{"field_fov", "1.0e4", "random light cone field of view in square arcseconds"},
		{"field_buffer", "1.0", "in physical Mpc"},
		{"field_min_mass", "1.0e9", "min mass of the halos in the light cone in solar masses"},
		
		// Field halos from an input file
		{"field_input_simulation_file", "halos.txt", "if set, the light cone is read from an input file"},
		
		// Main halos
		{"main_mass", "1e15", "main halo mass"},
		{"main_zlens", "0.42", "main halo redshift"},
		{"main_Rmax", "1.0", "main halo radius"},
		{"main_concentration", "5", "main halo concentration parameter"},
		{"main_slope", "1", "main halo slope"},
		{"main_sigma", "250", "velocity dispersion in km/s"},
		{"main_core", "1.0E-5", "core radius in Mpc"},
		{"main_axis_ratio", "0.8", "axis ratio for elliptical models"},
		{"main_pos_angle", "0", "inclination angle in degrees"},
		{"main_rscale", "", ""},
		
		// AnaNSIE perturbations
		{"main_NDistortionModes", "0", "number of ellipsoid distortion modes"},
		{"main_perturb_beta", "1.0", ""},
		{"main_perturb_kappa", "0.03", ""},
		{"main_perturb_gamma", "0.03", ""},
		{"main_perturb_monopole", "0.0", ""},
		{"main_perturb_quadrapole", "0.005", ""},
		{"main_perturb_hexopole", "0.005", ""},
		{"main_perturb_octopole", "0.01", ""},
		
		// AnaNSIE substructures
		{"main_sub_Ndensity", "0.0e6", "number density of substructure"},
		{"main_sub_beta", "-1.0", ""},
		{"main_sub_alpha", "-1.9", ""},
		{"main_sub_Rmax", "0.5e-3", ""},
		{"main_sub_mass_max", "1.0e9", ""},
		{"main_sub_mass_min", "1.0e6", ""},
		{"main_sub_type", "1", ""},
		
		// Stars
		{"main_stars_N", "0", "number of stars to be implanted"},
		{"main_stars_fraction", "0.5", "stellar mass fraction"},
		{"main_stars_mass", "0.5", "star mass in solar masses"},
		{"main_stars_mass_function", "", ""},
		{"main_stars_min_mass", "", ""},
		{"main_stars_max_mass", "", ""},
		{"main_stars_bending_point", "", ""},
		{"main_stars_lo_mass_slope", "", ""},
		{"main_stars_hi_mass_slope", "", ""},
		
		// MOKA lens halo model
		{"MOKA_input_file", "moka.fits", "MOKA FITS file"},
		{"MOKA_input_params", "1", "read parameters from MOKA FITS header"},
		{"MOKA_analyze", "0", ""},
		{"MOKA_background_field", "0", ""},
		
		// Uniform lens halo model
		{"kappa_uniform", "", ""},
		{"gamma_uniform_1", "", ""},
		{"gamma_uniform_2", "", ""},
		
		// 
		{"zsource_reference", "2.0", "reference redshift for halo quantities that depend on source z"},
		
		// MultiDark lenses
		{"MultiDark_input_file", "MultiDark.txt", "list of MOKA FITS files for MultiDark-like simulations"},
		
		// Type of source SB model
		{"SourceSBType", "Uniform", "Uniform, Gaussian, BLR_Disk, BLR_Sph1, BLR_Sph2"},
		
		// Gaussian source model
		{"gauss_r2", "0.0", ""},
		
		// BLR source model
		{"source_BHmass", "1.0e9", "BH mass of the quasar"},
		{"source_gamma", "-0.5", ""},
		{"source_inclin", "35.0", ""},
		{"source_opening_ang", "10", ""},
		{"source_r_in", "5.0e-9", ""},
		{"source_r_out", "5.0e-6", ""},
		{"source_nuo", "6.17284e14", ""},
		{"source_fK", "", ""},
		
		// SourceMultiAnaGalaxy & Sky
		{"source_input_galaxy_file", "sources.txt", "Millennium sources input file"},
		{"source_band", "", ""},
		{"source_mag_limit", "30", "minimum magnitude for sources"},
		{"source_sb_limit", "0", "minimum surface brightness for sources"}
	};
	
	// return a list of all labels from the parameter list
	std::set<std::string> get_labels()
	{
		std::set<std::string> labels;
		for(std::size_t i = 0; i < sizeof(parameter_list)/sizeof(const char*[3]); ++i)
			labels.insert(parameter_list[i][0]);
		return labels;
	}
}

/**
 * Check if a parameter label exists.
 */
bool InputParams::check_label(const std::string& name)
{
	static std::set<std::string> labels = get_labels();
	return (labels.find(name) != labels.end());
}

/**
 * \brief Sample input parameters.
 */
InputParams InputParams::sample()
{
	// load all entries from the parameter list
	InputParams params;
	for(std::size_t i = 0; i < sizeof(parameter_list)/sizeof(const char*[3]); ++i)
		params.put(parameter_list[i][0], parameter_list[i][1], parameter_list[i][2]);
	return params;
}
