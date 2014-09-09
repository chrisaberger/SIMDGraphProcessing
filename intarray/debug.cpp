// class templates
#include "structures/Matrix.cpp"
#include "structures/Common.hpp"

using namespace std;

namespace my_app{
  Matrix *graph;
  common::type my_type = common::HYBRID;

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
    cout << "n: " << n << " nbr: " << nbr << endl;
    long count = graph->row_intersect(result,n,nbr);
    cout << count << endl;
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

  common::startClock();
  VectorGraph *vg = ReadFile(argv[1],atoi(argv[2]));
  common::stopClock("INPUT");
  cout << endl;


  my_app::graph = new Matrix(vg,&my_app::myNodeSelection,&my_app::myEdgeSelection,my_app::my_type);
  my_app::result = new unsigned short[vg->num_nodes*(vg->num_nodes/2)];

  my_app::graph->print_columns(70266,68872);
  my_app::num_triangles = my_app::graph->row_intersect(my_app::result,70266,68872);
  cout << "Count: " << my_app::num_triangles << endl;

  return 0;
}
