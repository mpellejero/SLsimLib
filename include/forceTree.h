/*
 * forceTree.h
 *
 *  Created on: Oct 14, 2011
 *      Author: bmetcalf
 */



#include <list.h>
#include "simpleTree.h"
#include <multiplane.h>

#ifndef FORCE_TREE_H_
#define FORCE_TREE_H_

///enum PartProf {gaussian,powerlaw};

struct HaloStructure;

/** \ingroup DeflectionL2
 *
 * \brief Object used to calculate the force or deflection caused by a collection
 * of "particles" by the tree method.
 *
 * The particles can be point masses or have multiple sizes.  They can also have the
 * same mass or multiple masses.
 *
 * xp[][], masses[] and rsph[] need to be allocated before constructing a ForceTree is
 * constructed and de-allocated after it is destruction.  Multiple ForceTrees can be
 * made from the same particles.  Do not rotate the particles without reconstructing
 * a ForceTree.
 *
 * Most of the code in TreeNBForce.c is duplicated here as private methods and
 * a few public ones.
 */
class ForceTree : public SimpleTree {
public:
	ForceTree(PosType **xp,IndexType Npoints,float *masses,float *rsph,bool Multimass,bool Multisize
			,int bucket = 5,int dimensions = 2,bool median = false,PosType theta = 0.1
			);
	virtual ~ForceTree();

	/// calculated sph smoothing and store them in the tree, also provide pointer to them
	float * CalculateSPHsmoothing(int N);
	/// calculate the deflection and lensing properties
	void force2D(PosType *ray,PosType *alpha,PosType *kappa,PosType *gamma,bool no_kappa = true);
	/// provides a way to change the profiles of the particles, by default Gaussian
	//void ChangeParticleProfile(PartProf partprof);

protected:

	bool init;

	/// true if particles have different masses.
	bool MultiMass;
	/// true if particles have different sizes.
	bool MultiRadius;
	/// Array of particle masses
	float *masses;
	/// Array of particle sizes
	float *rsph;


	PosType force_theta;

	//PosType (*alpha_particle)(PosType r,float rmax);
	//PosType (*kappa_particle)(PosType r,float rmax);
	//PosType (*gamma_particle)(PosType r,float rmax);

	//PosType (*alpha_halo)(PosType r,HaloInternal &par);
	//PosType (*kappa_halo)(PosType r,HaloInternal &par);
	//PosType (*gamma_halo)(PosType r,HaloInternal &par);

	void CalcMoments();
	void rotate_coordinates(PosType **coord);
	void spread_particles();

	// Internal profiles for a Gaussian particle
	double alpha_o(double r2,float sigma);
	double kappa_o(double r2,float sigma);
	double gamma_o(double r2,float sigma);
	double phi_o(double r2,float sigma);

	/// These will be overridden in a derived class with a specific halo internal structures
	virtual double alpha_h(double r2,HaloStructure &par){return 0.0;}
	virtual double kappa_h(double r2,HaloStructure &par){return 0.0;}
	virtual double gamma_h(double r2,HaloStructure &par){return 0.0;}
	virtual double phi_h(double r2,HaloStructure &par){return 0.0;}

	bool haloON;
	HaloStructure *halo_params;
};

typedef ForceTree *ForceTreeHndl;

/** \ingroup DeflectionL2
 *
 * \brief A class for calculating the deflection, kappa and gamma caused by a collection of halos
 * with truncated power-law mass profiles.
 *
 * Derived from the ForceTree class.  The "particles" are replaced with spherical halos.
 *The truncation is in 2d not 3d. \f$ \Sigma \propto r^\beta \f$ so beta would usually be negative.
 *
 */
class ForceTreePowerLaw : public ForceTree{

public:
	ForceTreePowerLaw(float beta,PosType **xp,IndexType Npoints,HaloStructure *par_internals,bool Multisize = true
			,int bucket = 5,int dimensions = 2,bool median = false,PosType theta = 0.1
			);
	~ForceTreePowerLaw();

private:
	float beta; // logorithmic slop of 2d mass profile

	// Override internal structure of halos
	double alpha_h(double r2,HaloStructure &par);
	double kappa_h(double r2,HaloStructure &par);
	double gamma_h(double r2,HaloStructure &par);
	double phi_h(double r2,HaloStructure &par);
};

/** \ingroup DeflectionL2
 *
 * \brief A class for calculating the deflection, kappa and gamma caused by a collection of NFW
 * halos.
 *
 * Derived from the ForceTree class.  The "particles" are replaced with spherical NFW halos.
 *
 * This class uses the true expressions for the NFW profile.  This is
 * time consuming and not usually necessary. See ForceTreePseudoNFW for a faster alternative.
 */
class ForceTreeNFW : public ForceTree{

public:
	ForceTreeNFW(PosType **xp,IndexType Npoints,HaloStructure *par_internals,bool Multisize = true
			,int bucket = 5,int dimensions = 2,bool median = false,PosType theta = 0.1
			);
	~ForceTreeNFW();

private:

	// Override internal structure of halos
	double alpha_h(double r2,HaloStructure &par);
	double kappa_h(double r2,HaloStructure &par);
	double gamma_h(double r2,HaloStructure &par);
	double phi_h(double r2,HaloStructure &par);

	double gfunction(double x);
	double ffunction(double x);
	double g2function(double x);
};

/** \ingroup DeflectionL2
 *
 * \brief A class for calculating the deflection, kappa and gamma caused by a collection of
 * halos with a double power-law mass profile.
 *
 * Derived from the ForceTree class.  The "particles" are replaced with spherical halos
 * with \f$ \Sigma \propto 1/(1 + (r/rscale) )^\beta \f$ so beta would usually be positive.
 *
 * An NFW profile is approximated beta = 2 .
 */
class ForceTreePseudoNFW : public ForceTree{

public:
	ForceTreePseudoNFW(float beta,PosType **xp,IndexType Npoints,HaloStructure *par_internals,bool Multisize = true
			,int bucket = 5,int dimensions = 2,bool median = false,PosType theta = 0.1
			);
	~ForceTreePseudoNFW();

private:

	double beta;

	// Override internal structure of halos
	double alpha_h(double r2,HaloStructure &par);
	double kappa_h(double r2,HaloStructure &par);
	double gamma_h(double r2,HaloStructure &par);
	double phi_h(double r2,HaloStructure &par);

	double mhat(double y);
};

#endif /* FORCE_TREE_H_ */
