/*
 * Galaxies/overzier.h
 *
 *  Created on: Mar 6, 2010
 *      Author: R.B. Metcalf
 */
#ifndef GALAXIES_OVERZIER_H_
#define GALAXIES_OVERZIER_H_

// define pi here if not done via include
#ifndef pi
#define pi 3.141592653589793238462643383279502884
#endif

/**
 *\brief Structure for holding parameters for one or more galaxy images according to
 * the Overzier model.
 */
struct OverGalaxy{
	OverGalaxy();
	OverGalaxy(double mag,double BtoT,double Reff,double Rh,double PA,double inclination,unsigned long my_id,double my_z=0,const double *theta=0);
	~OverGalaxy();

	void setInternals(double mag,double BtoT,double Reff,double Rh,double PA,double inclination,unsigned long my_id,double my_z=0,const double *my_theta=0);
	double SurfaceBrightness(double *x);
	void print();
	/// get magnitude of hole galaxy.  Which band this is in depends on which was passed in the constructor
	double getMag() const { return mag; }
	double getUMag() const { return mag_u; }
	double getGMag() const { return mag_g; }
	double getRMag() const { return mag_r; }
	double getIMag() const { return mag_i; }
	double getZMag() const { return mag_z; }
	double getJMag() const { return mag_J; }
	double getHMag() const { return mag_H; }
	double getKMag() const { return mag_Ks; }

	/// set u band magnitude
	void setUMag(double m) { mag_u = m; }
	/// set g band magnitude
	void setGMag(double m) { mag_g = m; }
	/// set r band magnitude
	void setRMag(double m) { mag_r = m; }
	/// set i band magnitude
	void setIMag(double m) { mag_i = m; }
	/// set z band magnitude
	void setZMag(double m) { mag_z = m; }
	/// set j band magnitude
	void setJMag(double m) { mag_J = m; }
	/// set h band magnitude
	void setHMag(double m) { mag_H = m; }
	/// set k band magnitude
	void setKMag(double m) { mag_Ks = m; }

	/// bulge half light radius
	double getReff() const { return Reff/(pi/180/60/60); }
	/// disk scale height
	double getRh() const { return Rh/(pi/180/60/60); }
	double getBtoT() const { return BtoT; }
	double getPA() const { return PA; }
	double getInclination() const { return inclination; }

	/// haloID
	unsigned long haloID;
	/// redshift
	double z;
	/// position on the sky
	double theta[2];
			// weighted mean between the radii that enclose 99% of the flux
			// in the pure De Vacouleur/exponential disk case
			// 6.670 = 3.975*Re = 3.975*1.678*Rh
	double getRadius(){return
		6.670*Rh*(1-BtoT)+18.936*Reff*BtoT;}

private:
	/// bulge half light radius
	double Reff;
	/// disk scale height
	double Rh;

	double BtoT;
	double PA;
	double inclination;

	double cxx,cyy,cxy;
	double sbDo;
	double sbSo;
	double mag;

	// colors
	double mag_u;
	double mag_g;
	double mag_r;
	double mag_i;
	double mag_z;
	double mag_J;
	double mag_H;
	double mag_Ks;
	double mag_i1;
	double mag_i2;

	// optional position variables
};

#endif /* GALAXIES_OVERZIER_H_ */
