#include "fem.h"

#include "pt3D.h"

using namespace std;

void Fem::readMesh(Settings const& mySets)
{
int ELEM, tags, reg, TYP;
string trash, symb;

string str = mySets.getPbName();
ifstream msh(str);   // ouverture du fichier probleme en lecture

if (!msh)
    {
    if(mySets.verbose) { cerr << "cannot open file " << str << endl; } 
    SYSTEM_ERROR;
    }

    string mshFormat;
msh >> symb;
    if(symb == "$MeshFormat")
        {
        msh >> mshFormat;
        if(mshFormat == "2.2" )
            std::cout << "mesh file format 2.2" << std::endl;
        else { std::cout <<"mesh file format not supported : " << mshFormat << std::endl; SYSTEM_ERROR;}
        }
        
    
while(msh >> symb){
    if (symb == "$Nodes")
        break;
    }

if (msh.fail())
    {
    if(mySets.verbose) { cerr << "could not find $Nodes" << endl; }
    SYSTEM_ERROR;
    }

double scale = mySets.getScale();
int NOD;    
msh >> NOD;        // lecture des noeuds
node.resize(NOD);

for (int i=0; i<NOD; i++)
    {
    msh >> trash >> node[i].p;
	node[i].p.rescale(scale);
	//std::cout <<"scale=" << scale <<";" << trash << ";" << node[i].p << std::endl;
	}

if (msh.fail())
    {
    if(mySets.verbose) {cerr << "error while reading nodes" << endl;}
    SYSTEM_ERROR;
    }

while(msh >> symb){
    if (symb == "$Elements")
        break;
    }

if (msh.fail())
    {
    if(mySets.verbose) {cerr << "could not find $Elements" << endl;}
    SYSTEM_ERROR;
    }

msh >> ELEM;        // lecture des elements

while(msh >> symb){
    if (symb == "$EndElements" || (symb=="$End"))
        break;
    msh >> TYP >> tags >> reg;
    for (int i=1; i<tags; i++)
        msh >> trash;
    switch (TYP){
        case 2:{
            Facette::Fac f(NOD);
            f.reg = reg;
			f.idxPrm = mySets.findFacetteRegionIdx(reg);
            msh >> f.ind[0] >> f.ind[1] >> f.ind[2];
	    for (int i=0; i<3; i++)
	        f.ind[i]--;           // passage convention Matlab/msh a C++
		
            fac.push_back(f);
            break;
	    }
        case 4:{
            Tetra::Tet te(NOD);
            te.reg = reg; 
			te.idxPrm = mySets.findTetraRegionIdx(reg);
            msh >> te.ind[0] >> te.ind[1] >> te.ind[2] >> te.ind[3];
	    for (int i=0; i<4; i++)
	    	te.ind[i]--;           // passage convention Matlab/msh a C++
		
            tet.push_back(te);
            break;
	    }
        default:
            getline(msh,trash);
	}
    }

if (msh.fail()){
#ifdef LIBRARY
    throw runtime_error("error while reading elements");
#else
    cerr << "error while reading elements" << endl;
    exit(1);
#endif
    }

msh.close();
}

void Fem::readSol(bool VERBOSE,double scaling, string fileName)
{
ifstream fin(fileName, std::ifstream::in);
if (!fin){
    if(VERBOSE) { cerr << "cannot open .sol file: " << fileName << endl; }
    SYSTEM_ERROR;}

string str;
getline(fin, str); // 1eme ligne

unsigned long idx = str.find(":");
idx +=2;

t = stod(str.substr(idx));

if(VERBOSE) { cout << ".sol file: " << str << " @ time t = " << t << endl; }

const int NOD = node.size();

for (int i=0; i<NOD; i++)
    {
    Nodes::Node & n = node[i];
    Nodes::Node node_;
    int i_;
    fin >> i_ >>  node_.p >> n.u >> n.phi;// carefull! >> is overloaded for class pt3D

    if (i!=i_)
        {
        if(VERBOSE) { cerr << ".sol file mismatch with mesh nodes"<< endl; }
        fin.close();
        SYSTEM_ERROR;
        }
    else
        {
        node_.p.rescale(scaling);
        if (( Pt::norme2(n.p - node_.p) > sq(diam * 1e-9))&&VERBOSE) 
            { cerr << "WARNING difference in node positions"  << i  << "\t" << n.p << endl << i_ << "\t" << node_.p << endl; }
        }
    }
fin.close();    			     
}

