// class templates
#include "AOA_Matrix.hpp"
#include "MutableGraph.hpp"

namespace application{
  AOA_Matrix *graph;
  long num_triangles = 0;
  unsigned int *thread_local_buffers;

  inline bool myNodeSelection(unsigned int node){
    (void)node;
    return true;
  }
  inline bool myEdgeSelection(unsigned int node, unsigned int nbr){
    return nbr < node;
  }
  struct thread_data{
    size_t thread_id;
    uint8_t *buffer;
    unsigned int *decoded_src;

    thread_data(size_t buffer_lengths, const size_t thread_id_in){
      thread_id = thread_id_in;
      decoded_src = new unsigned int[buffer_lengths];
      buffer = (uint8_t*) decoded_src; //should not be used
    }

    inline long edgeApply(unsigned int src, unsigned int dst){
      //cout << "src: " << src << " dst: " << dst << endl;
      long count = graph->row_intersect(buffer,src,dst,decoded_src);
      //cout << count << endl;
      return count;
    }
  };
  thread_data **t_data_pointers;

  inline void allocBuffers(size_t num_threads){    
    //setup
    #if COMPRESSION == 1
    thread_local_buffers = new unsigned int[graph->max_nbrhood_size*num_threads];
    #else
    thread_local_buffers = graph->row_lengths; //not used; alloc can be expensive
    #endif

    t_data_pointers = new thread_data*[num_threads];
    for(size_t k= 0; k < num_threads; k++){
      t_data_pointers[k] = new thread_data(graph->max_nbrhood_size,k);
    }
  }
  inline void queryOver(size_t num_threads){
    auto row_function = std::bind(&AOA_Matrix::sum_over_columns_in_row<long>, graph, _1, _2, _3);

#ifdef ENABLE_PCM
      SystemCounterState before_sstate = getSystemCounterState();
#endif
      size_t matrix_size = graph->matrix_size;

      thread* threads = new thread[num_threads];
      std::atomic<long> reducer;
      reducer = 0;
      const size_t block_size = 150; //matrix_size / num_threads;
      std::atomic<size_t> next_work;
      next_work = 0;

      uint8_t *result = (uint8_t*) thread_local_buffers;
      if(num_threads > 1){
        for(size_t k = 0; k < num_threads; k++){
          auto edge_function = std::bind(&thread_data::edgeApply,t_data_pointers[k],_1,_2);
          threads[k] = thread([k, &matrix_size, &next_work, &reducer, &thread_local_buffers, edge_function, &row_function](void) -> void {
            long t_local_reducer = 0;
            while(true) {
              size_t work_start = next_work.fetch_add(block_size, std::memory_order_relaxed);
              if(work_start > matrix_size)
                break;

              size_t work_end = min(work_start + block_size, matrix_size);
              for(size_t j = work_start; j < work_end; j++) {
                t_local_reducer += (row_function)(j,&thread_local_buffers[k*matrix_size],edge_function);
              }
            }
             reducer += t_local_reducer;
           });
        } 

        //cleanup
        for(size_t k = 0; k < num_threads; k++) {
          threads[k].join();
        }
      } else{
        auto edge_function = std::bind(&thread_data::edgeApply,t_data_pointers[0],_1,_2);
        long t_local_reducer = 0;
        for(size_t i = 0; i  < matrix_size;  i++){
          t_local_reducer += (row_function)(i,thread_local_buffers,edge_function);
        }
        reducer = t_local_reducer;
      }

#ifdef ENABLE_PCM
      SystemCounterState after_sstate = getSystemCounterState();
        std::cout << "Instructions per clock: " << getIPC(before_sstate, after_sstate) << std::endl
          << "L2 cache hit ratio: " << getL2CacheHitRatio(before_sstate, after_sstate) << std::endl
          << "L3 cache hit ratio: " << getL3CacheHitRatio(before_sstate, after_sstate) << std::endl
          << "L2 cache misses: " << getL2CacheMisses(before_sstate, after_sstate) << std::endl
          << "L3 cache misses: " << getL3CacheMisses(before_sstate, after_sstate) << std::endl
          << "Cycles lost due to L2 cache misses: " << getCyclesLostDueL2CacheMisses(before_sstate, after_sstate) << std::endl
          << "Cycles lost due to L3 cache misses: " << getCyclesLostDueL3CacheMisses(before_sstate, after_sstate) << std::endl
          << "Bytes read: " << getBytesReadFromMC(before_sstate, after_sstate) << std::endl;
#endif

    num_triangles = reducer;
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
  
  common::startClock();
  inputGraph->reorder_by_degree();
  common::stopClock("Reordering");
  
  cout << endl;

#ifdef ENABLE_PCM
  PCM * m = PCM::getInstance();

  switch(m->program()) {
     case PCM::Success:
        cout << "PCM initialized" << endl;
        break;
     case PCM::PMUBusy:
        m->resetPMU();
        return -1;
        break;
     default:
        return -1;
        break;
  }
#endif

  common::startClock();
  application::graph = AOA_Matrix::from_symmetric(inputGraph->out_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,inputGraph->max_nbrhood_size,
    node_selection,edge_selection,inputGraph->external_ids,layout);
  common::stopClock("selections");
  
  //inputGraph->MutableGraph::~MutableGraph(); 

  //application::graph->print_data("out.txt");
  common::startClock();
  application::allocBuffers(atoi(argv[2]));
  common::stopClock("buffer allocation");

  common::startClock();
  application::queryOver(atoi(argv[2]));
  common::stopClock(input_layout);
  //application::graph->AOA_Matrix::~AOA_Matrix(); 
  cout << "Count: " << application::num_triangles << endl << endl;

#ifdef ENABLE_PCM
  m->resetPMU();
#endif
  return 0;
}
