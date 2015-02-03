#ifndef _DIFFERENCE_H_
#define _DIFFERENCE_H_

namespace ops{
  inline Set<bitset>* set_difference(Set<bitset> *C_in, const Set<bitset> *A_in, const Set<bitset> *B_in){
    long count = 0l;
    C_in->number_of_bytes = 0;
    if(A_in->number_of_bytes > 0 && B_in->number_of_bytes > 0){
      const uint32_t *a_index = (uint32_t*) A_in->data;
      const uint32_t *b_index = (uint32_t*) B_in->data;
      uint32_t *c_index = (uint32_t*) B_in->data;

      uint64_t * const C = (uint64_t*)(C_in->data+sizeof(uint32_t));
      const uint64_t * const A = (uint64_t*)(A_in->data+sizeof(uint32_t));
      const uint64_t * const B = (uint64_t*)(B_in->data+sizeof(uint32_t));
      const size_t s_a = ((A_in->number_of_bytes-sizeof(uint32_t))/sizeof(uint64_t));
      const size_t s_b = ((B_in->number_of_bytes-sizeof(uint32_t))/sizeof(uint64_t));


      const bool a_big = a_index[0] > b_index[0];
      
      const uint32_t start_index = (a_big) ? a_index[0] : b_index[0];
      c_index[0] = start_index;

      const uint32_t a_start_index = (a_big) ? 0:(b_index[0]-a_index[0]);
      const uint32_t b_start_index = (a_big) ? (a_index[0]-b_index[0]):0;

      const uint32_t end_index = ((a_index[0]+s_a) > (b_index[0]+s_b)) ? (b_index[0]+s_b):(a_index[0]+s_a);
      const uint32_t total_size = (start_index > end_index) ? 0:(end_index-start_index);

      cout << "TOTAL SIZE: " << total_size << endl;

      //16 uint16_ts
      //8 ints
      //4 longs
      size_t i = 0;
      #if VECTORIZE == 1
      while((i+3) < total_size){
        __m256 a1 = _mm256_loadu_ps((const float*)&A[i+a_start_index]);
        __m256 a2 = _mm256_loadu_ps((const float*)&B[i+b_start_index]);
        
        __m256 r = _mm256_andnot_ps(a2, a1);

        _mm256_store_ps((float*)&C[i-c_index[0]], r);
        const size_t old_count = count;
        count += _mm_popcnt_u64(C[i-c_index[0]]);
        count += _mm_popcnt_u64(C[i-c_index[0]+1]);
        count += _mm_popcnt_u64(C[i-c_index[0]]+2);
        count += _mm_popcnt_u64(C[i-c_index[0]]+3);
        if(old_count == 0 && count != 0)
          c_index[0] = i;

        i += 32;
      }
      #endif

      for(; i < total_size; i++){
        const uint64_t result = A[i+a_start_index] & ~(B[i+b_start_index]);
        const size_t old_count = count;
        count += _mm_popcnt_u64(result);

        if(old_count == 0 && count != 0)
          c_index[0] = i;
        C[i-c_index[0]] = result;
      }

      for(; i < (s_a+a_index[0]); i++){
        const uint64_t result = A[i+a_start_index];
        const size_t old_count = count;
        count += _mm_popcnt_u64(result);
        if(old_count == 0 && count != 0)
          c_index[0] = i;
        C[i-c_index[0]] = result;
      }
      
      const double density = 0.0;//(count > 0) ? (double)count/(8*small_length) : 0.0;

      C_in->cardinality = count;
      C_in->number_of_bytes = (i-c_index[0])*sizeof(uint64_t) + sizeof(uint32_t);
      C_in->density = density;
      C_in->type = common::BITSET;
    }
    return C_in;
  }
  /*
  inline Set<uinteger>* set_difference(Set<uinteger> *C_in, const Set<bitset> *A_in, const Set<bitset> *B_in){
    long count = 0l;
    C_in->number_of_bytes = 0;
    if(A_in->number_of_bytes > 0 && B_in->number_of_bytes > 0){
      const uint32_t *a_index = (uint32_t*) A_in->data;
      const uint32_t *b_index = (uint32_t*) B_in->data;
      
      uint64_t * const C = (uint64_t*)(C_in->data+sizeof(uint32_t));
      const uint64_t * const A = (uint64_t*)(A_in->data+sizeof(uint32_t));
      const uint64_t * const B = (uint64_t*)(B_in->data+sizeof(uint32_t));
      const size_t s_a = ((A_in->number_of_bytes-sizeof(uint32_t))/sizeof(uint64_t));
      const size_t s_b = ((B_in->number_of_bytes-sizeof(uint32_t))/sizeof(uint64_t));

      #if WRITE_VECTOR == 0
      (void) C;
      #endif

      const bool a_big = a_index[0] > b_index[0];
      const uint32_t start_index = (a_big) ? a_index[0] : b_index[0];
      const uint32_t a_start_index = (a_big) ? 0:(b_index[0]-a_index[0]);
      const uint32_t b_start_index = (a_big) ? (a_index[0]-b_index[0]):0;

      const uint32_t end_index = ((a_index[0]+s_a) > (b_index[0]+s_b)) ? (b_index[0]+s_b):(a_index[0]+s_a);
      const uint32_t total_size = (start_index > end_index) ? 0:(end_index-start_index);

      //16 uint16_ts
      //8 ints
      //4 longs
      size_t i = 0;
      #if VECTORIZE == 1
      uint8_t *tmp_buffer = new uint8_t[32];
      while((i+3) < total_size){
        __m256 a1 = _mm256_loadu_ps((const float*)&A[i+a_start_index]);
        __m256 a2 = _mm256_loadu_ps((const float*)&B[i+b_start_index]);
        __m256 r = _mm256_andnot_ps(a2, a1);

        _mm256_storeu_ps((float*)&tmp_buffer[0], r);
        for(size_t ii = 0; ii < 32; ii++){
          uint8_t cur_word = tmp_buffer[ii];
          for(size_t j = 0; j < BITS_PER_WORD; j++){
            if((cur_word >> j) % 2){
              C[count++] = (BITS_PER_WORD*(ii+i) + j);
            }
          }
        }
        i += 32;
      }
      delete[] tmp_buffer;
      #endif

      for(; i < total_size; i++){
        uint8_t result = A[i+a_start_index] & ~(B[i+b_start_index]);
        for(size_t j = 0; j < BITS_PER_WORD; j++){
          if((result >> j) % 2){
            C[count++] = (BITS_PER_WORD*i + j);
          }
        }
        count += _mm_popcnt_u32(result);
      }

      #if WRITE_VECTOR == 1
      for(; i < s_a+a_index[0]; i++){
        C[i] = A[i+a_start_index];
        for(size_t j = 0; j < BITS_PER_WORD; j++){
          if((C[i] >> j) % 2){
            C[count++] = (BITS_PER_WORD*i + j);
          }
        }
      }
      #endif 

      // XXX: The density is broken
      const double density = 0.0;//((count > 1) ? ((double)count/(C[count - 1]-C[0])) : 0.0);

      C_in->cardinality = count;
      C_in->number_of_bytes = total_size;
      C_in->density = density;
      C_in->type = common::UINTEGER;
    }
    return C_in;
  }
  */
}

#endif
