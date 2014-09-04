// class templates
#include "structures/Matrix.cpp"
#include "structures/Common.hpp"

using namespace std;

namespace my_app{
  inline bool myNodeSelection(int node){
    return true;
  }
  inline bool myEdgeSelection(int node, int nbr){
    return nbr < node;
  }

  short *result;
  size_t num_triangles = 0;
  inline void triangle_counting(int n, int nbr, Matrix *m){
    size_t n_start = m->indicies[n];
    size_t n_end = m->indicies[n+1];

    size_t nbr_start = m->indicies[nbr];
    size_t nbr_end = m->indicies[nbr+1];

    size_t ncount = 0;
    ncount = integerarray::intersect(result,m->data+n_start,m->data+nbr_start,n_end-n_start,nbr_end-nbr_start,m->t);
    
    num_triangles += ncount;
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

  VectorGraph *vg = ReadFile(argv[1],atoi(argv[2]));
  cout << endl;

  common::type my_type = common::ARRAY16; 
  Matrix *graph = new Matrix(vg,&my_app::myNodeSelection,&my_app::myEdgeSelection,my_type);
  my_app::result = new short[vg->num_edges];
  common::startClock();
  //For each column
  graph->foreach_column(
    //for each row
    &Matrix::for_row,
      //apply triangle counting
      &my_app::triangle_counting
  );
  common::stopClock("V16: Triangle Counting");
  cout << "Count: " << my_app::num_triangles << endl;

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

  return 0;
}
