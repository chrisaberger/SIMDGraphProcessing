// class templates
#include "AOA_Matrix.hpp"
#include "MutableGraph.hpp"
#include "UnsignedIntegerArray.hpp"

namespace application{
  uint8_t *result;
  long num_triangles = 0;
  
  inline bool myNodeSelection(uint32_t node){
    (void)node;
    return true;
  }
  inline bool myEdgeSelection(uint32_t node, uint32_t nbr){
    (void) node; (void) nbr;
    return true;
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
  MutableGraph *inputGraph = MutableGraph::directedFromEdgeList(argv[1]); //filename, # of files
  common::stopClock("Reading File");
  
  cout << endl;

  common::startClock();
  AOA_Matrix *graph = AOA_Matrix::from_asymmetric(inputGraph,node_selection,edge_selection,layout);
  common::stopClock("selections");
  
  //inputGraph->MutableGraph::~MutableGraph(); 

  //graph->print_data("out.txt");
  
  common::startClock();
  UnsignedIntegerArray *frontier = UnsignedIntegerArray::fromRange(0,graph->matrix_size);
  frontier->length = 1;
  UnsignedIntegerArray *next_frontier = UnsignedIntegerArray::alloc(graph->matrix_size);

  UnsignedIntegerArray *notVisited = UnsignedIntegerArray::fromRange(1,graph->matrix_size);
  
  UnsignedIntegerArray *n2x = UnsignedIntegerArray::alloc(graph->matrix_size);
  UnsignedIntegerArray *tmp = UnsignedIntegerArray::alloc(graph->matrix_size);

  while(frontier->length != 0){
    //union up neighborhoods to get distinct
    n2x = graph->get_distinct_neighbors(n2x,frontier,tmp);

    UnsignedIntegerArray::intersect(next_frontier,n2x,notVisited); //n2x intersect with notVisited

    /*
    uint32_t *nf = (uint32_t*) next_frontier->data;
    for(size_t i=0; i<next_frontier->length; i++){
      cout << "next frontier: " << i << " data: " << nf[i] << endl; 
    }
    cout << endl;
    */
    
    //cout << "trippy: " << notVisited->length << " " << next_frontier->length << endl;
    UnsignedIntegerArray::difference(tmp,notVisited,next_frontier);     

    UnsignedIntegerArray *tmp2 = notVisited;
    notVisited = tmp;
    tmp = tmp2;
    
    /*
    uint32_t *nv_data = (uint32_t*)notVisited->data;
    for(size_t i=0; i<notVisited->length; i++){
      cout << "not visited: " << i << " data: " << nv_data[i] << endl; 
    }*/
    
    //not_visited = notVisited-next_frontier

    //switch cur and result;
    tmp2 = frontier;
    frontier = next_frontier;
    next_frontier = tmp2;
  }

  common::stopClock("BFS");
  /*
  common::startClock();
  application::queryOver();
  common::stopClock(input_layout);
  //application::graph->AOA_Matrix::~AOA_Matrix(); 
  cout << "Count: " << application::num_triangles << endl << endl;
  */
  return 0;
}
