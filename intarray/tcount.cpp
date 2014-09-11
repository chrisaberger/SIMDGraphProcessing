// class templates
#include "structures/Matrix.cpp"
#include "structures/Common.hpp"

using namespace std;

namespace my_app{
  Matrix *graph;
  common::type my_type = common::BITSET;

  unsigned short *result;
  long num_triangles = 0;
  
  inline bool myNodeSelection(unsigned int node){
    return true;
  }
  inline bool myEdgeSelection(unsigned int node, unsigned int nbr){
    return nbr < node;
  }
  //Our functor that gets applied to every edge (or set element in the matrix)
  inline long triangle_counting(unsigned int n, unsigned int nbr){
    //cout << "n: " << n << " nbr: " << nbr << endl;
    long count = graph->row_intersect(result,n,nbr);
    //cout << count << endl;
    return count;
  }
}


int main (int argc, char* argv[]) { 
  if(argc != 4){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of files> <# of threads>" << endl;
    exit(0);
  }

  cout << "Number of threads: " << atoi(argv[3]) << endl;
  omp_set_num_threads(atoi(argv[3]));        

  //common::startClock();
  VectorGraph *vg = ReadFile(argv[1],atoi(argv[2]));
  //common::stopClock("INPUT");
  cout << endl;


  my_app::graph = new Matrix(vg,&my_app::myNodeSelection,&my_app::myEdgeSelection,common::BITSET);
  my_app::result = new unsigned short[vg->num_nodes];
  
  common::startClock();
  my_app::num_triangles = my_app::graph->foreach_column(&Matrix::for_row,&my_app::triangle_counting);
  common::stopClock("BITSET TRIANGLE COUNTING");
  cout << "Count: " << my_app::num_triangles << endl;

  my_app::graph = new Matrix(vg,&my_app::myNodeSelection,&my_app::myEdgeSelection,common::ARRAY16);
  common::startClock();
  my_app::num_triangles = my_app::graph->foreach_column(&Matrix::for_row,&my_app::triangle_counting);
  common::stopClock("ARRAY 16 TRIANGLE COUNTING");
  cout << "Count: " << my_app::num_triangles << endl;
  
  /*  
  my_app::graph = new Matrix(vg,&my_app::myNodeSelection,&my_app::myEdgeSelection,common::ARRAY32);
  common::startClock();
  my_app::num_triangles = my_app::graph->foreach_column(&Matrix::for_row,&my_app::triangle_counting);
  common::stopClock("ARRAY 32 TRIANGLE COUNTING");
  cout << "Count: " << my_app::num_triangles << endl;

  my_app::graph = new Matrix(vg,&my_app::myNodeSelection,&my_app::myEdgeSelection,common::HYBRID);
  common::startClock();
  my_app::num_triangles = my_app::graph->foreach_column(&Matrix::for_row,&my_app::triangle_counting);
  common::stopClock("HYBRID TRIANGLE COUNTING");
  cout << "Count: " << my_app::num_triangles << endl;
  */
  return 0;
}
