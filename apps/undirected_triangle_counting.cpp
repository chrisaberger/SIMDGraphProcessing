// class templates
#include "Matrix.hpp"

namespace application{
  Matrix *graph;
  uint8_t *result;
  long num_triangles = 0;
  
  size_t huge_diff = 0;
  size_t num_int = 0;

  inline bool myNodeSelection(unsigned int node){
    (void)node;
    return true;
  }
  inline bool myEdgeSelection(unsigned int node, unsigned int nbr){
    return nbr < node;
  }
  //Our functor that gets applied to every edge (or set element in the matrix)
  inline long edge_apply(unsigned int src, unsigned int dst){
    //cout << "n: " << n << " nbr: " << nbr << endl;
    long count = graph->row_intersect(result,src,dst);
    //cout << "count: " << count << endl;
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

  application::result = new uint8_t[vg->num_nodes];

  application::graph = new Matrix(vg,&application::myNodeSelection,&application::myEdgeSelection,common::A32BITPACKED);  
  //application::graph->print_matrix();  
  /*
  common::startClock();
  //application::num_triangles = application::graph->foreach_row(&Matrix::for_row,&application::edge_apply);
  common::stopClock("BITSET TRIANGLE COUNTING");
  cout << "Count: " << application::num_triangles << endl;
  */

  application::graph = new Matrix(vg,&application::myNodeSelection,&application::myEdgeSelection,common::ARRAY16);
  common::startClock();
  application::num_triangles = application::graph->foreach_row(&Matrix::for_row,&application::edge_apply);
  common::stopClock("ARRAY 16 TRIANGLE COUNTING");
  cout << "Count: " << application::num_triangles << endl << endl;

  application::graph = new Matrix(vg,&application::myNodeSelection,&application::myEdgeSelection,common::ARRAY32);
  //application::graph->print_matrix();
  common::startClock();
  application::num_triangles = application::graph->foreach_row(&Matrix::for_row,&application::edge_apply);
  common::stopClock("ARRAY 32 TRIANGLE COUNTING");
  cout << "Count: " << application::num_triangles << endl << endl;
  
  application::graph = new Matrix(vg,&application::myNodeSelection,&application::myEdgeSelection,common::HYBRID);
  common::startClock();
  application::num_triangles = application::graph->foreach_row(&Matrix::for_row,&application::edge_apply);
  common::stopClock("HYBRID TRIANGLE COUNTING");
  cout << "Count: " << application::num_triangles << endl << endl;

  return 0;
}
