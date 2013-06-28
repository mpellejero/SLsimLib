/*
 * implant_stars.c
 *
 *  Created on: Jun 5, 2010
 *      Author: R.B. Metcalf
 */

#include "slsimlib.h"

using namespace std;
/** \ingroup ChangeLens
 * \brief  Implants stars into the lens around the images.
 *
 * lens->Nstars and lens->stars_fstars must be set before calling
 * allocates all memory for stars
 */

void LensHalo::implant_stars(PosType **centers,unsigned long Nregions,long *seed, IMFtype type){
	PosType r,theta,NstarsPerImage;
	double *mean_mstar;
	unsigned long i,j,m,k;

	if(stars_N < 1.0  || star_fstars <= 0) return;
	if(star_fstars > 1.0){ std::printf("fstars > 1.0\n"); exit(0); }
	if(!(stars_implanted) ){

		star_masses = new float[stars_N];
		stars = new unsigned long[stars_N];
		stars_xp = Utilities::PosTypeMatrix(stars_N,3);
		star_theta_force = 1.0e-1;
		assert(Nregions > 0);
		star_Nregions = Nregions;
		star_region = new double[Nregions];
		mean_mstar = new double[Nregions];
		star_kappa = new double[Nregions];
		star_xdisk = Utilities::PosTypeMatrix(Nregions,2);

	}else{
		// free star_tree
		delete star_tree;
	}

	if(stars_N < 1  || star_fstars <= 0){
		stars_implanted = true;
		stars_N = 0;
		star_fstars = 0.0;

		for(j=0,m=0;j<Nregions;++j){
			star_region[j] = 0.0;
			star_kappa[j] = 0.0;
			mean_mstar[j] = 0.0;
			star_xdisk[j][0] = centers[j][0];
			star_xdisk[j][1] = centers[j][1];

		}
		return;
	}

	//params.get("main_sub_mass_max",sub_Mmax)
	NstarsPerImage = stars_N/star_Nregions;
	star_masses=stellar_mass_function(type, stars_N, seed, min_mstar, max_mstar, bend_mstar,lo_mass_slope,hi_mass_slope);
	if (type==One){
		for(j=0;j<Nregions;++j){
			mean_mstar[j]=1.0;
		}
	}
	else if(type==Mono){
		for(j=0;j<Nregions;++j){
			mean_mstar[j]=min_mstar;
		}
	}
	else{
		for(j=0,m=0;j<Nregions;++j){
			mean_mstar[j] = 0.0;
			for(i=0;i<NstarsPerImage;++i,++m){
				mean_mstar[j]+=star_masses[m];
			}
			mean_mstar[j]/=NstarsPerImage;
			//cout << "mean_mstar: " << j << " " << mean_mstar[j] << endl;
		}
	}

	for(j=0,m=0;j<Nregions;++j){
		double alpha[2];
		KappaType kappa, gamma[3];
		alpha[0]=alpha[1]=gamma[0]=gamma[1]=gamma[2]=0.;
		kappa=0.;
		double xcm[2];
		xcm[0] = centers[j][0];
		xcm[1] = centers[j][1];

		force_halo(alpha,&kappa,gamma,xcm,false,false);
		star_kappa[j] = kappa;
		star_region[j] = 1.0/sqrt(pi*star_kappa[j]/(mean_mstar[j]*(float)NstarsPerImage));

		star_xdisk[j][0] = centers[j][0];
		star_xdisk[j][1] = centers[j][1];

		//printf("kappa = %e  star_region = %e\n",star_kappa[j],star_region[j]);
		char *fname = "stars0.dat";
		if(j==0){fname = "stars0.dat";}
		if(j==1){fname = "stars1.dat";}
		if(j==2){fname = "stars2.dat";}
		if(j==3){fname = "stars3.dat";}
		ofstream fstars(fname);
		if(j==0){fname = "masses0.dat";}
		if(j==1){fname = "masses1.dat";}
		if(j==2){fname = "masses2.dat";}
		if(j==3){fname = "masses3.dat";}
		ofstream mstars(fname);

		for(i=0;i<NstarsPerImage;++i,++m){
			//m=j*NstarsPerImage+i;
			r = star_region[j]*sqrt(ran2(seed));
			theta=2*pi*ran2(seed);
			stars_xp[m][0] = star_xdisk[j][0] + r*cos(theta);
			stars_xp[m][1] = star_xdisk[j][1] + r*sin(theta);
			stars_xp[m][2] = 0.0;
			//cout << m << " " << star_masses[m] << endl;

			//if(maxr<r){
			//	maxr=r;
			//}
			// check to make see if star is in another centers region
			//   and remove it
			//cout << "bla: " << j << " " <<  m << " " << i << endl;
			for(k=0;k<j;++k){
				if(  star_region[k] > sqrt(pow(centers[k][0]-stars_xp[m][0],2)
						+ pow(centers[k][1]-stars_xp[m][1],2)) ){
					//--NstarsPerImage;
					//--i;
					--m;
					break;
				}
			}
			fstars << scientific << stars_xp[m][0] << " " << stars_xp[m][1] << endl;
			mstars << scientific << star_masses[m] << endl;

			//cout << "max r" << maxr << " " << star_region[j] << endl;
			//printf("%e %e\n",stars_xp[m][0],stars_xp[m][1]);
		}
		fstars.close();
		mstars.close();

	}

	assert(m <= stars_N);
	stars_N = m;

	//std::printf("last star x = %e %e\n",stars_xp[stars_N-1][0],stars_xp[stars_N-1][1]);

	float dummy=0;
	//star_tree = new TreeForce(stars_xp,stars_N,star_masses,&dummy
	//		,false,false,5,2,false,star_theta_force);

	star_tree = new TreeQuad(stars_xp,star_masses,&dummy,stars_N
			,false,false,0,4,star_theta_force);

	// visit every branch to find center of mass and cutoff scale */
	stars_implanted = true;

	return ;
}

// This allows the stars to be turned off after they have been implanted.
/*void AnaNSIELensHalo::toggleStars(bool implanted){
	stars_implanted = implanted;
}
*/

/// subtracts the mass in stars from the smooth model to compensate
/// for the mass of the stars the lensing quantities are all updated not replaced
void LensHalo::substract_stars_disks(double *ray,double *alpha
		,KappaType *kappa,KappaType *gamma){

	if(!(stars_implanted)) return;

	double xcm,ycm,r;
	float tmp;
	double mass;
	int i;

	for(i=0;i<star_Nregions;++i){
		xcm = ray[0] - star_xdisk[i][0];
		ycm = ray[1] - star_xdisk[i][1];
		r=sqrt(xcm*xcm + ycm*ycm);

		if(r < star_region[i]){
			alpha[0] += star_kappa[i]*xcm;
			alpha[1] += star_kappa[i]*ycm;
			*kappa -= star_kappa[i];
		}else{

			mass = star_kappa[i]*pow(star_region[i],2)/r/r;
			alpha[0] += mass*xcm;
			alpha[1] += mass*ycm;

			tmp = 2*mass/r/r;
			gamma[0] += 0.5*(xcm*xcm-ycm*ycm)*tmp;
			gamma[1] += xcm*ycm*tmp;
		}
	}

	return;
}

// random stellar masses according to IMF of choice
/* mtype defines the stellar mass function
 * 0 - always the same stellar mass (e.g. 1Msol)
 * 1 - salpeter imf, i.e. slope = -2.35
 * 2 - broken power law, requires lower mass end slope (powerlo), high mass slope (powerhi), bending point (bendmass)
 * 3 - further IMF models may follow
 */
float* LensHalo::stellar_mass_function(IMFtype type, unsigned long Nstars, long *seed
		,double minmass, double maxmass, double bendmass, double powerlo, double powerhi){

	//if(!(stars_implanted)) return;
	unsigned long i;
	double powerp1,powerp2,powerp3,powerlp1,bendmass1,bendmass2,shiftmax,shiftmin,n0,n1,n2,n3,rndnr,tmp0,tmp1,tmp2;
	float *stellar_masses = new float[Nstars];

	if(type==One){
		for(i = 0; i < Nstars; i++){
				stellar_masses[i]=1.0;
		}
	}

	if(type==Mono){
		if((minmass!=maxmass)){
		    cout << "For IMF type Mono min_mstar and max_mstar must be defined in parameter file and they must be equal" << endl;
		    exit(1);
		}
		for(i = 0; i < Nstars; i++){
			stellar_masses[i]=minmass;
		}
	}

    if(type==Salpeter){
    	if((minmass==maxmass)){
    			    cout << "For IMF type Salpeter min_mstar and max_mstar must be defined in parameter file" << endl;
    			    exit(1);
    	}
    	powerp1 = -1.35;
    	n0 = (pow(maxmass,powerp1)) /powerp1;
    	n1 =  n0 - (pow(minmass,powerp1)) / powerp1;
    	for(i = 0; i < Nstars; i++){
    		stellar_masses[i] = pow( (-powerp1*(n1*ran2(seed)-n0)),(1.0/powerp1) );
    	}
	}

    if(type==SinglePowerLaw){
       	if((minmass==maxmass) || (powerlo!=powerhi) || ((powerlo==0)&(powerhi==0))){
       			    cout << "For IMF type SinglePowerLaw min_mstar, max_mstar and slope_1 must be defined in parameter file. Slope_1 must be equal to slope_2, min_mstar must be different from max_mstar!" << endl;
       			    exit(1);
       	}

       	powerp1 = powerlo+1.0;
       	n0 = (pow(maxmass,powerp1)) /powerp1;
       	n1 =  n0 - (pow(minmass,powerp1)) / powerp1;
       	for(i = 0; i < Nstars; i++){
       		stellar_masses[i] = pow( (-powerp1*(n1*ran2(seed)-n0)),(1.0/powerp1) );
       	}
   	}


    if(type==Chabrier){
		if(minmass==maxmass){
			cout << "For IMF type Chabrier min_mstar and max_mstar must be defined in parameter file!" << endl;
			exit(1);
		}
		double chab_param[]={0.086,0.22,0.57};
		powerp1=-1.3;
		tmp0=2.0*chab_param[2]*chab_param[2];
		tmp1=chab_param[0]/(log(10.0)*log(10.0))*exp((-(log10(chab_param[1]))*(log10(chab_param[1])))/tmp0);
		tmp2=-0.5*chab_param[0]/log(10)*sqrt(pi)*sqrt(tmp0);
		n1=tmp2*(erff(log10(chab_param[1])/sqrt(tmp0))-erff((log10(chab_param[1])-log10(minmass))/sqrt(tmp0)));
		n2=tmp1/powerp1*(pow(maxmass,powerp1)-1.0);
		n0=n1+n2;
		for(i = 0; i < Nstars; i++){
			rndnr=ran2(seed);
			if(rndnr<(n1/n0)){
				stellar_masses[i]=pow(10.0,(log10(chab_param[1])-sqrt(2.*chab_param[2]*chab_param[2])*erfinv((n0*rndnr)/tmp2+erff((log10(chab_param[1])-log10(minmass))/(sqrt(2.*chab_param[2]*chab_param[2])) ) )));
			}
			else{
				stellar_masses[i]=pow( ((n0*rndnr-n1)*powerp1/tmp1 +1.0),(1./powerp1) );
			}
		}
	}

    if(type==BrokenPowerLaw){
    	if((powerlo==powerhi)){
    		cout << "For IMF type BrokenPowerLaw inner slope (slope_1) and outer slope (slope_2) must be defined in parameter file" << endl;
    		exit(1);
    	}
    	else{
    		powerlp1=powerlo+1.0;
    		powerp1 = powerhi+1.0;
    		shiftmax=maxmass/bendmass;
    		shiftmin=minmass/bendmass;
    		n1=(1./powerlp1)-(pow(shiftmin, powerlp1))/powerlp1;
    		n2=( pow(shiftmax,powerp1) )/powerp1-(1./powerp1);
    		n0=n1+n2;
    		for(i = 0; i < Nstars; i++){
    			rndnr=ran2(seed);
    			if(rndnr<(n1/n0)){
    				stellar_masses[i]=(pow( ((n0*rndnr)*(powerlp1)+ pow(shiftmin,powerlp1)),(1.0/powerlp1)))*bendmass;
    			}
    			else{
    				stellar_masses[i]=(pow( ((n0*rndnr-n1)*powerp1+1.0),(1.0/powerp1)))*bendmass;
    			}
    		}
    	}
    }

    if(type==Kroupa){
    	bendmass1=0.08;
    	bendmass2=0.5;
		if((minmass>bendmass1)||(maxmass<bendmass2)){
			cout << "For IMF type Kroupa min_mstar<0.08 and max_mstar>0.5 must be defined in parameter file!" << endl;
			exit(1);
		}
		else{
			double kroupa_param[]={-0.3,-1.3,-2.3};
			powerp1=kroupa_param[0]+1.0;
			powerp2=kroupa_param[1]+1.0;
			powerp3=kroupa_param[2]+1.0;
			tmp1=pow(bendmass1,(kroupa_param[0]-kroupa_param[1]));
			tmp2=tmp1*pow(bendmass2,(kroupa_param[1]-kroupa_param[2]));
			n1=(pow(bendmass1,powerp1)-pow(minmass, powerp1))/powerp1;
			n2=tmp1*((pow(bendmass2,powerp2)-pow(bendmass1, powerp2))/powerp2);
			n3=tmp2*((pow(maxmass,powerp3)-pow(bendmass2, powerp3))/powerp3);
			n0=n1+n2+n3;

			for(i = 0; i < Nstars; i++){
				rndnr=ran2(seed);
				if(rndnr<(n1/n0)){
					stellar_masses[i]=(pow( ((n0*rndnr)*(powerp1)+ pow(minmass,powerp1)),(1.0/powerp1)));

				}
				else if(rndnr<((n1+n2)/n0)) {
					stellar_masses[i]=(pow( ((n0*rndnr-n1)*(powerp2/tmp1)+pow(bendmass1,powerp2)),(1.0/powerp2)));

				}
				else{
					tmp0= ((n0*rndnr-n1-n2)*(powerp3/tmp2)+pow(bendmass2,powerp3));
					stellar_masses[i]=(pow(tmp0,(1.0/powerp3)));
				}
			}
		}
    }
    return stellar_masses;
}

