#ifndef _INTERSECTION_H_
#define _INTERSECTION_H_

namespace ops{
  inline size_t intersect(Set<hybrid> C_in, Set<hybrid> A_in, Set<hybrid> B_in) {
    if(A_in.type == common::ARRAY32){
      if(B_in.type == common::ARRAY32){
        return ops::intersect((Set<uint32>)C_in,(Set<uint32>)A_in,(Set<uint32>)B_in);
      }
    }
    return 0;
  }
  inline size_t intersect(Set<uint32> C_in, Set<uint32> A_in, Set<uint32> B_in) {
    uint32_t *C = (uint32_t*) C_in.data; 
    uint32_t *A = (uint32_t*) A_in.data;
    uint32_t *B = (uint32_t*) B_in.data;
    size_t s_a = A_in.cardinality;
    size_t s_b = B_in.cardinality;
 
    size_t count = 0;
    size_t i_a = 0, i_b = 0;

    // trim lengths to be a multiple of 4
    #if VECTORIZE == 1
    size_t st_a = (s_a / 4) * 4;
    size_t st_b = (s_b / 4) * 4;
    while(i_a < st_a && i_b < st_b) {
      //[ load segments of four 32-bit elements
      __m128i v_a = _mm_loadu_si128((__m128i*)&A[i_a]);
      __m128i v_b = _mm_loadu_si128((__m128i*)&B[i_b]);
      //]

      //[ move pointers
      uint32_t a_max = A[i_a+3];
      uint32_t b_max = B[i_b+3];
      i_a += (a_max <= b_max) * 4;
      i_b += (a_max >= b_max) * 4;
      //]

      //[ compute mask of common elements
      uint32_t right_cyclic_shift = _MM_SHUFFLE(0,3,2,1);
      __m128i cmp_mask1 = _mm_cmpeq_epi32(v_a, v_b);    // pairwise comparison
      v_b = _mm_shuffle_epi32(v_b, right_cyclic_shift);       // shuffling
      __m128i cmp_mask2 = _mm_cmpeq_epi32(v_a, v_b);    // again...
      v_b = _mm_shuffle_epi32(v_b, right_cyclic_shift);
      __m128i cmp_mask3 = _mm_cmpeq_epi32(v_a, v_b);    // and again...
      v_b = _mm_shuffle_epi32(v_b, right_cyclic_shift);
      __m128i cmp_mask4 = _mm_cmpeq_epi32(v_a, v_b);    // and again.
      __m128i cmp_mask = _mm_or_si128(
              _mm_or_si128(cmp_mask1, cmp_mask2),
              _mm_or_si128(cmp_mask3, cmp_mask4)
      ); // OR-ing of comparison masks
      // convert the 128-bit mask to the 4-bit mask
      uint32_t mask = _mm_movemask_ps((__m128)cmp_mask);
      //]

      //[ copy out common elements
      #if WRITE_VECTOR == 1
      //cout << "mask: " << mask << endl;
      __m128i p = _mm_shuffle_epi8(v_a, shuffle_mask32[mask]);
      _mm_storeu_si128((__m128i*)&C[count], p);
      //cout << "C[" << count << "]: " << C[count] << endl;

      #endif

      count += _mm_popcnt_u32(mask); // a number of elements is a weight of the mask
      //]
    }
    #endif

    // intersect the tail using scalar intersection
    bool notFinished = i_a < s_a  && i_b < s_b;
    while(notFinished){
      while(notFinished && B[i_b] < A[i_a]){
        ++i_b;
        notFinished = i_b < s_b;
      }
      if(notFinished && A[i_a] == B[i_b]){
        #if WRITE_VECTOR == 1
        C[count] = A[i_a];
        #endif

        ++count;
      }
      ++i_a;
      notFinished = notFinished && i_a < s_a;
    }
  
    C_in.cardinality = count;
    C_in.number_of_bytes = count*sizeof(uint32_t);
    return count;
  }
}
#endif
