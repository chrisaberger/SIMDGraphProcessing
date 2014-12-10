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

    application(size_t num_numa_nodes_in, MutableGraph *inputGraph_in, size_t num_threads_in, string input_layout) {
      num_numa_nodes = num_numa_nodes_in;
      inputGraph = inputGraph_in; 
      num_threads = num_threads_in;
      layout = input_layout;

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
    uint8_t *f_data = new uint8_t[graphs[0]->matrix_size*sizeof(uint32_t)];
    uint32_t *start_array = new uint32_t[1];
    cout << "alloc size: " << graphs[0]->matrix_size*1000 << endl;
    cout << "start node: " << start_node << "  num nodes: " << graphs[0]->matrix_size << endl;
    start_array[0] = start_node;

    //Set<uinteger> frontier = Set<uinteger>::from_array(f_data,start_array,1);
    Set<hybrid> frontier = Set<uinteger>::from_array(f_data,start_array,1);

    //Set<uinteger> next_frontier(graphs[0]->matrix_size*sizeof(uint32_t));
    Set<bitset> next_frontier((graphs[0]->matrix_size/sizeof(uint32_t))+1);

    Set<bitset> visited((graphs[0]->matrix_size/sizeof(uint32_t))+1);
    bitset::set(start_node,visited.data);

    Set<bitset> old_visited((graphs[0]->matrix_size/sizeof(uint32_t))+1);

    double compute_timer  = common::startClock();

    //Set<T> outnbrs = graphs[0]->get_row(132365);
    bool finished = false;
    size_t path_length = 0;
    while(!finished){
      cout << endl << " Path: " << path_length << " F-TYPE: " << frontier.type <<  " CARDINALITY: " << frontier.cardinality << endl;
      double start_time = common::startClock();
      
      //double copy_time = common::startClock();
      old_visited.copy_from(visited);
      //common::stopClock("copy time",copy_time);

      //double union_time = common::startClock();
      //cout << "Path: " << path_length << endl;
      if(frontier.type == common::BITSET){
        common::par_for_range(num_threads, 0, graphs[0]->matrix_size,
          [&graphs, &visited, &frontier](size_t tid, size_t i) {
             (void) tid;

             if(!bitset::is_set(i,visited.data)) {
               Set<T> innbrs = graphs[0]->get_column(i);
               innbrs.foreach([frontier,i,&visited] (uint32_t nbr){
                 if(bitset::is_set(nbr,frontier.data)){
                   bitset::set(i,visited.data);
                 }
               });
             }
          }
        );
        //memset(next_frontier.data,(uint8_t)0,frontier.number_of_bytes);
      } else{
        frontier.par_foreach(num_threads,
          [&visited,&graphs] (size_t tid, uint32_t f){
             (void) tid;

             //cout << " Frontier: " << graphs[0]->id_map[f] << " " << f << endl;
             Set<T> outnbrs = graphs[0]->get_row(f);
             ops::set_union(visited,outnbrs);
        });
      }
      //common::stopClock("union time",union_time);


      //IF YOU WANT FUSED REPACKAGING 
/*
      double diff_time = common::startClock();
      frontier = ops::set_difference(next_frontier,visited,old_visited);  
      common::stopClock("difference",diff_time);
*/
      //CODE IF WE WANT TO REPACKAGE
      //double diff_time = common::startClock();
      next_frontier = ops::set_difference(next_frontier,visited,old_visited);  
      //common::stopClock("difference",diff_time);

      //double repack_time = common::startClock();
      frontier = ops::repackage(next_frontier,f_data);
      //common::stopClock("repack",repack_time);

      path_length++;
      finished = frontier.cardinality == 0;
      common::stopClock("Iteration",start_time);
    }
    common::stopClock("compute time",compute_timer);
    //cout << "path length: " << (path_length-1) << endl;
  }
  
  inline void run(){
    double selection_time = common::startClock();
    produceSubgraph();
    common::stopClock("Selections",selection_time);
    
    //graphs[0]->print_data("graph.txt");
    uint32_t start_node = graphs[0]->get_max_row_id();
    //uint32_t start_node = graphs[0]->get_internal_id(id);

    if(pcm_init() < 0)
      return;

    double bfs_time = common::startClock();
    queryOver(start_node);
    common::stopClock("BFS",bfs_time);

    pcm_cleanup();
  }
};

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) { 
  if(argc != 4){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of threads> <layout type=bs,a16,a32,hybrid,v,bp>" << endl;
    exit(0);
  }

  size_t num_threads = atoi(argv[2]);
  cout << endl << "Number of threads: " << num_threads << endl;
  omp_set_num_threads(num_threads);        

  std::string input_layout = argv[3];

  size_t num_nodes = 1;
  //common::startClock();
  MutableGraph *inputGraph = MutableGraph::directedFromBinary(argv[1]); //filename, # of files
  //common::stopClock("Reading File");

  if(input_layout.compare("a32") == 0){
    application<uinteger,uinteger> myapp(num_nodes,inputGraph,num_threads,input_layout);
    myapp.run();
  } else if(input_layout.compare("bs") == 0){
    application<bitset,bitset> myapp(num_nodes,inputGraph,num_threads,input_layout);
    myapp.run();  
  } else if(input_layout.compare("a16") == 0){
    application<pshort,pshort> myapp(num_nodes,inputGraph,num_threads,input_layout);
    myapp.run();  
  } else if(input_layout.compare("hybrid") == 0){
    application<hybrid,hybrid> myapp(num_nodes,inputGraph,num_threads,input_layout);
    myapp.run();  
  } 
  #if COMPRESSION == 1
  else if(input_layout.compare("v") == 0){
    application<variant,uinteger> myapp(num_nodes,inputGraph,num_threads,input_layout);
    myapp.run();  
  } else if(input_layout.compare("bp") == 0){
    application<bitpacked,uinteger> myapp(num_nodes,inputGraph,num_threads,input_layout);
    myapp.run();
  } 
  #endif
  else{
    cout << "No valid layout entered." << endl;
    exit(0);
  }

  return 0;
}
