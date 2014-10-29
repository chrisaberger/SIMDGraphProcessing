// class templates
#include "AOA_Matrix.hpp"

#include "MutableGraph.hpp"

namespace application{
  AOA_Matrix *graph;
  uint8_t *result;
  long num_triangles = 0;
  
  inline bool myNodeSelection(unsigned int node){
    (void)node;
    return true;
  }
  inline bool myEdgeSelection(unsigned int node, unsigned int nbr){
    return nbr < node;
  }
  inline long edgeApply(unsigned int src, unsigned int dst, unsigned int *src_nbrhood){
    //cout << "src: " << src << " dst: " << dst << endl;
    long count = graph->row_intersect(result,src,dst,src_nbrhood);
    //cout << count << endl;
    return count;
  }
  inline void queryOver(){
    auto edge_function = std::bind(&edgeApply, _1, _2, _3);
    auto row_function = std::bind(&AOA_Matrix::sum_over_columns_in_row<long>, graph, _1, _2);

    num_triangles = graph->sum_over_rows<long>(row_function,edge_function);
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
  } else if(input_layout.compare("bs") == 0){
    layout = common::BITSET;
  } else if(input_layout.compare("a16") == 0){
    layout = common::ARRAY16;
  } else if(input_layout.compare("hybrid") == 0){
    layout = common::HYBRID_PERF;
  } else if(input_layout.compare("v") == 0){
    layout = common::VARIANT;
  } else if(input_layout.compare("bp") == 0){
    layout = common::A32BITPACKED;
  } else{
    cout << "No valid layout entered." << endl;
    exit(0);
  }

  auto node_selection = std::bind(&application::myNodeSelection, _1);
  auto edge_selection = std::bind(&application::myEdgeSelection, _1, _2);

  common::startClock();
  MutableGraph *inputGraph = MutableGraph::undirectedFromBinary(argv[1]); //filename, # of files
  application::result = new uint8_t[inputGraph->num_nodes]; //we don't actually use this for just a count
  common::stopClock("Reading File");
  
  common::startClock();
  inputGraph->reorder_by_degree();
  common::stopClock("Reordering");
  
  cout << endl;

  common::startClock();
  application::graph = AOA_Matrix::from_symmetric(inputGraph->out_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    node_selection,edge_selection,inputGraph->external_ids,layout);
  common::stopClock("selections");
  inputGraph->MutableGraph::~MutableGraph(); 

  //application::graph->print_data("out.txt");
  
  common::startClock();
  application::queryOver();
  common::stopClock(input_layout);
  application::graph->AOA_Matrix::~AOA_Matrix(); 
  cout << "Count: " << application::num_triangles << endl << endl;
  
  return 0;
}
