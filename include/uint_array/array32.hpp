#include "common.hpp"

namespace array32 {
  static char max = char(0xff);
  static __m128i shuffle_mask32[16] = {        
    _mm_set_epi8(15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0), //0
    _mm_set_epi8(15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0), //1
    _mm_set_epi8(15,14,13,12,11,10,9,8,7,6,5,4,7,6,5,4), //2
    _mm_set_epi8(15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0), //3
    _mm_set_epi8(15,14,13,12,11,10,9,8,7,6,5,4,11,10,9,8), //4
    _mm_set_epi8(15,14,13,12,11,10,9,8,11,10,9,8,3,2,1,0), //5
    _mm_set_epi8(15,14,13,12,11,10,9,8,11,10,9,8,7,6,5,4), //6
    _mm_set_epi8(15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0), //7
    _mm_set_epi8(15,14,13,12,11,10,9,8,7,6,5,4,15,14,13,12), //8
    _mm_set_epi8(15,14,13,12,11,10,9,8,15,14,13,12,3,2,1,0), //9
    _mm_set_epi8(15,14,13,12,11,10,9,8,15,14,13,12,7,6,5,4), //10
    _mm_set_epi8(15,14,13,12,15,14,13,12,7,6,5,4,3,2,1,0), //11
    _mm_set_epi8(15,14,13,12,11,10,9,8,15,14,13,12,11,10,9,8), //12
    _mm_set_epi8(15,14,13,12,15,14,13,12,11,10,9,8,3,2,1,0), //13
    _mm_set_epi8(15,14,13,12,15,14,13,12,11,10,9,8,7,6,5,4), //14
    _mm_set_epi8(15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0), //15
  }; 
    //128 = 0xf0
  static __m128i shuffle_union_mask32 = _mm_set_epi8(15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);
 
  //128 = 0xf0
  static __m128i shuffle_difference_mask32_a[16] = {        
    _mm_set_epi8(15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0), //0
    _mm_set_epi8(max,max,max,max,15,14,13,12,11,10,9,8,7,6,5,4), //1
    _mm_set_epi8(max,max,max,max,11,10,9,8,7,6,5,4,3,2,1,0), //2
    _mm_set_epi8(max,max,max,max,max,max,max,max,15,14,13,12,11,10,9,8), //3
    _mm_set_epi8(max,max,max,max,15,14,13,12,7,6,5,4,3,2,1,0), //4
    _mm_set_epi8(max,max,max,max,max,max,max,max,15,14,13,12,7,6,5,4), //5 0101
    _mm_set_epi8(max,max,max,max,max,max,max,max,15,14,13,12,3,2,1,0), //6 0110
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,15,14,13,12), //7
    _mm_set_epi8(max,max,max,max,15,14,13,12,7,6,5,4,3,2,1,0), //8 1000
    _mm_set_epi8(max,max,max,max,max,max,max,max,11,10,9,8,7,6,5,4), //9 1001
    _mm_set_epi8(max,max,max,max,max,max,max,max,11,10,9,8,3,2,1,0), //10 1010
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,11,10,9,8), //11 1011
    _mm_set_epi8(max,max,max,max,max,max,max,max,7,6,5,4,3,2,1,0), //12 1100
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,7,6,5,4), //13 1101
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,3,2,1,0), //14
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //15
  }; 
  //128 = 0xf0
  static __m128i shuffle_difference_mask32_b[16] = {        
    _mm_set_epi8(3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12), //0
    _mm_set_epi8(7,6,5,4,11,10,9,8,15,14,13,12,max,max,max,max), //1
    _mm_set_epi8(3,2,1,0,7,6,5,4,15,14,13,12,max,max,max,max), //2
    _mm_set_epi8(11,10,9,8,15,14,13,12,max,max,max,max,max,max,max,max), //3
    _mm_set_epi8(3,2,1,0,7,6,5,4,15,14,13,12,max,max,max,max), //4
    _mm_set_epi8(7,6,5,4,15,14,13,12,max,max,max,max,max,max,max,max), //5 0101
    _mm_set_epi8(3,2,1,0,15,14,13,12,max,max,max,max,max,max,max,max), //6 0110
    _mm_set_epi8(15,14,13,12,max,max,max,max,max,max,max,max,max,max,max,max), //7
    _mm_set_epi8(3,2,1,0,7,6,5,4,11,10,9,8,max,max,max,max), //8 1000
    _mm_set_epi8(7,6,5,4,11,10,9,8,max,max,max,max,max,max,max,max), //9 1001
    _mm_set_epi8(3,2,1,0,11,10,9,8,max,max,max,max,max,max,max,max), //10 1010
    _mm_set_epi8(11,10,9,8,max,max,max,max,max,max,max,max,max,max,max,max), //11 1011
    _mm_set_epi8(3,2,1,0,7,6,5,4,max,max,max,max,max,max,max,max), //12 1100
    _mm_set_epi8(7,6,5,4,max,max,max,max,max,max,max,max,max,max,max,max), //13 1101
    _mm_set_epi8(3,2,1,0,max,max,max,max,max,max,max,max,max,max,max,max), //14
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //15
  };
  static __m128i setinel_mask_a[16] = {        
    _mm_set_epi8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0), //0
    _mm_set_epi8(max,max,max,max,0,0,0,0,0,0,0,0,0,0,0,0), //1
    _mm_set_epi8(max,max,max,max,0,0,0,0,0,0,0,0,0,0,0,0),//2
    _mm_set_epi8(max,max,max,max,max,max,max,max,0,0,0,0,0,0,0,0), //3
    _mm_set_epi8(max,max,max,max,max,max,max,max,0,0,0,0,0,0,0,0), //4
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,0,0,0,0), //5
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,0,0,0,0), //6
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,0,0,0,0), //7
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //8
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //9
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //10
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //11
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //12
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //13
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //14
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //15
  }; 
  static __m128i setinel_mask_b[16] = {        
    _mm_set_epi8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0), //0
    _mm_set_epi8(0,0,0,0,0,0,0,0,0,0,0,0,max,max,max,max), //1
    _mm_set_epi8(0,0,0,0,0,0,0,0,0,0,0,0,max,max,max,max),//2
    _mm_set_epi8(0,0,0,0,0,0,0,0,max,max,max,max,max,max,max,max), //3
    _mm_set_epi8(0,0,0,0,0,0,0,0,max,max,max,max,max,max,max,max), //4
    _mm_set_epi8(0,0,0,0,max,max,max,max,max,max,max,max,max,max,max,max), //5
    _mm_set_epi8(0,0,0,0,max,max,max,max,max,max,max,max,max,max,max,max), //6
    _mm_set_epi8(0,0,0,0,max,max,max,max,max,max,max,max,max,max,max,max), //7
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //8
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //9
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //10
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //11
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //12
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //13
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //14
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //15
  }; 

  inline size_t intersect(unsigned int *C, const unsigned int *A, const unsigned int *B, size_t s_a, size_t s_b) {
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
      unsigned int a_max = _mm_extract_epi32(v_a, 3);
      unsigned int b_max = _mm_extract_epi32(v_b, 3);
      i_a += (a_max <= b_max) * 4;
      i_b += (a_max >= b_max) * 4;
      //]

      //[ compute mask of common elements
      unsigned int cyclic_shift = _MM_SHUFFLE(0,3,2,1);
      __m128i cmp_mask1 = _mm_cmpeq_epi32(v_a, v_b);    // pairwise comparison
      v_b = _mm_shuffle_epi32(v_b, cyclic_shift);       // shuffling
      __m128i cmp_mask2 = _mm_cmpeq_epi32(v_a, v_b);    // again...
      v_b = _mm_shuffle_epi32(v_b, cyclic_shift);
      __m128i cmp_mask3 = _mm_cmpeq_epi32(v_a, v_b);    // and again...
      v_b = _mm_shuffle_epi32(v_b, cyclic_shift);
      __m128i cmp_mask4 = _mm_cmpeq_epi32(v_a, v_b);    // and again.
      __m128i cmp_mask = _mm_or_si128(
              _mm_or_si128(cmp_mask1, cmp_mask2),
              _mm_or_si128(cmp_mask3, cmp_mask4)
      ); // OR-ing of comparison masks
      // convert the 128-bit mask to the 4-bit mask
      unsigned int mask = _mm_movemask_ps((__m128)cmp_mask);
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
    
    return count;
  }
  inline size_t set_union(unsigned int *C, const unsigned int *A, const unsigned int *B, size_t s_a, size_t s_b) {
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

      //[ move pointers
      unsigned int a_max = _mm_extract_epi32(v_a, 3);
      unsigned int b_max = _mm_extract_epi32(v_b, 3);
      i_a += (a_max <= b_max) * 4;
      i_b += (a_max >= b_max) * 4;

      //[ compute mask of common elements
      unsigned int cyclic_shift = _MM_SHUFFLE(0,3,2,1);
      
      __m128i cmp_a_mask0 = _mm_cmpeq_epi32(v_a, v_b);    // pairwise comparison
      __m128i cmp_b_mask0 = cmp_a_mask0;   // pairwise comparison
      v_b = _mm_shuffle_epi32(v_b, cyclic_shift);       // shuffling

      __m128i cmp_a_mask1 = _mm_cmpeq_epi32(v_a, v_b);    // again...
      __m128i cmp_b_mask1 = _mm_shuffle_epi32(cmp_a_mask1, cyclic_shift);    // pairwise comparison
      v_b = _mm_shuffle_epi32(v_b, cyclic_shift);

      __m128i cmp_a_mask2 = _mm_cmpeq_epi32(v_a, v_b);    // and again...
      __m128i cmp_b_mask2 = _mm_shuffle_epi32(cmp_a_mask2, cyclic_shift);    // pairwise comparison
      v_b = _mm_shuffle_epi32(v_b, cyclic_shift);

      __m128i cmp_a_mask3 = _mm_cmpeq_epi32(v_a, v_b);    // and again.
      __m128i cmp_b_mask3 = _mm_shuffle_epi32(cmp_a_mask3, cyclic_shift);    // pairwise comparison
      v_b = _mm_shuffle_epi32(v_b, cyclic_shift);

      __m128i cmp_a_mask = _mm_or_si128(
              _mm_or_si128(cmp_a_mask0, cmp_a_mask1),
              _mm_or_si128(cmp_a_mask2, cmp_a_mask3)
      ); // OR-ing of comparison masks
      __m128i cmp_b_mask = _mm_or_si128(
              _mm_or_si128(cmp_b_mask0, cmp_b_mask1),
              _mm_or_si128(cmp_b_mask2, cmp_b_mask3)
      ); // OR-ing of comparison masks
      
      // convert the 128-bit mask to the 4-bit mask
      unsigned int mask_a = _mm_movemask_ps((__m128)cmp_a_mask);
      unsigned int mask_b = _mm_movemask_ps((__m128)cmp_b_mask);
      
      //[ copy out common elements
      #if WRITE_VECTOR == 1
      __m128i p_a = _mm_shuffle_epi8(v_a, shuffle_union_mask32);
      _mm_storeu_si128((__m128i*)&C[count], p_a);
      cout << "Mask_a: " << mask_a << endl;
      cout << "p_a[" << count << "]: " << C[count] << endl;
      cout << "p_a[" << count+1 << "]: " << C[count+1] << endl;
      cout << "p_a[" << count+2 << "]: " << C[count+2] << endl;
      cout << "p_a[" << count+3 << "]: " << C[count+3] << endl << endl;

      __m128i p_b = _mm_or_si128(setinel_mask_b[mask_b],_mm_shuffle_epi8(v_b, shuffle_difference_mask32_b[mask_b]));
      _mm_storeu_si128((__m128i*)&C[count], p_b);
      cout << "Mask_b: " << mask_b << endl;
      cout << "p_b[" << count << "]: " << C[count] << endl;
      cout << "p_b[" << count+1 << "]: " << C[count+1] << endl;
      cout << "p_b[" << count+2 << "]: " << C[count+2] << endl;
      cout << "p_b[" << count+3 << "]: " << C[count+3] << endl << endl;

      #endif

      count += 8-2*_mm_popcnt_u32(mask_a); // a number of elements is a weight of the mask

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

    return count;
  }
  inline size_t difference(unsigned int *C, const unsigned int *A, const unsigned int *B, size_t s_a, size_t s_b) {
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

      //[ move pointers
      unsigned int a_max = _mm_extract_epi32(v_a, 3);
      unsigned int b_max = _mm_extract_epi32(v_b, 3);
      i_a += (a_max <= b_max) * 4;
      i_b += (a_max >= b_max) * 4;

      //[ compute mask of common elements
      unsigned int cyclic_shift = _MM_SHUFFLE(0,3,2,1);
      
      __m128i cmp_a_mask0 = _mm_cmpeq_epi32(v_a, v_b);    // pairwise comparison
      __m128i cmp_b_mask0 = cmp_a_mask0;   // pairwise comparison
      v_b = _mm_shuffle_epi32(v_b, cyclic_shift);       // shuffling

      __m128i cmp_a_mask1 = _mm_cmpeq_epi32(v_a, v_b);    // again...
      __m128i cmp_b_mask1 = _mm_shuffle_epi32(cmp_a_mask1, cyclic_shift);    // pairwise comparison
      v_b = _mm_shuffle_epi32(v_b, cyclic_shift);

      __m128i cmp_a_mask2 = _mm_cmpeq_epi32(v_a, v_b);    // and again...
      __m128i cmp_b_mask2 = _mm_shuffle_epi32(cmp_a_mask2, cyclic_shift);    // pairwise comparison
      v_b = _mm_shuffle_epi32(v_b, cyclic_shift);

      __m128i cmp_a_mask3 = _mm_cmpeq_epi32(v_a, v_b);    // and again.
      __m128i cmp_b_mask3 = _mm_shuffle_epi32(cmp_a_mask3, cyclic_shift);    // pairwise comparison
      v_b = _mm_shuffle_epi32(v_b, cyclic_shift);

      __m128i cmp_a_mask = _mm_or_si128(
              _mm_or_si128(cmp_a_mask0, cmp_a_mask1),
              _mm_or_si128(cmp_a_mask2, cmp_a_mask3)
      ); // OR-ing of comparison masks
      __m128i cmp_b_mask = _mm_or_si128(
              _mm_or_si128(cmp_b_mask0, cmp_b_mask1),
              _mm_or_si128(cmp_b_mask2, cmp_b_mask3)
      ); // OR-ing of comparison masks
      
      // convert the 128-bit mask to the 4-bit mask
      unsigned int mask_a = _mm_movemask_ps((__m128)cmp_a_mask);
      unsigned int mask_b = _mm_movemask_ps((__m128)cmp_b_mask);
      
      unsigned int num_hit = _mm_popcnt_u32(mask_a);
      //[ copy out common elements
      #if WRITE_VECTOR == 1
      __m128i p_a = _mm_or_si128(setinel_mask_a[mask_a],_mm_shuffle_epi8(v_a, shuffle_difference_mask32_a[mask_a]));
      _mm_storeu_si128((__m128i*)&C[count], p_a);
      cout << "Mask_a: " << mask_a << endl;
      cout << "p_a[" << count << "]: " << C[count] << endl;
      cout << "p_a[" << count+1 << "]: " << C[count+1] << endl;
      cout << "p_a[" << count+2 << "]: " << C[count+2] << endl;
      cout << "p_a[" << count+3 << "]: " << C[count+3] << endl << endl;

      __m128i p_b = _mm_or_si128(setinel_mask_a[mask_b],_mm_shuffle_epi8(v_b, shuffle_difference_mask32_a[mask_b]));
      _mm_storeu_si128((__m128i*)&C[count], p_b);
      cout << "Mask_a: " << mask_b << endl;
      cout << "p_b[" << count << "]: " << C[count] << endl;
      cout << "p_b[" << count+1 << "]: " << C[count+1] << endl;
      cout << "p_b[" << count+2 << "]: " << C[count+2] << endl;
      cout << "p_b[" << count+3 << "]: " << C[count+3] << endl << endl;

      __m128i l_1 = _mm_min_epi32(p_a,p_b);
      __m128i h_1 = _mm_max_epi32(p_a,p_b);      

      // x x x v = l1 (v = valid data, x = tbd)
      // v x x x = h1 

      __m128i l_2 = _mm_min_epi32(_mm_shuffle_epi32(h_1, cyclic_shift),l_1);
      __m128i h_2 = _mm_max_epi32(h_1,_mm_srli_si128(l_1,32));

      // x x v v = l1 (v = valid data, x = tbd)
      // v v x x = h2 

      __m128i l_3 = _mm_min_epi32(_mm_shuffle_epi32(h_2, cyclic_shift),l_2);
      __m128i h_3 = _mm_max_epi32(h_2,_mm_srli_si128(l_2,32));

      _mm_storeu_si128((__m128i*)&C[count], l_3);
      cout << "Mask_b: " << mask_b << endl;
      cout << "result[" << count << "]: " << C[count] << endl;
      cout << "result[" << count+1 << "]: " << C[count+1] << endl;
      cout << "result[" << count+2 << "]: " << C[count+2] << endl;
      cout << "result[" << count+3 << "]: " << C[count+3] << endl << endl;

      _mm_storeu_si128((__m128i*)&C[count+(INTS_PER_REG-num_hit)], h_3);
      cout << "Mask_b: " << mask_b << endl;
      cout << "result[" << count << "]: " << C[count] << endl;
      cout << "result[" << count+(INTS_PER_REG-num_hit)+1 << "]: " << C[count+(INTS_PER_REG-num_hit)+1] << endl;
      cout << "result[" << count+(INTS_PER_REG-num_hit)+2 << "]: " << C[count+(INTS_PER_REG-num_hit)+2] << endl;
      cout << "result[" << count+(INTS_PER_REG-num_hit)+3 << "]: " << C[count+(INTS_PER_REG-num_hit)+3] << endl << endl;

      #endif

      count += (INTS_PER_REG*2)-(2*num_hit);// a number of elements is a weight of the mask

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
    
    return count;
  }
  template<typename T> 
  inline T sum_decoded(T (*function)(unsigned int,unsigned int,unsigned int*),unsigned int col,unsigned int *data, size_t length,unsigned int *outputA){
    T result = (T) 0;
    for(size_t i = 0; i < length; i++){
      result += function(col,data[i],outputA);
    }
    return result;
  }
  template<typename T> 
  inline T sum(unsigned int *data, size_t length, T *old_data, unsigned int *lengths){
    T result = 0.0;
    for(size_t i = 0; i < length; i++){
      result += old_data[data[i]]/lengths[data[i]];
    } 
    return result;
  }

  inline void print_data(unsigned int *data,size_t length,ofstream &file){
    //cout << "LEN: " << length << endl;
    for(size_t i = 0; i < length; i++){
      file << " Data: " << data[i] << endl;
    }
  }
  inline size_t preprocess(unsigned int *r, unsigned int *data, size_t length){
    std::copy(data,data+length,r);
    return length*4;
	}
} 