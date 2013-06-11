/*
 * base_analens.h
 *
 *  Created on: Jan 22, 2013
 *      Author: mpetkova
 */

#ifndef BASE_ANALENS_H_
#define BASE_ANALENS_H_

#include "forceTree.h"
#include "quadTree.h"
#include "source.h"
#include "InputParams.h"

/**
 * \brief An "analytic" model to represent a lens on a single plane.
 *
 * The lens consists of a "host" lens which is a non-singular isothermal ellipsoid (NSIE) plus axial distortion
 * modes, substructures and stars.
 *
 *<pre>
 * Input Parameters:
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
 *	beta_sub               Logarithmic slope of the internal clump profile.  Used if sub_type == powerlaw
 *	alpha_sub              Logarithmic slope of the mass function.
 *	R_submax               Maximum radius of most massive substructure (see Metcalf & Amara 2012)
 *	sub_mass_max               Maximum mass
 *	sub_mass_min               Minimum mass
 *	sub_type               Mass profile of clumps - 0 or nfw,1 or powerlaw, 2 or pointmass
 *
 *  **** Stars parameters
 *	Nstars                 Total number of stars that will be used in the simulation.  If zero the other star parameters are not needed.
 *	fstars                 Fraction of surface denity in stars.
 *	stars_mass             Mass of stars.
 *
 * The stars are not initially present.  They must be implanted later.
 *</pre>
 *
 * TODO BEN finish this documentation.
 */
class LensHaloBaseNSIE : public LensHaloSimpleNSIE{
public:
	LensHaloBaseNSIE(InputParams& params);
	virtual ~LensHaloBaseNSIE();

  /// critical surface density
  double getSigma_crit(){return Sigma_crit;}
  /// the time delay scale in days/Mpc^2
  double get_to(){return to;}
   /// Angular size distance to lens plane
  double get_Dl(){return Dl;}
  /// conversion factor from Mpc to Arcsec
  double get_MpcToAsec(){return MpcToAsec;}

  // private derived quantities

  /// substructures
  bool AreSubStructImaplated(){return substruct_implanted;}
  double sub_sigmaScale;
  double sub_Ndensity;
  /// actual number of substructures
  int sub_N;
  double **sub_x;
  /// slope of mass profile
  double sub_beta;
  /// slope of mass function
  double sub_alpha;
  /// radius of largest mass substructures
  double sub_Rmax;
  double sub_Mmax;
  double sub_Mmin;
  double sub_theta_force;
  LensHalo *subs;
  TreeQuad *sub_tree;
  IndexType *sub_substructures;
  ClumpInternal sub_type;

  /// stars
  bool AreStarsImaplated(){return stars_implanted;}
  int stars_N;
  IndexType *stars;
  PosType **stars_xp;
  //TreeForce *star_tree;
  TreeQuad *star_tree;
  double star_massscale;
  /// star masses relative to star_massscles
  float *star_masses;
  double star_fstars;
  double star_theta_force;
  int star_Nregions;
  double *star_region;

  void setZlens(CosmoHndl cosmo,double zlens,double zsource = 1000);
  void setInternalParams(CosmoHndl,SourceHndl);
  void setInternalParams(CosmoHndl,double);
  void assignParams(InputParams& params);
  void PrintLens(bool show_substruct,bool show_stars);
  void error_message1(std::string name,std::string filename);

  virtual void force_halo(double *alpha,KappaType *kappa,KappaType *gamma,double *xcm,bool no_kappa,bool subtract_point=false);

  // in randoimize_lens.c
  double averageSubMass();

  // in readlens_ana.c
  void reNormSubstructure(double kappa_sub);
  void substract_stars_disks(PosType *ray,PosType *alpha
                  ,KappaType *kappa,KappaType *gamma);
  void implant_stars(Point *centers,unsigned long Nregions,long *seed, IMFtype type=One);
  float* stellar_mass_function(IMFtype type, unsigned long Nstars, long *seed, double minmass=0.0, double maxmass=0.0
  		,double bendmass=0.0, double powerlo=0.0, double powerhi=0.0);

  //void toggleStars(bool implanted);

  double getHost_Dl(){return Dl;}

  double getEinstein_ro(){return Einstein_ro;}

  double getPerturb_beta(){return perturb_beta;}
  IMFtype getIMF_type(){return imf_type;}
  int getPerturb_Nmodes(){return perturb_Nmodes;}    /// this includes two for external shear
  double *perturb_modes;  ///first two are shear

protected:

  /// critical surface density
  double Sigma_crit;
   /// the time delay scale in days/Mpc^2
  double to;
   /// Angular size distance to lens plane
  double Dl;
  /// Einstein radius
  double Einstein_ro;

    // perturbations to host.  These are protected so that in some derived classes they can or cann't be changed.
  int perturb_Nmodes;    /// this includes two for external shear
  double perturb_beta;
  double *perturb_rms;

  bool substruct_implanted;

  bool stars_implanted;
  /// Number of regions to be subtracted to compensate for the mass in stars
  IMFtype imf_type;
  double min_mstar;
  double max_mstar;
  double bend_mstar;
  double lo_mass_slope;
  double hi_mass_slope;
  /// parameters for stellar mass function: minimal and maximal stellar mass, bending point for a broken power law IMF
  double *star_kappa;
  double **star_xdisk;

   // private derived quantities
   /// private: conversion factor between Mpc on the lens plane and arcseconds
   double MpcToAsec;


};

namespace Utilities{
	double RandomFromTable(double *table,unsigned long Ntable,long *seed);
	void rotation(float *xout,float *xin,double theta);
	void rotation(double *xout,double *xin,double theta);
}

void alphaNSIE(double *alpha,double *xt,double f,double bc,double theta);
KappaType kappaNSIE(double *xt,double f,double bc,double theta);
void gammaNSIE(KappaType *gam,double *xt,double f,double bc,double theta);
KappaType invmagNSIE(double *x,double f,double bc,double theta
                     ,float *gam,float kap);
double rmaxNSIE(double sigma,double mass,double f,double rc );
double ellipticRadiusNSIE(double *x,double f,double pa);
void quadMomNSIE(float mass,float Rmax,float f,float rc,float theta,double *quad);


//  in powerlow.c

void alphaPowLaw(double *alpha,double *x,double R,double mass,double beta,double *center,double Sigma_crit);
KappaType kappaPowLaw(double *x,double R,double mass,double beta,double *center,double Sigma_crit);
void gammaPowLaw(KappaType *gamma,double *x,double R,double mass,double beta,double *center,double Sigma_crit);
KappaType phiPowLaw(double *x,double R,double mass,double beta,double *center,double Sigma_crit);

// in nfw_lens.c
void alphaNFW(double *alpha,double *x,double Rtrunc,double mass,double r_scale
                ,double *center,double Sigma_crit);
KappaType kappaNFW(double *x,double Rtrunc,double mass,double r_scale
                ,double *center,double Sigma_crit);
void gammaNFW(KappaType *gamma,double *x,double Rtrunc,double mass,double r_scale
                ,double *center,double Sigma_crit);

// in hernquist_lens.cpp
void alphaHern(double *alpha,double *x,double Rtrunc,double mass,double r_scale
                ,double *center,double Sigma_crit);
KappaType kappaHern(double *x,double Rtrunc,double mass,double r_scale
                ,double *center,double Sigma_crit);
void gammaHern(KappaType *gamma,double *x,double Rtrunc,double mass,double r_scale
                ,double *center,double Sigma_crit);


// in lens_expand.c

double lens_expand(double beta,double *mod,int Nmodes,double *x,double *alpha,KappaType *gamma,KappaType *phi);

// in FullRange/implant_stars.c

//void implant_stars(Point *images,unsigned long Nimages,long *seed);
//void substract_stars_disks(LensHaloBaseNSIE *lens,PosType *ray,PosType *alpha
 //               ,PosType *kappa,PosType *gamma);


#endif /* BASE_ANALENS_H_ */
