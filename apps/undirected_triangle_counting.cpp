// class templates
#include "AOA_Matrix.hpp"
#include "MutableGraph.hpp"
#include "pcm_helper.hpp"

using namespace pcm_helper;

namespace application{
  AOA_Matrix** graphs;
  long num_triangles = 0;

  inline bool myNodeSelection(uint32_t node){
    (void)node;
    return true;
  }
  inline bool myEdgeSelection(uint32_t node, uint32_t nbr){
    return nbr < node;
  }

  struct global_thread_data {
    double t_all_begin;
    std::atomic<size_t> next_work;
    std::atomic<long> reducer;
    bool work;

    global_thread_data() {
       this->next_work = 0;
       this->reducer = 0;
       this->work = false;
    }
  };

  struct thread_data{
    size_t thread_id;
    uint8_t *buffer;
    uint32_t *decoded_src;
    AOA_Matrix *graph;
    global_thread_data* g_data;
    double thread_time;
    std::condition_variable cv_work;
    std::mutex cv_work_m;

    thread_data(
          global_thread_data* g_data_in,
          AOA_Matrix* graph_in,
          size_t buffer_lengths,
          const size_t thread_id_in ){
      g_data = g_data_in;
      graph = graph_in;
      thread_id = thread_id_in;
      decoded_src = new uint32_t[buffer_lengths];
      buffer = new uint8_t[buffer_lengths*sizeof(int)]; //should not be used
    }

    inline long edgeApply(uint32_t src, uint32_t dst){
      long count = graph->row_intersect(buffer,src,dst,decoded_src);
      return count;
    }

    void run() {
      std::unique_lock<std::mutex> lk(this->cv_work_m);
      this->cv_work.wait(lk, [&]{ return g_data->work; });

      const size_t block_size = 1500;
      auto row_function = std::bind(&AOA_Matrix::sum_over_columns_in_row<long>, graph, _1, _2, _3);
      auto edge_function = std::bind(&thread_data::edgeApply,this,_1,_2);
      size_t local_block_size = block_size;
      size_t matrix_size = graph->matrix_size;
      long t_local_reducer = 0;
      double t_begin = omp_get_wtime();

      while(true) {
        size_t work_start = g_data->next_work.fetch_add(local_block_size, std::memory_order_relaxed);
        if(work_start > matrix_size)
          break;

        size_t work_end = min(work_start + local_block_size, matrix_size);
        local_block_size = 100 + (work_start / matrix_size) * block_size;
        for(size_t j = work_start; j < work_end; j++) {
           t_local_reducer += (row_function)(j, this->decoded_src,edge_function);
        }
      }
      g_data->reducer += t_local_reducer;
      double t_end = omp_get_wtime();
      this->thread_time = t_end - g_data->t_all_begin;
    }
  };
  global_thread_data* g_data;
  thread_data **t_data_pointers;

  inline void allocBuffers(size_t num_nodes, size_t num_threads){
    g_data = new global_thread_data();
    t_data_pointers = new thread_data*[num_threads];
    int threads_per_node = (num_threads - 1) / num_nodes + 1;
    for(size_t k= 0; k < num_threads; k++){
      int node = k / threads_per_node;
      t_data_pointers[k] = new thread_data(g_data, graphs[node], graphs[node]->max_nbrhood_size,k);
    }
  }

  thread* create_thread_pool(size_t num_threads) {
     thread* threads = new thread[num_threads];
     for(size_t k = 0; k < num_threads; k++) {
        auto thread_fn = std::bind(&thread_data::run, t_data_pointers[k]);
        threads[k] = thread(thread_fn);
     }
     return threads;
  }

  inline void queryOver(thread* threads, size_t num_threads){
      system_counter_state_t before_sstate = pcm_get_counter_state();
      server_uncore_power_state_t* before_uncstate = pcm_get_uncore_power_state();

      g_data->t_all_begin = omp_get_wtime();
      g_data->work = true;

      for(size_t k = 0; k < num_threads; k++){
         t_data_pointers[k]->cv_work.notify_one();
      }

//        for(size_t k = 0; k < num_threads; k++){

 //       }

        //cleanup
     /*else{
        auto row_function = std::bind(&AOA_Matrix::sum_over_columns_in_row<long>, graphs[0], _1, _2, _3);
        auto edge_function = std::bind(&thread_data::edgeApply,t_data_pointers[0],_1,_2);
        long t_local_reducer = 0;
        double t_begin = omp_get_wtime();

        for(size_t i = 0; i < matrix_size; i++){
          t_local_reducer += (row_function)(i,t_data_pointers[0]->decoded_src,edge_function);
        }
        double t_end = omp_get_wtime();
        thread_times[0] = t_end - t_begin;
        reducer = t_local_reducer;
      }*/
      for(size_t k = 0; k < num_threads; k++){
          threads[k].join();
          std::cout << "Execution time of thread " << k << ": " << t_data_pointers[k]->thread_time << std::endl;
      }

    server_uncore_power_state_t* after_uncstate = pcm_get_uncore_power_state();
    pcm_print_uncore_power_state(before_uncstate, after_uncstate);
    system_counter_state_t after_sstate = pcm_get_counter_state();
    pcm_print_counter_stats(before_sstate, after_sstate);
    num_triangles = g_data->reducer;
  }
}

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

  auto node_selection = std::bind(&application::myNodeSelection, _1);
  auto edge_selection = std::bind(&application::myEdgeSelection, _1, _2);

  common::startClock();
  MutableGraph *inputGraph = MutableGraph::undirectedFromBinary(argv[1]); //filename, # of files
  common::stopClock("Reading File");

  size_t num_nodes = 1;
  application::graphs = new AOA_Matrix*[num_nodes];

  common::startClock();
  application::graphs[0] = AOA_Matrix::from_symmetric(inputGraph->out_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,inputGraph->max_nbrhood_size,
    node_selection,edge_selection,NULL,layout);
  common::stopClock("selections");

  common::startClock();
  for(size_t i = 1; i < num_nodes; i++) {
     application::graphs[i] = application::graphs[0]->clone_on_node(i);
  }
  common::stopClock("replication");


  if(pcm_init() < 0)
     return -1;

  //inputGraph->MutableGraph::~MutableGraph(); 

  //application::graph->print_data("out.txt");
  common::startClock();
  application::allocBuffers(num_nodes, num_threads);
  common::stopClock("buffer allocation");

  common::startClock();
  thread* threads = application::create_thread_pool(num_threads);
  common::stopClock("thread pool creation");

  common::startClock();
  application::queryOver(threads, num_threads);
  common::stopClock(input_layout);

  cout << "Count: " << application::num_triangles << endl << endl;
  pcm_cleanup();

  return 0;
}
