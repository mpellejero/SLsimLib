/*
 * Source/sersic.cpp
 *
 *  Created on: Feb 28, 2010
 *      Author: R.B. Metcalf
 */
#include "slsimlib.h"

SourceSersic::SourceSersic(
		double my_mag              /// Total magnitude
		,double my_Reff         /// Bulge half light radius (arcs)
		,double my_PA           /// Position angle (radians)
		,double my_index  /// Sersic index
		,double my_q     // axes ratio
		,double my_z            /// optional redshift
		,const double *my_theta          /// optional angular position on the sky
		)
: Source(),
  Reff(my_Reff*pi/180/60/60), mag(my_mag), PA(my_PA), index(my_index), q(my_q)
{
	setZ(my_z);
	
	if(my_theta)
		setX(my_theta[0], my_theta[1]);
	
	if ( q > 1)
	{
		std::cerr << "Error: q must be < 1!" << std::endl;
		exit(1);
	}
	setInternals();
}

SourceSersic::SourceSersic(InputParams& params)
: Source()
{
}

SourceSersic::SourceSersic(): Source()
{
}

SourceSersic::~SourceSersic()
{
}

std::size_t SourceSersic::Nparams() const
{
	return Source::Nparams() + 5;
}

double SourceSersic::getParam(std::size_t p) const
{
	if(p < Source::Nparams())
		return Source::getParam(p);
	
	switch(p - Source::Nparams())
	{
		case 0:
			// half light radius
			return Reff;
		case 1:
			// magnitude
			return mag;
		case 2:
			// position angle
			return PA;
		case 3:
			// Sersic index
			return index;
		case 4:
			// axes ratio
			return q;
		default:
			throw std::invalid_argument("bad parameter index for randomize()");
	}
}

double SourceSersic::setParam(std::size_t p, double val)
{
	double ret;
	
	if(p < Source::Nparams())
	{
		ret = Source::setParam(p, val);
	}
	else
	{
		switch(p - Source::Nparams())
		{
			case 0:
				// half light radius
				ret = (Reff = val);
				break;
			case 1:
				// magnitude
				ret = (mag = val);
				break;
			case 2:
				// position angle
				ret = (PA = val);
				break;
			case 3:
				// Sersic index
				ret = (index = val);
				break;
			case 4:
				// axes ratio
				q = val;
				
				// invert ellipsis if bigger than one
				if(q > 1.)
				{
					q = 1/q;
					PA = std::fmod(PA + pi, pi) - pi/2;
				}
				
				ret = q;
				break;
			default:
				throw std::invalid_argument("bad parameter index for randomize()");
		}
	}
	
	// update
	setInternals();
	
	return ret;
}

double SourceSersic::tweakParam(std::size_t p, double eps)
{
	double ret;
	
	if(p < Source::Nparams())
	{
		ret = Source::tweakParam(p, eps);
	}
	else
	{
		switch(p - Source::Nparams())
		{
			case 0:
				// half light radius
				ret = (Reff = std::max(1.e-10, Reff + eps*pi/180/60/60));
				break;
			case 1:
				// magnitude
				ret = (mag += eps*10);
				break;
			case 2:
				// position angle
				ret = (PA = std::fmod(PA + pi/2 + eps*pi, pi) - pi/2);
				break;
			case 3:
				// Sersic index
				ret = (index += eps);
				break;
			case 4:
				// axes ratio
				q = std::max(1.e-05, q + eps);
				
				// invert ellipsis if bigger than one
				if(q > 1.)
				{
					q = 1/q;
					PA = std::fmod(PA + pi, pi) - pi/2;
				}
				
				ret = q;
				break;
			default:
				throw std::invalid_argument("bad parameter index for randomize()");
		}
	}
	
	// update
	setInternals();
	
	return ret;
}

void SourceSersic::printCSV(std::ostream& out, bool header) const
{
	if(header)
	{
		out
		<< "mag" << ","
		<< "Reff" << ","
		<< "PA" << ","
		<< "n" << ","
		<< "q" << ","
		<< "z" << ","
		<< "x[0]" << ","
		<< "x[1]" << std::endl;
	}
	else
	{
		out
		<< mag << ","
		<< Reff << ","
		<< PA << ","
		<< index << ","
		<< q << ","
		<< zsource << ","
		<< source_x[0] << ","
		<< source_x[1] << std::endl;
	}
}

void SourceSersic::setInternals()
{
	// approximation valid for 0.5 < n < 8
	bn = 1.9992*index - 0.3271;
	flux = pow(10,-0.4*(mag+48.6));
	Ieff = flux/2./pi/Reff/Reff/exp(bn)/index*pow(bn,2*index)/tgamma(2*index)/q;
	
	// radius in Source
	// approximation of the radius that encloses 99% of the flux
	setRadius((3.73 - 0.926*index + 1.164*index*index)*Reff);
}

double SourceSersic::SurfaceBrightness(
		double *x  /// position in radians relative to center of source
		){

	double x_new[2];
	x_new[0] = (x[0]-source_x[0])*cos(PA)+(x[1]-source_x[1])*sin(PA);
	x_new[1] = (x[0]-source_x[0])*sin(PA)-(x[1]-source_x[1])*cos(PA);

	double r = sqrt(x_new[0]*x_new[0]+x_new[1]*x_new[1]/q/q);

	double sb = Ieff * exp(-bn*(pow(r/Reff,1./index)-1.))*inv_hplanck;
	if (sb < sb_limit) return 0.;
	return sb;
}

void SourceSersic::printSource(){}
void SourceSersic::assignParams(InputParams& /* params */){}
