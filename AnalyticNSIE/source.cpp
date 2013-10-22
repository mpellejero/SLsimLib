/*
 * source.cpp
 *
 *  Created on: Feb 8, 2012
 *      Author: mpetkova
 */

#include "slsimlib.h"
#include <typeinfo>

using namespace std;

Source::Source()
{
	source_r = 0;
	source_x[0] = 0;
	source_x[1] = 0;
	zsource = 0;
	setSBlimit_magarcsec(30.);
}

std::size_t Source::Nparams() const
{
	return 2;
}

double Source::getParam(std::size_t p) const
{
	switch(p)
	{
		case 0:
			// source x position
			return source_x[0];
		case 1:
			// source y position
			return source_x[1];
		default:
			throw std::invalid_argument("bad parameter index for getParam()");
	}
}

double Source::setParam(std::size_t p, double val)
{
	switch(p)
	{
		case 0:
			// source x position
			return (source_x[0] = val);
		case 1:
			// source y position
			return (source_x[1] = val);
		default:
			throw std::invalid_argument("bad parameter index for setParam()");
	}
}

double Source::tweakParam(std::size_t p, double eps)
{
	switch(p)
	{
		case 0:
			// source x position
			return (source_x[0] += eps*pi/180/60);
		case 1:
			// source y position
			return (source_x[1] += eps*pi/180/60);
		default:
			throw std::invalid_argument("bad parameter index for tweakParam()");
	}
}

void Source::printCSV(std::ostream&, bool header) const
{
	const std::type_info& type = typeid(*this);
	std::cerr << "Source subclass " << type.name() << " does not implement printCSV()" << std::endl;
	std::exit(1);
}

SourceUniform::SourceUniform(InputParams& params) : Source(){
	assignParams(params);
}

SourceGaussian::SourceGaussian(InputParams& params) : Source(){
	assignParams(params);
}

SourceBLR::SourceBLR(InputParams& params) : Source(){
	assignParams(params);
}

SourceBLRDisk::SourceBLRDisk(InputParams& params) : SourceBLR(params){

}

SourceBLRSph1::SourceBLRSph1(InputParams& params) : SourceBLR(params){

}

SourceBLRSph2::SourceBLRSph2(InputParams& params) : SourceBLR(params){

}

Source::~Source(){
}

SourceUniform::~SourceUniform(){
}

SourceGaussian::~SourceGaussian(){
}

SourceBLR::~SourceBLR(){
}

SourceBLRDisk::~SourceBLRDisk(){
}

SourceBLRSph1::~SourceBLRSph1(){
}

SourceBLRSph2::~SourceBLRSph2(){
}

void SourceUniform::assignParams(InputParams& params){

	if(!params.get("z_source",zsource)){
		  ERROR_MESSAGE();
		  cout << "parameter z_source needs to be set in parameter file " << params.filename() << endl;
		  exit(0);
	  }

	  printSource();
}

void SourceGaussian::assignParams(InputParams& params){


	if(!params.get("z_source",zsource)){
		  ERROR_MESSAGE();
		  cout << "parameter z_source needs to be set in parameter file " << params.filename() << endl;
		  exit(0);
	  }

	if(!params.get("gauss_r2",source_gauss_r2)){
		  ERROR_MESSAGE();
		  cout << "parameter gauss_r2 needs to be set in parameter file " << params.filename() << endl;
		  exit(0);
	  }

	  printSource();
}

void SourceBLR::assignParams(InputParams& params){

	bool fail = false;
	if(!params.get("source_z",zsource)){
		  ERROR_MESSAGE();
		  cout << "parameter source_z_source needs to be set in parameter file " << params.filename() << endl;
		  fail = true;
	  }
	if(!params.get("source_BHmass",source_BHmass)){
		  ERROR_MESSAGE();
		  cout << "parameter source_BHmass needs to be set in parameter file " << params.filename() << endl;
		  fail = true;
	  }

	if(!params.get("source_gamma",source_gamma)){
		  ERROR_MESSAGE();
		  cout << "parameter source_gamma needs to be set in parameter file " << params.filename() << endl;
		  fail = true;
	  }
	if(!params.get("source_inclin",source_inclination)){
		  ERROR_MESSAGE();
		  cout << "parameter source_inclin needs to be set in parameter file " << params.filename() << endl;
		  fail = true;
	  }
	if(!params.get("source_opening_ang",source_opening_angle)){
		  ERROR_MESSAGE();
		  cout << "parameter source_opening_ang needs to be set in parameter file " << params.filename() << endl;
		  fail = true;
	  }
	if(!params.get("source_r_in",source_r_in)){
		  ERROR_MESSAGE();
		  cout << "parameter source_r_in needs to be set in parameter file " << params.filename() << endl;
		  fail = true;
	  }
	if(!params.get("source_r_out",source_r_out)){
		  ERROR_MESSAGE();
		  cout << "parameter source_r_out needs to be set in parameter file " << params.filename() << endl;
		  fail = true;
	  }
	if(!params.get("source_nuo",source_nuo)){
		  ERROR_MESSAGE();
		  cout << "parameter source_nuo needs to be set in parameter file " << params.filename() << endl;
		  fail = true;
	  }
	if(!params.get("source_fK",source_fK)){
		  ERROR_MESSAGE();
		  cout << "parameter source_fK needs to be set in parameter file " << params.filename() << endl;
		  fail = true;
	  }

	if(fail) exit(1);

	source_inclination *= pi/180;
	source_opening_angle *= pi/180;
	source_monocrome = false;

	printSource();
}

void SourceUniform::printSource(){
	cout << endl << "**Source model**" << endl;

	cout << "z_source " << zsource << endl << endl;
}

void SourceGaussian::printSource(){
	cout << endl << "**Source model**" << endl;

	cout << "z_source " << zsource << endl;
	cout << "gauss_r2 " << source_gauss_r2 << endl << endl;
}

void SourceBLR::printSource(){
	cout << endl << "**Source model**" << endl;

	cout << "z_source " << zsource << endl;
	cout << "BHmass " << source_BHmass << endl;
	cout << "gamma " << source_gamma << endl;
	cout << "incl " << source_inclination*180/pi << endl;
	cout << "opening angl " << source_opening_angle*180/pi << endl;
	cout << "r_in " << source_r_in << endl;
	cout << "r_out " << source_r_out << endl;
	cout << "nuo " << source_nuo << endl;
	cout << "source_fK " << source_fK << endl << endl;
}

double SourceUniform::SurfaceBrightness(double *y){
	return (double)( (pow(y[0]-getX()[0],2) + pow(y[1]-getX()[1],2)) < source_r*source_r );
}

double SourceGaussian::SurfaceBrightness(double *y){
	return exp( -(pow(y[0]-getX()[0],2) + pow(y[1]-getX()[1],2))/source_gauss_r2 );
}
// surface brightness for models of the Broad Line Region
double SourceBLRDisk::SurfaceBrightness(double *y){
	double x[2] = {y[0]-getX()[0],y[1]-getX()[1]};
	return blr_surface_brightness_disk(x,this);
}

double SourceBLRSph1::SurfaceBrightness(double *y){
	return blr_surface_brightness_spherical_circular_motions(sqrt((pow(y[0]-getX()[0],2) + pow(y[1]-getX()[1],2))),this);
}
double SourceBLRSph2::SurfaceBrightness(double *y){
	return blr_surface_brightness_spherical_random_motions(sqrt((pow(y[0]-getX()[0],2) + pow(y[1]-getX()[1],2))),this);
}

//void in_source(double *y_source,ListHndl sourcelist){
//  return;
//}
SourcePixelled::SourcePixelled(InputParams& params)
{}

SourcePixelled::SourcePixelled(
		double my_z            /// redshift of the source
		, double* my_center  /// center (in rad)
		, int my_Npixels           /// number of pixels per side
		, double my_resolution  /// resolution (in rad)
		, double* arr_val          /// array of pixel values (must be of size = Npixels*Npixels)
		)
	:Source(), resolution(my_resolution), Npixels (my_Npixels){
	zsource = my_z;

	range = resolution*(Npixels-1);

	values.resize(Npixels*Npixels);
	for (int i = 0; i < Npixels*Npixels; i++)
			values[i] = arr_val[i];
	source_r =  range/sqrt(2.);
	source_x[0] = my_center[0];
	source_x[1] = my_center[1];
	calcTotalFlux();
	calcCentroid();
	calcEll();
	calcSize();
}

/** \brief Creates a SourcePixelled from a PixelMap image.
 *  The idea is to use stamps of observed galaxies as input sources for simuations.
 *  Surface brightness of the source is conserved, taking into account the input pixel size.
 *  Factor allows for rescaling of the flux, in case one wants to simulate a different observation.
 */
SourcePixelled::SourcePixelled(
		const PixelMap& gal_map  /// Input image and information
		, double my_z                 /// redshift of the source
		, double factor                /// optional rescaling factor for the flux
		)
	:Source(){

	zsource = my_z;
	resolution = gal_map.getResolution();
	Npixels = gal_map.getNpixels();
	range = resolution*(Npixels-1);
	source_x[0] = gal_map.getCenter()[0];
	source_x[1] = gal_map.getCenter()[1];
	source_r =  range/sqrt(2.);
	values.resize(Npixels*Npixels);
	for (int i = 0; i < Npixels*Npixels; i++)
			values[i] = gal_map[i]/resolution/resolution*factor;

	calcTotalFlux();
	calcCentroid();
	calcEll();
	calcSize();
}

SourcePixelled::~SourcePixelled(){
}

void SourcePixelled::calcCentroid(){
	double x_sum = 0;
	double y_sum = 0;
	double sum = 0;
	double x[2];
	for (unsigned long i = 0; i < Npixels*Npixels; i++)
	{
		Utilities::PositionFromIndex(i,x,Npixels,range,source_x);
		x_sum += x[0]*values[i];
		y_sum += x[1]*values[i];
		sum += values[i];
	}
	centroid[0] = x_sum/sum;
	centroid[1] = y_sum/sum;
}

void SourcePixelled::calcEll(){
	for (int j = 0; j < 2; j++)
	{
		for (int k = 0; k < 2; k++)
		{
			quad[j][k] = 0.;
		}
	}

	double x[2];
	for (unsigned long i = 0; i < Npixels*Npixels; i++)
	{
		Utilities::PositionFromIndex(i,x,Npixels,range,source_x);
		for (int j = 0; j < 2; j++)
		{
			for (int k = 0; k < 2; k++)
			{
				quad[j][k] += values[i]*(x[j]-centroid[j])*(x[k]-centroid[k]);
			}
		}
	}

	ell[0] = (quad[0][0] - quad[1][1])/(quad[0][0] + quad[1][1]);
	ell[1] = 2*quad[0][1]/(quad[0][0] + quad[1][1]);
}

void SourcePixelled::calcSize(){
	double r_sum = 0.;
	double sum = 0.;
	double rad;
	double x[2];

	for (unsigned long i = 0; i < Npixels*Npixels; i++)
	{
		Utilities::PositionFromIndex(i,x,Npixels,range,source_x);
		rad = sqrt((x[0]-centroid[0])*(x[0]-centroid[0])+(x[1]-centroid[1])*(x[1]-centroid[1]));
		r_sum += rad*values[i];
		sum += values[i];
	}
	size = r_sum/sum;
}

double SourcePixelled::SurfaceBrightness(double *y){
	long ix = Utilities::IndexFromPosition(y[0],Npixels,range,source_x[0]);
	long iy = Utilities::IndexFromPosition(y[1],Npixels,range,source_x[1]);
	if (ix>-1 && iy>-1)
	{
		return values[iy*Npixels+ix];
	}
	else
		return 0.;
}

void SourcePixelled::calcTotalFlux(){
	double val_tot = 0.;
	for (int i = 0; i < Npixels*Npixels; i++)
		val_tot += values[i];
	flux = val_tot*resolution*resolution;
}

void SourcePixelled::printSource(){}
void SourcePixelled::assignParams(InputParams& params){}

