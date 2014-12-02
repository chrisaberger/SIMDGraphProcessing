// class templates
#include "SparseMatrix.hpp"
#include "MutableGraph.hpp"
#include "pcm_helper.hpp"

using namespace pcm_helper;

template<class T,class R>
class thread_data{
  public:
    size_t thread_id;
    uint8_t *buffer;
    uint32_t *decoded_src;
    uint32_t *decoded_dst;

    SparseMatrix<T,R> *graph;

    thread_data(SparseMatrix<T,R>* graph_in, size_t buffer_lengths, const size_t thread_id_in){
      graph = graph_in;
      thread_id = thread_id_in;
      decoded_src = new uint32_t[buffer_lengths]; //space for A and B
      decoded_dst = new uint32_t[buffer_lengths]; //space for A and B
      buffer = new uint8_t[buffer_lengths*sizeof(int)];
    }
};

template<class T, class R>
class application{
  public:
    SparseMatrix<T,R>** graphs;
    long num_triangles;
    thread_data<T,R> **t_data_pointers;
    MutableGraph *inputGraph;
    size_t num_numa_nodes;
    size_t num_threads;
    string layout;

    application(size_t num_numa_nodes_in, MutableGraph *inputGraph_in, size_t num_threads_in, string input_layout){
      num_triangles = 0;
      num_numa_nodes = num_numa_nodes_in;
      inputGraph = inputGraph_in; 
      num_threads = num_threads_in;
      layout = input_layout;

      graphs = new SparseMatrix<T,R>*[num_numa_nodes];
      t_data_pointers = new thread_data<T,R>*[num_threads];
    }
    inline bool myNodeSelection(uint32_t node, uint32_t attribute){
      (void)node; (void) attribute;
      return true;
    }
    inline bool myEdgeSelection(uint32_t node, uint32_t nbr, uint32_t attribute){
      (void) attribute;
      return nbr < node;
    }
    inline void allocBuffers(){
      int threads_per_node = (num_threads - 1) / num_numa_nodes + 1;
      for(size_t k= 0; k < num_threads; k++){
        int node = k / threads_per_node;
        t_data_pointers[k] = new thread_data<T,R>(graphs[node], graphs[node]->max_nbrhood_size,k);
      }
    }
    inline void produceSubgraph(){
      auto node_selection = std::bind(&application::myNodeSelection, this, _1, _2);
      auto edge_selection = std::bind(&application::myEdgeSelection, this, _1, _2, _3);
      graphs[0] = SparseMatrix<T,R>::from_symmetric(inputGraph,node_selection,edge_selection);
      for(size_t i = 1; i < num_numa_nodes; i++) {
        graphs[i] = graphs[0]->clone_on_node(i);
      }
    }

    inline void queryOver(){
      system_counter_state_t before_sstate = pcm_get_counter_state();
      server_uncore_power_state_t* before_uncstate = pcm_get_uncore_power_state();

      size_t matrix_size = graphs[0]->matrix_size;

      //thread* threads = new thread[num_threads];
      double* thread_times = new double[num_threads];
      std::atomic<long> reducer;
      reducer = 0;
      //const size_t block_size = 1500; //matrix_size / num_threads;
      std::atomic<size_t> next_work;
      next_work = 0;

      uint32_t *src_buffer = t_data_pointers[0]->decoded_src;
      uint32_t *dst_buffer = t_data_pointers[0]->decoded_dst;
      Set<R> C(t_data_pointers[0]->buffer,0,0,(R::get_type()));

      long t_local_reducer = 0;
      double t_begin = omp_get_wtime();
      for(size_t i = 0; i < matrix_size; i++){
        Set<R> A = graphs[0]->get_decoded_row(i,src_buffer);
        A.foreach( [i,&A,&C,&dst_buffer,&graphs,&t_local_reducer] (uint32_t j){
          Set<R> B = graphs[0]->get_decoded_row(j,dst_buffer);
          t_local_reducer += ops::intersect(C,A,B).cardinality;
        });
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
  inline void run(){
    common::startClock();
    produceSubgraph();
    common::stopClock("Selections");

    //common::startClock();
    allocBuffers();
    //common::stopClock("Allocating Buffers");

    //graphs[0]->print_data("graph.txt");

    if(pcm_init() < 0)
       return;

    common::startClock();
    queryOver();
    common::stopClock("Application Time for Layout " + layout);

    cout << "Count: " << num_triangles << endl << endl;
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
  MutableGraph *inputGraph = MutableGraph::undirectedFromBinary(argv[1]); //filename, # of files
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
    num_nodes = 1;
  } 
  #endif
  else{
    cout << "No valid layout entered." << endl;
    exit(0);
  }

  return 0;
}
