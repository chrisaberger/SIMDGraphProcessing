// class templates
#include "SparseMatrix.hpp"
#include "MutableGraph.hpp"
#include "pcm_helper.hpp"

using namespace pcm_helper;

template<class T, class R>
class application{
  public:
    SparseMatrix<T,R>** graphs;
    MutableGraph *inputGraph;
    size_t num_numa_nodes;
    size_t num_threads;
    string layout;
    size_t id;

    application(size_t num_numa_nodes_in, MutableGraph *inputGraph_in, size_t num_threads_in, string input_layout, size_t id_in){
      num_numa_nodes = num_numa_nodes_in;
      inputGraph = inputGraph_in; 
      num_threads = num_threads_in;
      layout = input_layout;
      id = id_in;

      graphs = new SparseMatrix<T,R>*[num_numa_nodes];
    }
    inline bool myNodeSelection(uint32_t node, uint32_t attribute){
      (void)node; (void) attribute;
      return true;
    }
    inline bool myEdgeSelection(uint32_t node, uint32_t nbr, uint32_t attribute){
      (void) attribute; (void) nbr; (void) node;
      return true;
    }

    inline void produceSubgraph(){
      auto node_selection = std::bind(&application::myNodeSelection, this, _1, _2);
      auto edge_selection = std::bind(&application::myEdgeSelection, this, _1, _2, _3);
      graphs[0] = SparseMatrix<T,R>::from_asymmetric_graph(inputGraph,node_selection,edge_selection);
      for(size_t i = 1; i < num_numa_nodes; i++) {
        graphs[i] = graphs[0]->clone_on_node(i);
      }
    }

  inline void queryOver(uint32_t start_node){
    uint8_t *f_data = new uint8_t[graphs[0]->max_nbrhood_size*sizeof(uint32_t)];
    uint32_t *start_array = new uint32_t[1];
    cout << "start node: " << start_node << " max size: " << graphs[0]->max_nbrhood_size << endl;
    start_array[0] = start_node;
    Set<hybrid> frontier = Set<uinteger>::from_array(f_data,start_array,1);

    Set<uinteger> next_frontier((graphs[0]->matrix_size/sizeof(uint32_t))+1);
    Set<bitset> visited((graphs[0]->matrix_size/sizeof(uint32_t))+1);
    Set<bitset> old_visited((graphs[0]->matrix_size/sizeof(uint32_t))+1);

    bool finished = false;
    size_t path_length = 0;
    while(!finished){
      cout << "Path: " << path_length << endl;
      old_visited.copy_from(visited);
      frontier.foreach( [&visited,&graphs] (uint32_t f){
        //cout << " Frontier: " << graphs[0]->id_map[f] << " " << f << endl;
        Set<R> outnbrs = graphs[0]->get_row(f);
        ops::set_union(visited,outnbrs);
      });

      frontier = ops::set_difference(next_frontier,visited,old_visited);  

      //CODE IF WE WANT TO REPACKAGE
      //next_frontier = ops::set_difference(next_frontier,visited,old_visited);    
      //frontier = ops::repackage(next_frontier,frontier.data);
      cout << "F-TYPE: " << frontier.type <<  " CARDINALITY: " << frontier.cardinality << endl;

      path_length++;
      finished = path_length == 4;
    }
  }
  
  inline void run(){
    common::startClock();
    produceSubgraph();
    common::stopClock("Selections");
    
    //graphs[0]->print_data("graph.txt");
    cout << "searching for: " << id << endl;
    uint32_t start_node = graphs[0]->get_internal_id(id);

    if(pcm_init() < 0)
      return;

    common::startClock();
    queryOver(start_node);
    common::stopClock("Application Time for Layout " + layout);
    
    pcm_cleanup();
  }
};

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) { 
  if(argc != 5){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of threads> <layout type=bs,a16,a32,hybrid,v,bp> <start node id>" << endl;
    exit(0);
  }

  size_t num_threads = atoi(argv[2]);
  cout << endl << "Number of threads: " << num_threads << endl;
  omp_set_num_threads(num_threads);        

  std::string input_layout = argv[3];

  size_t num_nodes = 1;
  //common::startClock();
  MutableGraph *inputGraph = MutableGraph::directedFromEdgeList(argv[1]); //filename, # of files
  //common::stopClock("Reading File");

  if(input_layout.compare("a32") == 0){
    application<uinteger,uinteger> myapp(num_nodes,inputGraph,num_threads,input_layout,atoi(argv[4]));
    myapp.run();
  } else if(input_layout.compare("bs") == 0){
    //application<bitset,bitset> myapp(num_nodes,inputGraph,num_threads,input_layout);
    //myapp.run();  
  } else if(input_layout.compare("a16") == 0){
    //application<pshort,pshort> myapp(num_nodes,inputGraph,num_threads,input_layout);
    //myapp.run();  
  } else if(input_layout.compare("hybrid") == 0){
    //application<hybrid,hybrid> myapp(num_nodes,inputGraph,num_threads,input_layout);
    //myapp.run();  
  } 
  #if COMPRESSION == 1
  else if(input_layout.compare("v") == 0){
    //application<variant,uinteger> myapp(num_nodes,inputGraph,num_threads,input_layout);
    //myapp.run();  
  } else if(input_layout.compare("bp") == 0){
    //application<bitpacked,uinteger> myapp(num_nodes,inputGraph,num_threads,input_layout);
    //myapp.run();
  } 
  #endif
  else{
    cout << "No valid layout entered." << endl;
    exit(0);
  }

  return 0;
}
