// class templates
#include "structures/Matrix.cpp"
#include "structures/Common.hpp"

using namespace std;

namespace my_app{
  Matrix *graph;
  common::type my_type = common::ARRAY32;

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
    return graph->row_intersect(result,n,nbr);
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
  common::startClock();
  //For each column
  //my_app::graph->print_matrix();
  my_app::num_triangles = my_app::graph->foreach_column(&Matrix::for_row,&my_app::triangle_counting);
  common::stopClock("V16: Triangle Counting");
  cout << "Count: " << my_app::num_triangles << endl;

  /*
  my_app::num_triangles = 0;
  my_type = common::ARRAY32; 
  graph = new Matrix(vg,&my_app::myNodeSelection,&my_app::myEdgeSelection,my_type);

  common::startClock();
  //For each column
  graph->foreach_column(
    //for each row
    &Matrix::for_row,
      //apply triangle counting
      &my_app::triangle_counting
  );
  common::stopClock("V32: Triangle Counting");
  cout << "Count: " << my_app::num_triangles << endl;
  */

  return 0;
}
