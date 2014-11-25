#ifndef _INTERSECTION_H_
#define _INTERSECTION_H_

#include "sse_masks.hpp"

namespace ops{
 inline tuple<size_t,size_t,common::type> intersect_u32_u32(uint8_t *C_in, const uint32_t *A, const uint32_t *B, size_t s_a, size_t s_b) {
    size_t count = 0;
    size_t i_a = 0, i_b = 0;

    #if WRITE_VECTOR == 1
    C_in[0] = common::ARRAY32;
    uint32_t *C = (uint32_t*)&C_in[1];
    #else
    uint32_t *C = (uint32_t*) C_in;
    #endif

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

    #if WRITE_VECTOR == 0
    C = C;
    #endif
    
    return make_tuple(count,count*sizeof(uint32_t),common::ARRAY32);
  }
  inline size_t simd_intersect_vector16(uint16_t *C, const uint16_t *A, const uint16_t *B, const size_t s_a, const size_t s_b) {
    #if WRITE_VECTOR == 0
    (void)C;
    #endif
    
    size_t count = 0;
    size_t i_a = 0, i_b = 0;

    #if VECTORIZE == 1
    size_t st_a = (s_a / SHORTS_PER_REG) * SHORTS_PER_REG;
    size_t st_b = (s_b / SHORTS_PER_REG) * SHORTS_PER_REG;

    while(i_a < st_a && i_b < st_b) {
      __m128i v_a = _mm_loadu_si128((__m128i*)&A[i_a]);
      __m128i v_b = _mm_loadu_si128((__m128i*)&B[i_b]);    

      uint16_t a_max = _mm_extract_epi16(v_a, SHORTS_PER_REG-1);
      uint16_t b_max = _mm_extract_epi16(v_b, SHORTS_PER_REG-1);
      
     // __m128i res_v = _mm_cmpistrm(v_b, v_a,
     //         _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
      __m128i res_v = _mm_cmpestrm(v_b, SHORTS_PER_REG, v_a, SHORTS_PER_REG,
              _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
      uint32_t r = _mm_extract_epi32(res_v, 0);

      #if WRITE_VECTOR == 1
      __m128i p = _mm_shuffle_epi8(v_a, shuffle_mask16[r]);
      _mm_storeu_si128((__m128i*)&C[count], p);
     #endif

      count += _mm_popcnt_u32(r);
      
      i_a += (a_max <= b_max) * SHORTS_PER_REG;
      i_b += (a_max >= b_max) * SHORTS_PER_REG;
    }
    #endif

    // intersect the tail using scalar intersection
    //...
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
    return count;
  }
  inline tuple<size_t,size_t,common::type> intersect_pshort_pshort(uint8_t *C_in,const uint16_t *A, const uint16_t *B, const size_t s_a, const size_t s_b) {
    size_t i_a = 0, i_b = 0;
    size_t counter = 0;
    size_t count = 0;
    bool notFinished = i_a < s_a && i_b < s_b;

    #if WRITE_VECTOR == 1
    size_t *C_size = (size_t*)&C_in[1];
    C_in[0] = common::ARRAY16;
    uint16_t *C = (uint16_t*)&C_in[sizeof(size_t)+1];
    #else
    uint16_t *C = (uint16_t*) C_in;
    #endif

    //cout << lim << endl;
    while(notFinished) {
      //size_t limLower = limLowerHolder;
      if(A[i_a] < B[i_b]) {
        i_a += A[i_a + 1] + 2;
        notFinished = i_a < s_a;
      } else if(B[i_b] < A[i_a]) {
        i_b += B[i_b + 1] + 2;
        notFinished = i_b < s_b;
      } else {
        uint16_t partition_size = 0;
        //If we are not in the range of the limit we don't need to worry about it.
        #if WRITE_VECTOR == 1
        C[counter++] = A[i_a]; // write partition prefix
        #endif
        partition_size = simd_intersect_vector16(&C[counter+1],&A[i_a + 2],&B[i_b + 2],A[i_a + 1], B[i_b + 1]);
        #if WRITE_VECTOR == 1
        C[counter++] = partition_size; // write partition size
        #endif
        i_a += A[i_a + 1] + 2;
        i_b += B[i_b + 1] + 2;      

        count += partition_size;
        counter += partition_size;
        notFinished = i_a < s_a && i_b < s_b;
      }
    }

    #if WRITE_VECTOR == 1
    C_size[0] = counter*sizeof(short);
    #endif

    return make_tuple(count,counter*sizeof(short),common::ARRAY16);
  }
}

#endif
