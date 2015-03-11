#define WRITE_VECTOR 0

#include "SparseMatrix.hpp"
#include "MutableGraph.hpp"
#include "pcm_helper.hpp"
#include "Parser.hpp"

using namespace pcm_helper;

template<class R> size_t count_partitions(Set<R> s) {
  size_t result = 0;
  uint32_t prefix = 0xFFFFFFFF;

  s.foreach([&] (uint32_t x) {
      uint32_t x_prefix = x >> 16;
      if(prefix != x_prefix) {
        result++;
        prefix = x_prefix;
      }
      });

  return result;
}

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
      return nbr < node;
      //return true;
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

      common::alloc_scratch_space(512*graph->max_nbrhood_size*sizeof(uint32_t),num_threads);

      ParallelBuffer<uint8_t> *src_buffers_bs_new = new ParallelBuffer<uint8_t>(num_threads,graph->matrix_size*100*sizeof(uint64_t));
      ParallelBuffer<uint8_t> *src_buffers_bs = new ParallelBuffer<uint8_t>(num_threads,graph->matrix_size*100*sizeof(uint64_t));
      ParallelBuffer<uint8_t> *src_buffers_new = new ParallelBuffer<uint8_t>(num_threads,graph->matrix_size*100*sizeof(uint64_t));

      ParallelBuffer<uint8_t> *dst_buffers_bs_new = new ParallelBuffer<uint8_t>(num_threads,graph->matrix_size*100*sizeof(uint64_t));
      ParallelBuffer<uint8_t> *dst_buffers_bs = new ParallelBuffer<uint8_t>(num_threads,graph->matrix_size*100*sizeof(uint64_t));
      ParallelBuffer<uint8_t> *dst_buffers_new = new ParallelBuffer<uint8_t>(num_threads,graph->matrix_size*100*sizeof(uint64_t));

      ParallelBuffer<uint8_t> *buffers = new ParallelBuffer<uint8_t>(num_threads,graph->matrix_size*100*sizeof(uint64_t));

      double total_min = 0.0;
      double total_min_bs = 0.0;
      double total_min_new_bs = 0.0;
      double total_pshort_min = 0.0;

      size_t num_uint = 0;
      size_t num_pshort = 0;
      size_t num_bs = 0;
      size_t num_uint_ps = 0;
      size_t num_uint_bs = 0;
      size_t num_ps_bs = 0;

      struct set_stats {
        size_t num_uint;
        size_t num_pshort;
        size_t num_bitset;
        size_t num_uint_ps;
        size_t num_uint_bs;
        size_t num_ps_bs;
        size_t card;
        int64_t min_val;
        int64_t max_val;
        size_t num_hybrid_uint_uint;
        size_t num_hybrid_pshort_pshort;
        size_t num_hybrid_bitset_bitset;
        size_t num_hybrid_uint_pshort;
        size_t num_hybrid_pshort_bitset;
        size_t num_hybrid_uint_bitset;
        size_t num_bs_instead_of_ps_psbs;
        size_t num_ubs_instead_of_ups;
        size_t num_bsbs_instead_of_ups;
        size_t num_uu_instead_of_ups;
        double t_time;
        common::type my_type;
      };

      vector<set_stats> stats(graph->matrix_size);
      for(size_t i = 0; i < graph->matrix_size; i++) {
        stats[i].num_uint = 0;
        stats[i].num_pshort = 0;
        stats[i].num_bitset = 0;
        stats[i].card = 0;
        stats[i].min_val = -1;
        stats[i].max_val = -1;
      }

      size_t lens_ubs_instead_of_ups = 0;
      size_t lens_u_ubs_instead_of_ups = 0;
      size_t num_ubs_instead_of_ups = 0;
      size_t bytes_ubs_instead_of_ups = 0;
      size_t parts_ubs_instead_of_ups = 0;
      size_t lens_bsbs_instead_of_psbs = 0;
      size_t lens_bs_bsbs_instead_of_psbs = 0;
      size_t num_bsbs_instead_of_psbs = 0;
      size_t bytes_bsbs_instead_of_psbs = 0;
      size_t parts_bsbs_instead_of_psbs = 0;
      double time_bad_ups = 0.0;
      double time_bad_ups_if_ubs = 0.0;
      double time_bad_psbs = 0.0;
      double time_bad_psbs_if_bsbs = 0.0;
      double total_hybrid_time = 0.0;
      double total_uint_time = 0.0;
      double total_bs_time = 0.0;
      double total_new_bs_time = 0.0;
      double total_new_time = 0.0;

      size_t parts_psbs = 0;
      size_t num_psbs = 0;
      size_t parts_ups = 0;
      size_t num_ups = 0;

      size_t num_uint_ints[5] = {0,0,0,0,0};
      double lost_times[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

      double better_than_uint[6] = {0,0,0,0,0,0};

      struct psbs_data {
        uint32_t ps_card;
        uint32_t bs_card;
        double psbs_time;
        double bsbs_time;
      };
      vector<psbs_data> psbs_data_points;

      struct ups_data {
        uint32_t u_id;
        uint32_t ps_id;
        double ups_time;
        double ubs_time;
      };
      vector<ups_data> ups_data_points;

      const size_t matrix_size = graph->matrix_size;
      size_t *t_count = new size_t[num_threads * PADDING];
      common::par_for_range(num_threads, 0, matrix_size, 100,
        [&](size_t tid){
          src_buffers_bs_new->allocate(tid);
          src_buffers_bs->allocate(tid);
          src_buffers_new->allocate(tid);
          dst_buffers_bs_new->allocate(tid);
          dst_buffers_bs->allocate(tid);
          dst_buffers_new->allocate(tid);
          buffers->allocate(tid);
          t_count[tid*PADDING] = 0;
        },
        ////////////////////////////////////////////////////
        [&](size_t tid, size_t i) {
           long t_num_triangles = 0;
           uint8_t *src_buffer_bs_new = src_buffers_bs_new->data[tid];
           uint8_t *src_buffer_bs = src_buffers_bs->data[tid];
           uint8_t *src_buffer_new = src_buffers_new->data[tid];

           uint8_t *dst_buffer_bs_new = dst_buffers_bs_new->data[tid];
           uint8_t *dst_buffer_bs = dst_buffers_bs->data[tid];
           uint8_t *dst_buffer_new = dst_buffers_new->data[tid];

           Set<R> AA = this->graph->get_row(i);

           Set<uinteger> A_uint = AA;
           Set<bitset_new> A_new_bs = Set<bitset_new>::from_array(src_buffer_bs_new,(uint32_t*)AA.data,AA.cardinality);
           Set<bitset> A_bs = Set<bitset>::from_array(src_buffer_bs,(uint32_t*)AA.data,AA.cardinality);
           Set<new_type> A_new = Set<new_type>::from_array(src_buffer_new,(uint32_t*)AA.data,AA.cardinality);
           stats[i].card = AA.cardinality;

           AA.foreach([&] (uint32_t j){
            Set<R> C(buffers->data[tid]);
            stats[i].min_val = (stats[i].min_val == -1) ? j : min(stats[i].min_val, (int64_t) j);
            stats[i].max_val = (stats[i].max_val == -1) ? j : max(stats[i].max_val, (int64_t) j);

            Set<R> BB = this->graph->get_row(j);

            Set<uinteger> B_uint = BB;
            Set<bitset_new> B_new_bs = Set<bitset_new>::from_array(dst_buffer_bs_new,(uint32_t*)BB.data,BB.cardinality);
            Set<bitset> B_bs = Set<bitset>::from_array(dst_buffer_bs,(uint32_t*)BB.data,BB.cardinality);
            Set<new_type> B_new = Set<new_type>::from_array(dst_buffer_new,(uint32_t*)BB.data,BB.cardinality);

            size_t tmp_count = 0;

            Set<uinteger> A_uint_in = (AA.cardinality < BB.cardinality) ? A_uint : B_uint;
            Set<uinteger> B_uint_in = (AA.cardinality < BB.cardinality) ? B_uint : A_uint;

            double start_time_1s[6];
            start_time_1s[0] = common::startClock();
            tmp_count = ops::set_intersect_ibm((Set<uinteger>*)&C,&A_uint_in,&B_uint_in)->cardinality;
            start_time_1s[0] = common::stopClock(start_time_1s[0]);
            start_time_1s[1] = common::startClock();
            tmp_count = ops::set_intersect_v1((Set<uinteger>*)&C,&A_uint_in,&B_uint_in)->cardinality;
            start_time_1s[1] = common::stopClock(start_time_1s[1]);
            start_time_1s[2] = common::startClock();
            tmp_count = ops::set_intersect_v3((Set<uinteger>*)&C,&A_uint_in,&B_uint_in)->cardinality;
            start_time_1s[2] = common::stopClock(start_time_1s[2]);
            start_time_1s[3] = common::startClock();
            tmp_count = ops::set_intersect_galloping((Set<uinteger>*)&C,&A_uint_in,&B_uint_in)->cardinality;
            start_time_1s[3] = common::stopClock(start_time_1s[3]);
            start_time_1s[4] = common::startClock();
            tmp_count = ops::set_intersect_standard((Set<uinteger>*)&C,&A_uint_in,&B_uint_in)->cardinality;
            start_time_1s[4] = common::stopClock(start_time_1s[4]);

            // What would the optimizer do?
            start_time_1s[5] = common::startClock();
            tmp_count = ops::set_intersect((Set<uinteger>*)&C,&A_uint_in,&B_uint_in)->cardinality;
            start_time_1s[5] = common::stopClock(start_time_1s[5]);

            double start_time_1 = 1000.0;
            int min_uint_int = 0;
            for(size_t i = 0; i < 5; i++) {
              if(start_time_1 > start_time_1s[i]) {
                start_time_1 = start_time_1s[i];
                min_uint_int = i;
              }
            }

            total_uint_time += start_time_1;

            /*
            double start_time_2 = common::startClock();
            tmp_count = ops::set_intersect((Set<pshort>*)&C,&A_ps,&B_ps)->cardinality;
            start_time_2 = common::stopClock(start_time_2);
            */

            // New bitset
            double start_time_new_bs = common::startClock();
            tmp_count = ops::set_intersect((Set<bitset_new>*)&C,&A_new_bs,&B_new_bs)->cardinality;
            start_time_new_bs = common::stopClock(start_time_new_bs);
            total_new_bs_time += start_time_new_bs;

            double start_time_3 = common::startClock();
            tmp_count = ops::set_intersect((Set<bitset>*)&C,&A_bs,&B_bs)->cardinality;
            start_time_3 = common::stopClock(start_time_3);
            total_bs_time += start_time_3;

            double start_time_new = common::startClock();
            tmp_count = ops::set_intersect((Set<new_type>*)&C,&A_new,&B_new)->cardinality;
            start_time_new = common::stopClock(start_time_new);
            total_new_time += start_time_new;

            bool A_is_new_bs_in_u_new_bs = true;
            double start_time_u_new_bs_a = common::startClock();
            tmp_count = ops::set_intersect((Set<uinteger>*)&C,&A_new_bs,&B_uint)->cardinality;
            start_time_u_new_bs_a = common::stopClock(start_time_u_new_bs_a);
            double start_time_u_new_bs_b = common::startClock();
            tmp_count = ops::set_intersect((Set<uinteger>*)&C,&B_new_bs,&A_uint)->cardinality;
            start_time_u_new_bs_b = common::stopClock(start_time_u_new_bs_b);
            double start_time_u_new_bs = min(start_time_u_new_bs_a, start_time_u_new_bs_b);
            if(start_time_u_new_bs_a > start_time_u_new_bs_b)
              A_is_new_bs_in_u_new_bs = false;

            /*
            bool A_is_ps_in_psbs = true;
            double start_time_5_a = common::startClock();
            tmp_count = ops::set_intersect((Set<pshort>*)&C,&A_ps,&B_bs)->cardinality;
            start_time_5_a = common::stopClock(start_time_5_a);
            double start_time_5_b = common::startClock();
            tmp_count = ops::set_intersect((Set<pshort>*)&C,&B_ps,&A_bs)->cardinality;
            start_time_5_b = common::stopClock(start_time_5_b);
            double start_time_5 = min(start_time_5_a, start_time_5_b);
            if(start_time_5_a > start_time_5_b)
              A_is_ps_in_psbs = false;
              */

            bool A_is_u_in_ubs = true;
            double start_time_6_a = common::startClock();
            tmp_count = ops::set_intersect((Set<uinteger>*)&C,&A_bs,&B_uint)->cardinality;
            start_time_6_a = common::stopClock(start_time_6_a);
            double start_time_6_b = common::startClock();
            tmp_count = ops::set_intersect((Set<uinteger>*)&C,&B_bs,&A_uint)->cardinality;
            start_time_6_b = common::stopClock(start_time_6_b);
            double start_time_6 = min(start_time_6_a, start_time_6_b);
            if(start_time_6_a > start_time_6_b)
              A_is_u_in_ubs = false;

            total_min += min(start_time_1, min(start_time_new_bs, min(start_time_3, min(start_time_new, min(start_time_u_new_bs, start_time_6)))));
            total_min_bs += min(start_time_1, min(start_time_3, start_time_6));
            total_min_new_bs += min(start_time_1, min(start_time_new_bs, start_time_u_new_bs));

            double min_time = 0.0;

            bool bs_bs_best = false;
            bool u_bs_best = false;
            bool u_u_best = false;

            int best = 0;

            /*
            if(start_time_1 <= start_time_3 &&
              start_time_1 <= start_time_6){
              total_pshort_min += start_time_1;
            } else if(start_time_3 <= start_time_1 && 
              start_time_3 <= start_time_6){
              total_pshort_min += start_time_3;
            } else {
              total_pshort_min += start_time_6;
            }
            */

            if(start_time_1 <= start_time_1 &&
              //start_time_1 <= start_time_2 &&  
              start_time_1 <= start_time_3 && 
              //start_time_1 <= start_time_4 && 
              //start_time_1 <= start_time_5 && 
              start_time_1 <= start_time_6){
              num_uint++;
              min_time = start_time_1;
              stats[i].t_time += min_time;
              stats[j].t_time += min_time;
              stats[i].num_uint++;
              stats[j].num_uint++;
              u_u_best = true;
              num_uint_ints[min_uint_int]++;
              best = 0;
            }/* else if(start_time_2 <= start_time_1 &&
              start_time_2 <= start_time_2 &&  
              start_time_2 <= start_time_3 && 
              start_time_2 <= start_time_4 && 
              start_time_2 <= start_time_5 && 
              start_time_2 <= start_time_6){
              num_pshort++;
              min_time = start_time_2;
              stats[i].t_time += min_time;
              stats[j].t_time += min_time;
              stats[i].num_pshort++;
              stats[j].num_pshort++;
              best = 1;
            } */else if(start_time_3 <= start_time_1 &&
              //start_time_3 <= start_time_2 &&  
              start_time_3 <= start_time_3 && 
              //start_time_3 <= start_time_4 && 
              //start_time_3 <= start_time_5 && 
              start_time_3 <= start_time_6){
              num_bs++;
              min_time = start_time_3;
              stats[i].t_time += min_time;
              stats[j].t_time += min_time;
              stats[i].num_bitset++;
              stats[j].num_bitset++;
              bs_bs_best = true;
              best = 2;
            } /*else if(start_time_4 <= start_time_1 &&
              start_time_4 <= start_time_2 &&  
              start_time_4 <= start_time_3 && 
              start_time_4 <= start_time_4 && 
              start_time_4 <= start_time_5 && 
              start_time_4 <= start_time_6){
              num_uint_ps++;
              min_time = start_time_4;
              stats[i].t_time += min_time;
              stats[j].t_time += min_time;
              stats[i].num_uint_ps++;
              stats[j].num_uint_ps++;
              num_ups++;
              //parts_ups += count_partitions((A_is_ps_in_ups) ? AA : BB);

              if(AA.cardinality < BB.cardinality) {
                stats[i].num_uint++;
                stats[j].num_pshort++;
              } else {
                stats[j].num_uint++;
                stats[i].num_pshort++;
              }
              best = 3;
            } else if(start_time_5 <= start_time_1 &&
              start_time_5 <= start_time_2 &&  
              start_time_5 <= start_time_3 && 
              start_time_5 <= start_time_4 && 
              start_time_5 <= start_time_5 && 
              start_time_5 <= start_time_6){
              num_ps_bs++;
              min_time = start_time_5;
              stats[i].t_time += min_time;
              stats[j].t_time += min_time;
              stats[i].num_ps_bs++;
              stats[j].num_ps_bs++;
              num_psbs++;
              //parts_psbs += count_partitions((A_is_ps_in_psbs) ? AA : BB);

              if(AA.cardinality < BB.cardinality) {
                stats[i].num_pshort++;
                stats[j].num_bitset++;
              } else {
                stats[j].num_pshort++;
                stats[i].num_bitset++;
              }
              best = 4;
            }*/ else {
              num_uint_bs++;
              min_time = start_time_6;
              stats[i].t_time += min_time;
              stats[j].t_time += min_time;
              stats[i].num_uint_bs++;
              stats[j].num_uint_bs++;
              u_bs_best = true;
              /*
              if(AA.cardinality < BB.cardinality) {
                stats[i].num_uint++;
                stats[j].num_bitset++;
              } else {
                stats[j].num_uint++;
                stats[i].num_bitset++;
              }
              */
              best = 5;
            }

            double min_ratio = 2.0;
            double hybrid_intersection_time = 0.0;
            common::type a_type = hybrid::get_type((uint32_t*)AA.data,AA.cardinality);
            common::type b_type = hybrid::get_type((uint32_t*)BB.data,BB.cardinality);
            if(min_time > 0.0){
              stats[i].my_type = a_type;
              stats[j].my_type =b_type;
              if(a_type == common::UINTEGER && b_type == common::UINTEGER){
                hybrid_intersection_time = start_time_1s[5];
                lost_times[0] += hybrid_intersection_time - min_time;
                better_than_uint[best] += hybrid_intersection_time - min_time;
                if((hybrid_intersection_time/min_time) > min_ratio){
                  stats[i].num_hybrid_uint_uint++;
                  stats[j].num_hybrid_uint_uint++;
                }
              } /*else if(a_type == common::PSHORT && b_type == common::PSHORT){
                hybrid_intersection_time = start_time_2;
                lost_times[1] += hybrid_intersection_time - min_time;
                if((hybrid_intersection_time/min_time) > min_ratio){
                  stats[i].num_hybrid_pshort_pshort++;
                  stats[j].num_hybrid_pshort_pshort++;
                }
              }*/ else if(a_type == common::BITSET && b_type == common::BITSET){
                hybrid_intersection_time = start_time_3;
                lost_times[2] += hybrid_intersection_time - min_time;
                if((hybrid_intersection_time/min_time) > min_ratio){
                  stats[i].num_hybrid_bitset_bitset++;
                  stats[j].num_hybrid_bitset_bitset++;
                }
              } /*else if( (a_type == common::UINTEGER && b_type == common::PSHORT) ||
                (a_type == common::PSHORT && b_type == common::UINTEGER)){
                hybrid_intersection_time = start_time_4;
                lost_times[3] += hybrid_intersection_time - min_time;

                if(best == 5) {
                  uint32_t ps_id = (A_is_ps_in_psbs) ? i : j;
                  uint32_t bs_id = (A_is_ps_in_psbs) ? j : i;
                  psbs_data data_point = { ps_id, bs_id, start_time_5, min_time };
                  psbs_data_points.push_back(data_point);
                }

                if((hybrid_intersection_time/min_time) > min_ratio){
                    stats[i].num_hybrid_uint_pshort++;
                    stats[j].num_hybrid_uint_pshort++;

                    if(u_bs_best) {
                      stats[i].num_ubs_instead_of_ups++;
                      stats[j].num_ubs_instead_of_ups++;
                      lens_ubs_instead_of_ups += (a_type == common::PSHORT) ? AA.cardinality : BB.cardinality;
                      lens_u_ubs_instead_of_ups += (a_type == common::PSHORT) ? BB.cardinality : AA.cardinality;
                      num_ubs_instead_of_ups++;

                      time_bad_ups += start_time_4;
                      time_bad_ups_if_ubs += min_time;
                      bytes_ubs_instead_of_ups += (a_type == common::PSHORT) ? A_bs.number_of_bytes : B_bs.number_of_bytes;
                      parts_ubs_instead_of_ups += count_partitions((a_type == common::PSHORT) ? AA : BB);
                    }
                    else if(bs_bs_best) {
                      stats[i].num_bsbs_instead_of_ups++;
                      stats[j].num_bsbs_instead_of_ups++;
                    }
                    else if(u_u_best) {
                      stats[i].num_uu_instead_of_ups++;
                      stats[j].num_uu_instead_of_ups++;
                    }
                }
              }  else if( (a_type == common::BITSET && b_type == common::PSHORT) ||
                (a_type == common::PSHORT && b_type == common::BITSET)){
                hybrid_intersection_time = start_time_5;
                lost_times[4] += hybrid_intersection_time - min_time;

                if(bs_bs_best) {
                  //uint32_t ps_card = (A_is_ps_in_psbs) ? AA.cardinality : BB.cardinality;
                  //uint32_t bs_card = (A_is_ps_in_psbs) ? BB.cardinality : AA.cardinality;
                  //psbs_data data_point = { ps_card, bs_card, start_time_5, min_time };
                  //psbs_data_points.push_back(data_point);
                }

                if((hybrid_intersection_time/min_time) > min_ratio){
                    stats[i].num_hybrid_pshort_bitset++;
                    stats[j].num_hybrid_pshort_bitset++;

                    if(bs_bs_best) {
                      stats[i].num_bs_instead_of_ps_psbs++;
                      stats[j].num_bs_instead_of_ps_psbs++;
                      lens_bsbs_instead_of_psbs += (a_type == common::PSHORT) ? AA.cardinality : BB.cardinality;
                      lens_bs_bsbs_instead_of_psbs += (a_type == common::PSHORT) ? BB.cardinality : AA.cardinality;
                      num_bsbs_instead_of_psbs++;

                      time_bad_psbs += start_time_5;
                      time_bad_psbs_if_bsbs += min_time;
                      bytes_bsbs_instead_of_psbs += (a_type == common::PSHORT) ? A_bs.number_of_bytes : B_bs.number_of_bytes;
                      parts_bsbs_instead_of_psbs += count_partitions((a_type == common::PSHORT) ? AA : BB);
                    }
                }
              }*/  else if((a_type == common::UINTEGER && b_type == common::BITSET) ||
                (a_type == common::BITSET && b_type == common::UINTEGER)){
                hybrid_intersection_time = start_time_6;
                lost_times[5] += hybrid_intersection_time - min_time;
                if((hybrid_intersection_time/min_time) > min_ratio){
                    stats[i].num_hybrid_uint_bitset++;
                    stats[j].num_hybrid_uint_bitset++;
                }
              }

              total_hybrid_time += hybrid_intersection_time;
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

    cout << "Best cost time: " << total_min << endl;
    cout << "Best cost time (u/bs only): " << total_min_bs << endl;
    cout << "Best cost time (u/new_bs only): " << total_min_new_bs << endl;
    cout << "Hybrid time: " << total_hybrid_time << endl;
    cout << "U-Int time: " << total_uint_time << endl;
    cout << "BS time: " << total_bs_time << endl;
    cout << "New BS time: " << total_bs_time << endl;
    cout << "Block level time: " << total_new_time << endl;
    cout << "Uint: " << num_uint << endl;
    cout << "Pshort: " << num_pshort << endl;
    cout << "Bs: " << num_bs << endl;
    cout << "Uint/pshort: " << num_uint_ps << endl;
    cout << "PS/BS: " << num_ps_bs << endl;
    cout << "UINT/BS: " << num_uint_bs << endl;
    /*
    cout << "Avg. PS card when BSBS instead of PSBS: " << (lens_bsbs_instead_of_psbs / num_bsbs_instead_of_psbs) << endl;
    cout << "Avg. BS card when BSBS instead of PSBS: " << (lens_bs_bsbs_instead_of_psbs / num_bsbs_instead_of_psbs) << endl;
    cout << "Avg. PS card when UBS instead of UPS: " << (lens_ubs_instead_of_ups / num_ubs_instead_of_ups) << endl;
    cout << "Avg. U card when UBS instead of UPS: " << (lens_u_ubs_instead_of_ups / num_ubs_instead_of_ups) << endl;
    cout << "Avg. bytes BS when BSBS instead of PSBS: " << (bytes_bsbs_instead_of_psbs / num_bsbs_instead_of_psbs) << endl;
    cout << "Avg. bytes BS when UBS instead of UPS: " << (bytes_ubs_instead_of_ups / num_ubs_instead_of_ups) << endl;
    cout << "Avg. partitions PS when BSBS instead of PSBS: " << ((double) parts_bsbs_instead_of_psbs / num_bsbs_instead_of_psbs) << endl;
    cout << "Avg. partitions PS when UBS instead of UPS: " << ((double) parts_ubs_instead_of_ups / num_ubs_instead_of_ups) << endl;
    cout << "Avg. partitions PS when PSBS wins: " << ((double) parts_psbs / num_psbs) << endl;
    cout << "Avg. partitions PS when UPS wins: " << ((double) parts_ups / num_ups) << endl;
    cout << "Time UPS when UBS better than UPS: " << time_bad_ups << endl;
    cout << "Time UBS when UBS better than UPS: " << time_bad_ups_if_ubs << endl;
    cout << "Time PSBS when BSBS better than PSBS: " << time_bad_psbs << endl;
    cout << "Time BSBS when BSBS better than PSBS: " << time_bad_psbs_if_bsbs << endl;
    for(size_t i = 0; i < 5; i++) {
      cout << "UINT int " << i << ": " << num_uint_ints[i] << endl;
    }
    */

    string lost_names[] = {"U-Int/U-Int", "P-Short/P-Short", "Bitset/Bitset", "U-Int/P-Short", "P-Short/Bitset", "U-Int/Bitset"};
    for(size_t i = 0; i < 6; i++) {
      cout << "Lost time " << lost_names[i] << ": " << lost_times[i] << endl;
    }

    for(size_t i = 0; i < 6; i++) {
      cout << "Replace U-Int/U-Int with " << lost_names[i] << ": " << better_than_uint[i] << endl;
    }

    /*
    ofstream stats_file;
    stats_file.open("stats.csv");
    stats_file << "id,card,range,uint/uint,pshort/pshort,bitset/bitset,u/ps,ps/bs,u/bs,,eh uint/uint,eh pshort/pshort,eh bitset/bitset,eh u/ps,eh ps/bs,eh u/bs,percent time,ehtype,num_bs_instead_of_ps_psbs,num_ubs_instead_of_ups,num_bsbs_instead_of_ups,num_uu_instead_of_ups" << std::endl;
    for(size_t i = 0; i < graph->matrix_size; i++) {
      stats_file << i << ",";
      stats_file << stats[i].card << ",";
      stats_file << (stats[i].max_val - stats[i].min_val) << ",";
      stats_file << stats[i].num_uint << ",";
      stats_file << stats[i].num_pshort << ",";
      stats_file << stats[i].num_bitset << ",";
      stats_file << stats[i].num_uint_ps << ",";
      stats_file << stats[i].num_ps_bs << ",";
      stats_file << stats[i].num_uint_bs << ",,";
      stats_file << stats[i].num_hybrid_uint_uint << ",";
      stats_file << stats[i].num_hybrid_pshort_pshort << ",";
      stats_file << stats[i].num_hybrid_bitset_bitset << ",";
      stats_file << stats[i].num_hybrid_uint_pshort << ",";
      stats_file << stats[i].num_hybrid_pshort_bitset << ",";
      stats_file << stats[i].num_hybrid_uint_bitset << ",";
      stats_file << ((double)stats[i].t_time/total_min) << ",";
      if(stats[i].my_type == common::BITSET)
        stats_file << "bitset,";
      else if(stats[i].my_type == common::PSHORT)
        stats_file << "pshort,";
      else 
        stats_file << "uint,";
      stats_file << stats[i].num_bs_instead_of_ps_psbs << ",";
      stats_file << stats[i].num_ubs_instead_of_ups << ",";
      stats_file << stats[i].num_bsbs_instead_of_ups << ",";
      stats_file << stats[i].num_uu_instead_of_ups;
      stats_file << std::endl;
    }

    ofstream psbs_file;
    psbs_file.open("psbs.csv");
    psbs_file << "ps_card,bs_card,psbs,bsbs" << endl;
    for(auto data_point : psbs_data_points) {
      psbs_file << data_point.ps_card << ",";
      psbs_file << data_point.bs_card << ",";
      psbs_file << data_point.psbs_time << ",";
      psbs_file << data_point.bsbs_time << endl;
    }
    psbs_file.close();*/

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
