//
//  lightcone_construction.h
//  GLAMER
//
//  Created by Ben Metcalf on 26/10/16.
//
//

#ifndef __GLAMER__lightcone_construction__
#define __GLAMER__lightcone_construction__

#include <stdio.h>
#include "point.h"
#include "lens_halos.h"
#include "geometry.h"


/*** \brief Class for constructing light cones form simulation boxes.
*
*
 */
class LightCone{
public:
  LightCone(double angular_radius);
  
  struct DataRockStar{
    unsigned int id;
    Point_3d x;
    double mass;
    double Rvir;     // in comoving units
    double Rscale;   // in comoving units
    double Vmax;
  };
  
  //void ReadBoxRockStar(std::string filename,Point_3d xo,Point_3d V
  //                                ,double rlow,double rhigh
  //                                ,std::vector<DataRockStar> &conehalos
  //                     ,bool periodic_boundaries = true
  //                     ,bool allow_subhalos = false);
  
  static void ReadLightConeNFW(std::string filename,COSMOLOGY &cosmo
                               ,std::vector<LensHalo* > &lensVec
                               ,PosType &theta_max);
  
  static void ReadLightConeParticles(std::string filename,COSMOLOGY &cosmo
                                     ,std::vector<LensHaloParticles *> &lensVec
                                     ,int Nplanes
                                     ,float particle_mass
                                     ,float particle_size
                                     ,bool angular_sizes = false
                                     ,bool verbose = false);

  static void ReadLightConeParticles(std::string filename,COSMOLOGY &cosmo
                                     ,std::vector<LensHaloMassMap *> &lensVec
                                     ,int Nplanes
                                     ,float particle_mass
                                     ,float angular_resolution
                                     ,bool verbose = false);
  
  static void WriteLightCone(std::string filename,std::vector<DataRockStar> &vec);
  static void WriteLightCone(std::string filename,std::vector<Point_3d> &vec);
  
  friend class MultiLightCone;
  
  template <typename T>
  void select(Point_3d xo,Point_3d v,double Length,double rlow,double rhigh
              ,T* begin
              ,T* end
              ,Utilities::LockableContainer<std::vector<T> > &incone
              ,bool periodic_boundaries = true);
  
private:

  double r_theta; // angular radius of light cone (radians)
  double sin_theta_sqrt;
};

class MultiLightCone{
public:
  MultiLightCone(
                 double angular_radius
                 ,const std::vector<Point_3d> &observers    /// postion of observers within the simulation box
                 ,const std::vector<Point_3d> &directions   /// direction of light cones
                 ):
  xos(observers),vs(directions)
  {
    assert(observers.size() == directions.size());
    for(int i=0;i<observers.size(); ++i) cones.push_back(angular_radius);
  }
  
  /** Read the points in from a snapshot of the halos created by RockStar.
   The output will be the halos in the cone in coordinates where the x-axis is
   the center of the cone.  Changing the observer, xo, and direction of view, V,
   effectively translates and rotates the box, respectively.  Only halos with radii
   between rlow and rhigh are added.
   */
  void ReadBoxRockStar(std::string filename
                       ,double rlow,double rhigh
                       ,std::vector<std::vector<LightCone::DataRockStar> > &conehalos
                       ,bool periodic_boundaries = true
                       ,bool allow_subhalos = false);
  
  void ReadBoxXYZ(std::string filename
                  ,double rlow,double rhigh
                  ,std::vector<std::vector<Point_3d> > &conehalos
                  ,double hubble
                  ,double BoxLength
                  ,bool periodic_boundaries = true
                  );
  
  void ReadBoxToMaps(std::string filename
                     ,double rlow,double rhigh
                     ,std::vector<std::vector<PixelMap>  > &maps
                     ,double hubble
                     ,double BoxLength
                     ,bool periodic_boundaries = true
                     );
  
  

private:

  std::vector<Point_3d> xos;
  std::vector<Point_3d> vs;
  std::vector<LightCone> cones;
};


class MultiPlaneLightCones{
  
public:
  MultiPlaneLightCones(COSMOLOGY &cosmo,int Nplanes,float max_redshift,double BoxLength,double hubble
                       ,std::vector<std::string> snap_filenames
                       ,std::vector<float> snap_redshifts
                       ,std::vector<std::vector<PixelMap> > &maps
                       ,const std::vector<Point_3d> &observers    /// postion of observers within the simulation box
                       ,const std::vector<Point_3d> &directions   /// direction of light cones
                       ,double range
                       ,double resolution
                       ,bool angular = true
                       ,bool verbose = false
                       ){

    // set coordinate distances for planes
    double d_max = cosmo.coorDist(max_redshift);
    d_planes.resize(Nplanes);
    for(int i = 0 ; i < Nplanes ; ++i ) d_planes[i] = (2*i+1)*d_max/Nplanes/2;

    maps.clear();
    Point_2d center;
    double angular_resolution = resolution;
    // allocate mamory for all maps
    maps.resize(Nplanes);
    for(auto &map_v : maps){  // loop through cones
      map_v.reserve(Nplanes);
      for(int i=0;i<Nplanes;++i){
        if(!angular) angular_resolution = resolution/d_planes[i];
        map_v.emplace_back(center.x
                                   ,(size_t)(range/angular_resolution)
                                   ,(size_t)(range/angular_resolution)
                                   ,angular_resolution);
      }
    }
    
    // find unique redshifts
    std::vector<double> z_unique(1,snap_redshifts[0]);
    for(auto z : snap_redshifts) if(z != z_unique.back() ) z_unique.push_back(z);
    
    // find redshift ranges for each snapshot
    
    
    // loop through files
    for(int i_file=0 ; i_file < snap_filenames.size() ; ++i_file){
      
      //open file
      if(verbose) std::cout <<" Opening " << snap_filenames[i_file] << std::endl;
      std::ifstream file(snap_filenames[i_file].c_str());
      if(!file){
        std::cout << "Can't open file " << snap_filenames[i_file] << std::endl;
        ERROR_MESSAGE();
        throw std::runtime_error(" Cannot open file.");
      }
    

      // read header ??
      // find r range using snap_redshifts
      // loop lines / read
        // loop cones
          // see if in cone
          // find nearest plane
          // find pixel
          // add mass or distribute mass to pixels
      // close file
      file.close();
    }
    
  };
  
  void OutputConeData(std::string basefilename);
  static void InportConeData(std::string basefile,std::vector<PixelMap> &mass_maps);
  
private:
  std::vector<double> d_planes;
};

/// select the halos from the box that are within the light cone
template <typename T>
void LightCone::select(Point_3d xo,Point_3d v,double Length,double rlow,double rhigh
            ,T* begin
            ,T* end
            ,Utilities::LockableContainer<std::vector<T> > &incone
            ,bool periodic_boundaries){
  
  if(begin == end) return;
  
  Point_3d dx,x;
  double r2,xp;
  Point_3d n;
  double rhigh2 = rhigh*rhigh;
  double rlow2 = rlow*rlow;
  T b;
  
  v /= v.length();
  
  // standard coordinates so cone is always centered on the x-axis
  Point_3d y_axis,z_axis;
  
  y_axis[0] = -v[1];
  y_axis[1] =  v[0];
  y_axis[2] =  0;
  
  y_axis /= y_axis.length();
  z_axis = v.cross(y_axis);
  
  //std::cout << v*y_axis << " " << v*z_axis << " " << y_axis*z_axis << std::endl;
  
  // we can do better than this !!!!
  int n0[2] = {(int)((rhigh + xo[0])/Length),(int)((xo[0] - rhigh)/Length) - 1 };
  int n1[2] = {(int)((rhigh + xo[1])/Length),(int)((xo[1] - rhigh)/Length) - 1 };
  int n2[2] = {(int)((rhigh + xo[2])/Length),(int)((xo[2] - rhigh)/Length) - 1 };
  if(!periodic_boundaries){
    n0[0] = n0[1] = 0;
    n1[0] = n1[1] = 0;
    n2[0] = n2[1] = 0;
  }
  for(auto a = begin ; a != end ; ++a){
    dx[0] = (*a).x[0] - xo[0];
    dx[1] = (*a).x[1] - xo[1];
    dx[2] = (*a).x[2] - xo[2];
    
    int count = 0;
    
    for(n[0] = n0[1]; n[0] <= n0[0] ; ++n[0]){
      for(n[1] = n1[1]; n[1] <= n1[0] ; ++n[1]){
        for(n[2] = n2[1]; n[2] <= n2[0] ; ++n[2]){
          
          x = dx + n*Length;
          xp = x*v;
          if(xp > 0){
            r2 = x.length_sqr();
            
            if(r2 > rlow2 && r2 < rhigh2){
              if( r2*sin_theta_sqrt > r2 - xp*xp){
                //std::cout << n << " | " << xo << " | "
                //          << v << " |  " << x << std::endl;
                
                b = *a;
                //b.x = x;
                // rotate to standard reference frame
                b.x[0] = xp;
                b.x[1] = y_axis*x;
                b.x[2] = z_axis*x;
                
                //std::cout << "b = " << b.x << std::endl;
                
                incone.push_back(b);
                ++count;
              }
            }
          }
        }
      }
    }
    if(count > 1) std::cout << " Warning: Same halo appears " << count << " times in light-cone." << std::endl;
  }
}

#endif /* defined(__GLAMER__lightcone_construction__) */
