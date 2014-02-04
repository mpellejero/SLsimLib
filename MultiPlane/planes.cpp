/*
 * planes.cpp
 *
 *  Created on: May 21, 2013
 *      Author: mpetkova
 */

#include "planes.h"

#include <iterator>

LensPlaneTree::LensPlaneTree(PosType **xpt,LensHaloHndl *my_halos,IndexType Nhalos,PosType my_sigma_background)
: LensPlane(), halos(my_halos, my_halos + Nhalos)
{
	halo_tree = new TreeQuad(xpt,my_halos,Nhalos,my_sigma_background);
}

LensPlaneTree::~LensPlaneTree(){
	delete halo_tree;
}

void LensPlaneTree::force(PosType *alpha,KappaType *kappa,KappaType *gamma,PosType *xx,bool kappa_off){
	halo_tree->force2D_recur(xx,alpha,kappa,gamma,kappa_off);
}

void LensPlaneTree::addHalo(LensHalo* halo)
{
	bool not_yet_implemented = false;
	assert(not_yet_implemented);
}

void LensPlaneTree::removeHalo(LensHalo* halo)
{
	bool not_yet_implemented = false;
	assert(not_yet_implemented);
}

std::vector<LensHalo*> LensPlaneTree::getHalos()
{
	return halos;
}

std::vector<const LensHalo*> LensPlaneTree::getHalos() const
{
	return std::vector<const LensHalo*>(halos.begin(), halos.end());
}

LensPlaneSingular::LensPlaneSingular(LensHalo** my_halos, std::size_t my_Nhalos)
: LensPlane(), halos(my_halos, my_halos + my_Nhalos)
{
}

LensPlaneSingular::~LensPlaneSingular()
{
}

void LensPlaneSingular::force(PosType *alpha,KappaType *kappa,KappaType *gamma,PosType *xx,bool kappa_off){
	PosType alpha_tmp[2],x_tmp[2];
	KappaType kappa_tmp, gamma_tmp[3];
    PosType * xxhalo = new PosType[2];
    
	alpha[0] = alpha[1] = 0.0;
	*kappa = 0.0;
	gamma[0] = gamma[1] = gamma[2] = 0.0;
    xxhalo[0] = xxhalo[1] = 0.0;

	for(std::size_t i = 0, n = halos.size(); i < n; ++i)
	{
		alpha_tmp[0] = alpha_tmp[1] = 0.0;
		kappa_tmp = 0.0;
		gamma_tmp[0] = gamma_tmp[1] = gamma_tmp[2] = 0.0;

        halos[i]->getX(xxhalo);
        // std::cout << "xx[0] = " << xx[0] << " ; xx[1] = " << xx[1] << std::endl;
        // std::cout << "xxhalo[0] = " << xxhalo[0] << " ; xxhalo[1] = " << xxhalo[1] << std::endl;
        x_tmp[0] = xx[0] - xxhalo[0];
        x_tmp[1] = xx[1] - xxhalo[1];
        
		halos[i]->force_halo(alpha_tmp,&kappa_tmp,gamma_tmp,x_tmp,kappa_off,false);
    
		alpha[0] -= alpha_tmp[0];
		alpha[1] -= alpha_tmp[1];
		*kappa += kappa_tmp;
		gamma[0] += gamma_tmp[0];
		gamma[1] += gamma_tmp[1];
		gamma[2] += gamma_tmp[2];
	}
}

void LensPlaneSingular::addHalo(LensHalo* halo)
{
	halos.push_back(halo);
}

void LensPlaneSingular::removeHalo(LensHalo* halo)
{
	std::vector<LensHalo*>::iterator it = std::find(halos.begin(), halos.end(), halo);
	if(it != halos.end())
		halos.erase(it);
}

std::vector<LensHalo*> LensPlaneSingular::getHalos()
{
	return halos;
}

std::vector<const LensHalo*> LensPlaneSingular::getHalos() const
{
	return std::vector<const LensHalo*>(halos.begin(), halos.end());
}
