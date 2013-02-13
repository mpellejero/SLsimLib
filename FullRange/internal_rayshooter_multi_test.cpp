/*
 * internal_rayshooter_multi_test.cpp
 *
 *  Created on: Jan 28, 2013
 *      Author: mpetkova
 */

#include "slsimlib.h"

#ifndef N_THREADS
#define N_THREADS 1
#endif

void *compute_rays_parallel_halos(void *_p);


/**
 * A data structure for temporarily distribute the data amongst threads (for the halo ray shooter).
 */
struct TmpParamsHalos{
  Point *i_points;
  double *Dl_halos;
  double *dDl_halos;
  double **halo_pos_Mpc;
  double *halo_zs;
  HaloStructure *halos;
  IndexType Nhalos;
  bool kappa_off;
  int tid;
  int start;
  int size;
  MultiLens *lens;
};

/*
 * A method that calculates the relative error between the lensing properties on the image plane,
 * by considering all lensing halos as single objects and the previously calculated properties
 * from the standard rayshooter.
 */
void MultiLens::rayshooterInternal_halos_diff(
		unsigned long Npoints   /// number of points to be shot
		,Point *i_points        /// point on the image plane
		,bool kappa_off         /// turns calculation of convergence and shear off to save time.
		,double *Dl_halos
		,double *dDl_halos
		){

  if(Npoints == 0) return;

  int nthreads, rc;

  nthreads = N_THREADS;

  int chunk_size;
  do{
    chunk_size = (int)Npoints/nthreads;
    if(chunk_size == 0) nthreads /= 2;
  }while(chunk_size == 0);

  pthread_t threads[nthreads];
  TmpParamsHalos *thread_params = new TmpParamsHalos[nthreads];

  for(int i=0; i<nthreads;i++){
    thread_params[i].i_points = i_points;
    thread_params[i].Dl_halos = Dl_halos;
    thread_params[i].dDl_halos = dDl_halos;
    thread_params[i].halo_pos_Mpc = halo_pos_Mpc;
    thread_params[i].halo_zs = halo_zs;
    thread_params[i].halos = halos;
    thread_params[i].Nhalos = Nhalos;
    thread_params[i].kappa_off = kappa_off;
    thread_params[i].size = chunk_size;
    if(i == nthreads-1)
      thread_params[i].size = (int)Npoints - (nthreads-1)*chunk_size;
    thread_params[i].start = i*chunk_size;
    thread_params[i].tid = i;
    thread_params[i].lens = this;
    rc = pthread_create(&threads[i], NULL, compute_rays_parallel_halos, (void*) &thread_params[i]);
    assert(rc==0);
  }

  for(int i = 0; i < nthreads; i++){
    rc = pthread_join(threads[i], NULL);
    assert(rc==0);
  }

  delete[] thread_params;
}


void *compute_rays_parallel_halos(void *_p){
	  TmpParamsHalos *p = (TmpParamsHalos *) _p;
	  bool kappa_off = p->kappa_off;
	  MultiLens *lens = p->lens;
	  int tid        = p->tid;
	  int chunk_size = p->size;
	  int start      = p->start;
	  int end        = start + chunk_size;

	  int i, j;

	  double xx[2],xcm[2],fac;
	  double aa,bb,cc;
	  double alpha[2];
	  KappaType kappa,gamma[3];
	  double xminus[2],xplus[2];
	  double kappa_minus,gamma_minus[3],kappa_plus,gamma_plus[3];

	  double new_image_x[2];
	  KappaType new_kappa,new_gamma[3],new_invmag;

	  for(i = start; i< end; i++){

		  if(p->i_points[i].in_image == MAYBE)
				 continue;

	    // find position on first lens plane in comoving units
	    new_image_x[0] = p->i_points[i].x[0]*p->Dl_halos[0];
	    new_image_x[1] = p->i_points[i].x[1]*p->Dl_halos[0];

	    xminus[0] = 0;
	    xminus[1] = 0;

	    // Set magnification matrix on first plane.  Also the default if kappa_off == false
	    kappa_minus = 0;
	    gamma_minus[0] = 0;
	    gamma_minus[1] = 0;
	    gamma_minus[2] = 0;

	    new_kappa = 1;  // This is actually 1-kappa until after the loop through the planes.
	    new_gamma[0] = 0;
	    new_gamma[1] = 0;
	    new_gamma[2] = 0;

	    for(j = 0; j < p->Nhalos ; j++){  // each iteration leaves i_point[i].image on plane (j+1)

	      // convert to physical coordinates on the plane j

	      xx[0] = new_image_x[0]/(1+p->halo_zs[j]);
	      xx[1] = new_image_x[1]/(1+p->halo_zs[j]);

	      assert(xx[0] == xx[0] && xx[1] == xx[1]);

	      xcm[0] = p->halo_pos_Mpc[j][0] - xx[0];
	      xcm[1] = p->halo_pos_Mpc[j][1] - xx[1];

	      lens->halo_tree[0]->force_halo_external(
	    			alpha
	    			,&kappa
	    			,gamma
	    			,xcm
	    			,p->halos[j].mass
	    			,p->halos[j].Rmax
	    			,p->halos[j].rscale
	    			,kappa_off);

	      assert(alpha[0] == alpha[0] && alpha[1] == alpha[1]);

	      if(!kappa_off){
	    	  fac = 1/(1+p->halo_zs[j]);
	    	  /* multiply by fac to obtain 1/comoving_distance/physical_distance
	    	   * such that a multiplication with the charge (in units of physical distance)
	    	   * will result in a 1/comoving_distance quantity */
	    	  kappa*=fac;
	    	  gamma[0]*=fac;
	    	  gamma[1]*=fac;
	    	  gamma[2]*=fac;

	    	  assert(gamma[0] == gamma[0] && gamma[1] == gamma[1]);
	    	  assert(kappa == kappa);
	      }

	      if(lens->flag_switch_deflection_off)
	    	  alpha[0] = alpha[1] = 0.0;

	      aa = (p->dDl_halos[j+1]+p->dDl_halos[j])/p->dDl_halos[j];
	      bb = p->dDl_halos[j+1]/p->dDl_halos[j];
    	  cc = lens->charge*p->dDl_halos[j+1];

	      xplus[0] = aa*new_image_x[0] - bb*xminus[0] - cc*alpha[0];
	      xplus[1] = aa*new_image_x[1] - bb*xminus[1] - cc*alpha[1];

	      xminus[0] = new_image_x[0];
	      xminus[1] = new_image_x[1];

	      new_image_x[0] = xplus[0];
	      new_image_x[1] = xplus[1];

	      if(!kappa_off){

	    	  aa = (p->dDl_halos[j+1]+p->dDl_halos[j])*p->Dl_halos[j]/p->dDl_halos[j]/p->Dl_halos[j+1];

	    	  if(j>0){
	    		  bb = p->dDl_halos[j+1]*p->Dl_halos[j-1]/p->dDl_halos[j]/p->Dl_halos[j+1];
	    	  }
	    	  else
	    		  bb = 0;

	    	  if(lens->flag_input_lens && j == (lens->flag_input_lens % lens->Nplanes))
	    		  cc = p->dDl_halos[j+1]*p->Dl_halos[j]/p->Dl_halos[j+1];
	    	  else
	    		  cc = lens->charge*p->dDl_halos[j+1]*p->Dl_halos[j]/p->Dl_halos[j+1];

	    	  kappa_plus = aa*new_kappa - bb*kappa_minus
	    			  - cc*(kappa*new_kappa + gamma[0]*new_gamma[0] + gamma[1]*new_gamma[1]);

	    	  gamma_plus[0] = aa*new_gamma[0] - bb*gamma_minus[0]
	    	          - cc*(gamma[0]*new_kappa + kappa*new_gamma[0] - gamma[1]*new_gamma[2]);

	    	  gamma_plus[1] = aa*new_gamma[1] - bb*gamma_minus[1]
	    	          - cc*(gamma[1]*new_kappa + kappa*new_gamma[1] + gamma[0]*new_gamma[2]);

	    	  gamma_plus[2] = aa*new_gamma[2] - bb*gamma_minus[2]
	    	          - cc*(kappa*new_gamma[2] - gamma[1]*new_gamma[0] + gamma[0]*new_gamma[1]);

	    	  kappa_minus = new_kappa;
	    	  gamma_minus[0] = new_gamma[0];
	    	  gamma_minus[1] = new_gamma[1];
	    	  gamma_minus[2] = new_gamma[2];

	    	  assert(kappa_plus==kappa_plus && gamma_minus[0]==gamma_minus[0] &&gamma_minus[1]==gamma_minus[1] && gamma_minus[2]==gamma_minus[2]);

	    	  new_kappa = kappa_plus;
	    	  new_gamma[0] = gamma_plus[0];
	    	  new_gamma[1] = gamma_plus[1];
	    	  new_gamma[2] = gamma_plus[2];

	      }
	    }

	    // Convert units back to angles.
	    new_image_x[0] /= p->Dl_halos[p->Nhalos];
	    new_image_x[1] /= p->Dl_halos[p->Nhalos];

	    new_kappa = 1 - new_kappa;

	    if(!kappa_off) new_invmag = (1-new_kappa)*(1-new_kappa)
			     - new_gamma[0]*new_gamma[0]
			     - new_gamma[1]*new_gamma[1]
			     + new_gamma[2]*new_gamma[2];
	    else new_invmag = 0.0;

	    if(new_image_x[0] != new_image_x[0] ||
	       new_image_x[1] != new_image_x[1] ||
	       new_invmag != new_invmag){
	      ERROR_MESSAGE();
	      std::cout << new_image_x[0] << "  " << new_image_x[1] << "  " << new_invmag << std::endl;
	      std::cout << new_gamma[0] << "  " << new_gamma[1] << "  " << new_gamma[2] << "  " <<
	    		  new_kappa << "  "  << kappa_off << std::endl;
	      //	assert(0);
	      exit(1);
	    }

	    p->i_points[i].image->x[0] = p->i_points[i].image->x[0]/new_image_x[0] - 1.0;
	    p->i_points[i].image->x[1] = p->i_points[i].image->x[1]/new_image_x[1] - 1.0;
	    p->i_points[i].kappa = p->i_points[i].kappa/new_kappa - 1.0;
	    p->i_points[i].gamma[0] = p->i_points[i].gamma[0]/new_gamma[0] - 1.0;
	    p->i_points[i].gamma[1] = p->i_points[i].gamma[1]/new_gamma[1] - 1.0;
	    p->i_points[i].gamma[2] = p->i_points[i].gamma[2]/new_gamma[2] - 1.0;
	    p->i_points[i].invmag = p->i_points[i].invmag/new_invmag - 1.0;

	  }

	  return 0;

	}