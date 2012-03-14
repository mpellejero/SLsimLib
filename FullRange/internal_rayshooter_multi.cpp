/*
 * internal_rayshooter_multi.cpp
 *
 *  Created on: Jan 19, 2012
 *      Author: mpetkova, bmetcalf
 */

#include <slsimlib.h>

/** \ingroup DeflectionL2
 *
 * \brief This is the function that does the deflection calculation with multiple lens planes.
 *
 * The convergence, shear and rotation will be calculated if kappa_off == false
 *
 * Needs to be calculated before calling:
 *
 *
 * Dl[j = 0...Nplanes-1] - The angular size distance between the observer and the jth plane not counting the observer plane.
 * 	                    Dl[0] is the first plane with mass on it and Dl[Nplane-1] is the distance to the source plane.
 *
 * dDl[j = 0...Nplanes-1] - The angular size distance between the (j-1)th and jth planes counting the observer plane as j = -1.
 *                      dDl[0] = Dl[0], dDl[Nplane-1] is between the last plane with mass and the source plane.
 *
 * charge = 4*G*mass_scale/c^2 in units of Mpc
 *
 * i_points[].x[] is in angular units.
 *
 * Warning: Is not valid for a non-flat universe.
 */

void MultiLens::rayshooterInternal(unsigned long Npoints, Point *i_points, bool kappa_off){
	unsigned long i;

	for(i = 0; i< Npoints; i++){

		double kappa,aa,bb,cc;
	    double alpha[2], gamma[3];
	    double xminus[2],xplus[2];
	    double kappa_minus,gamma_minus[3],kappa_plus,gamma_plus[3];

		// find position on first lens plane in comoving units
		i_points[i].image->x[0] = i_points[i].x[0]*Dl[0];
       	i_points[i].image->x[1] = i_points[i].x[1]*Dl[0];

       	xminus[0] = 0;
		xminus[1] = 0;

		// Set magnification matrix on first plane.  Also the default if kappa_off == false
		kappa_minus = 0;
		gamma_minus[0] = 0;
		gamma_minus[1] = 0;
		gamma_minus[2] = 0;

		i_points[i].kappa = 1;  // This is actually 1-kappa until after the loop through the planes.
		i_points[i].gamma[0] = 0;
		i_points[i].gamma[1] = 0;
		i_points[i].gamma[2] = 0;

		for(int j = 0; j < Nplanes-1 ; j++){  // each iteration leaves i_point[i].image on plane (j+1)

			// convert to physical coorditanes on the plane j
			double xx[2];
			xx[0] = i_points[i].image->x[0]/(1+redshift[j]);
			xx[1] = i_points[i].image->x[1]/(1+redshift[j]);

			if(j == flag_analens && j > 0){
				analens->rayshooterInternal(&xx[0],&alpha[0],&gamma[0],&kappa,kappa_off);
				cc = dDl[j+1];
			}
			else{
				halo_tree[j]->force2D(&xx[0],&alpha[0],&kappa,&gamma[0],kappa_off);
				cc = charge*dDl[j+1];

				/* multiply by the scale factor to obtain 1/comoving_distance/physical_distance
				 * such that a multiplication with the charge will result in a 1/comoving_distance quantity */
				kappa/=(1+redshift[j]);
				gamma[0]/=(1+redshift[j]);
				gamma[1]/=(1+redshift[j]);
				gamma[2]/=(1+redshift[j]);
			}

			aa = (dDl[j+1]+dDl[j])/dDl[j];
			bb = dDl[j+1]/dDl[j];

			xplus[0] = aa*i_points[i].image->x[0] - bb*xminus[0] - cc*alpha[0];
       		xplus[1] = aa*i_points[i].image->x[1] - bb*xminus[1] - cc*alpha[1];

			xminus[0] = i_points[i].image->x[0];
			xminus[1] = i_points[i].image->x[1];

			i_points[i].image->x[0] = xplus[0];
			i_points[i].image->x[1] = xplus[1];

			if(!kappa_off)
    		{

				aa = (dDl[j+1]+dDl[j])*Dl[j]/dDl[j]/Dl[j+1];

				//bb = dDl[j+1]*Dl[ (j < 1) ? 0 : j-1]/dDl[j]/Dl[j+1];
				// removed the line above because Dl[-1] = observer plane = 0 and is different from Dl[0]
				if(j>0){
					bb = dDl[j+1]*Dl[j-1]/dDl[j]/Dl[j+1];
				}
				else
					bb = 0;

				if(j == flag_analens && j > 0)
					cc = dDl[j+1]*Dl[j]/Dl[j+1];
				else
					cc = charge*dDl[j+1]*Dl[j]/Dl[j+1];

				// still not positive about sign convention
				kappa_plus = aa*i_points[i].kappa - bb*kappa_minus
						- cc*(kappa*i_points[i].kappa - gamma[0]*i_points[i].gamma[0] - gamma[1]*i_points[i].gamma[1]);

				gamma_plus[0] = aa*i_points[i].gamma[0] - bb*gamma_minus[0]
						+ cc*(gamma[0]*i_points[i].kappa - kappa*i_points[i].gamma[0] + gamma[1]*i_points[i].gamma[2]);

				gamma_plus[1] = aa*i_points[i].gamma[1] - bb*gamma_minus[1]
						+ cc*(gamma[1]*i_points[i].kappa - kappa*i_points[i].gamma[1] - gamma[0]*i_points[i].gamma[2]);

				gamma_plus[2] = aa*i_points[i].gamma[2] - bb*gamma_minus[2]
						+ cc*(gamma[1]*i_points[i].gamma[0] - gamma[0]*i_points[i].gamma[1] - kappa*i_points[i].gamma[2]);

				kappa_minus = i_points[i].kappa;
				gamma_minus[0] = i_points[i].gamma[0];
				gamma_minus[1] = i_points[i].gamma[1];
				gamma_minus[2] = i_points[i].gamma[2];

				i_points[i].kappa = kappa_plus;
				i_points[i].gamma[0] = gamma_plus[0];
				i_points[i].gamma[1] = gamma_plus[1];
				i_points[i].gamma[2] = gamma_plus[2];
    		}

		}
		// Convert units back to angles.

		i_points[i].image->x[0] /= Dl[Nplanes-1];
		i_points[i].image->x[1] /= Dl[Nplanes-1];

		i_points[i].kappa = 1 - i_points[i].kappa;

		i_points[i].invmag = (1-i_points[i].kappa)*(1-i_points[i].kappa)
		  	    - i_points[i].gamma[0]*i_points[i].gamma[0]
		  	    - i_points[i].gamma[1]*i_points[i].gamma[1]
		  	    - i_points[i].gamma[2]*i_points[i].gamma[2];

		/*if(i<10){
			cout << i_points[i].x[0]*analens->Dl  << " " << i_points[i].x[1]*analens->Dl  << " ";
			cout << i_points[i].image->x[0]*analens->Dl   << " " << i_points[i].image->x[1]*analens->Dl   << " ";
			cout << i_points[i].invmag << " " << i_points[i].kappa << " ";
			cout << i_points[i].gamma[0] << " " << i_points[i].gamma[1] << " " << i_points[i].gamma[2] << endl;
			}*/
    }

    return;
}
