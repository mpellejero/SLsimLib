/*
 * Code Name:     tree.c                                       
 * Programmer:    R Ben Metcalf
 * Last Revised:  Nov, 2005                                   
 * Discription:  2 way tree data structure with 2 branches
 * Comments:                           
 */

#include "slsimlib.h"

/***** Structs *****/

/* Branch: Private struct, not exported */ 


/************************************************************************
 * NewBranch
 * Returns pointer to new Branch struct.  Initializes children pointers to NULL,
 * and sets data field to input.  Private.
 ************************************************************************/
/** \ingroup ConstructorL2
 *
 * Gives each branch a unique number even if branches are destroed.
 */
/*Branch *NewBranch(Point *points,unsigned long npoints
		  ,double boundary_p1[2],double boundary_p2[2]
		  ,double center[2],int level){

    //Branch *branch;
    int i;
    static unsigned long number = 0;

    //branch = (Branch *)malloc(sizeof(Branch));
    if (!branch){
      ERROR_MESSAGE(); std::cout << "allocation failure in NewBranch()" << std::endl;
      assert(branch);
      exit(1);
    }
    branch->points = points;
    branch->npoints = npoints;

    branch->center[0]=center[0];
    branch->center[1]=center[1];
    branch->level=level;

    for(i=0;i<2;++i){
      branch->boundary_p1[i]= boundary_p1[i];
      branch->boundary_p2[i]= boundary_p2[i];
    }

    branch->number = number;
    ++number;

    branch->child1 = NULL;
    branch->child2 = NULL;
    branch->brother = NULL;
    branch->prev = NULL;
    branch->refined = false;

    //return branch;
}*/

/// print out all member data for testing purposes
void Point::Print(){
	 std::cout << "Point Data : " << std::endl;
	 std::cout << "  next " << next << std::endl;
	 std::cout << "  prev " << prev << std::endl;
	 std::cout << "  image " << image << std::endl;
	 std::cout << "  id " << id << std::endl;
	 std::cout << "  x " << x[0] << "    " << x[1] << std::endl;
	 std::cout << "  head " << head << std::endl;
	 std::cout << "  in_image " << in_image << std::endl;
	 std::cout << "  kappa " << kappa << std::endl;
	 std::cout << "  gamma " << gamma[0] << " " << gamma[1] << " " << gamma[3] << std::endl;
	 std::cout << "  dt " << dt << std::endl;
	 std::cout << "  invmag " << invmag << std::endl;
	 std::cout << "  gridsize " << gridsize << std::endl;
	 std::cout << "  surface_brightness " << surface_brightness << std::endl;
	 std::cout << "  leaf " << leaf << std::endl;
}
/// print just position and gridsize
void Point::print(){
	std::cout << x[0] <<  "  " << x[1] << "  " << gridsize << std::endl;
}



unsigned long Branch::countID = 0;

Branch::Branch(Point *my_points,unsigned long my_npoints
		  ,double my_boundary_p1[2],double my_boundary_p2[2]
		  ,double my_center[2],int my_level){

    points = my_points;
    npoints = my_npoints;

    center[0]=my_center[0];
    center[1]=my_center[1];
    level=my_level;

    for(int i=0;i<2;++i){
      boundary_p1[i]= my_boundary_p1[i];
      boundary_p2[i]= my_boundary_p2[i];
    }

    number = countID++;

    child1 = NULL;
    child2 = NULL;
    brother = NULL;
    prev = NULL;
    refined = false;
}
Branch::~Branch(){
	neighbors.clear();
}
/// print out all member data for testing purposes
void Branch::print(){
	 std::cout << "Branch Data : " << std::endl;
	 std::cout << "  points " << points << std::endl;
	 std::cout << "  npoints " << npoints << std::endl;
	 std::cout << "  level " << level << std::endl;
	 std::cout << "  center " << center[0] << "    " << center[1] << std::endl;
	 std::cout << "  number " << number << std::endl;
	 std::cout << "  boundary_p1 " << boundary_p1[0] << "    " << boundary_p1[1] << std::endl;
	 std::cout << "  boundary_p1 " << boundary_p2[0] << "    " << boundary_p2[1] << std::endl;
	 std::cout << "  child1 " << child1 << std::endl;
	 std::cout << "  child2 " << child2 << std::endl;
	 std::cout << "  brother " << brother << std::endl;
	 std::cout << "  prev " << prev << std::endl;
	 std::cout << "  refined " << refined << std::endl;
}

/** \ingroup ConstructorL2
*
void FreeBranch(Branch *branch){

    assert( branch != NULL);
    free(branch);

    return;
}*/

  /** \ingroup ConstructorL2
 **/
Point *NewPointArray(
		unsigned long N  /// number of points in array
		,bool NewXs   /// Allocate memory for point positions or assign pointer to existing position
		){
  Point *points;
  unsigned long i;

  if(N <= 0) return NULL;
  //points = (Point *) calloc(N, sizeof(Point));
  points = new Point[N];
  //if(NewXs) points[0].x = (double *) calloc(2,sizeof(double));
  if(NewXs) points[0].x = new double[2];
  points[0].head = N;
  points[0].in_image = FALSE;
  points[0].surface_brightness = 0;
  points[0].leaf = NULL;
  points[0].image = NULL;

  for(i = 1; i < N; i++)
  	  {
	  	  //if(NewXs) points[i].x = (double *) calloc(2,sizeof(double));
	  	  if(NewXs) points[i].x = new double[2];
	  	  points[i].head = 0;
	  	  points[i].in_image = FALSE;
	  	  points[i].surface_brightness = 0;
	  	  points[i].leaf = NULL;
        points[i].image = NULL;
  	  }

  return points;
}

/** \ingroup ConstructorL2
 *
 */
void FreePointArray(Point *array,bool NewXs){
  /* Note: this deallocates positions!! */
  unsigned long i;

  if(array[0].head){
	  //if(NewXs) for(i=0;i<array[0].head;++i) free(array[i].x);
	  //free(array);
	  if(NewXs) for(i=0;i<array[0].head;++i) delete[] array[i].x;
	  delete[] array;
  }else{
	  ERROR_MESSAGE();
	  std::cout << "ERROR: FreePointArray, miss aligned attempt to free point array" << std::endl;
	  exit(1);
  }
}

/**
 *  \brief  Make a new tree and the linked list of points in it.  Does
 *  not build the tree structure.  The other constructor should be used
 *  to build the whole tree.
 */
TreeStruct::TreeStruct(
		Point *xp   /// array of points to be added to the tree
		,unsigned long npoints   /// number of points
		,double boundary_p1[2]   /// bottom left hand corner of root
		,double boundary_p2[2]   /// upper right hand corner of root
		,double center[2]        /// center of root (this could be the center of mass)
		,int my_Nbucket             /// maximum number of points allowed in a leaf
		){
	construct_root(xp,npoints,boundary_p1,boundary_p2,center,my_Nbucket);
}
/// Basic construction of root with all particles in it but no children
void TreeStruct::construct_root(
		Point *xp   /// array of points to be added to the tree
		,unsigned long npoints   /// number of points
		,double boundary_p1[2]   /// bottom left hand corner of root
		,double boundary_p2[2]   /// upper right hand corner of root
		,double center[2]        /// center of root (this could be the center of mass)
		,int my_Nbucket             /// maximum number of points allowed in a leaf
		){
  unsigned long i;

  /*TreeStruct *tree;
  tree = (TreeStruct *)malloc(sizeof(TreeStruct));
  if (!tree){
    ERROR_MESSAGE();
    std::cout << "ERROR: allocation failure in NewTree()" << std::endl;
    exit(1);
  }*/

    /* make linked list of points */
  pointlist=NewList();
   //EmptyList(pointlist);
  for(i=0;i<npoints;++i){
    InsertPointAfterCurrent(pointlist,&xp[i]);
    MoveDownList(pointlist);
  }

  top = new Branch(pointlist->top,npoints,boundary_p1,boundary_p2
		      ,center,0);

  Nbranches = 1;
  current = top;

  Nbucket = my_Nbucket;
  //return(tree);
}

/** \ingroup ConstructorL2
 * \brief Free tree and the linked list of points in it.
 */
TreeStruct::~TreeStruct(){

	emptyTree();
	free(current);
	--Nbranches;

	free(pointlist);
	//free(tree);

	/*tree = NULL;
	return 1;*/
}


/***** Access functions *****/

/************************************************************************
 * isEmpty
 * Returns "true" if the Tree is empty and "false" otherwise.  Exported.
 ************************************************************************/
bool TreeStruct::isEmpty(){

    return(Nbranches == 0);
}

/************************************************************************
 * atTop
 * Returns "true" if current is the same as top and "false" otherwise.
 * Exported.
 * Pre: !isEmpty(tree)
 ************************************************************************/
bool TreeStruct::atTop(){

    if( isEmpty() ){
	
	ERROR_MESSAGE();
    std::cout << "Tree Error: calling atTop() on empty tree" << std::endl;
	exit(1);
    }
    return(current == top);
}

/************************************************************************
 * noChild
 * Returns "true" if the child of the current branch does not exist and "false" otherwise.
 * Exported.
 * Pre: !isEmpty(tree)
 ************************************************************************/
bool TreeStruct::noChild(){

	if( isEmpty() ){
		ERROR_MESSAGE();
		std::cout << "Tree Error: calling atTop() on empty tree" << std::endl;
		exit(1);
    }

    if( (current->child1 == NULL) || (current->child2 == NULL) ) return true;
    return false;
}

/************************************************************************
 * offEnd
 * Returns "true" if current is off end and "false" otherwise.  Exported.
 ************************************************************************/
bool TreeStruct::offEnd(){
    return(current == NULL);
}

bool TreeStruct::CurrentIsSquareBranch(){
	if( fabs(1 - (current->boundary_p2[0] - current->boundary_p1[0])
			    /(current->boundary_p2[1] - current->boundary_p1[1]) )
			< 0.05){
		return true;
	}

	return false;
}

/************************************************************************
 * getCurrent
 * Returns the points of current.  Exported.
 * Pre: !offEnd(tree)
 ************************************************************************/
void TreeStruct::getCurrent(Point *points,unsigned long *npoints){

    if( offEnd() ){
    	ERROR_MESSAGE();
    	std::cout << "Tree Error: calling getCurrent() when current is off end" << std::endl;
    	exit(1);
    }

    *npoints=current->npoints;
    points=current->points;

    return;
}

/************************************************************************
 * getNbranches
 * Returns the Nbranches of tree.  Exported.
 ************************************************************************/
unsigned long TreeStruct::getNbranches(){

    return(Nbranches);
}

/***** Manipulation procedures *****/

/************************************************************************
 * movePrev
 * Moves current to the branch before it in tree.  This can move current
 * off end.  Exported.
 * Pre: !offEnd(tree)
 ************************************************************************/
bool TreeStruct::moveToChild(int child){
    
    assert(current != NULL);

    if(child==1){
      if( current->child1 == NULL ) return false;
      current = current->child1;
      return true;
    }
    if(child==2){
      if( current->child2 == NULL ) return false;
      current = current->child2;
      return true;
    }
    return false;
}

bool TreeStruct::moveUp(){

    assert(!offEnd());
    /*if( offEnd(tree) ){
      ERROR_MESSAGE();
      std::cout << "Tree Error: calling moveUp() when current is off end" << std::endl;
      exit(1);
    }*/

    if( current == top ) return false;
    assert(current->prev);
    current = current->prev;  /* can move off end */
    return true;
}

/************************************************************************
 * moveToChild
 * Moves current to child branch after it in tree.  This can move current off
 * end.  Exported.
 * Pre: !offEnd(tree)
 ************************************************************************/
/************************************************************************
 * insertAfterCurrent
 * Inserts a new Branch after the current branch in the tree and sets the
 * data field of the new Branch to input.  Exported.
 * Pre: !offEnd(tree)
 ************************************************************************/
/*void insertChildToCurrent(TreeHndl tree,Point *points,unsigned long npoints
			  ,double boundary_p1[2],double boundary_p2[2]
			  ,double center[2],int child){
    
    Branch *branch;

    //branch = NewBranch(points,npoints,boundary_p1,boundary_p2,center
 	//	       ,tree->current->level+1);
    branch = new Branch(points,npoints,boundary_p1,boundary_p2,center
 		       ,tree->current->level+1);

    assert(tree != NULL);
    
    if( offEnd(tree) ){
    	ERROR_MESSAGE();
        std::cout << "Tree Error: calling insertChildToCurrent() when current is off end" << std::endl;
    	exit(1);
    }

    branch->prev = tree->current;

    if(child==1){
      if(tree->current->child1 != NULL){
    	  ERROR_MESSAGE();
          std::cout << "Tree Error: calling insertChildToCurrent() when child1 already exists" << std::endl;
    	  exit(1);
      }
      tree->current->child1 = branch;
      tree->current->child1->brother = tree->current->child2;
    }
    if(child==2){
      if(tree->current->child2 != NULL){
    	  ERROR_MESSAGE();
          std::cout << "Tree Error: calling insertChildToCurrent() when child2 already exists" << std::endl;
          exit(1);
      }
      tree->current->child2 = branch;      
      tree->current->child2->brother = tree->current->brother;
    }

    tree->Nbranches++;

    return;
}*/
void TreeStruct::insertChildToCurrent(Branch *branch,int child){

    assert(branch->boundary_p1[0] >= current->boundary_p1[0]);
    assert(branch->boundary_p1[1] >= current->boundary_p1[1]);
    assert(branch->boundary_p2[0] <= current->boundary_p2[0]);
    assert(branch->boundary_p2[1] <= current->boundary_p2[1]);

    if( offEnd() ){
    	ERROR_MESSAGE();
        std::cout << "Tree Error: calling insertChildToCurrent() when current is off end" << std::endl;
    	exit(1);
    }

    branch->prev = current;

    if(child==1){
      if(current->child1 != NULL){
    	  ERROR_MESSAGE();
          std::cout << "Tree Error: calling insertChildToCurrent() when child1 already exists" << std::endl;
    	  exit(1);
      }
      current->child1 = branch;
      current->child1->brother = current->child2;
    }
    if(child==2){
      if(current->child2 != NULL){
    	  ERROR_MESSAGE();
          std::cout << "Tree Error: calling insertChildToCurrent() when child2 already exists" << std::endl;
          exit(1);
      }
      current->child2 = branch;
      current->child2->brother = current->brother;
    }

    if(branch->npoints > 0){
    	pointlist->current = branch->points;
    	for(unsigned long i=0;i<branch->npoints;++i){
    		pointlist->current->leaf = branch;
    		MoveDownList(pointlist);
    	}
    }

    Nbranches++;

    return;
}

  /* same as above but takes a branch structure */
/*
void attachChildToCurrent(TreeHndl tree,Branch data,int child){

	  //insertChildToCurrent(tree,data.points,data.npoints,data.boundary_p1,data.boundary_p2,data.center,child);
	  insertChildToCurrent(tree,data,child);
  return;
}*/

void TreeStruct::attachChildrenToCurrent(Branch* child1,Branch* child2){
	// this is an addition that keeps assigns the brother pointers

	/*
	insertChildToCurrent(tree,child1.points,child1.npoints
			,child1.boundary_p1,child1.boundary_p2,child1.center,1);
	insertChildToCurrent(tree,child2.points,child2.npoints
			,child2.boundary_p1,child2.boundary_p2,child2.center,2);
*/

	assert(current->child1 == NULL);
	insertChildToCurrent(child1,1);
	assert(current->child2 == NULL);
	insertChildToCurrent(child2,2);

	current->child1->brother = current->child2;
	current->child2->brother = current->brother;

	// update lists of branch neighbors
	Branch *neighbor;
	current->child1->neighbors.push_back(current->child2);
	current->child2->neighbors.push_back(current->child1);
	std::list<Branch *>::iterator jt,it;
	for( it=current->neighbors.begin() ; it != current->neighbors.end() ; ++it){
		neighbor = *it;//current->neighbors[i];
		for(jt=neighbor->neighbors.begin();jt != neighbor->neighbors.end();++jt){
			if(*jt == current){
				if(AreBoxNeighbors(neighbor,current->child1)){
					current->child1->neighbors.push_back(neighbor);
					neighbor->neighbors.insert(jt,current->child1);
				}
				if(AreBoxNeighbors(neighbor,current->child2)){
					current->child2->neighbors.push_back(neighbor);
					neighbor->neighbors.insert(jt,current->child2);
				}
				assert(*jt == current);
				jt = neighbor->neighbors.erase(jt);
				--jt;
			}
		}
	}

	current->neighbors.clear();
	return;
}

/***** Other operations *****/


/************************************************************************
 * printTree
 * Prints the contents of tree to stdout.  The current element, if there
 * is one, will be enclosed in []'s.  Currently, only to be used when the
 * TreeElements is are integers.  Exported.
 ************************************************************************/  
void TreeStruct::printTree(){
  int i;

    printBranch(current);
    pointlist->current=current->points;
    for(i=0;i<current->npoints;++i){
      std::cout << pointlist->current->id << " " << pointlist->current->x[0] << " " << pointlist->current->x[1] << std::endl;
      MoveDownList(pointlist);
    }
    if(current->child1 == NULL) return;

    if( (current->boundary_p1[0]==current->boundary_p2[0]) ||
    		(current->boundary_p1[0]==current->boundary_p2[0])	){
    	ERROR_MESSAGE();
    	std::cout << "ERROR: zero area branch" << std::endl;
    	exit(0);
    }
    moveToChild(1);
    printTree();

    moveUp();

    moveToChild(2);
    printTree();

    moveUp();

    return;
}

void printBranch(Branch *data){

  std::cout << "******* branch *******" << std::endl;
  std::cout << "level=" << data->level << " number=" << data->number << std::endl;
  std::cout << "center = [" << data->center[0] << "," << data->center[1] << "]" << std::endl;
  std::cout << "p1 = [" << data->boundary_p1[0] << "," << data->boundary_p1[1] << "]" << std::endl;
  std::cout << "p2 = [" << data->boundary_p2[0] << "," << data->boundary_p2[1] << "]" << std::endl;
  std::cout<< "number of points = " << data->npoints << std::endl;
}
/****************************************************************
 *  code for testing tree
 *****************************************************************/

void TreeStruct::checkTree(){
	Branch *initial;
	unsigned long count=0;

	initial=current;

	moveTop();
	_checkTree(&count);

	if(count != Nbranches){ std::cout << "checkTree did not reach all branches" << std::endl; exit(0);}
	current=initial;
	return;
}

void TreeStruct::_checkTree(unsigned long *count){
	int checkBranch(Branch *branch);

	//std::printf("     hello\n");
	++*count;
	if(checkBranch(current)) exit(1);

	if(current->child1 != NULL){
		moveToChild(1);
		_checkTree(count);
		moveUp();
	}

	if(current->child2 != NULL){
		moveToChild(2);
		_checkTree(count);
		moveUp();
	}

    return;
}

// put in test for each branch
int checkBranch(Branch *branch){
   	if(branch->boundary_p1[0] >= branch->boundary_p2[0]
   	  || branch->boundary_p1[1] >= branch->boundary_p2[1] ){
    		std::cout << std::endl << "this branch is screwed" << std::endl;
    		printBranch(branch);
    		PrintPoint(branch->points);
    		return 1;
   	}
   	return 0;
}

Point *AddPointToArray(Point *points,unsigned long N,unsigned long Nold){

  if(N==Nold) return points;

  unsigned long i;
  Point *newpoints;

  if(Nold==0){
	  newpoints = NewPointArray(N,true);
  }else{
	  if(points[0].head != Nold){ ERROR_MESSAGE(); std::cout << "ERROR: AddPointToArray head not set correctly" << std::endl; exit(0);}
	  newpoints = NewPointArray(N,false);
	  for(i=N;i<Nold;++i) free(points[i].x);

	  assert(points);
	  for(i=0;(i<N && i<Nold);++i){
		  PointCopy(&newpoints[i],&points[i]);
		  assert(newpoints[i].x);
	  }
	  for(i=Nold;i<N;++i) newpoints[i].x = (double *) malloc(2*sizeof(double));
  }

  FreePointArray(points,false);
  return newpoints;
}

Point *NewPoint(double *x,unsigned long id){
  Point *point;

  point=(Point *) malloc(sizeof(Point));
  assert(point);

  point->head = 1;
  point->id=id;
  point->x=x;
  point->in_image=FALSE;

  if (!point){
    ERROR_MESSAGE(); std::cout << "allocation failure in NewPoint()" << std::endl;
    exit(1);
  }
  point->next = point->prev = NULL;
  point->leaf=NULL;

  return(point);
}

/** SWAPS information in points without changing
* pointers to prev and next
* changes links of image points to follow */
void SwapPointsInArray(Point *p1,Point *p2){
  Point pt;

  if(p1==p2) return;

  PointCopy(&pt,p1);
  PointCopy(p1,p2);
  PointCopy(p2,&pt);
}
/** SWAPS information in points without changing
* pointers to prev and next
* Does not change links of image points to follow */
void SwapPointsInArrayData(Point *p1,Point *p2){
  Point pt;

  if(p1==p2) return;

  PointCopyData(&pt,p1);
  PointCopyData(p1,p2);
  PointCopyData(p2,&pt);
}

/** copies information in point without copying
 * pointers to prev and next, but moving the link
 * to the image point, does not touch head */
void PointCopy(Point *pcopy,Point *pin){
  pcopy->id = pin->id;
  pcopy->image = pin->image;
  pcopy->invmag = pin->invmag;
  pcopy->kappa = pin->kappa;
  pcopy->gamma[0] = pin->gamma[0];
  pcopy->gamma[1] = pin->gamma[1];
  pcopy->gamma[2] = pin->gamma[2];
  pcopy->dt = pin->dt;
  pcopy->x = pin->x;
  pcopy->gridsize = pin->gridsize;
  pcopy->in_image = pin->in_image;
  pcopy->surface_brightness = pin->surface_brightness;
  pcopy->leaf = pin->leaf;

  if((pin->image != NULL) && (pin->image->image == pin)) pin->image->image = pcopy;
}
void PointCopyData(Point *pcopy,Point *pin){
  /* copies information in point without copying */
  /* pointers to prev and next */
  /* does copy image pointer, does not touch head */
  pcopy->id = pin->id;
  pcopy->image = pin->image;
  pcopy->invmag = pin->invmag;
  pcopy->kappa = pin->kappa;
  pcopy->gamma[0] = pin->gamma[0];
  pcopy->gamma[1] = pin->gamma[1];
  pcopy->gamma[2] = pin->gamma[2];
  pcopy->dt = pin->dt;
  pcopy->x = pin->x;
//  pcopy->x[0] = pin->x[0];
//  pcopy->x[1] = pin->x[1];
  pcopy->gridsize = pin->gridsize;
  pcopy->in_image = pin->in_image;
  pcopy->surface_brightness = pin->surface_brightness;
  pcopy->leaf = pin->leaf;
}

/*********************************/
/*  point extraction routines */
/*********************************/
void TreeStruct::PointsInCurrent(unsigned long *ids,double **x){
  Point *point;
  unsigned long i;

  pointlist->current=current->points;

  point=current->points;
  for(i=0;i<current->npoints;++i){
    x[i]=pointlist->current->x;
    ids[i]=pointlist->current->id;
    MoveDownList(pointlist);
  }

  return;
}

void PrintPoint(Point *point){
  std::cout << "Point id = " << point->id << std::endl;
  std::cout << "   x= " <<  point->x[0] << " " << point->x[1] << " gridsize = " << point->gridsize << std::endl;
  std::cout << "   kappa= " << point->kappa;
  std::cout << " gamma = " << point->gamma[0] << " " << point->gamma[1];
  std::cout << " invmag " << point->in_image << std::endl;
}
/** \ingroup Constructor
 * Make an array of imageinfo types.
 */
ImageInfo::ImageInfo(){

  imagekist = new Kist<Point>;
  innerborder = new Kist<Point>;
  outerborder = new Kist<Point>;
  
  ShouldNotRefine = 0;
  uniform_mag = unchecked;

  area = area_error = 0.0;
  centroid[0] = centroid[1] = 0.0;
  gridrange[0] = gridrange[1] = gridrange[2] = 0.0;
}

/*ImageInfo *NewImageInfo(int Nimages){
  ImageInfo *imageinfo;
  int i;

  imageinfo=(ImageInfo *) malloc(Nimages*sizeof(ImageInfo));
  assert(imageinfo);

  for(i = 0;i < Nimages; i++)
  {
	imageinfo[i].imagekist = new Kist<Point>;
	imageinfo[i].Npoints=0;
    imageinfo[i].innerborder = new Kist<Point>;
    imageinfo[i].outerborder = new Kist<Point>;
  }

  return imageinfo;
}*/

/** \ingroup Constructor
 * Destructor of imageinfo types.
 */
ImageInfo::~ImageInfo(){
	delete imagekist;
    delete innerborder;
    delete outerborder;
}
/**
 * \brief Copy all information about the image including making copies of the imagekist,
 * innerborder and outerborder.  Previous information in the image will be
 */
void ImageInfo::copy(
                     ImageInfo &image   /// image to be copied
                     ,bool copykists    /// Turn off copying imagekist, innerborder and outerborder.
                                        /// This is useful when the grid has already been distroyed.
                     ){

  if(copykists){
    imagekist->copy(image.imagekist);
    innerborder->copy(image.innerborder);
    outerborder->copy(image.outerborder);
  }
  
	gridrange[0] = image.gridrange[0];
	gridrange[1] = image.gridrange[1];
	gridrange[2] = image.gridrange[2];

	centroid[0] = image.centroid[0];
	centroid[1] = image.centroid[1];

	area = image.area;
	area_error = image.area_error;
	ShouldNotRefine = image.ShouldNotRefine;
	uniform_mag = image.uniform_mag;
}
/*
void freeImageInfo(ImageInfo *imageinfo,int Nimages){
	int i;

	//freeKist(imageinfo->imagekist);
	for(i=0;i<Nimages;++i){
		delete imageinfo[i].imagekist;
	    delete imageinfo[i].innerborder;
	    delete imageinfo[i].outerborder;
	}
	free(imageinfo);
}
*/

OldImageInfo::OldImageInfo(){

  innerborder = new Kist<Point>;
  outerborder = new Kist<Point>;
}
OldImageInfo::~OldImageInfo(){
    delete innerborder;
    delete outerborder;
}

/** \ingroup LowLevel
 *  step for walking tree by iteration instead of recursion
 */
bool TreeStruct::TreeWalkStep(bool allowDescent){

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

