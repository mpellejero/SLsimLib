/*
 * halo_model.cpp
 *
 *  Created on: Jan 14, 2012
 *      Author: mpetkova
 */

#include <slsimlib.h>
#include <sstream>
#include <string>
#include <utilities.h>
#include <source.h>
#include <sourceAnaGalaxy.h>

using namespace std;

const long NTABLE = 1000;
const double maxz = 5.0;

HaloData::HaloData(CosmoHndl cosmo
		,double mass
		,double zlens){
	allocation_flag = true;
	kappa_background = 0.0;
	Nhalos = 1;
	halos = new HaloStructure[Nhalos];
	pos = PosTypeMatrix(0,Nhalos-1,0,2);

	HALO *ha = new HALO(cosmo,mass,zlens);

	int i;
	for(i = 0; i < Nhalos; i++){
		pos[i][0] = 0.0;
		pos[i][1] = 0.0;
		pos[i][2] = 0.0;

		halos[i].mass = mass;
		halos[i].Rmax = ha->getRvir()*cosmo->gethubble();
		halos[i].rscale = halos[i].Rmax/ha->getConcentration(0);

		cout << endl << halos[i].mass  << " " << halos[i].Rmax << " " << halos[i].rscale << endl;
	}

	delete ha;
}

HaloData::HaloData(HaloStructure *halostrucs,PosType **positions,unsigned long Nhaloss):
	pos(positions), halos(halostrucs), Nhalos(Nhaloss)
{
	allocation_flag = false;
	kappa_background = 0.0;  //TODO This should be set properly at some point.
}

/**
 *  \brief In this constructor the halos are created from a mass function and given random positions on the lens plane
 */

HaloData::HaloData(
		double fov            /// field of view in square degrees
		,double min_mass      /// Minimum mass of a halo
		,double mass_scale    /// mass scale
		,double z1            /// lowest redshift
		,double z2            /// highest redshift
		,int mass_func_type   /// mass function type: 0 Press-Schechter, 1 Sheth-Tormen, 2 Power Law
		,double alpha		/// slope of the Power Law
		,CosmoHndl cosmo     /// cosmology
		,long *seed
	)
	{

	allocation_flag = true;

    HALO *ha = new HALO(cosmo,min_mass,(z1+z2)/2);

	// calculate the mass density on the plane
    // and convert it to physical 1/physical_distance^2
	kappa_background = ha->totalMassDensityinHalos(0,1,alpha)*pow(cosmo->gethubble(),2)*pow(1+(z1+z2)/2,3);
	kappa_background *= cosmo->getOmega_matter()*cosmo->angDist(z1,z2)/mass_scale;

    //int iterator;
    //std::vector<double> vmasses,vsizes,vscale,vz;
    std::vector<int> vindex;
	std::vector<double> Logm,Nhalosbin;
	std::vector<double> Dli;

    Logm.resize(Nmassbin);
    Nhalosbin.resize(Nmassbin);

	/* fill the log(mass) vector */

	fill_linear(Logm,Nmassbin,min_mass,MaxLogm);

	double Nhaloestot;

	Nhalosbin[0] = cosmo->haloNumberDensityOnSky(pow(10,Logm[0])/cosmo->gethubble(),z1,z2,mass_func_type,alpha)*fov;

	Nhaloestot = Nhalosbin[0];
	Nhalosbin[0] = 1;
	int k;

#ifdef _OPENMP
#pragma omp parallel for
#endif
	for(k=1;k<Nmassbin;k++){
		double m = pow(10,Logm[k])/cosmo->gethubble();
		// cumulative number density in one square degree
		Nhalosbin[k] = cosmo->haloNumberDensityOnSky(m,z1,z2,mass_func_type,alpha)*fov;
		// normalize the cumulative distribution to one
		Nhalosbin[k] = Nhalosbin[k]/Nhaloestot;
	}

#ifdef _OPENMP
#pragma omp barrier
#endif

	Nhalos = (long)(poidev(float(Nhaloestot), seed) + 0.5);

	halos = new HaloStructure[Nhalos];
	pos = PosTypeMatrix(0,Nhalos-1,0,2);
	double rr,theta,maxr,zi;
	unsigned long i;
	for(i = 0; i < Nhalos; i++){
		zi = z1+(z2-z1)*ran2 (seed);
		Dli.push_back(cosmo->angDist(0,zi));
		maxr = pi*sqrt(fov/pi)/180*Dli[i]; // fov is a circle
		rr = maxr*sqrt(ran2(seed));
		theta = 2*pi*ran2(seed);
		pos[i][0] = rr*cos(theta);
		pos[i][1] = rr*sin(theta);

		halos[i].mass = pow(10,InterpolateYvec(Nhalosbin,Logm,ran2 (seed)));
		ha->reset(halos[i].mass,zi);
		halos[i].mass /= mass_scale;
		//halos[i].mass = vmasses[i];
		//halos[i].Rmax = vsizes[i];
		halos[i].Rmax = ha->getRvir()*cosmo->gethubble();
		//halos[i].rscale = vsizes[i]/vscale[i]; // get the Rscale=Rmax/c
		halos[i].rscale = halos[i].Rmax/ha->getConcentration(0); // get the Rscale=Rmax/c
		pos[i][2] = 0.0;//halos[i].Rmax;
	}

	delete ha;
}

HaloData::~HaloData(){

	if(allocation_flag){
		free_PosTypeMatrix(pos,0,Nhalos-1,0,2);
		delete[] halos;
	}
}


double *MultiLens::coorDist_table = NULL;
long MultiLens::ob_count = 0;

void MultiLens::make_table(CosmoHndl cosmo){
	int i;
	double x, dx = maxz/(double)NTABLE;

	coorDist_table = new double[NTABLE];

#ifdef _OPENMP
#pragma omp parallel for
#endif
	for(i = 0 ; i< NTABLE; i++){
		x = (i+1)*dx;
		coorDist_table[i] = cosmo->coorDist(0,x);
	}

#ifdef _OPENMP
#pragma omp barrier
#endif
}

/*
 * \ingroup Constructor
 * allocates space for the halo trees and the inout lens, if there is any
 */
MultiLens::MultiLens(string filename,long *my_seed) : Lens(){
	readParamfile(filename);

	// flag to determine if halos are created randomly or read in from a external simulation.
	if(input_sim_file.size() < 1) sim_input_flag = false;
	else sim_input_flag = true;

	std::cout << input_sim_file.c_str() << std::endl;

	if(input_gal_file.size() < 1) gal_input_flag = false;
	else gal_input_flag = true;

	std::cout << input_gal_file.c_str() << std::endl;

	plane_redshifts = new double[Nplanes];
	Dl = new double[Nplanes];
	dDl = new double[Nplanes];

	charge = 4*pi*Grav*mass_scale;

	//halo_tree = new auto_ptr<ForceTree>[Nplanes-1];
	halo_tree = new auto_ptr<QuadTree>[Nplanes-1];
	halodata = new auto_ptr<HaloData>[Nplanes-1];

	switch(flag_input_lens){
	case ana_lens:
		input_lens = new AnaLens(filename);
		analens = static_cast<AnaLens*>(input_lens);
		break;
	case moka_lens:
#ifdef ENABLE_FITS
		input_lens = new MOKALens(filename);
		mokalens = static_cast<MOKALens*>(input_lens);
		fieldofview = pow(1.5*mokalens->map->boxl*180/pi,2.0);
#else
		cout << "Enable the CCFITS handling first" << endl;
		exit(1);
#endif
		break;
	}

	flag_implanted_source = 0;
	seed = my_seed;

	// In the case of implanted sources from a file initialize.
	cout << "In MultiLens" << endl;
	if(gal_input_flag) anasource = auto_ptr<MultiSourceAnaGalaxy>(new MultiSourceAnaGalaxy(input_gal_file));

}

MultiLens::~MultiLens(){

	delete[] halo_tree;
	for(int j=0;j<Nplanes-1;j++) delete &(halo_tree[j]);
	delete[] Dl;
	delete[] plane_redshifts;
	delete[] dDl;
	for(int j=0;j<Nplanes-1;j++) delete &(halodata[j]);
	delete[] halodata;

	if(sim_input_flag){  // Otherwise these deallocations are done in the destructor of halodata's
		delete[] halos;
		delete[] halo_zs;
		free_PosTypeMatrix(halo_pos,0,Nhalos-1,0,2);
	}

	if(flag_input_lens)
		delete input_lens;

	--ob_count;
	if(ob_count == 0){
		// remove tables for the coordiante distances
		delete[] coorDist_table;
	}
}

void MultiLens::readParamfile(string filename){
      const int MAXPARAM = 50;
	  string label[MAXPARAM], rlabel, rvalue;
	  void *addr[MAXPARAM];
	  int id[MAXPARAM];
	  stringstream ss;
	  int i ,n;
	  int myint;
	  double mydouble;
	  string mystring;
	  char dummy[100];
	  string escape = "#";
	  int flag;

	  n = 0;

	  /// id[] = 2 = string, 1 = int, 0 = double
	  addr[n] = &outputfile;
	  id[n] = 2;
	  label[n++] = "outputfile";

	  addr[n] = &Nplanes;
	  id[n] = 1;
	  label[n++] = "Nplanes";

	  addr[n] = &min_mass;
	  id[n] = 0;
	  label[n++] = "min_mass";

	  addr[n] = &mass_scale;
	  id[n] = 0;
	  label[n++] = "mass_scale";

	  addr[n] = &flag_input_lens;
	  id[n] = 1;
	  label[n++] = "flag_input_lens";

	  addr[n] = &fieldofview;
	  id[n] = 0;
	  label[n++] = "fov";

	  addr[n] = &mass_func_type;
	  id[n] = 1;
	  label[n++] = "mass_func_type";

	  addr[n] = &internal_profile;
	  id[n] = 1;
	  label[n++] = "internal_profile";

	  addr[n] = &input_sim_file;
	  id[n] = 2;
	  label[n++] = "input_simulation_file";

	  addr[n] = &input_gal_file;
	  id[n] = 2;
	  label[n++] = "input_galaxy_file";

	  addr[n] = &pw_alpha;
	  id[n] = 0;
	  label[n++] = "alpha";

	  addr[n] = &pw_beta;
	  id[n] = 0;
	  label[n++] = "internal_slope_pw";

	  addr[n] = &pnfw_beta;
	  id[n] = 0;
	  label[n++] = "internal_slope_pnfw";

	  addr[n] = &flag_switch_deflection_off;
	  id[n] = 1;
	  label[n++] = "deflection_off";


	  cout << "Multi lens: reading from " << filename << endl;

	  ifstream file_in(filename.c_str());
	  if(!file_in){
	    cout << "Can't open file " << filename << endl;
	    exit(1);
	  }

	  // output file
	  while(!file_in.eof()){
		  file_in >> rlabel >> rvalue;
		  file_in.getline(dummy,100);

		  if(rlabel[0] == escape[0])
			  continue;

		  flag = 0;

		  for(i = 0; i < n; i++){
			  if(rlabel == label[i]){

				  flag = 1;
				  ss << rvalue;

				  switch(id[i]){
				  case 0:
					  ss >> mydouble;
					  *((double *)addr[i]) = mydouble;
					  break;
				  case 1:
					  ss >> myint;
					  *((int *)addr[i]) = myint;
					  break;
				  case 2:
					  ss >> mystring;
					  *((string *)addr[i]) = mystring;
					  break;
				  }

				  ss.clear();
				  ss.str(string());

				  id[i] = -1;
			  }
		  }
	  }

	  for(i = 0; i < n; i++){
		  if(id[i] >= 0 && addr[i] != &input_sim_file && addr[i] != &input_gal_file &&
				  addr[i] != &pw_alpha && addr[i] != &pw_beta && addr[i] != &pnfw_beta &&
				  addr[i] != &flag_switch_deflection_off){
			  ERROR_MESSAGE();
			  cout << "parameter " << label[i] << " needs to be set in the parameter file " << filename << endl;
			  exit(0);
		  }

		  /// DEFAULT VALUES
		  /// in case they are not set in the parameter file
		  if(id[i] >= 0 && addr[i] == &pw_alpha){
			  pw_alpha = 1./6.;
		  }
		  if(id[i] >= 0 && addr[i] == &pw_beta){
			  pw_beta = -1.0;
		  }
		  if(id[i] >= 0 && addr[i] == &pnfw_beta){
			  pnfw_beta = 2.0;
		  }
		  if(id[i] >= 0 && addr[i] == &flag_switch_deflection_off){
			  flag_switch_deflection_off = 0; //false, deflection is on
		  }
	  }

	  file_in.close();

	  if(pw_beta >= 0){
		  ERROR_MESSAGE();
		  cout << "Internal slope >=0 not possible." << endl;
		  exit(1);
	  }

	  if(pnfw_beta <= 0){
		  ERROR_MESSAGE();
		  cout << "Internal slope <=0 not possible." << endl;
		  exit(1);
	  }

	  if(pnfw_beta / floor(pnfw_beta) > 1.0){
		  ERROR_MESSAGE();
		  cout << "Internal slope needs to be a whole number." << endl;
		  exit(1);
	  }

	  // to compensate for the last plane, which is the source plane
	  Nplanes++;

	  // to compenstate for additional lens planes
	  if(flag_input_lens)
		  Nplanes++;

	  // convert to square degrees
	  fieldofview /= 3600. * 3600.;

	  printMultiLens();
}



void MultiLens::printMultiLens(){
	cout << endl << "outputfile " << outputfile << endl;

	cout << endl << "**multi lens model**" << endl;

	cout << "Nplanes " << Nplanes << endl;

	cout << "mass scale " << mass_scale << endl;

	cout << "min mass " << min_mass << endl;

	cout << "flag input lens " << flag_input_lens << endl;
	switch(flag_input_lens){
	case null:
		cout << "  No input lens specified " << endl;
		break;
	case ana_lens:
		cout << "  AnaLens " << endl;
		break;
	case moka_lens:
		cout << "  MOKALens " << endl;
		break;
	}

	cout << "field of view " << fieldofview << endl;

	cout << "internal profile type " << internal_profile << endl;
	switch(internal_profile){
	case PowerLaw:
		cout << "  Power law internal profile " << endl;
		cout << "  slope: " << pw_beta << endl;
		break;
	case NFW:
		cout << "  NFW internal profile " << endl;
		break;
	case PseudoNFW:
		cout << "  Pseudo NFW internal profile " << endl;
		cout << "  slope: " << pnfw_beta << endl;
		break;
	}

	cout << "mass function type " << mass_func_type << endl;
	switch(mass_func_type){
		case PS:
			cout << "  Press-Schechter mass function " << endl;
			break;
		case ST:
			cout << "  Sheth-Tormen mass function " << endl;
			break;
		case PL:
			cout << "  Power law mass function " << endl;
			cout << "  slope: " << pw_alpha << endl;
			break;
		}

	cout << endl;
}

/**
 * Populates the planes with halos by dividing the space around the planes into
 * equal redshift distances, where the plane with the input lens is excluded
 * since it will not contain any halos
 *
 * Then the halo trees are built, depending on the internal profile model that
 * has been chosen in the parameter file
 */
void MultiLens::buildHaloTrees(
		CosmoHndl cosmo /// the cosmology
		,double zsource /// the source resdhift
		){
	int i, j, Ntot;
	double z1, z2;

	if(!sim_input_flag){   /// If no input file is provided synthesize halos

		for(j=0,Ntot=0;j<Nplanes-1;j++){
			if(flag_input_lens && j == (flag_input_lens % Nplanes))
				continue;

			/*
			 * Setting the redshift range
			 * If there is a plane with an input lens on it, it is skipped over
			 * since it will not contain any halos
			 */
			if(j == 0) z1 = 0.0;
			else z1 = plane_redshifts[j] - 0.5*(plane_redshifts[j] - plane_redshifts[j-1]);

			if(j-1 == (flag_input_lens % Nplanes)) z1 = plane_redshifts[j] - 0.5*(plane_redshifts[j] - plane_redshifts[j-2]);

			if(j == Nplanes-2) z2 = zsource;
			else z2 = plane_redshifts[j] + 0.5*(plane_redshifts[j+1] - plane_redshifts[j]);

			if(j+1 == (flag_input_lens % Nplanes)) z2 = plane_redshifts[j] + 0.5*(plane_redshifts[j+2] - plane_redshifts[j]);

			//halodata[j] = new HaloData(fieldofview,min_mass,z1,z2,mass_func_type,cosmo,seed);
			halodata[j] = auto_ptr<HaloData>(new HaloData(fieldofview,min_mass,mass_scale,z1,z2,mass_func_type,pw_alpha,cosmo,seed));

			Ntot+=halodata[j]->Nhalos;
		}

	}else{
		// If input file is provided, read it in and put the halos onto lens planes.

		readInputSimFile(cosmo);
		unsigned long j1,j2;

		for(j=0,Ntot=0;j<Nplanes-1;j++){
			if(flag_input_lens && j == (flag_input_lens % Nplanes))
				continue;

			/*
			 * Setting the redshift range
			 * If there is a plane with an input lens on it, it is skipped over
			 * since it will not contain any halos
			 */
			if(j == 0) z1 = 0.0;
			else z1 = plane_redshifts[j] - 0.5*(plane_redshifts[j] - plane_redshifts[j-1]);

			if(j-1 == (flag_input_lens % Nplanes)) z1 = plane_redshifts[j] - 0.5*(plane_redshifts[j] - plane_redshifts[j-2]);

			if(j == Nplanes-2) z2 = zsource;
			else z2 = plane_redshifts[j] + 0.5*(plane_redshifts[j+1] - plane_redshifts[j]);

			if(j+1 == (flag_input_lens % Nplanes)) z2 = plane_redshifts[j] + 0.5*(plane_redshifts[j+2] - plane_redshifts[j]);

			/// Find which halos are in redshift range

			locateD(halo_zs-1,Nhalos,z1,&j1);
			if(j1 == Nhalos) j1 = Nhalos-1;
			locateD(halo_zs-1,Nhalos,z2,&j2);
			if(j2 == Nhalos) j2 = Nhalos-1;

			/// Use other constructor to create halo data

			//halodata[j] = new HaloData(&halos[j1],&halo_pos[j1],j2-j1);
			halodata[j] = auto_ptr<HaloData>(new HaloData(&halos[j1],&halo_pos[j1],j2-j1));

			//for(int i = 0; i<10 ;++i) cout << "Rmax:" << halos[j1+i].Rmax << "mass:" << halos[j1+i].mass << "rscale:" << halos[j1+i].rscale << "x = " << halo_pos[j1+i][0] << " " << halo_pos[j1+i][1] << endl;

			Ntot += halodata[j]->Nhalos;
		}

	}

	for(j=0;j<Nplanes-1;j++){
		if(flag_input_lens && j == (flag_input_lens % Nplanes))
			continue;

		switch(internal_profile){
		case PowerLaw:
			//halo_tree[j] = auto_ptr<ForceTree>(new ForceTreePowerLaw(1.9,&halodata[j]->pos[0],halodata[j]->Nhalos
			//		,halodata[j]->halos,true,halodata[j]->kappa_background));
			halo_tree[j] = auto_ptr<QuadTree>(new QuadTreePowerLaw(pw_beta,&halodata[j]->pos[0],halodata[j]->Nhalos
							,halodata[j]->halos,halodata[j]->kappa_background));
			break;
		case NFW:
			//halo_tree[j] = auto_ptr<ForceTree>(new ForceTreeNFW(&halodata[j]->pos[0],halodata[j]->Nhalos
			//		,halodata[j]->halos,true,halodata[j]->kappa_background));
			halo_tree[j] = auto_ptr<QuadTree>(new QuadTreeNFW(&halodata[j]->pos[0],halodata[j]->Nhalos
							,halodata[j]->halos,halodata[j]->kappa_background));
			break;
		case PseudoNFW:
			//halo_tree[j] = auto_ptr<ForceTree>(new ForceTreePseudoNFW(2,&halodata[j]->pos[0],halodata[j]->Nhalos
			//		,halodata[j]->halos,true,halodata[j]->kappa_background));
			halo_tree[j] = auto_ptr<QuadTree>(new QuadTreePseudoNFW(pnfw_beta,&halodata[j]->pos[0],halodata[j]->Nhalos
							,halodata[j]->halos,halodata[j]->kappa_background));
			break;
		default:
			cout << "There is no such case for the halo trees." << endl;
			ERROR_MESSAGE();
			exit(1);
			break;
		}

	}


	cout << "constructed " << Ntot << " halos" << endl;

	/*
	stringstream f;
	f << "halos_" << zsource << ".data";
	string filename = f.str();
	ofstream file_area(filename.c_str());
	if(!file_area){
		cout << "unable to create file " << filename << endl;
		exit(1);
	}

	for(j = 0; j < Nplanes-1; j++){

		if(flag_analens && j == (flag_analens % Nplanes))
			continue;

		for(i = 0; i < halodata[j]->Nhalos; i++){

			double fac = 180/pi/3600./Dl[j]*(1+plane_redshifts[j]);

			file_area << plane_redshifts[j] << " ";
			file_area << i << " " << halodata[j]->halos[i].mass << " " << fac*halodata[j]->halos[i].Rmax << " " << fac*halodata[j]->halos[i].rscale << " ";
			file_area << fac*halodata[j]->pos[i][0] << " " << fac*halodata[j]->pos[i][1] << endl;

		}
	}


	file_area.close();
	*/
	/*
	for(int l=0; l < 100; l++){
	double ray[2];

	double fac = l/100.;

	ray[0] = fac*sqrt(fieldofview);
	ray[1] = ray[0];

	ray[0] = ray[0]*pi/180.;
	ray[1] = ray[1]*pi/180.;

	double xx[2]={0.0,0.0};

	int halos;

	for(j = 0, halos = 0; j < Nplanes-1; j++){

		if(flag_analens && j == (flag_analens % Nplanes))
			continue;

		xx[0] = ray[0]*Dl[j]/(1+plane_redshifts[j]);
		xx[1] = ray[1]*Dl[j]/(1+plane_redshifts[j]);

		for(i = 0; i < halodata[j]->Nhalos; i++){
			double r2 = (halodata[j]->pos[i][0] - xx[0])*(halodata[j]->pos[i][0] - xx[0])
						+ (halodata[j]->pos[i][1] - xx[1])*(halodata[j]->pos[i][1] - xx[1]);

			if(r2 <= halodata[j]->halos[i].Rmax*halodata[j]->halos[i].Rmax)
				halos++;
		}

	}


	cout << "ray: x " << ray[0]*180/pi << " y " << ray[1]*180/pi << ", halos: " << halos/float(Ntot) << endl;
	}

	 */

	/*
	double xp,x,xo;
	double yp,y,yo;
	double alpha_x_one, alpha_x_two;
	double alpha_y_one, alpha_y_two;

	double fac_one = (Dl[Nplanes-1]-Dl[0])/Dl[Nplanes-1];
	double fac_two = (Dl[Nplanes-1]-Dl[1])/Dl[Nplanes-1];

	x=xo=0; y=yo=0;

	xp = x*Dl[0]/(1+redshift[0]);
	yp = y*Dl[0]/(1+redshift[0]);

	xp = haloModel[0]->pos[0][0]-xp;
	yp = haloModel[0]->pos[0][1]-yp;

	double r2 = xp*xp+yp*yp;

	double ratio = 0.5*r2/haloModel[0]->halos[0].Rmax/haloModel[0]->halos[0].Rmax;

	alpha_x_one = 4*Grav*haloModel[0]->halos[0].mass*xp*(exp(-ratio)-1)/r2;
	alpha_y_one = 4*Grav*haloModel[0]->halos[0].mass*yp*(exp(-ratio)-1)/r2;

	double fac = (Dl[1]-Dl[0])/Dl[1];

	// angle on the second plane
	x = xo - fac*alpha_x_one;
	y = yo - fac*alpha_y_one;

	xp = x*Dl[1]/(1+redshift[1]);
	yp = y*Dl[1]/(1+redshift[1]);

	xp = haloModel[1]->pos[0][0]-xp;
	yp = haloModel[1]->pos[0][1]-yp;

	r2 = xp*xp+yp*yp;

	ratio = 0.5*r2/haloModel[1]->halos[0].Rmax/haloModel[1]->halos[0].Rmax;

	alpha_x_two = 4*Grav*haloModel[1]->halos[0].mass*xp*(exp(-ratio)-1)/r2;
	alpha_y_two = 4*Grav*haloModel[1]->halos[0].mass*yp*(exp(-ratio)-1)/r2;

	x = xo - fac_one*alpha_x_one - fac_two*alpha_x_two;
	y = yo - fac_one*alpha_y_one - fac_two*alpha_y_two;

	cout << "0 " << alpha_x_one << " " << alpha_y_one << endl;
	cout << "1 " << alpha_x_two << " " << alpha_y_two << endl;
	cout << xo << " " << yo << " " << x << " " << y << endl << endl;
	*/

}

/**
 * Set the redshifts of the planes by mapping the correct
 * redshift by using the coordinate distance table
 */
void MultiLens::setRedshifts(){
	int i;
	/// assigns the redshifts and plugs in the input plane

	cout << "z: ";
	for(i=0; i<Nplanes; i++){
		plane_redshifts[i] = QuickFindFromTable(Dl[i]);
		cout << plane_redshifts[i] << " ";
	}
	cout << endl;
}

/**
 * Uses a quick bi-division search to find the index of the coordinate distance from the
 * table that matches the coordinate distance of the plane in interest and returns the
 * redshift, by averaging the two closest values of the redshift
 *
 * This need to be done, rather than a simple interpolation, since delta coordinate
 * distance in the table is not constant
 */
double MultiLens::QuickFindFromTable(double Dplane){
	int j, j_min, j_max, j_mean;

	j_max = NTABLE-1;
	j_min = 0;

	do{
		j_mean = int((j_max-j_min)/2.0 + j_min);

		if(Dplane > coorDist_table[j_mean]){
			j_min = j_mean;
		}
		else{
			j_max = j_mean;
		}
	}while(j_max - j_min > 1);

	/// return the redshift, adding +2 to the indexes since in make_tables() the indexing starts from 1, not 0
	return maxz/float(NTABLE)*0.5*(j_max+j_min+2);
}

/**
 * Set the coordinate distances of the planes by dividing the coordinate distance space into equal intervals
 * and then plugging the input plane inbetween
 *
 * After this flag_input_lens will hold the index of the plane it is on
 * In case it is on the first plane, it will hold the index Nplanes, to make
 * sure that it is not zero (i.e. not set)
 */
void MultiLens::setCoorDist(CosmoHndl cosmo, double zsource){
	std:: vector<double> lD;
	int i, Np;

	if(flag_input_lens)
		Np = Nplanes;
	else
		Np = Nplanes+1;

	double Ds = cosmo->coorDist(0,zsource);
	double Dlens = cosmo->coorDist(0,input_lens->getZlens());

	/// spaces lD equally up to the source, including 0 and Ds
	/// therefore we need Nplanes+1 values
	/// however, if there is an input plane, we will need Nplanes values, since the input plane will take up a value itself
	fill_linear(lD,Np,0.,Ds);

	/// puts the input plane first if the case
	int j=0, flag=0;
	if(flag_input_lens && Dlens < lD[1]){
		Dl[j] = Dlens;
		flag_input_lens = (InputLens)Nplanes;
		flag = 1;
		j++;
	}

	/// assigns the redshifts and plugs in the input plane
	for(i=1; i<Np; i++){
		Dl[j] = lD[i];

		if(flag_input_lens && flag == 0)
			if(Dlens > lD[i] && Dlens <= lD[i+1]){
				Dl[j] = lD[i];
				Dl[++j] = Dlens;
				flag_input_lens = (InputLens)j;
				flag = 1;
			}
		j++;
	}

	dDl[0] = Dl[0];  // distance between jth plane and the previous plane
	for(j = 1; j < Nplanes; j++){
		dDl[j] = Dl[j] - Dl[j-1]; // distance between jth plane and the previous plane
	}

	if(flag_input_lens)
		cout << "zlens " << input_lens->getZlens() << " on plane number " << (flag_input_lens % Nplanes) << endl;

	cout << "Dl: ";
	for(j = 0; j < Nplanes; j++)
		cout << Dl[j] << " ";
	cout << endl;

	cout << "dDl: ";
	for(j = 0; j < Nplanes; j++)
		cout << dDl[j] << " ";
	cout << endl << endl;

}

double MultiLens::getZlens(){
	if(flag_input_lens)
		return input_lens->getZlens();
	else
		return plane_redshifts[0];
}

void MultiLens::setZlens(double z){
	if(flag_input_lens)
		input_lens->setZlens(z);
	else{
		cout << "It is not possible to set the redshift of the input plane in this case" << endl;
		ERROR_MESSAGE();
	}
}

/// read in halos from a simulation file
void MultiLens::readInputSimFile(CosmoHndl cosmo){

	char c;
	double ra,dec,z,vmax,vdisp,r_halfmass;
	string strg;
	unsigned long i,j;
	unsigned long id,np;

	//int index;

	if(internal_profile != PowerLaw){
		std::cout << "The internal profile of the halos while using simulation input files must be a Power Law."
				<< std::endl << "Change this is parameter file" << std::endl;
		exit(1);
	}

	ifstream file_in(input_sim_file.c_str());
	if(!file_in){
		cout << "Can't open file " << input_sim_file << endl;
		exit(1);
	}

	// skip through header information in data file
	while(file_in.peek() == '#'){
		file_in.ignore(10000,'\n');
		++i;
	}
	std::cout << "skipped "<< i << " comment lines in " << input_sim_file << std::endl;

	//file_in >> Nhalos;
	//Nhalos = 10;
	//cout << Nhalos << endl;

	std::vector<HaloStructure> halo_vec;
	std::vector<double> halo_zs_vec;
	std::vector<double *> halo_pos_vec;

	// read in data
	double mass_max=0,R_max=0,V_max=0;
	double *theta;
	HaloStructure halo;

	for(i=0,j=0 ; c != '#'; ++i){

		// read a line of data
		file_in >> id >> c >> id >> c >> ra >> c >> dec >> c >> z
				 >> c >> np >> c >> vdisp >> c >> vmax >> c >> r_halfmass >> c;  //TODO the GalID will miss the first digit using this method.  No other method stops at the end of file.
		//std::cout << id << c << id << c << ra << c << dec << c << z
		//				 << c << np << c << vdisp << c << vmax << c << r_halfmass << std::endl;
		//cout << i << "  z: " << z << " np: " << np << " vmax:" << vmax << "  " << file_in.peek() << endl;

		if(np > 0.0 && vdisp > 0.0){

			halo_vec.push_back(halo);

			halo_vec[j].mass = np*8.6e8/cosmo->gethubble();
			halo_vec[j].Rmax = halo_vec[j].mass*4.7788e-20/2/pow(vdisp/2.9979e5,2);  // SIS value
			halo_vec[j].rscale = halo_vec[j].Rmax;   //TODO This is a kluge.  It should no be necessary to remember to do this whenever the PowerLaw model is used.

			if(halo_vec[j].mass > mass_max) mass_max = halo_vec[j].mass;
			if(halo_vec[j].Rmax > R_max) R_max = halo_vec[j].Rmax;
			if(vdisp > V_max) V_max = vdisp;
			/*
			halo_vec[j].mass = mass*1.0e10*cosmo->gethubble();
			halo_vec[j].Rmax = cosmo->R200(z,mass*1.0e10*cosmo->gethubble());
			assert(halo_vec[j].Rmax > 0.0);
			cout << "Rmax:" << halo_vec[j].Rmax << endl;
			halo_vec[j].rscale = halo_vec[j].Rmax/cosmo->NFW_Concentration(vmax,halo_vec[j].mass,halo_vec[j].Rmax);
			 */

			halo_zs_vec.push_back(z);

			halo_vec[j].mass /= mass_scale;

			theta = new double[2];
			theta[0] = ra;
			theta[1] = dec;
			halo_pos_vec.push_back(theta);

			++j;
		}
	}
	file_in.close();
	std::cout << halo_vec.size() << " halos read in."<< std::endl
			<< "Max input mass = " << mass_max << "  R max = " << R_max << "  V max = " << V_max << std::endl;

	Nhalos = halo_vec.size();

	halos = new HaloStructure[Nhalos];
	halo_zs = new double[Nhalos];
	halo_pos = PosTypeMatrix(0,Nhalos-1,0,2);


	for(i=0;i<Nhalos;++i){
		halo_zs[i] = halo_zs_vec[i];
		halo_pos[i] = halo_pos_vec[i];
		halos[i] = halo_vec[i];
	}

	cout << halos[9].Rmax << endl;

	// sort the halos by readshift
	MultiLens::quicksort(halos,halo_pos,halo_zs,Nhalos);

}


/** Sets the internal parameters of the multiple lens model
 * first the redshifts of the planes are calculated
 * then the coordinate distances to the different planes
 * the planes are populated by halos and the halo trees are built
 * and finally the internal parameters of the input plane are set, in case there is one
 */
void MultiLens::setInternalParams(CosmoHndl cosmo, SourceHndl source){

	int j;

	if(flag_input_lens)
		input_lens->setInternalParams(cosmo,source);

	if( (cosmo->getOmega_matter() + cosmo->getOmega_lambda()) != 1.0 ){
		printf("ERROR: MultiLens can only handle flat universes at present.  Must change cosmology.\n");
		exit(1);
	}

	/// makes the oordinate distance table for the calculation of the redshifts of the different planes
	if(ob_count == 0) make_table(cosmo);
	setCoorDist(cosmo,source->getZ());
	setRedshifts();

	buildHaloTrees(cosmo,source->getZ());
	std:: cout << " done " << std:: endl;
}

/** \brief Read in information from a Virgo Millennium Data Base http://gavo.mpa-garching.mpg.de/MyMillennium/
 *
 * query select * from MockLensing.dbo.m21_20_39_021_bc03_Ben_halos
 *
 * This is information on the dark matter halos only.  There are 13 entries in each line separated by commas.
 * The comments must be removed from the beginning of the data file and the total number of halos must be added
 * as the first line.
 */
/// Sort halos[] and brr[][] by content off arr[]
void MultiLens::quicksort(HaloStructure *halos,double **brr,double *arr,unsigned long N){
	double pivotvalue;
	unsigned long pivotindex,newpivotindex,i;

	if(N <= 1) return ;

	// pick pivot as the median of the first, last and middle values
	if ((arr[0] >= arr[N/2] && arr[0] <= arr[N-1])
			|| (arr[0] >= arr[N-1] && arr[0] <= arr[N/2])) pivotindex = 0;
	else if ((arr[N/2] >= arr[0] && arr[N/2] <= arr[N-1])
			|| (arr[N/2] >= arr[N-1] && arr[N/2] <= arr[0])) pivotindex = N/2;
	else pivotindex = N-1;
	pivotvalue=arr[pivotindex];

	// move pivet to end of array
	swap(&arr[pivotindex],&arr[N-1]);
	//SwapPointsInArray(&pointarray[pivotindex],&pointarray[N-1]);
	swap(&halos[pivotindex],&halos[N-1]);
	swap(&brr[pivotindex],&brr[N-1]);
	newpivotindex=0;

	// partition list and array
	for(i=0;i<N;++i){
		if(arr[i] <= pivotvalue){
			swap(&arr[newpivotindex],&arr[i]);
			//SwapPointsInArray(&pointarray[newpivotindex],&pointarray[i]);
			swap(&halos[newpivotindex],&halos[i]);
			swap(&brr[newpivotindex],&brr[i]);
			++newpivotindex;
		}
	}
	--newpivotindex;

	quicksort(halos,brr,arr,newpivotindex);
	quicksort(&halos[newpivotindex+1],&brr[newpivotindex+1],&arr[newpivotindex+1],N-newpivotindex-1);

	return ;
}
/**
 * \brief Pull a source into the simulated volume.  When rays are subsequently shot through the
 * simulation the surface brightness of the source will be added to the point->surface_brightness.
 * Sources can be implanted without altering the existing lens or rays.  The rays need to be re-shot
 * after each source is implanted.  Past sources are removed when a new source is implanted.
 */
void MultiLens::ImplantSource(
		CosmoHndl cosmo           /// cosmology
		,double z                 /// redshift of implanted source
		,double theta[]           /// position of source in angle on the sky (rad)
		,OverGalaxy *ana_source   /// model for source surface brightness distribution
		){
	unsigned long j;

	if(z > plane_redshifts[Nplanes-1]){
		cout << "Warning: Implanted source is at higher redshift than simulation was constructed for." << endl
		<< "It is not being added." << endl;
		return;
	}

	Ds_implant = cosmo->angDist(0,z);
	zs_implant = z;

	ys_implant[0] = Ds_implant*theta[0];
	ys_implant[1] = Ds_implant*theta[1];

	locateD(plane_redshifts-1,Nplanes,zs_implant,&j);

	dDs_implant = cosmo->coorDist(z,plane_redshifts[j]);

	anasource = auto_ptr<MultiSourceAnaGalaxy>(new MultiSourceAnaGalaxy(ana_source));
	flag_implanted_source = j;
}
/**
 * \brief Change the implanted source in the Multilens.
 *
 * When rays are subsequently shot through the simulation the surface brightness of the source will
 * be added to the point->surface_brightness.  Sources can be implanted without altering the existing
 * lens or rays.  The rays need to be re-shot after the index is re-shot.
 *
 * This is meant for use when the internal AnaSource has already been initialized with multiple sources.
 */
void MultiLens::ImplantSource(
		unsigned long index        /// the index of the galaxy to be made the current galaxy
		,CosmoHndl cosmo           /// cosmology
		){

	if(!gal_input_flag){
		ERROR_MESSAGE();
		std::cout << "The AnaSource has not been constructed within MultiLens" << std::endl;
		exit(1);
	}
	unsigned long j;
	double z;

	anasource->setIndex(index);
	z = anasource->getZ();

	if(anasource->getZ() > plane_redshifts[Nplanes-1]){
		cout << "Warning: Implanted source is at higher redshift than simulation was constructed for." << endl
		<< "It is not being added." << endl;
		return;
	}

	Ds_implant = cosmo->angDist(0,z);
	zs_implant = z;


	//ys_implant[0] = Ds_implant*anasource->get_theta()[0];
	//ys_implant[1] = Ds_implant*anasource->get_theta()[1];

	ys_implant[0] = ys_implant[1] = 0.0;


	locateD(plane_redshifts-1,Nplanes,zs_implant,&j);

	dDs_implant = cosmo->coorDist(z,plane_redshifts[j]);

	flag_implanted_source = j;
}


void swap(double **a,double **b){
	double *tmp;
	tmp=*a;
	*a=*b;
	*b=tmp;
}

