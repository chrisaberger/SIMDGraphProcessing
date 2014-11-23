// class templates
#include "AOA_Matrix.hpp"
#include "MutableGraph.hpp"
#include "pcm_helper.hpp"

using namespace pcm_helper;

class thread_data{
  public:
    size_t thread_id;
    uint8_t *buffer;
    uint32_t *decoded_src;
    AOA_Matrix<uint32> *graph;

    thread_data(AOA_Matrix<uint32>* graph_in, size_t buffer_lengths, const size_t thread_id_in){
      graph = graph_in;
      thread_id = thread_id_in;
      decoded_src = new uint32_t[buffer_lengths];
      buffer = new uint8_t[buffer_lengths*sizeof(int)]; //should not be used
    }

    inline long edgeApply(uint32_t src, uint32_t dst){
      long count = graph->row_intersect(buffer,src,dst,decoded_src);
      return count;
    }
};

class application{
  public:
    AOA_Matrix<uint32>** graphs;
    long num_triangles;
    thread_data **t_data_pointers;
    MutableGraph *inputGraph;
    size_t num_numa_nodes;

    application(size_t num_numa_nodes_in, MutableGraph *inputGraph_in, size_t num_threads){
      num_triangles = 0;
      graphs = new AOA_Matrix<uint32>*[num_numa_nodes];
      num_numa_nodes = num_numa_nodes_in;
      inputGraph = inputGraph_in; 

      t_data_pointers = new thread_data*[num_threads];
      int threads_per_node = (num_threads - 1) / num_numa_nodes + 1;
      for(size_t k= 0; k < num_threads; k++){
        int node = k / threads_per_node;
        t_data_pointers[k] = new thread_data(graphs[node], graphs[node]->max_nbrhood_size,k);
      }
    }

    inline bool myNodeSelection(uint32_t node, uint32_t attribute){
      (void)node; (void) attribute;
      return true;
    }
    inline bool myEdgeSelection(uint32_t node, uint32_t nbr, uint32_t attribute){
      (void) attribute;
      return nbr < node;
    }

    inline void produceSubgraph(){
      auto node_selection = std::bind(&application::myNodeSelection, this, _1, _2);
      auto edge_selection = std::bind(&application::myEdgeSelection, this, _1, _2, _3);
      graphs[0] = AOA_Matrix<uint32>::from_symmetric(inputGraph,node_selection,edge_selection);
      for(size_t i = 1; i < num_numa_nodes; i++) {
        graphs[i] = graphs[0]->clone_on_node(i);
      }
    }

    inline void queryOver(size_t num_nodes, size_t num_threads){
      system_counter_state_t before_sstate = pcm_get_counter_state();
      server_uncore_power_state_t* before_uncstate = pcm_get_uncore_power_state();

      size_t matrix_size = graphs[0]->matrix_size;

      thread* threads = new thread[num_threads];
      double* thread_times = new double[num_threads];
      std::atomic<long> reducer;
      reducer = 0;
      const size_t block_size = 1500; //matrix_size / num_threads;
      std::atomic<size_t> next_work;
      next_work = 0;

      long t_local_reducer = 0;
      double t_begin = omp_get_wtime();
      for(size_t i = 0; i < matrix_size; i++){
        graphs[0]->foreach_column_in_row(i, ([&t_local_reducer,&graphs,&t_data_pointers] (uint32_t src, uint32_t dst){ 
          t_local_reducer += graphs[0]->row_intersect(t_data_pointers[0]->buffer,src,dst,t_data_pointers[0]->decoded_src);
        }));
      }
      double t_end = omp_get_wtime();
      thread_times[0] = t_end - t_begin;
      reducer = t_local_reducer;

      for(size_t k = 0; k < num_threads; k++){
          std::cout << "Execution time of thread " << k << ": " << thread_times[k] << std::endl;
      }

    server_uncore_power_state_t* after_uncstate = pcm_get_uncore_power_state();
    pcm_print_uncore_power_state(before_uncstate, after_uncstate);
    system_counter_state_t after_sstate = pcm_get_counter_state();
    pcm_print_counter_stats(before_sstate, after_sstate);
    num_triangles = reducer;
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

  common::type layout;
  if(input_layout.compare("a32") == 0){
    layout = common::ARRAY32;
  } else if(input_layout.compare("bs") == 0){
    layout = common::BITSET;
  } else if(input_layout.compare("a16") == 0){
    layout = common::ARRAY16;
  } else if(input_layout.compare("hybrid") == 0){
    layout = common::HYBRID_PERF;
  } 
  #if COMPRESSION == 1
  else if(input_layout.compare("v") == 0){
    layout = common::VARIANT;
  } else if(input_layout.compare("bp") == 0){
    layout = common::A32BITPACKED;
  } 
  #endif
  else{
    cout << "No valid layout entered." << endl;
    exit(0);
  }

  common::startClock();
  MutableGraph *inputGraph = MutableGraph::undirectedFromEdgeList(argv[1]); //filename, # of files
  common::stopClock("Reading File");

  size_t num_nodes = 1;
  cout << inputGraph->num_nodes << endl;
  application myapp(num_nodes,inputGraph,num_threads);
  myapp.produceSubgraph();

  cout << "Application object allocated." << endl;

  if(pcm_init() < 0)
     return -1;

  myapp.graphs[0]->print_data("out.txt");

  common::startClock();
  myapp.queryOver(num_nodes, num_threads);
  common::stopClock(input_layout);

  cout << "Count: " << myapp.num_triangles << endl << endl;
  pcm_cleanup();

  return 0;
}
