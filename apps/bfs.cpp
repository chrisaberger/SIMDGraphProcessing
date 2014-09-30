// class templates
#include "Matrix.hpp"
#include "MutableGraph.hpp"

namespace application{
  Matrix *graph;
  uint8_t *result;
  long num_triangles = 0;
  common::type graphType = common::ARRAY32;
  
  inline bool myNodeSelection(unsigned int node){
    (void)node;
    return true;
  }
  inline bool myEdgeSelection(unsigned int node, unsigned int nbr){
    return nbr < node;
  }
  inline long edgeApply(unsigned int src, unsigned int dst){
    return graph->row_intersect(result,src,dst);
  }
  inline void queryOver(){
    num_triangles = graph->reduce_row(&Matrix::reduce_column_in_row,&edgeApply);
  }
}

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) { 
  if(argc != 3){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of threads>" << endl;
    exit(0);
  }

  cout << endl << "Number of threads: " << atoi(argv[2]) << endl;
  omp_set_num_threads(atoi(argv[2]));        

  common::startClock();
  MutableGraph inputGraph = MutableGraph::undirectedFromAdjList(argv[1],1); //filename, # of files
  application::result = new uint8_t[inputGraph.num_nodes]; //we don't actually use this for just a count
    //for more sophisticated queries this would be used.
  common::stopClock("Reading File");

  common::startClock();
  Matrix allocated_graph = Matrix::buildSymetric(inputGraph.out_neighborhoods,
    inputGraph.num_nodes,inputGraph.num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,application::graphType);
  application::graph = &allocated_graph;
  common::stopClock("Building Graph");

  common::startClock();
  application::queryOver();
  common::stopClock("CSR TRIANGLE COUNTING");

  cout << "Count: " << application::num_triangles << endl << endl;

  return 0;
}
