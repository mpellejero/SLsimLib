/*
 * internal_rayshooter_multi.cpp
 *
 *  Created on: Jan 19, 2012
 *      Author: mpetkova, bmetcalf
 */

#include "slsimlib.h"

/** \ingroup DeflectionL2
 *
 * \brief This is the function that does the deflection calculation with multiple lens planes.
 *
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
 * charge = 4*pi*G*mass_scale/c^2 in units of Mpc // Fabien : we should remove this ?
 * charge = 4*pi*G in units of physical Mpc
 *
 * i_points[].x[] is in angular units.
 *
 * Warning: Is not valid for a non-flat universe.
 */

#ifndef N_THREADS
#define N_THREADS 1
#endif

void *compute_rays_parallel(void *_p);

/**
 * \brief A data structure for temporarily distribute the data amongst threads.
 */
struct TmpParams{
  Point *i_points;
  int tid;
  int start;
  int size;
  int NPlanes;
  bool flag_switch_deflection_off;
  bool flag_switch_lensing_off;
  PosType charge;
  LensPlane** lensing_planes;
  PosType* plane_redshifts;
  PosType* Dl;
  PosType* dDl;
};

/** \brief This function calculates the deflection, shear, convergence, rotation
 and time-delay of rays in parallel.
 */
void Lens::rayshooterInternal(
                              unsigned long Npoints   /// number of points to be shot
                              ,Point *i_points        /// point on the image plane
){
  
  // To force the computation of convergence, shear... -----
  // -------------------------------------------------------
  
  int NLastPlane;
  PosType tmpDs,tmpdDs,tmpZs;
  
  // If there are no points to shoot, then we quit.
  if(Npoints == 0) return;
  
  // If a lower redshift source (compared to the farthest lens plane) is being used
  if(toggle_source_plane)
  {
    NLastPlane = index_of_new_sourceplane;
    
    assert(NLastPlane <= lensing_planes.size());
    tmpDs = Dl[index_of_new_sourceplane];
    tmpdDs = dDl[index_of_new_sourceplane];
    tmpZs = plane_redshifts[index_of_new_sourceplane];
    
    Dl[index_of_new_sourceplane] = Ds_implant;
    dDl[index_of_new_sourceplane] = dDs_implant;
    plane_redshifts[index_of_new_sourceplane] = zs_implant;
  }
  else{ NLastPlane = lensing_planes.size(); }
  
  
  // For refining the grid and shoot new rays.
  int nthreads, rc;
  nthreads = N_THREADS;
  
  int chunk_size;
  do{
    chunk_size = (int)Npoints/nthreads;
    if(chunk_size == 0) nthreads /= 2;
  }while(chunk_size == 0);
  
  pthread_t threads[nthreads];
  TmpParams *thread_params = new TmpParams[nthreads];
  
  // This is for multi-threading :
  for(int i=0; i<nthreads;i++)
  {
    thread_params[i].i_points = i_points;
    thread_params[i].size = chunk_size;
    if(i == nthreads-1)
      thread_params[i].size = (int)Npoints - (nthreads-1)*chunk_size;
    thread_params[i].start = i*chunk_size;
    thread_params[i].tid = i;
    thread_params[i].flag_switch_deflection_off = flag_switch_deflection_off;
    thread_params[i].flag_switch_lensing_off = flag_switch_lensing_off;
    thread_params[i].charge = charge;
    thread_params[i].lensing_planes = &lensing_planes[0];
    thread_params[i].plane_redshifts = &plane_redshifts[0];
    thread_params[i].Dl = &Dl[0];
    thread_params[i].dDl = &dDl[0];
    thread_params[i].NPlanes = NLastPlane;
    rc = pthread_create(&threads[i], NULL, compute_rays_parallel, (void*) &thread_params[i]);
    assert(rc==0);
  }
  
  for(int i = 0; i < nthreads; i++)
  {
    rc = pthread_join(threads[i], NULL);
    assert(rc==0);
  }
  
  delete[] thread_params;
  
  if(toggle_source_plane)
  {
    // The initial values for the plane are reset here
    Dl[index_of_new_sourceplane] = tmpDs;
    dDl[index_of_new_sourceplane] = tmpdDs;
    plane_redshifts[index_of_new_sourceplane] = tmpZs;
  }
  
}



void *compute_rays_parallel(void *_p)
{
  TmpParams *p = (TmpParams *) _p;
  int chunk_size = p->size;
  int start      = p->start;
  int end        = start + chunk_size;
  
  int i, j;
  
  PosType xx[2],fac;
  PosType aa,bb,cc;
  PosType alpha[2];
  
  KappaType kappa,gamma[3];
  
  PosType xminus[2],xplus[2];
  PosType kappa_minus,gamma_minus[3],kappa_plus,gamma_plus[3];
  
  KappaType phi;
  
  // Main loop : loop over the points of the image
  for(i = start; i < end; i++)
  {
    
    // In case e.g. a temporary point is outside of the grid.
    if(p->i_points[i].in_image == MAYBE) continue;
    
    // find position on first lens plane in comoving units
    p->i_points[i].image->x[0] = p->i_points[i].x[0] * p->Dl[0];
    p->i_points[i].image->x[1] = p->i_points[i].x[1] * p->Dl[0];
    
    xminus[0] = 0;
    xminus[1] = 0;
    
    // Set magnification matrix on first plane.
    kappa_minus = 0;
    gamma_minus[0] = 0;
    gamma_minus[1] = 0;
    gamma_minus[2] = 0;
    
    // Setting phi on the first plane.
    phi = 0.0;
    
    // Default values :
    p->i_points[i].kappa = 1;  // This is actually 1-kappa until after the loop through the planes.
    p->i_points[i].gamma[0] = 0;
    p->i_points[i].gamma[1] = 0;
    p->i_points[i].gamma[2] = 0;
    p->i_points[i].dt = 0;
    
    // In case we don't want to compute the values :
    if(p->flag_switch_lensing_off)
    {
      p->i_points[i].image->x[0] /= p->Dl[0];
      p->i_points[i].image->x[1] /= p->Dl[0];
      p->i_points[i].kappa = p->i_points[i].image->kappa = 0.0;
      p->i_points[i].invmag = 1.0;
      p->i_points[i].dt = 0.0;
      
      continue;
    }
    
    
    /* ************************************************************************************
     We compute deflection, shear, convergence, rotation
     and time-delay of rays in parallel.
     ************************************************************************************ */
    
    // Time delay at first plane : position on the observer plane is (0,0) => no need to take difference of positions.
    p->i_points[i].dt = 0.5*( p->i_points[i].image->x[0]*p->i_points[i].image->x[0] + p->i_points[i].image->x[1]*p->i_points[i].image->x[1] )/ p->dDl[0] ;

    // Begining of the loop through the planes :
    // Each iteration leaves i_point[i].image on plane (j+1)
    for(j = 0; j < p->NPlanes ; ++j)
    {
      
      // convert to physical coordinates on the plane j
      xx[0] = p->i_points[i].image->x[0]/(1+p->plane_redshifts[j]);
      xx[1] = p->i_points[i].image->x[1]/(1+p->plane_redshifts[j]);
      // PhysMpc = ComMpc / (1+z)
      
      assert(xx[0] == xx[0] && xx[1] == xx[1]);
      
      // std::cout << "p->i_points[i].image->x[0] = " << p->i_points[i].image->x[0] << " , p->i_points[i].image->x[1] = " << p->i_points[i].image->x[1] << std::endl ;
      
      ////////////////////////////////////////////////////////////////
      
      p->lensing_planes[j]->force(alpha,&kappa,gamma,&phi,xx); // Computed in physical coordinates. // xx is in PhysMpc
      
      ////////////////////////////////////////////////////////////////
      
      assert(alpha[0] == alpha[0] && alpha[1] == alpha[1]);
      assert(gamma[0] == gamma[0] && gamma[1] == gamma[1]);
      assert(kappa == kappa);
      if(isinf(kappa)) { std::cout << "xx = " << xx[0] << " " << xx[1] << std::endl ;}
      assert(!isinf(kappa));
      
      fac = 1/(1+p->plane_redshifts[j]);
      /* multiply by fac to obtain 1/comoving_distance/physical_distance
       * such that a multiplication with the charge (in units of physical distance)
       * will result in a 1/comoving_distance quantity */ // 1 / comoving_distance squared ?
      kappa *= fac;
      gamma[0] *= fac;
      gamma[1] *= fac;
      gamma[2] *= fac;
      
      assert(gamma[0] == gamma[0] && gamma[1] == gamma[1]);
      assert(kappa == kappa);
      assert(phi == phi);
      
      if(p->flag_switch_deflection_off){ alpha[0] = alpha[1] = 0.0; }
      
      
      // This computes \vec{x}^{j+1} in terms of \vec{x}^{j} , \vec{x}^{j-1} and \vec{\alpha}^{j}
      // according to Eq. (19) of paper GLAMER II -----------------------------------------------
      
      aa = (p->dDl[j+1] + p->dDl[j])/p->dDl[j]; // (Dls*(1+zs) + Dl*(1+zl)) / Dl*(1+zl) = 1 + (Dls/Dl)*((1+zs)/(1+zl)) ;
      bb = p->dDl[j+1]/p->dDl[j];               // (Dls/Dl)*((1+zs)/(1+zl)) ;
      cc = p->charge * p->dDl[j+1];             // 4 pi G * Dls * (1+zs) ;
      
      assert(aa == aa);
      assert(bb == bb);
      assert(cc == cc);
      
      xplus[0] = aa*p->i_points[i].image->x[0] - bb*xminus[0] - cc*alpha[0];
      xplus[1] = aa*p->i_points[i].image->x[1] - bb*xminus[1] - cc*alpha[1];
      // x (should be) in physical Mpc, cc*alpha in (PhysMpc/mass)*(mass/Mpc)=1/(1+z).
        
      // For the test with a source at the position (0,0) centered on the lens :
      // std::cout << "alpha we have at the end of force_halo : " << alpha[0] << " " << alpha[1] << std::endl ;
      // std::cout << "alpha we should have at the end of force_halo : " << aa*p->i_points[i].image->x[0] / cc << " " << aa*p->i_points[i].image->x[1] / cc << std::endl << std::endl ;

      xminus[0] = p->i_points[i].image->x[0];
      xminus[1] = p->i_points[i].image->x[1];
      
      // Change in the value of the position.
      p->i_points[i].image->x[0] = xplus[0];
      p->i_points[i].image->x[1] = xplus[1];
      
      // std::cout << "In rayshooter : !!! " << p->i_points[i].image->x[0] / p->Dl[p->NPlanes] << " " << p->i_points[i].image->x[1] / p->Dl[p->NPlanes] << " !!!" << std::endl ; // This step is done after !
      // ----------------------------------------------------------------------------------------
      
      
      // This computes (\kappa^{j+1}, \gamma_1^{j+1}, \gamma_2^{j+1}, \gamma_3^{j+1})
      // in terms of the j-plane quantities and according to Eq. (22) of GLAMER II.
      
      // Here the coefficients aa, bb and cc are used for a completely different calculation,
      // they are not the same as they were defined above. ----------------------------------
      aa = (p->dDl[j+1] + p->dDl[j]) * p->Dl[j] / p->dDl[j] / p->Dl[j+1];
      if(j>0)
      {
        bb = p->dDl[j+1] * p->Dl[j-1] / p->dDl[j] / p->Dl[j+1];
      }
      else bb = 0;
      cc = p->charge * p->dDl[j+1] * p->Dl[j] / p->Dl[j+1];
      // ------------------------------------------------------------------------------------
      
      
      // Computation of the "plus quantities", i.e. the  next plane quantities --------------------
      kappa_plus = aa*p->i_points[i].kappa - bb*kappa_minus
      - cc*(kappa*p->i_points[i].kappa + gamma[0]*p->i_points[i].gamma[0] + gamma[1]*p->i_points[i].gamma[1]);
      
      gamma_plus[0] = aa*p->i_points[i].gamma[0] - bb*gamma_minus[0]
      - cc*(gamma[0]*p->i_points[i].kappa + kappa*p->i_points[i].gamma[0] - gamma[1]*p->i_points[i].gamma[2]);
      
      gamma_plus[1] = aa*p->i_points[i].gamma[1] - bb*gamma_minus[1]
      - cc*(gamma[1]*p->i_points[i].kappa + kappa*p->i_points[i].gamma[1] + gamma[0]*p->i_points[i].gamma[2]);
      
      gamma_plus[2] = aa*p->i_points[i].gamma[2] - bb*gamma_minus[2]
      - cc*(kappa*p->i_points[i].gamma[2] - gamma[1]*p->i_points[i].gamma[0] + gamma[0]*p->i_points[i].gamma[1]);
      // ------------------------------------------------------------------------------------------

      // std::cout << std::endl ;
      // std::cout << "RayshooterInternal : kappa_minus = " << kappa_minus << " , kappa = " << kappa << " , gamma[0] = " << gamma[0] << " , gamma[1] = " << gamma[1] << std::endl ;
      // std::cout << "RayshooterInternal : aa = " << aa << " , bb = " << bb << " , cc = " << cc << std::endl ;
      // std::cout << "RayshooterInternal : p->points[i] : kappa = " << p->i_points[i].kappa << " , gamma[0] = " << p->i_points[i].gamma[0] << " , gamma[1] = " << p->i_points[i].gamma[1] << std::endl ;

      // std::cout << "RayshooterInternal : plane " << j << " , z = " << p->plane_redshifts[j] << " , 1-kappa_plus = " << 1-kappa_plus << std::endl ;
        
      // Assigning them to the "minus quantities" for next plane occurence of the loop ------------
      kappa_minus = p->i_points[i].kappa;
      gamma_minus[0] = p->i_points[i].gamma[0];
      gamma_minus[1] = p->i_points[i].gamma[1];
      gamma_minus[2] = p->i_points[i].gamma[2];
      // ------------------------------------------------------------------------------------------
      
      
      if(!(kappa_plus==kappa_plus && gamma_minus[0]==gamma_minus[0] && gamma_minus[1]==gamma_minus[1] && gamma_minus[2]==gamma_minus[2])){
        std::cout << "p->dDl[j-1]" << "\t" << "p->dDl[j]" << "\t" << "p->dDl[j+1]" << std::endl;
        std::cout << p->dDl[j-1] << "\t" << p->dDl[j] << "\t" << p->dDl[j+1] << std::endl;
        std::cout << "aa" << "\t" << "bb" << "\t" << "cc" << std::endl;
        std::cout << aa << "\t" << bb << "\t" << cc << std::endl;
        
        
        std::cout << "kappa" << "\t" << "gamma[0]" << "\t" << "gamma[1]" << "\t" << "gamma[2]" << std::endl ;
        std::cout << kappa << "\t" << gamma[0] << "\t" << gamma[1] << "\t" << gamma[2] << std::endl ;
        
        
        std::cout << "p->i_points[i].kappa" << "\t" << "p->i_points[i].gamma[0]" << "\t" << "p->i_points[i].gamma[1]" << "\t" << "p->i_points[i].gamma[2]" << std::endl ;
        std::cout << p->i_points[i].kappa << "\t" << p->i_points[i].gamma[0] << "\t" << p->i_points[i].gamma[1] << "\t" << p->i_points[i].gamma[2] << std::endl ;
        
        std::cout << "x" << "\t" << "y" << std::endl;
        std::cout << p->i_points[i].image->x[0] << "\t" << p->i_points[i].image->x[1] << std::endl;
        std::cout << "kappa_minus" << "\t" << "gamma_minus[0]" << "\t" << "gamma_minus[1]" << "\t" << "gamma_minus[2]" << "\t" << std::endl ;
        std::cout << kappa_minus << "\t" << gamma_minus[0] << "\t" << gamma_minus[1] << "\t" << gamma_minus[2] << "\t" << std::endl ;
        std::cout << "kappa_plus" << "\t" << "gamma_plus[0]" << "\t" << "gamma_plus[1]" << "\t" << "gamma_plus[2]" << "\t" << std::endl ;
        std::cout << kappa_plus << "\t" << gamma_plus[0] << "\t" << gamma_plus[1] << "\t" << gamma_plus[2] << "\t" << std::endl ;
      }
      
      assert(kappa_plus==kappa_plus && gamma_minus[0]==gamma_minus[0] && gamma_minus[1]==gamma_minus[1] && gamma_minus[2]==gamma_minus[2]);
      
      
      // Updating the point quantities ----------------
      p->i_points[i].kappa = kappa_plus;
      p->i_points[i].gamma[0] = gamma_plus[0];
      p->i_points[i].gamma[1] = gamma_plus[1];
      p->i_points[i].gamma[2] = gamma_plus[2];
      // ----------------------------------------------
      
      
      // Geometric time delay with added potential
      p->i_points[i].dt += 0.5*( (xplus[0] - xminus[0])*(xplus[0] - xminus[0]) + (xplus[1] - xminus[1])*(xplus[1] - xminus[1]) )/p->dDl[j+1] - (1 + p->plane_redshifts[j]) * phi * p->charge ; /// in Mpc
      
      // Check that the 1+z factor must indeed be there (because the x positions have been rescaled, so it may be different compared to the draft).
      // Remark : Here the true lensing potential is not "phi" but "phi * p->charge = phi * 4 pi G".
      
      
      // std::cout << "Comp dist : " << p->Dl[p->NPlanes] << " " << p->dDl[j+1] << std::endl ;
      
    } // End of the loop going through the planes
    
    
    // Subtracting off a term that makes the unperturbed ray to have zero time delay
    p->i_points[i].dt -= 0.5*( p->i_points[i].image->x[0]*p->i_points[i].image->x[0] + p->i_points[i].image->x[1]*p->i_points[i].image->x[1] ) / p->Dl[p->NPlanes];
    
    
    // Convert units back to angles.
    p->i_points[i].image->x[0] /= p->Dl[p->NPlanes];
    p->i_points[i].image->x[1] /= p->Dl[p->NPlanes];
    
    // std::cout << "   Ds = " << p->Dl[p->NPlanes] << std::endl;
    // std::cout << "   dx = " << p->i_points[i].image->x[0]-p->i_points[i].x[0] << "  " << p->i_points[i].image->x[1]-p->i_points[i].x[1] << std::endl;
    
    // We go from kappa denoting 1-kappa to kappa denoting kappa
    p->i_points[i].kappa = 1 - p->i_points[i].kappa;
    
    // std::cout << "RayshooterInternal : kappa final = " << p->i_points[i].kappa << std::endl ;
    // std::cout << std::endl ;
      
    // Computation of the inverse magnitude --------------------------------------------------------
    p->i_points[i].invmag = (1-p->i_points[i].kappa)*(1-p->i_points[i].kappa)
    - p->i_points[i].gamma[0]*p->i_points[i].gamma[0]
    - p->i_points[i].gamma[1]*p->i_points[i].gamma[1]
    + p->i_points[i].gamma[2]*p->i_points[i].gamma[2];
    // ---------------------------------------------------------------------------------------------
    
    
    // Conversion of dt from Mpc (physical Mpc) to Years (also possible into days) -----------------
    
    p->i_points[i].dt *= MpcToSeconds * SecondToYears ;
    
    // ---------------------------------------------------------------------------------------------
    
    
    
    // Putting the final values of the quantities in the real image point -----
    p->i_points[i].image->invmag = p->i_points[i].invmag;
    p->i_points[i].image->kappa = p->i_points[i].kappa;
    p->i_points[i].image->gamma[0] = p->i_points[i].gamma[0];
    p->i_points[i].image->gamma[1] = p->i_points[i].gamma[1];
    p->i_points[i].image->gamma[2] = p->i_points[i].gamma[2];
    p->i_points[i].image->dt = p->i_points[i].dt;
    // ------------------------------------------------------------------------
    
    
    
    
    /*
     // TODO: check
     if(p->i_points[i].image->x[0] != p->i_points[i].image->x[0] ||
     p->i_points[i].image->x[1] != p->i_points[i].image->x[1] ||
     p->i_points[i].invmag != p->i_points[i].invmag)
     {
     ERROR_MESSAGE();
     std::cout << p->i_points[i].image->x[0] << "  " << p->i_points[i].image->x[1] << "  " << p->i_points[i].invmag << std::endl;
     std::cout << p->i_points[i].gamma[0] << "  " << p->i_points[i].gamma[1] << "  " << p->i_points[i].gamma[2] << "  " <<
     p->i_points[i].kappa << std::endl;
     //	assert(0);
     exit(1);
     }
     */
    
  } // End of the main loop.
  
  return 0;
  
}
/** \brief Collects information about the halos and kappa contributions along the light path
 
 Information on the nearest halos is collected where nearest is defined by rmax and mode.
 When mode == 2 the unlensed angular coordinates are used to evaluate proximity not the lensed ones.
 
 */
void Lens::info_rayshooter(
                           Point *i_point     /// point to be shot, must have image point linked
                           ,std::vector<std::vector<double>> & ang_positions  /// angular positions on each plane
                           ,std::vector<KappaType> & kappa_on_planes          /// convergence on each plane
                           ,std::vector<std::vector<LensHalo*>> & halo_neighbors  /// neighboring halos within rmax of ray on each plane
                           ,LensHalo *halo_max
                           ,KappaType &kappa_max
                           ,KappaType gamma_max[]
                           ,PosType rmax  /// distance from ray on each plane, units depend on mode parameter
                           ,short mode  /// 0:physical distance (Mpc), 1: comoving distance (Mpc), 2: angular distance (rad)
)
{
  
  
  // !!! would like to find the maximum contributing halo so that its contribution can be subtracted from the total
  
  int NLastPlane = lensing_planes.size() ;
  
  PosType tmpDs,tmpdDs,tmpZs;
  
  // If there are no points to shoot, then we quit.
  
  // If a lower redshift source (compared to the farthest lens plane) is being used
  if(toggle_source_plane)
  {
    NLastPlane = index_of_new_sourceplane;
    
    assert(NLastPlane <= lensing_planes.size());
    tmpDs = Dl[index_of_new_sourceplane];
    tmpdDs = dDl[index_of_new_sourceplane];
    tmpZs = plane_redshifts[index_of_new_sourceplane];
    
    Dl[index_of_new_sourceplane] = Ds_implant;
    dDl[index_of_new_sourceplane] = dDs_implant;
    plane_redshifts[index_of_new_sourceplane] = zs_implant;
  }
  else
  {
    NLastPlane = lensing_planes.size();
    tmpDs = cosmo.angDist(zsource);
    assert(plane_redshifts[NLastPlane] == zsource);
    tmpdDs = cosmo.angDist(plane_redshifts[NLastPlane-1], zsource);
    tmpZs = zsource ;
  }
  
  
  ang_positions.resize(NLastPlane);
  for(int ii=0;ii<NLastPlane;++ii) ang_positions[ii].resize(2);
  halo_neighbors.resize(NLastPlane);
  kappa_on_planes.resize(NLastPlane);
  
  int j;
  
  PosType xx[2];
  PosType aa,bb,cc;
  PosType alpha[2];
  
  PosType xminus[2],xplus[2],tmp_r,x_tmp[2];
  KappaType gamma[3],phi,kappa_tmp;
  kappa_max = -1.0;
  
  // find position on first lens plane in comoving units
  i_point->image->x[0] = i_point->x[0] * Dl[0];
  i_point->image->x[1] = i_point->x[1] * Dl[0];
  
  xminus[0] = 0;
  xminus[1] = 0;
  
  // Begining of the loop through the planes :
  // Each iteration leaves i_point[i].image on plane (j+1)
  
  for(j = 0; j < NLastPlane ; ++j)
  {
    
    // convert to physical coordinates on the plane j
    xx[0] = i_point->image->x[0]/(1+plane_redshifts[j]);
    xx[1] = i_point->image->x[1]/(1+plane_redshifts[j]);
    
    assert(xx[0] == xx[0] && xx[1] == xx[1]);
    
    lensing_planes[j]->force(alpha,&kappa_on_planes[j],gamma,&phi,xx); // Computed in physical coordinates.
    if(flag_switch_deflection_off){ alpha[0] = alpha[1] = 0.0; }
    
    tmp_r = rmax;
    if(mode == 1) tmp_r /= (1+plane_redshifts[j]);
    if(mode == 2) tmp_r *= Dl[j];
    lensing_planes[j]->getNeighborHalos(xx,tmp_r,halo_neighbors[j]);
    
    ang_positions[j][0] = i_point->image->x[0]/Dl[j];
    ang_positions[j][1] = i_point->image->x[1]/Dl[j];
    
    PosType SigmaCrit = cosmo.SigmaCrit(plane_redshifts[j],tmpZs);
    
    // Find the halo with the largest kappa
    for(int ii=0;ii<halo_neighbors[j].size();++ii){
      alpha[0] = alpha[1] = 0.0;
      kappa_tmp = 0.0;
      gamma[0] = gamma[1] = gamma[2] = 0.0;
      phi = 0.0;
      
      // Getting the halo position (in physical Mpc) :
      halo_neighbors[j][ii]->getX(x_tmp);
      
      // Taking the shift into account :
      x_tmp[0] = xx[0] - x_tmp[0];
      x_tmp[1] = xx[1] - x_tmp[1];
      
      halo_neighbors[j][ii]->force_halo(alpha,&kappa_tmp,gamma,&phi,x_tmp,false);
      kappa_tmp /=SigmaCrit;
      if(kappa_tmp > kappa_max){
        kappa_max = kappa_tmp;
        halo_max = halo_neighbors[j][ii];
        gamma_max[0] = gamma[0]/SigmaCrit;
        gamma_max[1] = gamma[1]/SigmaCrit;
      }
    }
    
    //kappa_on_planes[j] *= 1/(1+plane_redshifts[j]);
    kappa_on_planes[j] /= SigmaCrit;
    
    assert(dDl[j] != 0);
    
    aa = (dDl[j+1] + dDl[j])/dDl[j];
    bb = dDl[j+1]/dDl[j];
    cc = charge * dDl[j+1];
    
    xplus[0] = aa*i_point->image->x[0] - bb*xminus[0] - cc*alpha[0];
    xplus[1] = aa*i_point->image->x[1] - bb*xminus[1] - cc*alpha[1];
    
    xminus[0] = i_point->image->x[0];
    xminus[1] = i_point->image->x[1];
    
    
    // Change in the value of the position.
    i_point->image->x[0] = xplus[0];
    i_point->image->x[1] = xplus[1];
    
    
  } // End of the loop going through the planes
  
  // Convert units back to angles.
  i_point->image->x[0] /= Dl[NLastPlane];
  i_point->image->x[1] /= Dl[NLastPlane];
  
  return;
  
}
