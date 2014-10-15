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
  inline long edgeApply(unsigned int src, unsigned int dst, unsigned int *decodedA){
    //cout << "src: " << src << " dst: " << dst << endl;
    long count = graph->row_intersect(result,src,dst,decodedA);
    //cout << count << endl;
    return count;
  }
  inline void queryOver(){
    num_triangles = graph->sum_over_rows(&Matrix::sum_over_columns_in_row,&edgeApply);
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
  MutableGraph *inputGraph = MutableGraph::undirectedFromEdgeList(argv[1],1); //filename, # of files
  application::result = new uint8_t[inputGraph->num_nodes]; //we don't actually use this for just a count
  common::stopClock("Reading File");

  cout << endl;

  application::graph = new Matrix(inputGraph->out_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,layout);
  inputGraph->MutableGraph::~MutableGraph(); 
  
  common::startClock();
  application::queryOver();
  common::stopClock(input_layout);
  application::graph->Matrix::~Matrix(); 
  cout << "Count: " << application::num_triangles << endl << endl;
  
  return 0;
}
