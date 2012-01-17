/*
 * forceTree.h
 *
 *  Created on: Oct 14, 2011
 *      Author: bmetcalf
 */

#ifndef FORCE_TREE_H_
#define FORCE_TREE_H_

#include <list.h>
#include "simpleTree.h"

enum PartProf {gaussian};
/**
 * \brief A C++ class wrapper for the tree force calculator.
 *
 * Most of the code in TreeNBForce.c is duplicated here as private methods and
 * a few public ones.
 */
class ForceTree : public SimpleTree {
public:
	ForceTree(PosType **xp,IndexType Npoints,float *masses,float *rsph,bool Multimass,bool Multisize
			,int bucket = 5,int dimensions = 2,bool median = false,PosType theta = 0.1
			);
	~ForceTree();

	/// calculated sph smoothing and store them in the tree, also provide pointer to them
	float * CalculateSPHsmoothing(int N);
	/// calculate the deflection and lensing properties
	void force2D(PosType *ray,PosType *alpha,PosType *kappa,PosType *gamma,bool no_kappa = true);
	/// provides a way to change the profiles of the particles, by default Gaussian
	void ChangeParticleProfile(PartProf partprof);

private:

	PosType force_theta;
	PosType (*alpha_internal)(PosType r,float rmax);
	PosType (*kappa_internal)(PosType r,float rmax);
	PosType (*gamma_internal)(PosType r,float rmax);

	void CalcMoments();
	void rotate_coordinates(PosType **coord);
};

#endif /* FORCE_TREE_H_ */
