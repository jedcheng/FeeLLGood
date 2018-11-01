/** \file linear_algebra.h
\brief secondary header, it grabs altogether the linear algebra by the solver to apply fem method  <br>
It encapsulates the calls to MTL4 , the assemblage and projection of the matrix for all elements <br> 
projection and matrix assembly is multithreaded for tetrahedron, monothread for facette
*/

#include <thread>
#include <mutex>
#include <future>

#include "boost/numeric/mtl/mtl.hpp"
#include "boost/numeric/itl/itl.hpp"

#include <boost/numeric/mtl/matrix/inserter.hpp>
#include <boost/numeric/mtl/operation/set_to_zero.hpp>
#include <boost/numeric/mtl/interface/vpt.hpp>

#include "fem.h"

#ifndef linear_algebra_h
#define linear_algebra_h

/** convenient typedef for mtl4 */
typedef typename mtl::Collection< mtl::compressed2D<double> >::value_type v_type;

/** convenient typedef for mtl4 inserter */
typedef mtl::mat::inserter< mtl::compressed2D<double>,mtl::update_plus<v_type> > sparseInserter;


class Obj
    {
    public:
        inline Obj(const int _N,const int _ind[],mtl::dense2D <double> const& K,mtl::dense_vector <double> const& L):N(_N)
        {
        ind = new int[N];
        for(int i=0;i<N;i++) {ind[i] = _ind[i];}   
        Ke=K;Le=L;
        }
        
        const int N;
        int *ind;
        mtl::dense2D <double> Ke;
        mtl::dense_vector <double> Le;
    };


/** \class LinAlgebra
convenient class to grab altogether some part of the calculations involved using gmm solver at each timestep
*/
class LinAlgebra
{
public:
	/** constructor */	
    inline LinAlgebra(Settings & s,const int _NOD,
                      std::vector<Node> & myNode,
                      std::vector <Tetra::Tet> & myTet,
                      std::vector <Facette::Fac> & myFace,const int _Nb) : NOD(_NOD),refNode(&myNode),refFac(&myFace) , NbTH(_Nb)
{
    settings = &s; 
    my_lock = new std::mutex;
    lock_buff = new std::mutex;
    tab_TH.resize(NbTH+1);
    
    refTet.resize(NbTH);
    const unsigned long block_size = std::distance(myTet.begin(),myTet.end())/NbTH;

    std::vector<Tetra::Tet>::iterator it_begin = myTet.begin();

    for(int i=0;i<(NbTH-1);i++) 
        {
        std::vector<Tetra::Tet>::iterator it_end = it_begin;
        std::advance(it_end,block_size);
        refTet[i].resize(block_size);
        std::copy( it_begin, it_end, refTet[i].begin() );
        it_begin = it_end;
        }
    const unsigned long last_block_size = std::distance(it_begin,myTet.end());
    refTet[NbTH-1].resize(last_block_size);
    std::copy( it_begin, myTet.end(), refTet[NbTH-1].begin() );
    
    std::cout<<"splitted copy of vector(tet) done."<<std::endl;
}
    
	
	/** pointer to diagonal preconditionner  */
	itl::pc::diagonal < mtl::compressed2D<double> > *prc;

    /** solver, uses bicgstab and gmres */
	int  vsolve(long nt);

    /** setter for dt */
    inline void set_dt(double _dt){dt = _dt;}
    
    /** setter for DW_dz */
    inline void set_DW_vz(double vz){DW_vz = vz;}    

    /** setter for Hext */
    inline void set_Hext(double Hx,double Hy,double Hz){Hext[0]=Hx;Hext[1]=Hy;Hext[2]=Hz;}

    /** getter for v_max */
    inline double get_v_max() {return v_max;}
    
    /** getter node */
    inline Node getNode(int i) {return (*refNode)[i];}
    
    /** getter node physical position */
    inline Pt::pt3D getNodePhysPos(int i) {return (*refNode)[i].p;} 
    
    /** pointer to sparse matrix inserter */
	sparseInserter *ins;
    
private:
    const int NOD;/** total number of nodes, also an offset for filling sparseMatrix, initialized by constructor */
    std::vector<Node>  *refNode;/**< direct access to the Nodes */
	std::vector <Facette::Fac> *refFac; /**< direct access to the faces */
	
	std::vector < std::vector <Tetra::Tet> > refTet; /**< splitted copy of the tetrahedrons for multithreading */
	
	
	
	double Hext[DIM];/**< applied field */
    double dt;/**< timestep */
    double DW_vz;/**< speed of the domain wall */
	Settings *settings;/**< pointer to the settings */
    double v_max;/**< maximum speed */
	
	/** mutex to avoid improper access to inserter */
    std::mutex *my_lock;
    
    /** mutex to avoid improper access to buffer */
    std::mutex *lock_buff;
    
    /** number of threads, initialized by constructor */ 
    const int NbTH;
    
    /** buffer when try_lock failed on assemblage */
    std::vector<Obj> buff_TH;
    /** thread vector */
    std::vector<std::thread> tab_TH;
    
/** computes the local vector basis {ep,eq} in the tangeant plane for projection on the elements */
inline void base_projection(void)
	{ std::for_each(refNode->begin(),refNode->end(),[](Node &n) { n.buildBase_epeq();}); }
	
	/** assemblage without mutexes to empty buffer */
    void assemblage_monoThread(const int N, const int ind[],
           mtl::dense2D <double> const& Ke, mtl::dense_vector <double> const& Le,
           mtl::dense_vector<double> &L);
    
    
    /**
    perform the matrix and vector assembly with all the contributions of the tetrahedrons, use two mutexs
    */
    void assemblage(const int N,const int ind[],mtl::dense2D <double> const& Ke, mtl::dense_vector <double> const& Le,
                    mtl::dense_vector<double> &L);
    
}; // fin class linAlgebra

#endif
