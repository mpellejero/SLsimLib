/*
 * MOKAlens.h
 *
 *  Created on: Jun 8, 2012
 *      Author: mpetkova
 */


#ifndef MOKALENS_H_
#define MOKALENS_H_

#include "standard.h"
#include "MOKAfits.h"
#include "profile.h"
#include "InputParams.h"

//TODO Improve this comment with more complete description of what a MOKAmap is used for.
/**
 * \brief the MOKA map structure, containing all quantities that define it
 *
 * Note: To use this class requires setting the ENABLE_FITS compiler flag and linking
 * the cfits library.

#include <MOKAfits.h>

 */
struct MOKAmap{
	/// values for the map
	std::valarray<float> convergence;
	std::valarray<float> alpha1;
	std::valarray<float> alpha2;
	std::valarray<float> gamma1;
	std::valarray<float> gamma2;
	std::valarray<float> gamma3;
	std::valarray<float> Signlambdar;
	std::valarray<float> Signlambdat;
	std:: vector<double> x;	 
    int nx,ny;
    // boxlMpc is Mpc/h for MOKA
    double boxlarcsec,boxlMpc,boxlrad,zlens,zsource,omegam,omegal,h,DL;
	double inarcsec;
	double center[2];
};
//TODO Improve this comment.
/**
 *  \brief A class to represents the MOKA lens map
 *
 * Note: To use this class requires setting the ENABLE_FITS compiler flag and linking
 * the cfits library.
 */

//TODO Change to physical length units !!!!
class MOKALens : public Lens{
public:

	//MOKALens(std::string);
	//MOKALens(std::string paramfile,LensHalo *LH);
	MOKALens(InputParams& params);
	MOKALens(InputParams& params,LensHalo *LH);

	~MOKALens();

	bool set;	/// the name of the MOKA input file
	std::string MOKA_input_file;
	/// if >=1 (true), do analyzis only; if = 0 (false) change units to internal GLAMER units and prepare for ray-shooting
	int flag_MOKA_analyze;
	int flag_background_field;

	void assignParams(InputParams& params);
	void rayshooterInternal(double *ray, double *alpha, KappaType *gamma, KappaType *kappa, bool kappa_off);
	void rayshooterInternal(unsigned long Npoints, Point *i_points, bool kappa_off){ERROR_MESSAGE(); exit(1);};
	void setZlens(CosmoHndl cosmo,double zlens,double zsource = 1000);
	double getZlens();
	void setInternalParams(CosmoHndl,SourceHndl);
	void saveImage(GridHndl grid, bool saveprofile=true);
	void saveImage(bool saveprofile=true);
	void saveKappaProfile();
	void saveGammaProfile();
	void saveProfiles(double &RE3, double &xxc, double &yyc);
	void initMap();

	MOKAmap *map;
	LensHalo *LH;

	void estSignLambdas();
	void EinsteinRadii(double &RE1, double &RE2, double &xxc, double &yyc);

};

#endif /* MOKALENS_H_ */

