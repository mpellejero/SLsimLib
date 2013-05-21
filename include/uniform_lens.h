/*
 * uniform_lens.h
 *
 *  Created on: Jan 21, 2013
 *      Author: D. Leier
 */

#ifndef UNIFORM_LENS_H_
#define UNIFORM_LENS_H_

#include "base_analens.h"

/**
 * \brief An uniform model to represent a lens on a single plane.
 *
 * The lens consists of a "host" lens which is a non-singular isothermal ellipsoid (NSIE) plus axial distortion
 * modes, substructures and stars.
 *
 *<pre>
 * Input Parameters:
 *
 *  ****  Uniform model parameters
 *	kappa_uniform
 *  gamma_uniform_1
 *  gamma_uniform_2
 *
 *  **** Distortion parameters
 *	NDistortionModes       Number of distortion modes to be used.  If zero the other distortion parameters are not needed.
 *	beta_perturb
 *	kappa_peturb
 *	gamma_peturb
 *	monopole_peturb
 *	quadrapole_peturb
 *	hexopole_peturb
 *	octopole_peturb
 *
 *  **** Substructure parameters
 *	NdensitySubstruct      Number density of substructures.  They are distributed uniformly.  If zero the other substructure parameters are not needed.
 *	beta_sub               Logorithmic slope of the internal clump profile.  Used if sub_type == powerlaw
 *	alpha_sub              Logorithmic slope of the mass function.
 *	R_submax               Maximum radius of most massive substructure (see Metcalf & Amara 2012)
 *	mass_max               Maximum mass
 *	mass_min               Minimum mass
 *	sub_type               Mass profile of clumps - 0 or nfw,1 or powerlaw, 2 or pointmass
 *
 *  **** Stars parameters
 *	Nstars                 Total number of stars that will be used in the simulation.  If zero the other star parameters are not needed.
 *	fstars                 Fraction of surface denity in stars.
 *	stars_mass             Mass of stars.
 *
 *	stellar_mass_function  Choose between: One, Mono, Salpeter, BrokenPowerLaw, SinglePowerLaw, Chabrier.
 *						   One - all stellar masses set to 1 Msol - default value in case one does not give the IMFtype option in function implant_stars()
 *						   Mono - all stellar masses set to the same value - given in min_mstar and max_mstar, Note min_mstar and max_mstar must be equal.
 *						   Salpeter - Salpeter power law IMF with slope -2.35. Note: min_mstar and max_mstar must be defined and different from each other.
 * 						   BrokenPowerLaw - IMF for which two slopes (the low mass slope 'slope_1' and the high mass slope 'slope_2') can be defined as well as a bending point (bending_point).
 *                         SinglePowerLaw - Like Salpeter but with slope_1=slope_2 as a free parameter.
 * 						   Chabrier - Chabrier IMF with exponential cut-off toward low mass, bending_point is fixed to 1.0 Msol, Slope of powerlaw above 1 Msol is fixed to -2.3. Note that the three Chabrier Parameters are defined in implant_stars.cpp and (for now) fixed to [0.086,0.22,0.57].
 *	min_mstar          	   Lower mass limit of above IMFs (not used for IMFs One and Mono)
 *	max_mstar			   Upper mass limit of above IMFs (not used for IMFs One and Mono)
 *  bending_point      	   Bending mass for BrokenPowerLaw
 *  slope_1            	   index of the powerlaw (this is the index of the low mass end below bending_point if BrokenPowerLaw is selected)
 *  slope_2			   	   index of the powerlaw above bending_point only used for BrokenPowerLaw. Must be equal to slope_1 for SingelPowerLaw.
 *
 *  The stars are not initially present.  They must be implanted later.
 *</pre>
 *
 */

class UniNSIELensHalo: public BaseNSIELensHalo{
public:
	UniNSIELensHalo(InputParams& params);
	~UniNSIELensHalo();

	void assignParams(InputParams& params);
	void PrintLens(bool show_substruct,bool show_stars);
	void implant_stars(double x,double y,unsigned long Nregions,long *seed,IMFtype type=One);
	float getKappa_uniform(){return kappa_uniform;}
	float* getGamma_uniform(){return gamma_uniform;}
	double getAveMag(){ return 1.0/( pow(1-kappa_uniform,2) - gamma_uniform[0]*gamma_uniform[0] - gamma_uniform[1]*gamma_uniform[1]);}

protected:

   float kappa_uniform;
   float gamma_uniform[3];

};


#endif /* UNIFORM_LENS_H_ */
