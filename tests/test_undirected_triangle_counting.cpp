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
    return nbr < node;
  }
}

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) { 
  if(argc != 4){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of threads> <layout type=bs,a16,a32,hybrid,v,bp>" << endl;
    exit(0);
  }

  cout << endl << "Number of threads: " << atoi(argv[2]) << endl;
  omp_set_num_threads(atoi(argv[2]));        

  std::string input_layout = argv[3];

  common::type layout;
  if(input_layout.compare("a32") == 0){
    layout = common::ARRAY32;
  } else if(input_layout.compare("a16") == 0){
    layout = common::ARRAY16;
  } else if(input_layout.compare("hybrid") == 0){
    layout = common::HYBRID;
  } else if(input_layout.compare("v") == 0){
    layout = common::VARIANT;
  } else if(input_layout.compare("bp") == 0){
    layout = common::A32BITPACKED;
  } else{
    cout << "No valid layout entered." << endl;
    exit(0);
  }

  common::startClock();
  MutableGraph *inputGraph = MutableGraph::undirectedFromBinary(argv[1]); //filename, # of files
  application::result = new uint8_t[inputGraph->num_nodes]; //we don't actually use this for just a count
  common::stopClock("Reading File");

  cout << endl;

  application::graph = new Matrix(inputGraph->out_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,inputGraph->external_ids,layout);
  
  unsigned int n1 = 1098;
  unsigned int n2 = 1075;
  application::graph->print_rows(n1,n2,"a32_w.txt");
  unsigned int *dumb;
  application::num_triangles = application::graph->row_intersect(application::result,n1,n2,dumb);
  application::graph->Matrix::~Matrix(); 
  cout << "Count: " << application::num_triangles << endl << endl;
  
  application::graph = new Matrix(inputGraph->out_neighborhoods,
      inputGraph->num_nodes,inputGraph->num_edges,
      &application::myNodeSelection,&application::myEdgeSelection,inputGraph->external_ids,common::HYBRID);
  application::graph->print_rows(n1,n2,"hybrid.txt");
  application::num_triangles = application::graph->row_intersect(application::result,n1,n2,dumb);
  application::graph->Matrix::~Matrix(); 
  cout << "Count: " << application::num_triangles << endl << endl;
  
  
  /*
  application::graph = new Matrix(inputGraph.out_neighborhoods,
      inputGraph.num_nodes,inputGraph.num_edges,
      &application::myNodeSelection,&application::myEdgeSelection,common::A32BITPACKED);
  common::startClock();
  application::graph->print_data("a32bitpacked.txt");
  application::queryOver();
  common::stopClock("BP COUNTING");
  cout << "Count: " << application::num_triangles << endl << endl;
  */

  return 0;
}
