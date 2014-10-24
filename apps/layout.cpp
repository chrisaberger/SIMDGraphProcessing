// class templates
#include "Matrix.hpp"
#include "MutableGraph.hpp"

namespace application{
  Matrix *graph;
  uint8_t *result;
  long num_triangles = 0;
  common::type graphType = common::A32BITPACKED;
  
  inline bool myNodeSelection(unsigned int node){
    (void)node;
    return true;
  }
  inline bool myEdgeSelection(unsigned int node, unsigned int nbr){
    return true;
  }
}

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) { 
  if(argc != 2){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder>" << endl;
    exit(0);
  }

  //cout << endl << "Number of threads: " << atoi(argv[2]) << endl;
  //omp_set_num_threads(atoi(argv[2]));        

  common::startClock();
  MutableGraph *inputGraph = MutableGraph::directedFromEdgeList(argv[1],1); //filename, # of files
  application::result = new uint8_t[inputGraph->num_nodes]; //we don't actually use this for just a count
  common::stopClock("Reading File");

  cout << endl << "HYBRID" << endl;
  application::graph = new Matrix(inputGraph->out_neighborhoods,inputGraph->in_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,inputGraph->external_ids,common::HYBRID);
  application::graph->Matrix::~Matrix(); 

  inputGraph->reorder_runs();

  cout << endl << "HYBRID" << endl;
  application::graph = new Matrix(inputGraph->out_neighborhoods,inputGraph->in_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,inputGraph->external_ids,common::HYBRID);
  application::graph->Matrix::~Matrix(); 
  
  cout << endl << "ARRAY 32" << endl;
  application::graph = new Matrix(inputGraph->out_neighborhoods,inputGraph->in_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,inputGraph->external_ids,common::ARRAY32);
  application::graph->Matrix::~Matrix(); 
  
  cout << endl << "ARRAY 16" << endl;
  application::graph = new Matrix(inputGraph->out_neighborhoods,inputGraph->in_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,inputGraph->external_ids,common::ARRAY16);
  application::graph->Matrix::~Matrix(); 

  cout << endl << "VARIANT" << endl;
  application::graph = new Matrix(inputGraph->out_neighborhoods,inputGraph->in_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,inputGraph->external_ids,common::VARIANT);
  application::graph->Matrix::~Matrix(); 

  cout << endl << "SIMD BPD" << endl;
  application::graph = new Matrix(inputGraph->out_neighborhoods,inputGraph->in_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,inputGraph->external_ids,common::A32BITPACKED);
  application::graph->Matrix::~Matrix(); 

  cout << endl << "DENSE RUNS" << endl;
  application::graph = new Matrix(inputGraph->out_neighborhoods,inputGraph->in_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,inputGraph->external_ids,common::DENSE_RUNS);
  application::graph->Matrix::~Matrix(); 
  
  return 0;
}
