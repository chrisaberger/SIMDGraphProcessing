// class templates
#include "Matrix.hpp"
#include "MutableGraph.hpp"

namespace application{
  Matrix *graph;
  
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
  if(argc != 3){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of threads>" << endl;
    exit(0);
  }

  cout << endl << "Number of threads: " << atoi(argv[2]) << endl;
  omp_set_num_threads(atoi(argv[2]));        

  common::startClock();
  MutableGraph *inputGraph = MutableGraph::undirectedFromBinary(argv[1]); //filename, # of files
  common::stopClock("Reading File");

  vector<string> layout_names;
  layout_names.push_back("BITSET");
  layout_names.push_back("ARRAY16");
  layout_names.push_back("ARRAY32");
  layout_names.push_back("A32BITPACKED");
  layout_names.push_back("VARIANT");
  layout_names.push_back("HYBRID_PERF");
  //layout_names.push_back("HYBRID_COMP");
  
  inputGraph->reorder_random();
  cout << endl << "RANDOM" << endl;
  for(uint8_t i =5; i < (uint8_t)layout_names.size(); i++){
    common::type layout = (common::type) i;
    common::startClock();
    application::graph = new Matrix(inputGraph->out_neighborhoods,
      inputGraph->num_nodes,inputGraph->num_edges,
      &application::myNodeSelection,&application::myEdgeSelection,inputGraph->external_ids,layout);
    common::stopClock(layout_names.at(i));
    application::graph->Matrix::~Matrix(); 
  }

  inputGraph->reorder_strong_run();
  cout << endl << "STRONG RUNS" << endl;
  for(uint8_t i =5; i < (uint8_t)layout_names.size(); i++){
    common::type layout = (common::type) i;
    common::startClock();
    application::graph = new Matrix(inputGraph->out_neighborhoods,
      inputGraph->num_nodes,inputGraph->num_edges,
      &application::myNodeSelection,&application::myEdgeSelection,inputGraph->external_ids,layout);
    common::stopClock(layout_names.at(i));
    application::graph->Matrix::~Matrix(); 
  }

  inputGraph->reorder_by_rev_degree();
  cout << endl << "REV DEGREE" << endl;
  for(uint8_t i =5; i < (uint8_t)layout_names.size(); i++){
    common::type layout = (common::type) i;
    common::startClock();
    application::graph = new Matrix(inputGraph->out_neighborhoods,
      inputGraph->num_nodes,inputGraph->num_edges,
      &application::myNodeSelection,&application::myEdgeSelection,inputGraph->external_ids,layout);
    common::stopClock(layout_names.at(i));
    application::graph->Matrix::~Matrix(); 
  }

  inputGraph->reorder_by_degree();
  cout << endl << "DEGREE" << endl;
  for(uint8_t i =5; i < (uint8_t)layout_names.size(); i++){
    common::type layout = (common::type) i;
    common::startClock();
    application::graph = new Matrix(inputGraph->out_neighborhoods,
      inputGraph->num_nodes,inputGraph->num_edges,
      &application::myNodeSelection,&application::myEdgeSelection,inputGraph->external_ids,layout);
    common::stopClock(layout_names.at(i));
    application::graph->Matrix::~Matrix(); 
  }
  
  inputGraph->reorder_bfs();
  cout << endl << "BFS" << endl;
  for(uint8_t i =5; i < (uint8_t)layout_names.size(); i++){
    common::type layout = (common::type) i;
    common::startClock();
    application::graph = new Matrix(inputGraph->out_neighborhoods,
      inputGraph->num_nodes,inputGraph->num_edges,
      &application::myNodeSelection,&application::myEdgeSelection,inputGraph->external_ids,layout);
    common::stopClock(layout_names.at(i));
    application::graph->Matrix::~Matrix(); 
  }

  inputGraph->MutableGraph::~MutableGraph(); 
  return 0;
}

