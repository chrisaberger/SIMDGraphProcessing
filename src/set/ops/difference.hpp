#ifndef _DIFFERENCE_H_
#define _DIFFERENCE_H_

namespace ops{
  inline Set<uinteger>* set_difference(Set<uinteger> *C_in, const Set<bitset> *A_in, const Set<bitset> *B_in){
    long count = 0l;
    if(A_in->number_of_bytes > 0 && B_in->number_of_bytes > 0){
      const uint64_t *a_index = (uint64_t*) A_in->data;
      const uint64_t *b_index = (uint64_t*) B_in->data;

      uint32_t * const C = (uint32_t*)C_in->data;
      const uint64_t * const A = (uint64_t*)(A_in->data+sizeof(uint64_t));
      const uint64_t * const B = (uint64_t*)(B_in->data+sizeof(uint64_t));
      const size_t s_a = ((A_in->number_of_bytes-sizeof(uint64_t))/sizeof(uint64_t));
      const size_t s_b = ((B_in->number_of_bytes-sizeof(uint64_t))/sizeof(uint64_t));

      const bool a_big = a_index[0] > b_index[0];
      const uint64_t start_index = (a_big) ? a_index[0] : b_index[0];

      const uint64_t a_start_index = (a_big) ? 0:(b_index[0]-a_index[0]);
      const uint64_t b_start_index = (a_big) ? (a_index[0]-b_index[0]):0;

      const uint64_t end_index = ((a_index[0]+s_a) > (b_index[0]+s_b)) ? (b_index[0]+s_b):(a_index[0]+s_a);
      const uint64_t total_size = (start_index > end_index) ? 0:(end_index-start_index);

      //16 uint16_ts
      //8 ints
      //4 longs
      size_t i = 0;
      #if VECTORIZE == 1
      uint64_t tmp[4];
      for(; (i+3) < total_size; i += 4){
        const __m256 a1 = _mm256_loadu_ps((const float*)&A[i+a_start_index]);
        const __m256 a2 = _mm256_loadu_ps((const float*)&B[i+b_start_index]);
        const __m256 r = _mm256_andnot_ps(a2, a1);

        _mm256_storeu_ps((float*)&tmp, r);

        for(size_t offset = 0; offset < 4; offset++){
          if(tmp[offset] != 0){
            for(size_t j = 0; j < BITS_PER_WORD; j++){
              if((tmp[offset] >> j) % 2){
                C[count] = (BITS_PER_WORD*(start_index+i+offset) + j);
                count++;
              }
            }
          }
        }
      }
      #endif

      for(; i < total_size; i++){
        const uint64_t result = A[i+a_start_index] & ~(B[i+b_start_index]);
        if(result != 0) {
          for(size_t j = 0; j < BITS_PER_WORD; j++){
            if((result >> j) % 2){
              C[count] = (BITS_PER_WORD*(start_index+i) + j);
              count++;
            }
          }
        }
      }

      for(; i < (s_a+a_index[0]); i++){
        const uint64_t result = A[i+a_start_index];
        if(result != 0){
          for(size_t j = 0; j < BITS_PER_WORD; j++){
            if((result >> j) % 2){
              C[count] = (BITS_PER_WORD*(start_index+i) + j);
              count++;
            }
          }
        }
      }

      const double density = 0.0;//(count > 0) ? (double)count/(8*small_length) : 0.0;

      C_in->cardinality = count;
      C_in->number_of_bytes = count*sizeof(uint32_t);
      C_in->density = density;
      C_in->type = common::UINTEGER;
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
