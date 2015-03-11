#define WRITE_VECTOR 0

#include "SparseMatrix.hpp"
#include "MutableGraph.hpp"
#include "pcm_helper.hpp"
#include "Parser.hpp"

using namespace pcm_helper;

string name = "symbiocity";
const static double N = 0.5;

template<class T, class R>
class application{
  public:
    SparseMatrix<T,R>* graph;
    long count;
    MutableGraph *inputGraph;
    size_t num_threads;
    string layout;

    application(Parser input_data){
      count = 0;
      inputGraph = input_data.input_graph;
      num_threads = input_data.num_threads;
      layout = input_data.layout;
    }

    inline bool myNodeSelection(uint32_t node, uint32_t attribute){
      (void)node; (void) attribute;
      return true;
    }
    inline bool myEdgeSelection(uint32_t node, uint32_t nbr, uint32_t attribute){
      (void) node; (void) nbr; (void) attribute;
      return true;
    }

    inline void produceSubgraph(){
      auto node_selection = std::bind(&application::myNodeSelection, this, _1, _2);
      auto edge_selection = std::bind(&application::myEdgeSelection, this, _1, _2, _3);

      graph = SparseMatrix<T,R>::from_asymmetric_graph(inputGraph,node_selection,edge_selection,num_threads);
    }

    inline void queryOver(){
      system_counter_state_t before_sstate = pcm_get_counter_state();
      server_uncore_power_state_t* before_uncstate = pcm_get_uncore_power_state();

      ParallelBuffer<uint8_t> *buffers = new ParallelBuffer<uint8_t>(num_threads,512*graph->max_nbrhood_size*sizeof(uint32_t));
      common::alloc_scratch_space(512*graph->max_nbrhood_size*sizeof(uint32_t),num_threads);

      size_t *t_count = new size_t[num_threads * PADDING];
      common::par_for_range(num_threads, 0, graph->matrix_size, 100,
        [this,buffers,t_count](size_t tid){
          buffers->allocate(tid);
          t_count[tid*PADDING] = 0;
        },
        ////////////////////////////////////////////////////
        [&](size_t tid, size_t i) {
           long t_num_sim_nodes = 0;
           Set<R> A = this->graph->get_row(i);
           Set<R> B = this->graph->get_column(i);
           Set<R> C(buffers->data[tid]);
           const size_t num_in_out = ops::set_intersect(&C,&A,&B)->cardinality;
           double ratio = (double) num_in_out / (A.cardinality + B.cardinality - num_in_out);
           if(ratio > N) {
             t_num_sim_nodes++;
           }

           t_count[tid * PADDING] += t_num_sim_nodes;
        },
        ////////////////////////////////////////////////////////////
        [this,t_count](size_t tid){
          count += t_count[tid*PADDING];
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
    common::stopClock(name, start_time);

    cout << "Count: " << count << endl << endl;

    common::dump_stats();

    pcm_cleanup();
  }
};

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) {
  Parser input_data = input_parser::parse(argc, argv, "symbiosity", common::DIRECTED);

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
  } else if(input_data.layout.compare("new_type") == 0){
    application<new_type,new_type> myapp(input_data);
    myapp.run();
  } else if(input_data.layout.compare("bitset_new") == 0){
    application<bitset_new,bitset_new> myapp(input_data);
    myapp.run();
  }
  #if COMPRESSION == 1
  else if(input_data.layout.compare("v") == 0){
    application<variant,uinteger> myapp(input_data);
    myapp.run();
  } else if(input_data.layout.compare("bp") == 0){
    application<bitpacked,uinteger> myapp(input_data);
    myapp.run();
  }
  #endif
  else{
    cout << "No valid layout entered." << endl;
    exit(0);
  }
  return 0;
}
