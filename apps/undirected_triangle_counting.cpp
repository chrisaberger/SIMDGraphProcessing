#define WRITE_VECTOR 0

#include "emptyheaded.hpp"

using namespace pcm_helper;

inline bool myEdgeSelection(uint32_t node, uint32_t nbr, uint32_t attribute){
  (void) attribute;
  return nbr < node;
}

template<class T, class R>
SparseMatrix<T,R>* materialize_graph(MutableGraph *input_graph, const size_t num_threads){
  auto node_selection = [](uint32_t node, uint32_t attribute){
    (void) node; (void) attribute;
    return true;
  };
  auto edge_selection = [](uint32_t node, uint32_t nbr, uint32_t attribute){
    (void) attribute;
    return nbr < node;
  };
  return SparseMatrix<T,R>::from_symmetric_graph(input_graph,node_selection,edge_selection,num_threads);
}

template<class T, class R>
void application(Parser input_data){
  const size_t num_threads = input_data.num_threads;
  const string layout = input_data.layout;
  MutableGraph *input_graph = input_data.input_graph;

  SparseMatrix<T,R>* graph = materialize_graph<T,R>(input_graph,num_threads);
  long num_triangles = 0;

  double start_time = common::startClock();

  ParallelBuffer<uint8_t> *buffers = new ParallelBuffer<uint8_t>(num_threads,512*graph->max_nbrhood_size*sizeof(uint32_t));

  const size_t matrix_size = graph->matrix_size;
  size_t *t_count = new size_t[num_threads * PADDING];
  common::par_for_range(num_threads, 0, matrix_size, 100,
    [&](size_t tid){
      buffers->allocate(tid);
      t_count[tid*PADDING] = 0;
    },
    ////////////////////////////////////////////////////
    [&](size_t tid, size_t i) {
       long t_num_triangles = 0;
       Set<R> A = graph->get_row(i);
       Set<R> C(buffers->data[tid]);

       A.foreach([&] (uint32_t j){
        Set<R> B = graph->get_row(j);
        t_num_triangles += ops::set_intersect(&C,&A,&B)->cardinality;
       });

       t_count[tid*PADDING] += t_num_triangles;
    },
    ////////////////////////////////////////////////////////////
    [&](size_t tid){
      num_triangles += t_count[tid*PADDING];
    }
  );
  common::stopClock("UNDIRECTED TRIANGLE COUNTING",start_time);

  cout << "Number of Triangles: " << num_triangles << endl;
}
/*
template<class T, class R>
class application{
  public:
    SparseMatrix<T,R>* graph;
    long num_triangles;
    MutableGraph *inputGraph;
    size_t num_threads;
    string layout;

    application(Parser input_data){
      num_triangles = 0;
      inputGraph = input_data.input_graph;
      num_threads = input_data.num_threads;
      layout = input_data.layout;
    }

    inline bool myNodeSelection(uint32_t node, uint32_t attribute){
      (void)node; (void) attribute;
      return true;
    }
    inline bool myEdgeSelection(uint32_t node, uint32_t nbr, uint32_t attribute){
      (void) attribute;
      return nbr < node;
    }

    inline void queryOver(){
      system_counter_state_t before_sstate = pcm_get_counter_state();
      server_uncore_power_state_t* before_uncstate = pcm_get_uncore_power_state();

      ParallelBuffer<uint8_t> *buffers = new ParallelBuffer<uint8_t>(num_threads,512*graph->max_nbrhood_size*sizeof(uint32_t));

      const size_t matrix_size = graph->matrix_size;
      size_t *t_count = new size_t[num_threads * PADDING];
      common::par_for_range(num_threads, 0, matrix_size, 100,
        [this,buffers,t_count](size_t tid){
          buffers->allocate(tid);
          t_count[tid*PADDING] = 0;
        },
        ////////////////////////////////////////////////////
        [this,buffers,t_count](size_t tid, size_t i) {
           long t_num_triangles = 0;
           Set<R> A = this->graph->get_row(i);
           Set<R> C(buffers->data[tid]);

           A.foreach([this, i, &A, &C, &t_num_triangles] (uint32_t j){
            Set<R> B = this->graph->get_row(j);
            t_num_triangles += ops::set_intersect(&C,&A,&B)->cardinality;
           });

           t_count[tid*PADDING] += t_num_triangles;
        },
        ////////////////////////////////////////////////////////////
        [this,t_count](size_t tid){
          num_triangles += t_count[tid*PADDING];
        }
      );

    server_uncore_power_state_t* after_uncstate = pcm_get_uncore_power_state();
    pcm_print_uncore_power_state(before_uncstate, after_uncstate);
    system_counter_state_t after_sstate = pcm_get_counter_state();
    pcm_print_counter_stats(before_sstate, after_sstate);
  }

  inline void run(){
    double start_time = common::startClock();
    produceSubgraph();
    common::stopClock("Selections",start_time);

    if(pcm_init() < 0)
       return;


    start_time = common::startClock();
    queryOver();
    common::stopClock("UNDIRECTED TRIANGLE COUNTING",start_time);

    cout << "Count: " << num_triangles << endl << endl;

    common::dump_stats();

    pcm_cleanup();
  }
};*/