#ifndef _INTERSECTION_H_
#define _INTERSECTION_H_

#include "../Set.hpp"
#include "sse_masks.hpp"

namespace ops{
  inline Set<uinteger> set_intersect(Set<uinteger> C_in, Set<uinteger> A_in, Set<uinteger> B_in){
    uint32_t *C = (uint32_t*) C_in.data; 
    const uint32_t *A = (uint32_t*) A_in.data;
    const uint32_t *B = (uint32_t*) B_in.data;
    const size_t s_a = A_in.cardinality;
    const size_t s_b = B_in.cardinality;

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

    #if WRITE_VECTOR == 0
    C = C;
    #endif

    double density = ((count > 0) ? (double)((C[count]-C[0])/count) : 0.0);

    C_in.cardinality = count;
    C_in.number_of_bytes = count*sizeof(uint32_t);
    C_in.density = density;
    C_in.type = common::UINTEGER;
    return C_in;
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
  inline Set<pshort> set_intersect(Set<pshort> C_in, Set<pshort> A_in, Set<pshort> B_in){
    uint16_t *C = (uint16_t*)C_in.data;
    const uint16_t *A = (uint16_t*)A_in.data;
    const uint16_t *B = (uint16_t*)B_in.data;
    const size_t s_a = A_in.number_of_bytes/sizeof(uint16_t);
    const size_t s_b = B_in.number_of_bytes/sizeof(uint16_t);

    size_t i_a = 0, i_b = 0;
    size_t counter = 0;
    size_t count = 0;
    bool notFinished = i_a < s_a && i_b < s_b;

    uint16_t *last_ptr = 0;
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
        last_ptr = &C[counter];
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

    double density = 0.0;
    if(count > 0){
      uint32_t first = ((uint32_t)C[0] << 16) | C[2];
      size_t last = ((uint32_t)last_ptr[0] << 16) | last_ptr[last_ptr[1]];
      density = (last-first)/count;
    }

    C_in.cardinality = count;
    C_in.number_of_bytes = counter*sizeof(short);
    C_in.density = density;
    C_in.type = common::PSHORT;
    return C_in;
  }
  inline Set<bitset> set_intersect(Set<bitset> C_in, Set<bitset> A_in, Set<bitset> B_in){
    uint8_t *C = C_in.data;
    const uint8_t *A = A_in.data;
    const uint8_t *B = B_in.data;
    const size_t s_a = A_in.number_of_bytes;
    const size_t s_b = B_in.number_of_bytes;

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
      
      __m128i r = _mm_and_si128(a1, a2);
      
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
      uint8_t result = small[i] & large[i];

      #if WRITE_VECTOR == 1
      C[i] = result;
      #endif
      
      count += _mm_popcnt_u32(result);
    }
    double density = (count > 0) ? (sizeof(uint8_t)*(small_length-0))/count : 0.0;

    C_in.cardinality = count;
    C_in.number_of_bytes = small_length;
    C_in.density = density;
    C_in.type = common::BITSET;
    return C_in;
  }
  inline Set<pshort> set_intersect(Set<pshort> C_in, Set<pshort> A_in, Set<bitset> B_in){
    uint16_t *C = (uint16_t*)C_in.data;
    const unsigned short *A = (uint16_t*)A_in.data;
    const uint8_t *B = B_in.data;
    const size_t s_a = A_in.number_of_bytes/sizeof(uint16_t);
    const size_t s_b = B_in.number_of_bytes;

    #if WRITE_VECTOR == 0
    (void) C;   
    #endif 

    uint16_t *last_ptr = &C[0];
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
        last_ptr = &C[old_counter];
        C[old_counter] = (prefix >> 16);
        C[old_counter+1] = old_count-count;
      }
    }

    double density = 0.0;
    if(count > 0){
      uint32_t first = ((uint32_t)C[0] << 16) | C[2];
      size_t last = ((uint32_t)last_ptr[0] << 16) | last_ptr[last_ptr[1]];
      density = (last-first)/count;
    }

    C_in.cardinality = count;
    C_in.number_of_bytes = counter*sizeof(uint16_t);
    C_in.density = density;
    C_in.type = common::PSHORT;
    return C_in;
  }
  inline Set<pshort> set_intersect(Set<pshort> C_in, Set<bitset> A_in, Set<pshort> B_in){
    return set_intersect(C_in,B_in,A_in);
  }
  inline Set<uinteger> set_intersect(Set<uinteger> C_in, Set<uinteger> A_in, Set<bitset> B_in){
    uint32_t *C = (uint32_t*)C_in.data;
    const uint32_t *A = (uint32_t*)A_in.data;
    const uint8_t *B = B_in.data;
    const size_t s_a = A_in.cardinality;
    const size_t s_b = B_in.number_of_bytes;

    #if WRITE_VECTOR == 0
    (void) C;   
    #endif

    size_t count = 0;
    for(size_t i = 0; i < s_a; i++){
      uint32_t cur = A[i];
      if(bitset_ops::word_index(cur) < s_b && bitset_ops::is_set(cur,B)){
        #if WRITE_VECTOR == 1
        C[count] = cur;
        #endif
        count++;
      }
    }
    double density = ((count > 0) ? (double)((C[count]-C[0])/count) : 0.0);

    C_in.cardinality = count;
    C_in.number_of_bytes = count*sizeof(uint32_t);
    C_in.density = density;
    C_in.type = common::UINTEGER;
    return C_in;
  }
  inline Set<uinteger> set_intersect(Set<uinteger> C_in, Set<bitset> A_in, Set<uinteger> B_in){
    return set_intersect(C_in,B_in,A_in);
  }
  inline Set<uinteger> set_intersect(Set<uinteger> C_in, Set<uinteger> A_in, Set<pshort> B_in){
    uint32_t *C = (uint32_t*)C_in.data;
    const uint32_t *A = (uint32_t*)A_in.data;
    const uint16_t *B = (uint16_t*)B_in.data;
    const size_t s_a = A_in.cardinality;
    const size_t s_b = B_in.number_of_bytes/sizeof(uint16_t);

    #if WRITE_VECTOR == 0
    (void)C;
    #endif

    size_t a_i = 0;
    size_t b_i = 0;
    size_t count = 0;

    bool not_finished = a_i < s_a && b_i < s_b;
    while(not_finished){
      uint32_t prefix = (B[b_i] << 16);
      uint16_t b_inner_size = B[b_i+1];
      uint32_t cur_match = A[a_i];
      size_t inner_end = b_i+b_inner_size+2;
      //cout << endl;
      //cout << "Bi: " << b_i << " Bsize: " << s_b << " InnerEnd: " << inner_end << endl;

      if(prefix < (cur_match & 0xFFFF0000)){
        //cout << "1" << endl;
        b_i = inner_end;
        not_finished = b_i < s_b;
      } else if(prefix > cur_match){
        //cout << prefix << " " << cur_match << endl;
        //cout << "2" << endl;
        a_i++;
        not_finished = a_i < s_a;
      } else{
        //cout << "3" << endl;
        b_i += 2;
        size_t i_b = 0;

        #if VECTORIZE == 1
        bool a_continue = (a_i+SHORTS_PER_REG) < s_a && (A[a_i+SHORTS_PER_REG-1] & 0xFFFF0000) == prefix;
        size_t st_b = (b_inner_size / SHORTS_PER_REG) * SHORTS_PER_REG;
        while(a_continue && i_b < st_b) {
          __m128i v_a_1_32 = _mm_loadu_si128((__m128i*)&A[a_i]);
          __m128i v_a_2_32 = _mm_loadu_si128((__m128i*)&A[a_i+(SHORTS_PER_REG/2)]);

          __m128i v_a_1 = _mm_shuffle_epi8(v_a_1_32,_mm_set_epi8(uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x0D),uint8_t(0x0C),uint8_t(0x09),uint8_t(0x08),uint8_t(0x05),uint8_t(0x04),uint8_t(0x01),uint8_t(0x0)));
          __m128i v_a_2 = _mm_shuffle_epi8(v_a_2_32,_mm_set_epi8(uint8_t(0x0D),uint8_t(0x0C),uint8_t(0x09),uint8_t(0x08),uint8_t(0x05),uint8_t(0x04),uint8_t(0x01),uint8_t(0x0),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80)));
          
          __m128i v_a = _mm_or_si128(v_a_1,v_a_2);
            
          //uint16_t *t = (uint16_t*) &v_a;
          //cout << "Data: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << " " << t[4] << " " << t[5] << " " << t[6] << " " << t[7] << endl;

          __m128i v_b = _mm_loadu_si128((__m128i*)&B[b_i+i_b]);    

          uint16_t a_max = _mm_extract_epi16(v_a, SHORTS_PER_REG-1);
          uint16_t b_max = _mm_extract_epi16(v_b, SHORTS_PER_REG-1);
          
          __m128i res_v = _mm_cmpestrm(v_b, SHORTS_PER_REG, v_a, SHORTS_PER_REG,
                  _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
          uint32_t r = _mm_extract_epi32(res_v, 0);

          #if WRITE_VECTOR == 1
          uint32_t r_lower = r & 0x0F;
          uint32_t r_upper = (r & 0xF0) >> 4;
          __m128i p = _mm_shuffle_epi8(v_a_1_32,shuffle_mask32[r_lower]);
          _mm_storeu_si128((__m128i*)&C[count], p);
          
          //uint32_t *t = (uint32_t*) &p;
          //cout << "Data: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;

          p = _mm_shuffle_epi8(v_a_2_32,shuffle_mask32[r_upper]);
          _mm_storeu_si128((__m128i*)&C[count+_mm_popcnt_u32(r_lower)], p);
          C[count] = A[a_i];
          #endif

          count += _mm_popcnt_u32(r);
          a_i += (a_max <= b_max) * SHORTS_PER_REG;
          a_continue = (a_i+SHORTS_PER_REG) < s_a && (A[a_i+SHORTS_PER_REG-1] & 0xFFFF0000) == prefix;
          i_b += (a_max >= b_max) * SHORTS_PER_REG;
        }
        #endif

        bool notFinished = a_i < s_a  && i_b < b_inner_size && (A[a_i] & 0xFFFF0000) == prefix;
        while(notFinished){
          while(notFinished && (uint32_t)(prefix | B[i_b+b_i]) < A[a_i]){
            ++i_b;
            notFinished = i_b < b_inner_size;
          }
          if(notFinished && A[a_i] == (uint32_t)(prefix | B[i_b+b_i])){
            #if WRITE_VECTOR == 1
            C[count] = A[a_i];
            #endif
            ++count;
          }
          ++a_i;
          notFinished = notFinished && a_i < s_a && (A[a_i] & 0xFFFF0000) == prefix;
        }
        b_i = inner_end;
        not_finished = a_i < s_a && b_i < s_b;
      }
    }
    double density = ((count > 0) ? (double)((C[count]-C[0])/count) : 0.0);

    C_in.cardinality = count;
    C_in.number_of_bytes = count*sizeof(uint32_t);
    C_in.density = density;
    C_in.type = common::UINTEGER;

    return C_in;
  }
  inline Set<uinteger> set_intersect(Set<uinteger> C_in, Set<pshort> A_in, Set<uinteger> B_in){
    return set_intersect(C_in,B_in,A_in);
  }
  inline Set<hybrid> set_intersect(Set<hybrid> C_in, Set<hybrid> A_in, Set<hybrid> B_in){
    if(A_in.type == common::UINTEGER){
      if(B_in.type == common::UINTEGER){
        return (Set<hybrid>)set_intersect((Set<uinteger>)C_in,(Set<uinteger>)A_in,(Set<uinteger>)B_in);
      } else if(B_in.type == common::PSHORT){
        return (Set<hybrid>)set_intersect((Set<uinteger>)C_in,(Set<uinteger>)A_in,(Set<pshort>)B_in);
      } else if(B_in.type == common::BITSET){
        return (Set<hybrid>)set_intersect((Set<uinteger>)C_in,(Set<uinteger>)A_in,(Set<bitset>)B_in);
      }
    } else if(A_in.type == common::PSHORT){
      if(B_in.type == common::PSHORT){
        return (Set<hybrid>)set_intersect((Set<pshort>)C_in,(Set<pshort>)A_in,(Set<pshort>)B_in);
      } else if(B_in.type == common::UINTEGER){
        return (Set<hybrid>)set_intersect((Set<uinteger>)C_in,(Set<uinteger>)B_in,(Set<pshort>)A_in);
      } else if(B_in.type == common::BITSET){
        return (Set<hybrid>)set_intersect((Set<pshort>)C_in,(Set<pshort>)A_in,(Set<bitset>)B_in);
      } 
    } else if(A_in.type == common::BITSET){
      if(B_in.type == common::BITSET){
        return (Set<hybrid>)set_intersect((Set<bitset>)C_in,(Set<bitset>)A_in,(Set<bitset>)B_in);
      } else if(B_in.type == common::UINTEGER){
        return (Set<hybrid>)set_intersect((Set<uinteger>)C_in,(Set<uinteger>)B_in,(Set<bitset>)A_in);
      } else if(B_in.type == common::PSHORT){
        return (Set<hybrid>)set_intersect((Set<pshort>)C_in,(Set<pshort>)B_in,(Set<bitset>)A_in);
      }
    }

    cout << "ERROR" << endl;
    return NULL;
  }
}

#endif
