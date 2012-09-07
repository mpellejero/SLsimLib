/*
 * internal_rayshooter_nfw.c
 *
 *  Created on: Dec 8, 2009
 *      Author: R.B. Metcalf
 */

#include <slsimlib.h>

/** \ingroup DeflectionL2
 *
 * \brief Routine for calculating the deflection and other lensing quantities for
 * a analytic one plane lens (AnaLens).
 *
 * Can be switched with rayshooterNB() to change
 * to particle lens model.  This transition needs to be made more automatic and
 * fail safe.
 */
void AnaLens::rayshooterInternal(unsigned long Npoints, Point *i_points, bool kappa_off){
	/* i_points need to be already linked to s_points */

    if(this == NULL || !set){
    	ERROR_MESSAGE();
    	std::printf("ERROR: rayshooterInternal  lens not set!\n");
    	exit(0);
    }

	double convert_factor = star_massscale / Sigma_crit;
	int i;

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for(i = 0; i < Npoints; i++){
    	double x_rescale[2];
        long i,j;
        float dt = 0,tmp = 0;
    	double alpha[2];
    	float gamma[3];

    	alpha[0]=alpha[1]=0.0;
    	gamma[0]=gamma[1]=gamma[2]=0.0;

    	i_points[i].dt = 0.0;
    	i_points[i].gamma[2] = 0.0;

    	// host lens
    	if(host_ro > 0){
    		x_rescale[0] = i_points[i].x[0] / host_ro;
    		x_rescale[1] = i_points[i].x[1] / host_ro;

    		alphaNSIE(i_points[i].image->x, x_rescale, host_axis_ratio,
    				host_core / host_ro, host_pos_angle);

    		if(!kappa_off){
    			gammaNSIE(i_points[i].gamma,x_rescale,host_axis_ratio
    					,host_core/host_ro,host_pos_angle);
    			i_points[i].kappa=kappaNSIE(x_rescale,host_axis_ratio
    					,host_core/host_ro,host_pos_angle);
    			i_points[i].dt = phiNSIE(x_rescale,host_axis_ratio
    					,host_core/host_ro,host_pos_angle);
    		}
    		else{
    			i_points[i].kappa=0;
    			i_points[i].gamma[0]=i_points[i].gamma[1]=0.0;
    			i_points[i].dt = 0.0;
    		}

    		i_points[i].image->x[0] *= host_ro;
    		i_points[i].image->x[1] *= host_ro;

    	}
    	else{
    		i_points[i].image->x[0] = 0.0;
    		i_points[i].image->x[1] = 0.0;
    		i_points[i].kappa=0;
    		i_points[i].gamma[0]=i_points[i].gamma[1]=0;
    		i_points[i].dt = 0.0;
    	}

    	// perturbations of host lens
    	if(perturb_Nmodes > 0){
    		i_points[i].kappa += lens_expand(perturb_beta,perturb_modes
    				,perturb_Nmodes,i_points[i].x,alpha,gamma,&dt);

    		i_points[i].image->x[0] += alpha[0];
    		i_points[i].image->x[1] += alpha[1];

    		if(!kappa_off){
    			i_points[i].gamma[0] += gamma[0];
    			i_points[i].gamma[1] += gamma[1];
    			i_points[i].dt += dt;
    		}
    		else
    			i_points[i].kappa = 0;
    	} // end of perturb modes

    	alpha[0]=alpha[1]=0.0;
    	gamma[0]=gamma[1]=gamma[2]=0.0;

    	// add substructure
    	if(substruct_implanted){
    		for(j=0;j<sub_N;++j){
    			sub_alpha_func(alpha,i_points[i].x,sub_Rcut[j],sub_mass[j],sub_beta,sub_x[j],Sigma_crit);

    			i_points[i].image->x[0] += alpha[0];
    			i_points[i].image->x[1] += alpha[1];

    			if(!kappa_off){
    				i_points[i].kappa += sub_kappa_func(i_points[i].x,sub_Rcut[j],sub_mass[j],sub_beta,sub_x[j],Sigma_crit);
    				sub_gamma_func(gamma,i_points[i].x,sub_Rcut[j],sub_mass[j],sub_beta,sub_x[j],Sigma_crit);
    				i_points[i].gamma[0] += gamma[0];
    				i_points[i].gamma[1] += gamma[1];
    				i_points[i].dt += sub_phi_func(i_points[i].x,sub_Rcut[j],sub_mass[j],sub_beta,sub_x[j],Sigma_crit);

    			}
    		}
    	} // end of substructure

    	if(!kappa_off){
    		i_points[i].dt = 0.5*(i_points[i].image->x[0]*i_points[i].image->x[0]
    		                     + i_points[i].image->x[1]*i_points[i].image->x[1])
      		                     - i_points[i].dt;
    		i_points[i].dt *= to;
    	}

    	i_points[i].image->x[0] = i_points[i].x[0] - i_points[i].image->x[0];
    	i_points[i].image->x[1] = i_points[i].x[1] - i_points[i].image->x[1];

    	alpha[0]=alpha[1]=0.0;
    	gamma[0]=gamma[1]=gamma[2]=0.0;

    	if(stars_N > 0 && stars_implanted){
    		// add stars for microlensing
    		substract_stars_disks(i_points[i].x,i_points[i].image->x,
    				&(i_points[i].kappa),i_points[i].gamma);

    		// do stars with tree code
    		//star_tree->force2D(i_points[i].x,temp[i].alpha,&tmp,temp[i].gamma,true);
    		star_tree->force2D_recur(i_points[i].x,alpha,&tmp,gamma,true);

    		i_points[i].image->x[0] += convert_factor*alpha[0];
    		i_points[i].image->x[1] += convert_factor*alpha[1];

    		if(!kappa_off){
    			i_points[i].kappa += convert_factor*tmp;
    			i_points[i].gamma[0] += convert_factor*gamma[0];
    			i_points[i].gamma[1] += convert_factor*gamma[1];
    		}

    	} // end of stars


		// final operations to get the inverse magnification
		i_points[i].invmag = (1-i_points[i].kappa)*(1-i_points[i].kappa)
				- i_points[i].gamma[0]*i_points[i].gamma[0] - i_points[i].gamma[1]*i_points[i].gamma[1];

    	i_points[i].image->invmag=i_points[i].invmag;
    	i_points[i].image->kappa=i_points[i].kappa;
    	i_points[i].image->gamma[0]=i_points[i].gamma[0];
    	i_points[i].image->gamma[1]=i_points[i].gamma[1];
    	i_points[i].image->dt = i_points[i].dt;
    }

#ifdef _OPENMP
#pragma omp barrier
#endif

    return ;
}


/** \ingroup DeflectionL2
   *
   * \brief Routine for calculating the deflection and other lensing quantities for
   * a analytic one plane lens (AnaLens), for just one ray!!
   *
*/
void AnaLens::rayshooterInternal(double *ray, double *alpha, float *gamma, float *kappa, bool kappa_off){
     double x_rescale[2];
     long j;
     double alpha_tmp[2];
     float gamma_tmp[3], dt = 0,tmp = 0;

     gamma_tmp[0] = gamma_tmp[1] = gamma_tmp[2] = 0.0;
     alpha_tmp[0] = alpha_tmp[1] = 0.0;

     alpha[0] = alpha[1] = 0.0;
     gamma[0] = gamma[1] = gamma[2] = 0.0;
     *kappa = 0.0;

     double convert_factor = star_massscale/Sigma_crit;

     if(host_ro > 0){
    	 x_rescale[0] = ray[0]/host_ro;
    	 x_rescale[1] = ray[1]/host_ro;

    	 alphaNSIE(alpha,x_rescale,host_axis_ratio,host_core/host_ro,host_pos_angle);

    	 if(!kappa_off){
    		 gammaNSIE(gamma,x_rescale,host_axis_ratio,host_core/host_ro,host_pos_angle);
    		 *kappa=kappaNSIE(x_rescale,host_axis_ratio,host_core/host_ro,host_pos_angle);
    	 }

    	 alpha[0] *= host_ro;
    	 alpha[1] *= host_ro;
     }

  // perturbations of host lens
     if(perturb_Nmodes > 0){
    	 *kappa += lens_expand(perturb_beta,perturb_modes
    			 ,perturb_Nmodes,ray,alpha_tmp,gamma_tmp,&dt);

    	 alpha[0] += alpha_tmp[0];
    	 alpha[1] += alpha_tmp[1];

   	      if(!kappa_off){
   	    	  gamma[0] += gamma_tmp[0];
   	    	  gamma[1] += gamma_tmp[1];
   	      }

   	     gamma_tmp[0] = gamma_tmp[1] = gamma_tmp[2] = 0.0;
   	     alpha_tmp[0] = alpha_tmp[1] = 0.0;
     }

     // add substructure
     if(substruct_implanted){
    	 for(j=0;j<sub_N;++j){
    		 sub_alpha_func(alpha_tmp,ray,sub_Rcut[j],sub_mass[j],sub_beta,sub_x[j],Sigma_crit);

    		 alpha[0] += alpha_tmp[0];
    		 alpha[1] += alpha_tmp[1];

    		 if(!kappa_off){
    			 *kappa += sub_kappa_func(ray,sub_Rcut[j],sub_mass[j],sub_beta,sub_x[j],Sigma_crit);
    			 sub_gamma_func(gamma_tmp,ray,sub_Rcut[j],sub_mass[j],sub_beta,sub_x[j],Sigma_crit);

    			 gamma[0] += gamma_tmp[0];
    			 gamma[1] += gamma_tmp[1];
    		 }
    	 }

         gamma_tmp[0] = gamma_tmp[1] = gamma_tmp[2] = 0.0;
         alpha_tmp[0] = alpha_tmp[1] = 0.0;
     }

     /// TODO: MARGARITA/BEN make sure units here are OK!!!
     // add stars for microlensing
     if(stars_N > 0 && stars_implanted){

    	 std::cout << "MAke sure the units here are OK (the substract disk) , then delete this line" << std::endl;
    	 exit(1);

    	 substract_stars_disks(ray,alpha,kappa,gamma);

    	 // do stars with tree code
    	 //star_tree->force2D(ray,alpha_tmp,&tmp,gamma_tmp,true);
    	 star_tree->force2D_recur(ray,alpha_tmp,&tmp,gamma_tmp,true);

    	 alpha[0] += convert_factor*alpha_tmp[0];
    	 alpha[1] += convert_factor*alpha_tmp[1];

    	 if(!kappa_off){
    		 *kappa += convert_factor*tmp;
    		 gamma[0] += convert_factor*gamma_tmp[0];
    		 gamma[1] += convert_factor*gamma_tmp[1];
    	 }
     }

     // final operations on results
     convert_factor = 4*pi*Grav*Sigma_crit;

     // convert from physical distance on the lens plane to an angle
	 alpha[0] *= convert_factor;
	 alpha[1] *= convert_factor;

	 // in the multi-plane formalism G^i=partial deflection_angle^i / partial x^i
	 // therefore the quantities need to be in units (1/comoving_distance)
	 // --> convert from unitless quantity to (1/comoving_distance)
	 *kappa *= convert_factor/(1+zlens);
	 gamma[0] *= convert_factor/(1+zlens);
	 gamma[1] *= convert_factor/(1+zlens);
	 gamma[2] *= convert_factor/(1+zlens);

     return ;
}
