/**
 * simpleTreeVec.h
 *
 *  Created on: Oct 14, 2011
 *      Author: bmetcalf
 */

#ifndef SIMP_TREE_V_
#define SIMP_TREE_V_

#include "standard.h"
//#include "Tree.h"
//#include "lens_halos.h"

// used for default position of object
//template<class T>
//PosType  *defaultposition(T &in){return in.x;}

    /** \brief
     * A tree for doing quick searches in multidimensional space.  A pointer to an array of
     *  objects type T is provided.  If they are in a vector, vector.data() can be put in the constructor.
     *  The order of objects in the array is not changed.  If Mypos is not specified the method PosType * T::x() must exist.
     *  It is this coordinate that is used for the object's position.  Another definition for the position of and object can made by specifying Mypos.
     */
template <class T>
class TreeSimpleVec {
public:
    
	TreeSimpleVec(
                  T *xpt                 /// array of object, xpt[i].x[0...dimensions-1] must exist or Mypos must be specified
                  ,IndexType Npoints      /// number of object in array
                  ,int bucket = 5         /// number of points in leaves of tree
                  ,int dimensions = 2     /// dimension of space
                  ,bool median = true     /// whether to use a median cut or a space cut in splitting branches
                //  ,PosType *(*Mypos)(T&) = defaultposition  /// function that takes
                ,PosType *(*Mypos)(T&) = [](T& in){return in.x;}  /// function that takes the object T and returns a pointer to its position, default is t.x[]
                  ){
        index = new IndexType[Npoints];
        IndexType ii;
        position = Mypos;
    
        Nbucket = bucket;
        Ndimensions = dimensions;
        median_cut = median;
        Nparticles = Npoints;
        
        points = xpt;

        for(ii=0;ii<Npoints;++ii) index[ii] = ii;
        
        BuildTree();
    }

  virtual ~TreeSimpleVec()
    {
        freeTree();
        delete[] index;
        return;
    };
  
	/// \brief Finds the points within a circle around center and puts their index numbers in a list
	void PointsWithinCircle(PosType center[2],float radius,std::list<unsigned long> &neighborkist);
	/// \brief Finds the points within an ellipse around center and puts their index numbers in a list
	void PointsWithinEllipse(PosType center[2],float a_max,float a_min,float posangle,std::list<unsigned long> &neighborkist);
	/// \brief Finds the nearest N neighbors and puts their index numbers in an array, also returns the distance to the Nth neighbor for calculating smoothing
	void NearestNeighbors(PosType *ray,int Nneighbors,float *radius,IndexType *neighbors);
    
    /** \brief Box representing a branch in a tree.  It has four children.  Used in TreeNBStruct which is used in TreeForce.
     */

    struct BranchV{
        
        int Ndim;
        /// array of branch_index in BranchV
        IndexType *branch_index;
        IndexType nparticles;
        /// level in tree
        int level;
        unsigned long number;
        /// bottom, left, back corner of box
        PosType *boundary_p1;
        /// top, right, front corner of box
        PosType *boundary_p2;
        BranchV *child1;
        BranchV *child2;
        /// father of branch
        BranchV *prev;
        /// Either child2 of father is branch is child1 and child2 exists or the brother of the father.
        /// Used for iterative tree walk.
        BranchV *brother;
        
        
        BranchV(int my_Ndim,IndexType *my_branch_index,IndexType my_nparticles
                ,PosType my_boundary_p1[],PosType my_boundary_p2[]
                ,int my_level,unsigned long my_BranchVnumber)
        :Ndim(my_Ndim),branch_index(my_branch_index),nparticles(my_nparticles)
         ,level(my_level),number(my_BranchVnumber)
        {
            
            boundary_p1 = new PosType[Ndim*2];
            boundary_p2 = &boundary_p1[Ndim];
            
            for(size_t i=0;i<Ndim;++i){
                boundary_p1[i]= my_boundary_p1[i];
                boundary_p2[i]= my_boundary_p2[i];
            }
            
            child1 = NULL;
            child2 = NULL;
            prev = NULL;
            brother = NULL;
        };
        BranchV(BranchV &branch){
                
                Ndim = branch.Ndim;
                branch_index = branch.branch_index;
                nparticles = branch.nparticles;
                level = branch.level;
                number = branch.number;

                boundary_p1 = new PosType[Ndim*2];
                boundary_p2 = &boundary_p1[Ndim];
                
                for(size_t i=0;i<Ndim;++i){
                    boundary_p1[i]= branch.boundary_p1[i];
                    boundary_p2[i]= branch.boundary_p2[i];
                }
                
                child1 = NULL;
                child2 = NULL;
                prev = NULL;
                brother = NULL;
            };
        
        ~BranchV(){
            delete[] boundary_p1;
        };
        
        
    };
    
protected:
    
	int incell,incell2;
	IndexType *index;
	IndexType Nparticles;
	bool median_cut;
	int Nbucket;
	PosType realray[2];
	T *points;
  PosType *(*position)(T&);
    
    BranchV *top;
    BranchV *current;
    /// number of branches in tree
    unsigned long Nbranches;
    /// Dimension of tree, 2 or 3.  This will dictate how the force is calculated.
    short Ndimensions;
    
	void BuildTree();
    
	void _BuildTree(IndexType nparticles,IndexType *tmp_index);
    
	void _PointsWithin(PosType *ray,float *rmax,std::list<unsigned long> &neighborkist);
	void _NearestNeighbors(PosType *ray,int Nneighbors,unsigned long *neighbors,PosType *rneighbors);
    
	void freeTree();
    
	short emptyTree();
	void _freeTree(short child);
	bool isEmpty();
	bool atTop();
	bool noChild();
	bool offEnd();
	void getCurrent(IndexType *branch_index,IndexType *nparticles);
	unsigned long getNbranches();
	void moveTop();
	void moveUp();
	void moveToChild(int child);
	void attachChildToCurrent(IndexType *branch_index,IndexType nparticles
                              ,PosType boundary_p1[],PosType boundary_p2[]
                              ,int child);
	void attachChildToCurrent(BranchV &data,int child);
	bool TreeWalkStep(bool allowDescent);
    
	inline bool atLeaf(){
		return (current->child1 == NULL)*(current->child2 == NULL);
	}
	inline bool inbox(const PosType* center,PosType *p1,PosType *p2){
        return (center[0]>=p1[0])*(center[0]<=p2[0])*(center[1]>=p1[1])*(center[1]<=p2[1]);
	}


};
/*
template <class T>
TreeSimpleVec<T>::TreeSimpleVec<T>(T *xpt,IndexType Npoints,int bucket,int dimensions,bool median){
        index = new IndexType[Npoints];
        IndexType ii;
        
        Nbucket = bucket;
        Ndimensions = dimensions;
        median_cut = median;
        Nparticles = Npoints;
        
        points = xpt;
        
        for(ii=0;ii<Npoints;++ii) index[ii] = ii;
        
        BuildTree();
};*/

/************************************************************************
 * NewBranchV
 * Returns pointer to new BranchV struct.  Initializes children pointers to NULL,
 * and sets data field to input.  Private.
 ************************************************************************/



template <class T>
void TreeSimpleVec<T>::PointsWithinEllipse(
                                        PosType *ray     /// center of ellipse
                                        ,float rmax      /// major axis
                                        ,float rmin     /// minor axis
                                        ,float posangle  /// position angle of major axis, smallest angle between the x-axis and the long axis
                                        ,std::list <unsigned long> &neighborlist  /// output neighbor list, will be emptied if it contains anything on entry
                                        ){
	PosType x,y,cs,sn;
    
    
	if(rmax < rmin){float tmp = rmax; rmax=rmin; rmin=tmp;}
    
	if(rmax <=0.0 || rmin <= 0.0){ neighborlist.clear(); return; }
    
	// find point within a circle circumscribes the ellipse
	PointsWithinCircle(ray,rmax,neighborlist);
    
	cs = cos(posangle);
	sn = sin(posangle);
	// go through points within the circle and reject points outside the ellipse
	for(  std::list<unsigned long>::iterator it = neighborlist.begin();it != neighborlist.end();){
		x = points[*it][0]*cs - points[*it][1]*sn;
		y = points[*it][0]*sn + points[*it][1]*cs;
		if( ( pow(x/rmax,2) + pow(y/rmin,2) ) > 1) it = neighborlist.erase(it);
		else ++it;
	}
	return;
}

template <class T>
void TreeSimpleVec<T>::PointsWithinCircle(
                                       PosType *ray     /// center of circle
                                       ,float rmax      /// radius of circle
                                       ,std::list <unsigned long> &neighborlist  /// output neighbor list, will be emptied if it contains anything on entry
                                       ){
    
    neighborlist.clear();
    
    realray[0]=ray[0];
    realray[1]=ray[1];
    
    //cout << "ray = " << ray[0] << " " << ray[1] << " rmax = " << rmax << endl;
    moveTop();
    if( inbox(ray,current->boundary_p1,current->boundary_p2) == 0 ){
        
        ray[0] = (ray[0] > current->boundary_p1[0]) ? ray[0] : current->boundary_p1[0];
        ray[0] = (ray[0] < current->boundary_p2[0]) ? ray[0] : current->boundary_p2[0];
        
        ray[1] = (ray[1] > current->boundary_p1[1]) ? ray[1] : current->boundary_p1[1];
        ray[1] = (ray[1] < current->boundary_p2[1]) ? ray[1] : current->boundary_p2[1];
        
        //cout << "ray = " << ray[0] << " " << ray[1] << endl;
        //ray[0]=DMAX(ray[0],current->boundary_p1[0]);
        //ray[0]=DMIN(ray[0],current->boundary_p2[0]);
        
        //ray[1]=DMAX(ray[1],current->boundary_p1[1]);
        //ray[1]=DMIN(ray[1],current->boundary_p2[1]);
    }
    incell=1;
    
    _PointsWithin(ray,&rmax,neighborlist);
    
    for(auto i : neighborlist){
      assert( rmax*rmax > (position(points[i])[0]-ray[0])*(position(points[i])[0]-ray[0]) + (position(points[i])[1]-ray[1])*(position(points[i])[1]-ray[1]) );
    }
    return;
}
/**
 * Used in PointsWithinKist() to walk tree.*/
template <class T>
void TreeSimpleVec<T>::_PointsWithin(PosType *ray,float *rmax,std::list <unsigned long> &neighborlist){
    
    int j,incell2=1;
    unsigned long i;
    PosType radius;
    short pass;
    
    if(incell){  // not found cell yet
        
        if( inbox(ray,current->boundary_p1,current->boundary_p2) ){
            //cout << "   In box" << endl;
            
            // found the box small enough
            if( Utilities::cutbox(ray,current->boundary_p1,current->boundary_p2,*rmax)==1
               || atLeaf() ){
                //cout << "   Found cell" << endl;
                
                // whole box in circle or a leaf with ray in it
                
                incell=0;
                
                ray[0]=realray[0];
                ray[1]=realray[1];
                
                if( atLeaf() ){
                    // if leaf calculate the distance to all the points in cell
                    for(i=0;i<current->nparticles;++i){
                        for(j=0,radius=0.0;j<2;++j) radius+=pow(position(points[current->branch_index[i]])[j]-ray[j],2);
                        if( radius < *rmax**rmax ){
                            neighborlist.push_back(current->branch_index[i]);
                            //cout << "add point to list" << neighborlist.size() << endl;
                            //InsertAfterCurrentKist(neighborlist,current->branch_index[i]);
                        }
                    }
                }else{ // put all of points in box into getCurrentKist(imagelist)
                    for(i=0;i<current->nparticles;++i){
                        neighborlist.push_back(current->branch_index[i]);
                        //InsertAfterCurrentKist(neighborlist,current->branch_index[i]);
                        //cout << "add point to list" << neighborlist.size() << endl;
                        
                    }
                }
                
            }else{ // keep going down the tree
                
                if(current->child1 !=NULL){
                    moveToChild(1);
                    _PointsWithin(ray,rmax,neighborlist);
                    moveUp();
                    
                    incell2=incell;
                }
                
                if(current->child2 !=NULL){
                    moveToChild(2);
                    _PointsWithin(ray,rmax,neighborlist);
                    moveUp();
                }
                
                // if ray found in second child go back to first to search for neighbors
                if( (incell2==1) && (incell==0) ){
                    if(current->child1 !=NULL){
                        moveToChild(1);
                        _PointsWithin(ray,rmax,neighborlist);
                        moveUp();
                    }
                }
            }
        }  // not in the box
        
    }else{    // found cell
        
        pass=Utilities::cutbox(ray,current->boundary_p1,current->boundary_p2,*rmax);
        // does radius cut into the box
        if( pass ){
            //cout << "   Cell found searching other cells" << endl;
            if( atLeaf()  ){  /* leaf case */
                
                for(i=0;i<current->nparticles;++i){
                    for(j=0,radius=0.0;j<2;++j) radius+=pow(position(points[current->branch_index[i]])[j]-ray[j],2);
                    if( radius < *rmax**rmax ){
                        neighborlist.push_back(current->branch_index[i]);
                        //InsertAfterCurrentKist(neighborlist,current->branch_index[i]);
                        //cout << "add point to list" << neighborlist.size() << endl;
                        
                    }
                }
            }else if(pass==1){ // whole box is inside radius
                
                for(i=0;i<current->nparticles;++i){
                    neighborlist.push_back(current->branch_index[i]);
                    //InsertAfterCurrentKist(neighborlist,current->branch_index[i]);
                    //cout << "add point to list" << neighborlist.size() << endl;
                    
                }
            }else{
                if(current->child1 !=NULL){
                    moveToChild(1);
                    _PointsWithin(ray,rmax,neighborlist);
                    moveUp();
                }
                
                if(current->child2 !=NULL){
                    moveToChild(2);
                    _PointsWithin(ray,rmax,neighborlist);
                    moveUp();
                }
            }
            
        }
    }
    
    return;
}

/**
 *  \brief finds the nearest neighbors in whatever dimensions tree is defined in
 *  */
template <class T>
void TreeSimpleVec<T>::NearestNeighbors(
                                     PosType *ray       /// position
                                     ,int Nneighbors    /// number of neighbors to be found
                                     ,float *radius     /// distance of furthest neighbor found from ray[]
                                     ,IndexType *neighborsout  /// list of the indexes ofx the neighbors
                                     ){
    IndexType i;
    //static int count=0,oldNneighbors=-1;
    short j;
    
    PosType rneighbors[Nneighbors+Nbucket];
    IndexType neighbors[Nneighbors+Nbucket];
    
    if(top->nparticles < Nneighbors){
        ERROR_MESSAGE();
        printf("ERROR: in NearestNeighbors, number of neighbors > total number of particles\n");
      throw std::runtime_error("Asked for too many neighbors");
    }
    
    /* initalize distance to neighbors to a large number */
    for(i=0;i<Nbucket+Nneighbors;++i){
        rneighbors[i] = (10*(top->boundary_p2[0]-top->boundary_p1[0]));
        neighbors[i] = 0;
    }
    
    for(j=0;j<Ndimensions;++j) realray[j]=ray[j];
    
    moveTop();
    if( inbox(ray,current->boundary_p1,current->boundary_p2) == 0 ){
        //ERROR_MESSAGE();
        
        for(j=0;j<Ndimensions;++j){
            ray[j] = (ray[j] > current->boundary_p1[j]) ? ray[j] : current->boundary_p1[j];
            ray[j] = (ray[j] < current->boundary_p2[j]) ? ray[j] : current->boundary_p2[j];
            
            //ray[j]=DMAX(ray[j],current->boundary_p1[j]);
            //ray[j]=DMIN(ray[j],current->boundary_p2[j]);
        }
    }
    incell = 1;
    
    _NearestNeighbors(ray,Nneighbors,neighbors,rneighbors);
    
    for(i=0;i<Nneighbors;++i) neighborsout[i] = neighbors[i];
    *radius = rneighbors[Nneighbors-1];
    
    return;
}

template <class T>
void TreeSimpleVec<T>::_NearestNeighbors(PosType *ray,int Nneighbors,unsigned long *neighbors,PosType *rneighbors){
    
    int incellNB2=1;
    IndexType i;
    short j;
    
    if(incell){  /* not found cell yet */
        
        if( inbox(ray,current->boundary_p1,current->boundary_p2) ){
            
            /* found the box small enough */
            if( current->nparticles <= Nneighbors+Nbucket ){
                incell=0;
                for(j=0;j<Ndimensions;++j) ray[j]=realray[j];
                
                /* calculate the distance to all the particles in cell */
                for(i=0;i<current->nparticles;++i){
                    for(j=0,rneighbors[i]=0.0;j<Ndimensions;++j){
                        rneighbors[i] += pow(position(points[current->branch_index[i]])[j]-ray[j],2);
                    }
                    rneighbors[i]=sqrt( rneighbors[i] );
                    //assert(rneighbors[i] < 10);
                    neighbors[i]=current->branch_index[i];
                }
                
                Utilities::quicksort(neighbors,rneighbors,current->nparticles);
                
            }else{ /* keep going down the tree */
                
                if(current->child1 !=NULL){
                    moveToChild(1);
                    _NearestNeighbors(ray,Nneighbors,neighbors,rneighbors);
                    /*printf("moving up from level %i\n",current->level);*/
                    moveUp();
                    
                    incellNB2=incell;
                }
                
                if(current->child2 !=NULL){
                    /*printf("moving to child2 from level %i\n",current->level);*/
                    moveToChild(2);
                    _NearestNeighbors(ray,Nneighbors,neighbors,rneighbors);
                    /*printf("moving up from level %i\n",current->level);*/
                    moveUp();
                }
                
                /** if ray found in second child go back to first to search for neighbors **/
                if( (incellNB2==1) && (incell==0) ){
                    if(current->child1 !=NULL){
                        moveToChild(1);
                        _NearestNeighbors(ray,Nneighbors,neighbors,rneighbors);
                        moveUp();
                    }
                }
            }
        }
    }else{ // found cell
		/* does radius cut into the box */
        if( Utilities::cutbox(ray,current->boundary_p1,current->boundary_p2,rneighbors[Nneighbors-1]) ){
            
            if( (current->child1 == NULL)*(current->child2 == NULL)){  /* leaf case */
                
                /* combine found neighbors with particles in box and resort */
                for(i=Nneighbors;i<(current->nparticles+Nneighbors);++i){
                    for(j=0,rneighbors[i]=0.0;j<Ndimensions;++j){
                        rneighbors[i]+=pow(position(points[current->branch_index[i-Nneighbors]])[j]-ray[j],2);
                    }
                    rneighbors[i]=sqrt( rneighbors[i] );
                    assert(rneighbors[i] < 10);
                    neighbors[i]=current->branch_index[i-Nneighbors];
                }
                
                Utilities::quicksort(neighbors,rneighbors,Nneighbors+Nbucket);
                
            }else{
                
                if(current->child1 !=NULL){
                    moveToChild(1);
                    _NearestNeighbors(ray,Nneighbors,neighbors,rneighbors);
                    moveUp();
                }
                
                if(current->child2 !=NULL){
                    moveToChild(2);
                    _NearestNeighbors(ray,Nneighbors,neighbors,rneighbors);
                    moveUp();
                }
            }
        }
    }
    return;
}

template <class T>
void TreeSimpleVec<T>::BuildTree(){
    IndexType i;
    short j;
  std::vector<PosType> p1(Ndimensions),p2(Ndimensions);
  
  if(Nparticles > 0){
    for(j=0;j<Ndimensions;++j){
        p1[j] = position(points[0])[j];
        p2[j] = position(points[0])[j];
    }
    
    for(i=0;i<Nparticles;++i){
        for(j=0;j<Ndimensions;++j){
            if(position(points[i])[j] < p1[j] ) p1[j] = position(points[i])[j];
            if(position(points[i])[j] > p2[j] ) p2[j] = position(points[i])[j];
        }
    }
    
    /* Initialize tree root */
    top = current = new BranchV(Ndimensions,index,Nparticles,p1.data(),p2.data(),0,0);
    if (!(top)){
      ERROR_MESSAGE(); fprintf(stderr,"allocation failure in NewTree()\n");
      exit(1);
    }
  
    Nbranches = 1;
  
    /* build the tree */
    _BuildTree(Nparticles,index);
  }else{
    
    for(j=0;j<Ndimensions;++j){
      p1[j] *= 0;
      p2[j] *= 0;
    }

    top = current = new BranchV(Ndimensions,index,0,p1.data(),p2.data(),0,0);
    if (!(top)){
      ERROR_MESSAGE(); fprintf(stderr,"allocation failure in NewTree()\n");
      exit(1);
    }
    
    Nbranches = 1;
    
  }
    /* visit every branch to find center of mass and cutoff scale */
    moveTop();
}


// tree must be created and first branch must be set before start
template <class T>
void TreeSimpleVec<T>::_BuildTree(IndexType nparticles,IndexType *tmp_index){
    IndexType i,cut,dimension;
    BranchV *cbranch,branch1(*current),branch2(*current);
    PosType xcut;

    
    cbranch=current; /* pointer to current branch */
    
    //cbranch->center[0] = (cbranch->boundary_p1[0] + cbranch->boundary_p2[0])/2;
    //cbranch->center[1] = (cbranch->boundary_p1[1] + cbranch->boundary_p2[1])/2;
    //cbranch->quad[0]=cbranch->quad[1]=cbranch->quad[2]=0;
    
    /* leaf case */
    if(cbranch->nparticles <= Nbucket){
        return;
    }
       //cbranch->big_particle=0;
    
    // **** makes sure force does not require nbucket at leaf
    
    /* set dimension to cut box */
    dimension=(cbranch->level % Ndimensions);
  
    PosType *x = new PosType[cbranch->nparticles];
    for(i=0;i<cbranch->nparticles;++i) x[i] = position(points[tmp_index[i]])[dimension];
    
    if(median_cut){
        Utilities::quicksort(tmp_index,x,cbranch->nparticles);
        
        cut=(cbranch->nparticles)/2;
        branch1.boundary_p2[dimension]=x[cut];
        branch2.boundary_p1[dimension]=x[cut];
    }else{
        xcut=(cbranch->boundary_p1[dimension]+cbranch->boundary_p2[dimension])/2;
        branch1.boundary_p2[dimension]=xcut;
        branch2.boundary_p1[dimension]=xcut;
        
        Utilities::quickPartition(xcut,&cut,tmp_index
                                  ,x,cbranch->nparticles);
    }
    
    /* set particle numbers and pointers to my_index */
    branch1.prev=cbranch;
    branch1.nparticles=cut;
    //branch1.branch_index=branch_index+cbranch->big_particle;
    
    branch2.prev=cbranch;
    branch2.nparticles=cbranch->nparticles - cut;
    if(cut < (cbranch->nparticles) )
        branch2.branch_index = &tmp_index[cut];
    else branch2.branch_index=NULL;
    
    delete[] x;
  
    if(branch1.nparticles > 0) attachChildToCurrent(branch1,1);
    if(branch2.nparticles > 0) attachChildToCurrent(branch2,2);
    
    // work out brothers for children
    if( (cbranch->child1 != NULL) && (cbranch->child2 != NULL) ){
        cbranch->child1->brother = cbranch->child2;
        cbranch->child2->brother = cbranch->brother;
    }
    if( (cbranch->child1 == NULL) && (cbranch->child2 != NULL) )
        cbranch->child2->brother = cbranch->brother;
    if( (cbranch->child1 != NULL) && (cbranch->child2 == NULL) )
        cbranch->child1->brother = cbranch->brother;
    
    
    if( branch1.nparticles > 0 ){
        moveToChild(1);
        _BuildTree(branch1.nparticles,branch1.branch_index);
        moveUp();
    }
    
    if(branch2.nparticles > 0 ){
        moveToChild(2);
        _BuildTree(branch2.nparticles,branch2.branch_index);
        moveUp();
    }
    
    return;
}

/// Free all the tree branches
template <class T>
void TreeSimpleVec<T>::freeTree(){
    
	emptyTree();
  	delete top;

	return;
}

template <class T>
short TreeSimpleVec<T>::emptyTree(){
    
	moveTop();
	_freeTree(0);
    
	assert(Nbranches == 1);
    
	return 1;
}

template <class T>
void TreeSimpleVec<T>::_freeTree(short child){
	BranchV *branch;
    
	assert( current);
    
	if(current->child1 != NULL){
		moveToChild(1);
		_freeTree(1);
	}
    
    if(current->child2 != NULL){
        moveToChild(2);
        _freeTree(2);
    }
    
    if( (current->child1 == NULL)*(current->child2 == NULL) ){
        
    	if(atTop()) return;
        
    	branch = current;
    	moveUp();
       	delete branch;
        
    	/*printf("*** removing branch %i number of branches %i\n",branch->number
         ,Nbranches-1);*/
        
       	if(child==1) current->child1 = NULL;
    	if(child==2) current->child2 = NULL;
        
    	--Nbranches;
        
    	return;
    }
    
    return;
}

/************************************************************************
 * isEmpty
 * Returns "true" if the Tree is empty and "false" otherwise.  Exported.
 ************************************************************************/
template <class T>
bool TreeSimpleVec<T>::isEmpty(){
    return(Nbranches == 0);
}

/************************************************************************
 * atTop
 * Returns "true" if current is the same as top and "false" otherwise.
 * Exported.
 * Pre: !isEmpty()
 ************************************************************************/
template <class T>
bool TreeSimpleVec<T>::atTop(){
    
    if( isEmpty() ){
    	ERROR_MESSAGE();
    	fprintf(stderr, "Tree Error: calling atTop() on empty tree\n");
    	exit(1);
    }
    return(current == top);
}

/************************************************************************
 * noChild
 * Returns "true" if the child of the current BranchV does not exist and "false" otherwise.
 * Exported.
 * Pre: !isEmpty()
 ************************************************************************/
template <class T>
bool TreeSimpleVec<T>::noChild(){
    
    if( isEmpty() ){
        
        ERROR_MESSAGE(); fprintf(stderr, "Tree Error: calling atTop() on empty tree\n");
        exit(1);
    }
    
    if( (current->child1 == NULL) || (current->child2 == NULL) ) return true;
    return false;
}

/************************************************************************
 * offEnd
 * Returns "true" if current is off end and "false" otherwise.  Exported.
 ************************************************************************/
template <class T>
bool TreeSimpleVec<T>::offEnd(){
    
    return(current == NULL);
}

/************************************************************************
 * getCurrent
 * Returns the particuls of current.  Exported.
 * Pre: !offEnd()
 ************************************************************************/
template <class T>
void TreeSimpleVec<T>::getCurrent(IndexType *branch_index,IndexType *nparticles){
    
    if( offEnd() ){
        
        ERROR_MESSAGE(); fprintf(stderr, "Tree Error: calling getCurrent() when current is off end\n");
        exit(1);
    }
    
    *nparticles = current->nparticles;
    branch_index = current->branch_index;
    
    return;
}

/************************************************************************
 * getNbranches
 * Returns the NBranchVes of tree.  Exported.
 ************************************************************************/
template <class T>
size_t TreeSimpleVec<T>::getNbranches(){
    
    return(Nbranches);
}

/***** Manipulation procedures *****/

/************************************************************************
 * moveTop
 * Moves current to the front of tree.  Exported.
 * Pre: !isEmpty()
 ************************************************************************/
template <class T>
void TreeSimpleVec<T>::moveTop(){
   
    if( isEmpty() ){
        
        ERROR_MESSAGE(); fprintf(stderr, "Tree Error: calling moveTop() on empty tree\n");
        exit(1);
    }
    
    current = top;
    
	return;
}

/************************************************************************
 * movePrev
 * Moves current to the BranchV before it in tree.  This can move current
 * off end.  Exported.
 * Pre: !offEnd()
 ************************************************************************/
template <class T>
void TreeSimpleVec<T>::moveUp(){
    
    if( offEnd() ){
        ERROR_MESSAGE(); fprintf(stderr, "Tree Error: call to moveUp() when current is off end\n");
        exit(1);
    }
    if( current == top ){
        ERROR_MESSAGE(); fprintf(stderr, "Tree Error: call to moveUp() tried to move off the top\n");
        exit(1);
    }
    
    current = current->prev;  /* can move off end */
    return;
}

/************************************************************************
 * moveToChild
 * Moves current to child BranchV after it in tree.  This can move current off
 * end.  Exported.
 * Pre: !offEnd()
 ************************************************************************/
template <class T>
void TreeSimpleVec<T>::moveToChild(int child){
    
    if( offEnd() ){
        
        ERROR_MESSAGE(); fprintf(stderr, "Tree Error: calling moveChildren() when current is off end\n");
        exit(1);
    }
    if(child==1){
        if( current->child1 == NULL ){
            ERROR_MESSAGE(); fprintf(stderr, "Tree Error: moveToChild() typing to move to child1 when it doesn't exist\n");
            exit(1);
        }
        current = current->child1;
    }
    if(child==2){
        if( current->child2 == NULL ){
            ERROR_MESSAGE(); fprintf(stderr, "Tree Error: moveToChild() typing to move to child2 when it doesn't exist\n");
            exit(1);
        }
        current = current->child2;
    }
    return;
}

/************************************************************************
 * insertAfterCurrent
 * Inserts a new BranchV after the current BranchV in the tree and sets the
 * data field of the new BranchV to input.  Exported.
 * Pre: !offEnd()
 ************************************************************************/
template <class T>
void TreeSimpleVec<T>::attachChildToCurrent(IndexType *branch_index,IndexType nparticles
                                           ,PosType boundary_p1[],PosType boundary_p2[]
                                           ,int child){
    
    /*printf("attaching child%i  current paricle number %i\n",child,current->nparticles);*/


    BranchV *branchV= new BranchV(Ndimensions,branch_index,nparticles,boundary_p1,boundary_p2
                           ,current->level+1,Nbranches);
    
    if( offEnd() ){
        
        ERROR_MESSAGE(); fprintf(stderr, "Tree Error: calling attachChildToCurrent() when current is off end\n");
        exit(1);
    }
    
    branchV->prev = current;
    
    if(child==1){
        if(current->child1 != NULL){
            ERROR_MESSAGE(); fprintf(stderr, "Tree Error: calling attachChildToCurrent() when child1 alread exists\n");
            exit(1);
        }
        current->child1 = branchV;
    }
    if(child==2){
        if(current->child2 != NULL){
            ERROR_MESSAGE();
            fprintf(stderr, "Tree Error: calling attachChildToCurrent() when child2 alread exists\n  current level=%i Nbranches=%li\n"
                    ,current->level,Nbranches);
            exit(1);
        }
        current->child2 = branchV;
    }
    
    Nbranches++;
    
    return;
}

/* same as above but takes a BranchV structure */
template <class T>
void TreeSimpleVec<T>::attachChildToCurrent(BranchV &data,int child){
    
    attachChildToCurrent(data.branch_index,data.nparticles
                           ,data.boundary_p1,data.boundary_p2,child);
    return;
}

// step for walking tree by iteration instead of recursion
template <class T>
bool TreeSimpleVec<T>::TreeWalkStep(bool allowDescent){
	if(allowDescent && current->child1 != NULL){
		moveToChild(1);
		return true;
	}
	if(allowDescent && current->child2 != NULL){
		moveToChild(2);
		return true;
	}
    
	if(current->brother != NULL){
		current = current->brother;
		return true;
	}
	return false;
}
    
#endif /* SIMP_TREE_V_ */
