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
      return true;
      //return nbr < node;
    }
    #endif

    inline void produceSubgraph(){
      auto node_selection = std::bind(&application::myNodeSelection, this, _1, _2);
      auto edge_selection = std::bind(&application::myEdgeSelection, this, _1, _2, _3);

      graph = SparseMatrix<T,R>::from_symmetric_graph(inputGraph,node_selection,edge_selection,num_threads);
    }

    inline void queryOver(){
      //graph->print_data("graph.txt");

      system_counter_state_t before_sstate = pcm_get_counter_state();
      server_uncore_power_state_t* before_uncstate = pcm_get_uncore_power_state();

      ParallelBuffer<uint8_t> *src_buffers_ps = new ParallelBuffer<uint8_t>(num_threads,graph->matrix_size*10*sizeof(uint64_t));
      ParallelBuffer<uint8_t> *src_buffers_bs = new ParallelBuffer<uint8_t>(num_threads,graph->matrix_size*10*sizeof(uint64_t));

      ParallelBuffer<uint8_t> *dst_buffers_ps = new ParallelBuffer<uint8_t>(num_threads,graph->matrix_size*10*sizeof(uint64_t));
      ParallelBuffer<uint8_t> *dst_buffers_bs = new ParallelBuffer<uint8_t>(num_threads,graph->matrix_size*10*sizeof(uint64_t));

      ParallelBuffer<uint8_t> *buffers = new ParallelBuffer<uint8_t>(num_threads,graph->max_nbrhood_size*10*sizeof(uint64_t));

      double total_min = 0.0;

      const size_t matrix_size = graph->matrix_size;
      size_t *t_count = new size_t[num_threads * PADDING];
      common::par_for_range(num_threads, 0, matrix_size, 100,
        [this,src_buffers_ps,src_buffers_bs,dst_buffers_ps,dst_buffers_bs,buffers,t_count](size_t tid){
          src_buffers_ps->allocate(tid);
          src_buffers_bs->allocate(tid);
          dst_buffers_ps->allocate(tid);
          dst_buffers_bs->allocate(tid);
          buffers->allocate(tid);
          t_count[tid*PADDING] = 0;
        },
        ////////////////////////////////////////////////////
        [this,src_buffers_ps, src_buffers_bs, dst_buffers_ps,dst_buffers_bs,buffers,t_count, &total_min](size_t tid, size_t i) {
           long t_num_triangles = 0;
           uint8_t *src_buffer_ps = src_buffers_ps->data[tid];
           uint8_t *src_buffer_bs = src_buffers_bs->data[tid];

           uint8_t *dst_buffer_ps = dst_buffers_ps->data[tid];
           uint8_t *dst_buffer_bs = dst_buffers_bs->data[tid];

           Set<R> AA = this->graph->get_row(i);

           Set<uinteger> A_uint = AA;
           Set<pshort> A_ps = Set<pshort>::from_array(src_buffer_ps,(uint32_t*)AA.data,AA.cardinality);
           Set<bitset> A_bs = Set<bitset>::from_array(src_buffer_bs,(uint32_t*)AA.data,AA.cardinality);

           Set<R> C(buffers->data[tid]);

           AA.foreach([this, i, &AA, &A_uint, &A_ps, &A_bs, &C, &dst_buffer_ps, &dst_buffer_bs, &t_num_triangles, &total_min] (uint32_t j){
            Set<R> BB = this->graph->get_row(j);
            Set<uinteger> B_uint = BB;
            Set<pshort> B_ps = Set<pshort>::from_array(dst_buffer_ps,(uint32_t*)BB.data,BB.cardinality);
            Set<bitset> B_bs = Set<bitset>::from_array(dst_buffer_bs,(uint32_t*)BB.data,BB.cardinality);

            size_t tmp_count = 0;
            double start_time_1 = common::startClock();
            tmp_count = ops::set_intersect((Set<uinteger>*)&C,&A_uint,&B_uint)->cardinality;
            common::stopClock(start_time_1);

            double start_time_2 = common::startClock();
            tmp_count = ops::set_intersect((Set<pshort>*)&C,&A_ps,&B_ps)->cardinality;
            common::stopClock(start_time_2);

            double start_time_3 = common::startClock();
            tmp_count = ops::set_intersect((Set<bitset>*)&C,&A_bs,&B_bs)->cardinality;
            start_time_3 = common::stopClock(start_time_3);

            Set<pshort> A_ps_in = (AA.cardinality > BB.cardinality) ? A_ps:B_ps;
            Set<uinteger> B_uint_in = (AA.cardinality > BB.cardinality) ? B_uint:A_uint;

            double start_time_4 = common::startClock();
            tmp_count = ops::set_intersect((Set<uinteger>*)&C,&A_ps_in,&B_uint_in)->cardinality;
            start_time_4 = common::stopClock(start_time_4);

            A_ps_in = (AA.cardinality < BB.cardinality) ? A_ps:B_ps;
            Set<bitset> B_bs_in = (AA.cardinality < BB.cardinality) ? B_bs:A_bs;

            double start_time_5 = common::startClock();    
            tmp_count = ops::set_intersect((Set<pshort>*)&C,&A_ps_in,&B_bs_in)->cardinality;
            start_time_5 = common::stopClock(start_time_5);      

            Set<bitset> A_bs_in = (AA.cardinality < BB.cardinality) ? B_bs:A_bs;
            B_uint_in = (AA.cardinality < BB.cardinality) ? A_uint:B_uint;  

            double start_time_6 = common::startClock();    
            tmp_count = ops::set_intersect((Set<uinteger>*)&C,&A_bs_in,&B_uint_in)->cardinality;
            start_time_6 = common::stopClock(start_time_6);

            double min_12 = min(start_time_1,start_time_2);
            double min_34 = min(start_time_3,start_time_4);
            double min_56 = min(start_time_5,start_time_6);

            double min_1234 = min(min_12,min_34);
            double min_3456 = min(min_34,min_56);
            total_min += min(min_1234,min_3456);

            t_num_triangles += tmp_count;
           });

           t_count[tid*PADDING] += t_num_triangles;
        },
        ////////////////////////////////////////////////////////////
        [this,t_count](size_t tid){
          num_triangles += t_count[tid*PADDING];
        }
      );

    cout << "Best cost time: " << total_min << endl;

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

  application<uinteger,uinteger> myapp(input_data);
  myapp.run();
  return 0;
}
