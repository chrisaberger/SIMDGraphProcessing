#define WRITE_VECTOR 0

#include "SparseMatrix.hpp"
#include "MutableGraph.hpp"
#include "pcm_helper.hpp"
#include "Parser.hpp"

using namespace pcm_helper;

template<class T, class R>
class application{
  public:
    SparseMatrix<T,R>* graph;
    SparseMatrix<pshort,pshort>* ps_graph;
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

#ifdef ATTRIBUTES
    inline bool myNodeSelection(uint32_t node, uint32_t attribute){
      (void)node; (void) attribute;
      return true;//attribute > 500;
    }
    inline bool myEdgeSelection(uint32_t src, uint32_t dst, uint32_t attribute){
      (void) attribute;
      return attribute == 2012 && src < dst;
    }
#else
    inline bool myNodeSelection(uint32_t node, uint32_t attribute){
      (void)node; (void) attribute;
      return true;
    }
    inline bool myEdgeSelection(uint32_t node, uint32_t nbr, uint32_t attribute){
      (void) attribute;
      //return true;
      return nbr < node;
    }
    #endif

    inline void produceSubgraph(){
      auto node_selection = std::bind(&application::myNodeSelection, this, _1, _2);
      auto edge_selection = std::bind(&application::myEdgeSelection, this, _1, _2, _3);

      graph = SparseMatrix<T,R>::from_symmetric_graph(inputGraph,node_selection,edge_selection,num_threads);
      ps_graph = SparseMatrix<pshort,pshort>::from_symmetric_graph(inputGraph,node_selection,edge_selection,num_threads);
    }

    inline void queryOver(){
      //graph->print_data("graph.txt");

      system_counter_state_t before_sstate = pcm_get_counter_state();
      server_uncore_power_state_t* before_uncstate = pcm_get_uncore_power_state();

      ParallelBuffer<uint32_t> *src_buffers = new ParallelBuffer<uint32_t>(num_threads,graph->max_nbrhood_size);
      ParallelBuffer<uint32_t> *dst_buffers = new ParallelBuffer<uint32_t>(num_threads,graph->max_nbrhood_size);
      ParallelBuffer<uint8_t> *buffers = new ParallelBuffer<uint8_t>(num_threads,graph->max_nbrhood_size*sizeof(uint32_t));

      double intersect_time = 0.0;

      const size_t matrix_size = graph->matrix_size;
      size_t *t_count = new size_t[num_threads * PADDING];
      common::par_for_range(num_threads, 0, matrix_size, 100,
        [this,src_buffers,dst_buffers,buffers,t_count](size_t tid){
          src_buffers->allocate(tid);
          dst_buffers->allocate(tid);
          buffers->allocate(tid);
          t_count[tid*PADDING] = 0;
        },
        ////////////////////////////////////////////////////
        [this,src_buffers,dst_buffers,buffers,t_count,&intersect_time](size_t tid, size_t i) {
           long t_num_triangles = 0;

           Set<R> A = this->graph->get_row(i);
           Set<pshort> AA = this->ps_graph->get_row(i);

           Set<R> C(buffers->data[tid]);

           AA.foreach([this, i, &A, &AA, &C, &t_num_triangles,&intersect_time] (uint32_t j){
              size_t tmp_count = 0;
              Set<R> B = this->graph->get_row(j);
              if( A.cardinality > 30 && 
                ((double)A.cardinality/this->graph->matrix_size) < 0.03 &&
                abs(B.cardinality-A.cardinality) < 1000){
                //A.density > 0.003 && A.density < 0.03 && 
                //this->graph->row_lengths[j] > 100 && A.cardinality > 10){
                Set<pshort> BB = this->ps_graph->get_row(j);
                double timez = common::startClock();
                tmp_count = ops::set_intersect((Set<pshort>*)&C,&AA,&BB)->cardinality;
                intersect_time += common::stopClock(timez);
              } else{
                double timez = common::startClock();
                tmp_count  = ops::set_intersect(&C,&A,&B)->cardinality;
                intersect_time += common::stopClock(timez);
              }
              t_num_triangles += tmp_count;
           });

           t_count[tid*PADDING] += t_num_triangles;
        },
        ////////////////////////////////////////////////////////////
        [this,t_count](size_t tid){
          num_triangles += t_count[tid*PADDING];
        }
      );

    cout << "Intersection time: " << intersect_time << endl;

    server_uncore_power_state_t* after_uncstate = pcm_get_uncore_power_state();
    pcm_print_uncore_power_state(before_uncstate, after_uncstate);
    system_counter_state_t after_sstate = pcm_get_counter_state();
    pcm_print_counter_stats(before_sstate, after_sstate);
  }

  inline void run(){
    double start_time = common::startClock();
    produceSubgraph();
    common::stopClock("Selections",start_time);

    //graph->print_data("graph.txt");

    if(pcm_init() < 0)
       return;

    start_time = common::startClock();
    queryOver();
    common::stopClock("UNDIRECTED TRIANGLE COUNTING",start_time);

    cout << "Count: " << num_triangles << endl << endl;

    common::dump_stats();

    
    pcm_cleanup();
  }
};

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) { 
  Parser input_data = input_parser::parse(argc,argv,"undirected_triangle_counting");

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
