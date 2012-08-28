/**
 * simpleTree.h
 *
 *  Created on: Oct 14, 2011
 *      Author: bmetcalf
 */

#ifndef QUAD_TREE_H_
#define QUAD_TREE_H_

#include <simpleTree.h>
#include <cstdlib>
#include <iostream>

#ifndef PosType_declare
#define PosType_declare
typedef double PosType;
#endif

#ifndef IndexType_declare
#define IndexType_declare
typedef unsigned long IndexType;
#endif

#ifndef error_message
#define error_message
#define ERROR_MESSAGE() std::cout << "ERROR: file: " << __FILE__ << " line: " << __LINE__ << std::endl;
#endif

//short const treeNBdim = 2;

struct QBranchNB{
  QBranchNB();
  ~QBranchNB();

	/// array of particles in QBranchNB
  IndexType *particles;
  IndexType *big_particles;
  IndexType nparticles;
  /// the number of particles that aren't in children
  IndexType Nbig_particles;
  /// Size of largest particle in branch
  PosType maxrsph;
  /// center of mass
  PosType center[2];
  PosType mass;
  /// level in tree
  int level;
  unsigned long number;
  /// bottom, left, back corner of box
  PosType boundary_p1[2];
  /// top, right, front corner of box
  PosType boundary_p2[2];
  QBranchNB *child0;
  QBranchNB *child1;
  QBranchNB *child2;
  QBranchNB *child3;

  /// father of branch
  QBranchNB *prev;
  /// Either child2 of father is branch is child1 and child2 exists or the brother of the father.
  /// Used for iterative tree walk.
  QBranchNB *brother;

  /* projected quantities */
  /// quadropole moment of branch
  PosType quad[3];
  /// largest dimension of box
  PosType rmax;
  /// the critical distance below which a branch is opened in the
  PosType rcrit_angle;
                /* force calculation */
  PosType rcrit_part;
  //PosType cm[2]; /* projected center of mass */

};

/** \brief
 * QTreeNB: Tree structure used for force calculation with particles (i.e. stars, Nbody or halos).
 *
 * The tree also contains pointers to the list of positions, sizes and masses of the particles.
 * Also flags for the number of dimensions the tree is defined in (2 or 3), and if multiple
 * masses and sizes should be used.
 */
struct QTreeNB{
	QTreeNB(PosType **xp,IndexType *particles,IndexType nparticles
			 ,PosType boundary_p1[],PosType boundary_p2[]);
	~QTreeNB();

	//void freeQTreeNB();
	short empty();
	bool isEmpty();
	bool atTop();
	bool noChild();
	bool offEnd();
	void getCurrent(IndexType *particles,IndexType *nparticles);
	unsigned long getNbranches();
	void moveTop();
	void moveUp();
	void moveToChild(int child);
	void attachChildrenToCurrent(QBranchNB *branch0,QBranchNB *branch1
			,QBranchNB *branch2,QBranchNB *branch3);
	bool WalkStep(bool allowDescent);
	inline bool atLeaf(){
		return (current->child0 == NULL)*(current->child1 == NULL)
				*(current->child2 == NULL)*(current->child3 == NULL);
	}

	QBranchNB *top;
	QBranchNB *current;
	/// number of branches in tree
	unsigned long Nbranches;
	/// Array of particle positions
	PosType **xp;

private:
	void _freeQTree(short child);
};

typedef struct QTreeNB * QTreeNBHndl;
typedef int QTreeNBElement;

/**
 * \brief QuadTree is a class for calculating the deflection, kappa and gamma by tree method.
 *
 * QuadTree is evolved from SimpleTree and ForceTree.  It splits each cell into four equal area
 * subcells instead of being a binary tree like SimpleTree.  When the "particles" are given sizes
 * the tree is built in such a way the large particles are stored in branches that are no smaller
 * than their size.  In this way particles are stored on all levels of the tree and not just in the
 * leaves.  This improves efficiency when particles of a wide range of sizes overlap in 2D.
 *
  * The default value of theta = 0.1 generally gives better than 1% accuracy on alpha.
 * The shear and kappa is always more accurate than the deflection.
 *
 */
//TODO MARGARITA  I can't seem to reproduce the problem you told us about when QuadTree is made with just one particle.
// Does it crash or give the wrong answer?
class QuadTree {
public:
	QuadTree(
			PosType **xpt
			,float *my_masses
			,float *my_sizes
			,IndexType Npoints
			,bool Multimass
			,bool Multisize
			,double my_kappa_background = 0
			,int bucket = 5
			,double theta_force = 0.1
			);
	QuadTree(
			PosType **xpt
			,HaloStructure *my_halo_params
			,IndexType Npoints
			,double my_kappa_background = 0
			,int bucket = 5
			,double theta_force = 0.1
			);
	virtual ~QuadTree();

	void force2D(double *ray,double *alpha,double *kappa,double *gamma,bool no_kappa);
	void printParticlesInBranch(unsigned long number);
	void printBranchs(int level = -1);

protected:

	PosType **xp;
	bool MultiMass;
	bool MultiRadius;
	float *masses;
	float *sizes;

	IndexType Nparticles;
	double kappa_background;
	int Nbucket;

	double force_theta;

	QTreeNBHndl tree;
	IndexType *index;

	bool haloON;
	HaloStructure *halo_params;

	PosType realray[2];
	int incell,incell2;

	QTreeNBHndl BuildQTreeNB(PosType **xp,IndexType Nparticles,IndexType *particles);
	void _BuildQTreeNB(IndexType nparticles,IndexType *particles);

	inline short WhichQuad(double *x,QBranchNB &branch);

	// Things that could be in a utilities file.
	void quicksort(unsigned long *particles,double *arr,unsigned long N);
	void quickPartition(double pivotvalue,unsigned long *pivotindex,unsigned long *particles
			,double *arr,unsigned long N);
	//inline bool atLeaf();
	inline bool inbox(PosType *ray,PosType *p1,PosType *p2){
	  return (ray[0]>=p1[0])*(ray[0]<=p2[0])*(ray[1]>=p1[1])*(ray[1]<=p2[1]);
	}
	int cutbox(PosType *ray,PosType *p1,PosType *p2,float rmax);
	void swap(double *a,double *b);
	void swap(PosType a,PosType b);
	void swap(IndexType a,IndexType b);
	void swap(unsigned long *a,unsigned long *b);

	void CalcMoments();
	void rotate_coordinates(double **coord);

	// Internal profiles for a Gaussian particle
	virtual inline double alpha_h(double r2s2,double sigma){
	  return (sigma > 0.0 ) ? ( exp(-0.5*r2s2) - 1.0 ) : -1.0;
	}
	virtual inline double kappa_h(double r2s2,double sigma){
	  return 0.5*r2s2*exp(-0.5*r2s2);
	}
	virtual inline double gamma_h(double r2s2,double sigma){
	  return (sigma > 0.0 ) ? (-2.0 + (2.0 + r2s2)*exp(-0.5*r2s2) ) : -2.0;
	}
	virtual inline double phi_h(double r2s2,double sigma){
		ERROR_MESSAGE();  // not yet written
		exit(1);
		return 0;
	}

	QTreeNBHndl rotate_simulation(PosType **xp,IndexType Nparticles,IndexType *particles
			,double **coord,double theta,float *rsph,float *mass
			,bool MultiRadius,bool MultiMass);
	QTreeNBHndl rotate_project(PosType **xp,IndexType Nparticles,IndexType *particles
			,double **coord,double theta,float *rsph,float *mass
			,bool MultiRadius,bool MultiMass);
	 void cuttoffscale(QTreeNBHndl tree,double *theta);
};

/** \ingroup DeflectionL2
 *
 * \brief A class for calculating the deflection, kappa and gamma caused by a collection of halos
 * with truncated power-law mass profiles.
 *
 * Derived from the QuadTree class.  The "particles" are replaced with spherical halos.
 *The truncation is in 2d not 3d. \f$ \Sigma \propto r^\beta \f$ so beta would usually be negative.
 *
 *
 * The default value of theta = 0.1 generally gives better than 1% accuracy on alpha.
 * The shear and kappa is always more accurate than the deflection.
 */
class QuadTreePowerLaw : public QuadTree{

public:
	QuadTreePowerLaw(float beta,PosType **xp,IndexType Npoints,HaloStructure *par_internals
			,double my_kappa_bk=0.0,int bucket = 5,PosType theta = 0.1);

	~QuadTreePowerLaw();

private:
	float beta; // logorithmic slop of 2d mass profile

	// Override internal structure of halos
	inline double alpha_h(double r,double rm){
		return -1.0*pow(r/rm,beta+2);
	}
	inline double kappa_h(double r,double rm){
		return (beta+2)*pow(r/rm,beta)*r*r/(rm*rm);
	}
	inline double gamma_h(double r,double rm){
		return -0.5*beta*pow(r/rm,beta+2);
	}
	inline double phi_h(double r,double rm){
		ERROR_MESSAGE();
		std::cout << "time delay has not been fixed for PowerLaw profile yet." << std::endl;
		exit(1);
		return 0.0;
	}
};

/** \ingroup DeflectionL2
 *
 * \brief A class for calculating the deflection, kappa and gamma caused by a collection of NFW
 * halos.
 *
 * Derived from the QuadTree class.  The "particles" are replaced with spherical NFW halos.
 *
 * This class uses the true expressions for the NFW profile.  This is
 * time consuming and not usually necessary. See QuadTreePseudoNFW for a faster alternative.
 *
* The default value of theta = 0.1 generally gives better than 1% accuracy on alpha.
 * The shear and kappa is always more accurate than the deflection.
 *
 */
class QuadTreeNFW : public QuadTree{

public:
	QuadTreeNFW(PosType **xp,IndexType Npoints,HaloStructure *par_internals
			,double my_kappa_bk = 0.0,int bucket = 5,PosType theta = 0.1);
	~QuadTreeNFW();

private:

	//static double *ft, *gt, *g2t;
	static double *ftable,*gtable,*g2table;
	static long ob_count;


	// Override internal structure of halos
	inline double alpha_h(double r,double rm){
		return -1.0*InterpolateFromTable(gtable,r)/InterpolateFromTable(gtable,rm);
	}
	inline double kappa_h(double r,double rm){
		return 0.5*r*r*InterpolateFromTable(ftable,r)/InterpolateFromTable(gtable,rm);
	}
	inline double gamma_h(double r,double rm){
		return -0.25*r*r*InterpolateFromTable(g2table,r)/InterpolateFromTable(gtable,rm);
	}
	inline double phi_h(double r,double rm){
		ERROR_MESSAGE();
		std::cout << "time delay has not been fixed for NFW profile yet." << std::endl;
		exit(1);
		return 0.0;
	}
	void make_tables();

	double gfunctionRmax(double rm,double x);
	double ffunctionRmax(double rm,double x);
	double g2functionRmax(double rm,double x);
};

/** \ingroup DeflectionL2
 *
 * \brief A class for calculating the deflection, kappa and gamma caused by a collection of
 * halos with a double power-law mass profile.
 *
 * Derived from the QuadTree class.  The "particles" are replaced with spherical halos
 * with \f$ \Sigma \propto 1/(1 + r/r_s )^\beta \f$ so beta would usually be positive.
 *
 * An NFW profile is approximated beta = 2 .
 *
 * The default value of theta = 0.1 generally gives better than 1% accuracy on alpha.
 * The shear and kappa is always more accurate than the deflection.
 */
class QuadTreePseudoNFW : public QuadTree{

public:
	QuadTreePseudoNFW(double beta,PosType **xp,IndexType Npoints,HaloStructure *par_internals
			,double my_kappa_bk = 0.0,int bucket = 5,PosType theta = 0.1);
	~QuadTreePseudoNFW();

private:

	double beta;
	static double *mhattable;
	static long ob_count;


	// Override internal structure of halos
	inline double alpha_h(double r,double rm){
		return -1.0*InterpolateFromTable(mhattable,r)/InterpolateFromTable(mhattable,rm);
	}
	inline double kappa_h(double r,double rm){
		return 0.5*r*r/InterpolateFromTable(mhattable,rm)/pow(1+r,beta);
	}
	inline double gamma_h(double r,double rm){
		return (0.5*r*r/pow(1+r,beta) - InterpolateFromTable(mhattable,r))/InterpolateFromTable(mhattable,rm);
	}
	inline double phi_h(double r,double rm){
		ERROR_MESSAGE();
		std::cout << "time delay has not been fixed for PseudoNFW profile yet." << std::endl;
		exit(1);
		return 0.0;
	}
	void make_tables();
};

#endif /* QUAD_TREE_H_ */
