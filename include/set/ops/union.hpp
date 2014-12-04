#ifndef _UNION_H_
#define _UNION_H_

#include "../Set.hpp"
#include "sse_masks.hpp"

namespace ops{
  inline Set<uinteger> set_union(Set<uinteger> C_in, Set<uinteger> A_in, Set<uinteger> B_in){
    uint32_t *C = (uint32_t*)C_in.data;
    const uint32_t *A = (uint32_t*) A_in.data;
    const uint32_t *B = (uint32_t*) B_in.data;
    const size_t s_a = A_in.cardinality;
    const size_t s_b = B_in.cardinality;

    uint32_t *itv = std::set_union(&A[0],&A[s_a],&B[0],&B[s_b],&C[0]);
    itv = std::unique(&C[0],itv);

    size_t count = (itv - C);
    double density = ((count > 0) ? (double)((C[count]-C[0])/count) : 0.0);
    return Set<uinteger>(C_in.data,count,count*sizeof(uint32_t),density,common::UINTEGER);
  }

  inline tuple<size_t,size_t,common::type> union_bs_bs(uint8_t *C, const uint8_t *A, const uint8_t *B, const size_t s_a, const size_t s_b) {
    #if WRITE_VECTOR == 0
    (void) C;
    #endif

    long count = 0l;
    const uint8_t *small = (s_a > s_b) ? B : A;
    const size_t small_length = (s_a > s_b) ? s_b : s_a;
    const uint8_t *large = (s_a <= s_b) ? B : A;

    //16 unsigned shorts
    //8 ints
    //4 longs
    size_t i = 0;
    
    #if VECTORIZE == 1
    while((i+15) < small_length){
      __m128i a1 = _mm_loadu_si128((const __m128i*)&A[i]);
      __m128i a2 = _mm_loadu_si128((const __m128i*)&B[i]);
      
      __m128i r = _mm_or_si128(a1, a2);
      
      #if WRITE_VECTOR == 1
      _mm_storeu_si128((__m128i*)&C[i], r);
      #endif
      
      unsigned long l = _mm_extract_epi64(r,0);
      count += _mm_popcnt_u64(l);
      l = _mm_extract_epi64(r,1);
      count += _mm_popcnt_u64(l);

      i += 16;
    }
    #endif

    for(; i < small_length; i++){
      uint8_t result = small[i] | large[i];

      #if WRITE_VECTOR == 1
      C[i] = result;
      #endif
      
      count += _mm_popcnt_u32(result);
    }

    return make_tuple(count,small_length,common::BITSET);
  }
  /*
  inline tuple<size_t,size_t,common::type> intersect_pshort_bs(uint16_t *C, const unsigned short *A, const uint8_t *B, const size_t s_a, const size_t s_b) {
    #if WRITE_VECTOR == 0
    (void) C;   
    #endif 

    size_t count = 0;
    size_t counter = 0;
    for(size_t i = 0; i < s_a; i++){
      uint32_t prefix = (A[i] << 16);
      unsigned short size = A[i+1];
      i += 2;

      size_t old_count = count;
      size_t old_counter = counter;
      counter += 2;

      size_t inner_end = i+size;
      while(i < inner_end){
        uint32_t cur = prefix | A[i];
        if(bitset_ops::word_index(cur) < s_b && bitset_ops::is_set(cur,B)){
          #if WRITE_VECTOR == 1
          C[counter++] = A[i];
          #endif
          count++;
        }
        ++i;
      }
      i--;

      if(old_counter == (counter-2)){
        counter = old_counter;
      } else{
        C[old_counter] = (prefix >> 16);
        C[old_counter+1] = old_count-count;
      }
    }
    return make_tuple(count,counter*sizeof(uint16_t),common::PSHORT);
  }
  */
  inline tuple<size_t,size_t,common::type> union_uint_bs(uint32_t *C, const uint32_t *A, const uint8_t *B, const size_t s_a, const size_t s_b) {
    (void) s_a;
    size_t count = 0;
    size_t i_a = 0;
    for(size_t i_b = 0; i_b < s_b; i_b++){
      uint8_t a_write_data = 0;
      uint32_t cur = A[i_a];
      size_t cur_index = bitset_ops::word_index(cur);
      while(cur_index == i_b){
        a_write_data |=  (1 << (cur_index%BITS_PER_WORD));
        cur = A[++i_a];
        cur_index = bitset_ops::word_index(cur);
      }
      C[bitset_ops::word_index(i_b)] = (B[bitset_ops::word_index(i_b)] | a_write_data);
    }

    return make_tuple(count,count*sizeof(uint32_t),common::UINTEGER);
  }
}

#endif
