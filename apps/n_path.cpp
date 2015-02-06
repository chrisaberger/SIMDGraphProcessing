#define WRITE_VECTOR 1

#include "SparseMatrix.hpp"
#include "MutableGraph.hpp"
#include "pcm_helper.hpp"
#include "Parser.hpp"

using namespace pcm_helper;

template<class T, class R>
class application{
  public:
    SparseMatrix<T,R>* graph;
    MutableGraph *inputGraph;
    size_t num_threads;
    string layout;
    size_t depth;
    long start_node;

    application(Parser input_data) {
      inputGraph = input_data.input_graph; 
      num_threads = input_data.num_threads;
      layout = input_data.layout;
      depth = input_data.n;
      start_node = input_data.start_node;
    }

#ifdef ATTRIBUTES
    inline bool myNodeSelection(uint32_t node, uint32_t attribute){
      (void)node; (void) attribute;
      return attribute > 500;
    }
    inline bool myEdgeSelection(uint32_t src, uint32_t dst, uint32_t attribute){
      (void) attribute; (void) src; (void) dst;
      return attribute == 2012;
    }
#else
    inline bool myNodeSelection(uint32_t node, uint32_t attribute){
      (void)node; (void) attribute;
      return true;
    }
    inline bool myEdgeSelection(uint32_t node, uint32_t nbr, uint32_t attribute){
      (void) node; (void) nbr; (void) attribute;
      return true;
    }
#endif

    inline void produceSubgraph(){
      auto node_selection = std::bind(&application::myNodeSelection, this, _1, _2);
      auto edge_selection = std::bind(&application::myEdgeSelection, this, _1, _2, _3);

      graph = SparseMatrix<T,R>::from_asymmetric_graph(inputGraph,node_selection,edge_selection,num_threads);
    }

  inline void queryOver(uint32_t start_node){
    uint8_t *f_data = new uint8_t[graph->matrix_size*sizeof(uint64_t)];
    uint32_t *start_array = new uint32_t[1];
    start_array[0] = start_node;
    cout << "Start node: " << start_node << endl;

    const size_t bs_size = (((graph->matrix_size + 64) / 64) * 8) + 8;

    //allocate a new visited array and set the start node
    Set<bitset> visited(bs_size);
    Set<bitset> old_visited(bs_size);

    bitset::set(start_node,(uint64_t*)(visited.data+sizeof(uint64_t)),0);
    Set<uinteger> frontier = Set<uinteger>::from_array(f_data,start_array,1);
    bool dense_frontier = false;

    Set<bitset> **vis_bufs = new Set<bitset>*[num_threads];
    for(size_t i = 0; i < num_threads; i ++){
      vis_bufs[i] = new Set<bitset>(bs_size);
    }


    size_t path_length = 0;
    while(true){
      cout << endl << " Path: " << path_length << " F-TYPE: " << frontier.type <<  " CARDINALITY: " << frontier.cardinality << " DENSITY: " << dense_frontier << endl;

      double copy_time = common::startClock();
      old_visited.copy_from(visited);
      common::stopClock("copy time",copy_time);

      double union_time = common::startClock();
      if(dense_frontier){
        common::par_for_range(num_threads, 0, graph->matrix_size, 2048,
          [this, &visited, &old_visited, &frontier](size_t tid, size_t i) {
             (void) tid;
            if(!bitset::is_set(i,(uint64_t*)(visited.data+sizeof(uint64_t)),0)) {
               Set<T> innbrs = this->graph->get_column(i);
               innbrs.foreach_until([i,&visited, &old_visited] (uint32_t nbr) {
                if(bitset::is_set(nbr,(uint64_t*)(old_visited.data+sizeof(uint64_t)),0)){
                  bitset::set(i,(uint64_t*)(visited.data+sizeof(uint64_t)),0);
                  visited.cardinality++;
                  return true;
                }
                return false;
               });
             }
          }
        );
      } else{
        frontier.par_foreach(num_threads,
          [this, &vis_bufs] (size_t tid,uint32_t n){
            Set<T> outnbrs = this->graph->get_row(n);
            ops::set_union(vis_bufs[tid],&outnbrs);
        });
      }
      for(size_t i = 0; i < num_threads; i++){
        ops::set_union(&visited,vis_bufs[i]);
      }

      common::stopClock("union time",union_time);

      double diff_time = common::startClock();
      frontier = *ops::set_difference(&frontier,&visited,&old_visited);
      common::stopClock("difference",diff_time);

      //TO TURN BEAMER OFF COMMENT OUT THIS LINE
      dense_frontier = ((double)frontier.cardinality / graph->matrix_size) > 0.08;

      if(frontier.cardinality == 0 || path_length >= depth)
        break;
      path_length++;

    }

    cout << "path length: " << path_length << endl;
    cout << "frontier size: " << frontier.cardinality << endl;
  }

  inline void run(){
    double selection_time = common::startClock();
    produceSubgraph();
    common::stopClock("Selections",selection_time);

    //graph->print_data("graph.txt");
    uint32_t internal_start;
    if(start_node == -1)
      internal_start = graph->get_max_row_id();
    else
      internal_start = graph->get_internal_id(start_node);
    cout << "Start node - External: " << graph->id_map[internal_start] << " Internal: " << internal_start << endl;

    if(pcm_init() < 0)
      return;

    double bfs_time = common::startClock();
    queryOver(internal_start);
    common::stopClock("BFS",bfs_time);

    pcm_cleanup();
  }
};

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) { 
  Parser input_data = input_parser::parse(argc,argv,"n_path");

  if(input_data.layout.compare("uint") == 0){
    application<uinteger,uinteger> myapp(input_data);
    myapp.run();
  } else if(input_data.layout.compare("bs") == 0){
    application<bitset,bitset> myapp(input_data);
    myapp.run();  
  } else if(input_data.layout.compare("pshort") == 0){
    application<pshort,pshort> myapp(input_data);
    myapp.run();  
  } else if(input_data.layout.compare("hybrid") == 0){
    application<hybrid,hybrid> myapp(input_data);
    myapp.run();  
  } else if(input_data.layout.compare("v") == 0){
    application<variant,uinteger> myapp(input_data);
    myapp.run();  
  } else if(input_data.layout.compare("bp") == 0){
    application<bitpacked,uinteger> myapp(input_data);
    myapp.run();
  } else{
    cout << "No valid layout entered." << endl;
    exit(0);
  }
  return 0;
}
