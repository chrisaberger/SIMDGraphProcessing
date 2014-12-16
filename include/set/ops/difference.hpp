#ifndef _DIFFERENCE_H_
#define _DIFFERENCE_H_

namespace ops{
  inline Set<bitset> set_difference(const Set<bitset> &C_in, const Set<bitset> &A_in, const Set<bitset> &B_in){
    uint8_t * const C = C_in.data;
    const uint8_t * const A = A_in.data;
    const uint8_t * const B = B_in.data;
    const size_t s_a = A_in.number_of_bytes;
    const size_t s_b = B_in.number_of_bytes;

    #if WRITE_VECTOR == 0
    (void) C;
    #endif

    long count = 0l;

    const size_t small_length = (s_a > s_b) ? s_b : s_a;
    //16 uint16_ts
    //8 ints
    //4 longs
    size_t i = 0;
    #if VECTORIZE == 1
    while((i+31) < small_length){
      __m256 a1 = _mm256_loadu_ps((const float*)&A[i]);
      __m256 a2 = _mm256_loadu_ps((const float*)&B[i]);
      
      __m256 r = _mm256_andnot_ps(a2, a1);
      _mm256_storeu_ps((float*)&C[i], r);
      
      uint64_t* count_ptr = (uint64_t*)&C[i];
      count += _mm_popcnt_u64(count_ptr[0]);
      count += _mm_popcnt_u64(count_ptr[1]);
      count += _mm_popcnt_u64(count_ptr[2]);
      count += _mm_popcnt_u64(count_ptr[3]);

      i += 32;
    }
    #endif

    for(; i < small_length; i++){
      uint8_t result = A[i] & ~(B[i]);

      C[i] = result;
      count += _mm_popcnt_u32(result);
    }

    #if WRITE_VECTOR == 1
    for(; i < s_a; i++){
      C[i] = A[i];
      count += _mm_popcnt_u32(C[i]);
    }
    #endif 
    
    const double density = (count > 0) ? (double)count/(8*small_length) : 0.0;
    return Set<bitset>(C_in.data,count,small_length,density,common::BITSET);
  }
  inline Set<uinteger> set_difference(const Set<uinteger> &C_in, const Set<bitset> &A_in, const Set<bitset> &B_in){
    uint32_t * const C = (uint32_t*) C_in.data;
    const uint8_t * const A = A_in.data;
    const uint8_t * const B = B_in.data;
    const size_t s_a = A_in.number_of_bytes;
    const size_t s_b = B_in.number_of_bytes;

    #if WRITE_VECTOR == 0
    (void) C;
    #endif

    long count = 0l;
    const size_t small_length = (s_a > s_b) ? s_b : s_a;

    //16 uint16_ts
    //8 ints
    //4 longs
    size_t i = 0;
    #if VECTORIZE == 1
    uint8_t *tmp_buffer = new uint8_t[32];
    while((i+31) < small_length){
      __m256 a1 = _mm256_loadu_ps((const float*)&A[i]);
      __m256 a2 = _mm256_loadu_ps((const float*)&B[i]);
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

    for(; i < small_length; i++){
      uint8_t result = A[i] & ~(B[i]);
      for(size_t j = 0; j < BITS_PER_WORD; j++){
        if((result >> j) % 2){
          C[count++] = (BITS_PER_WORD*i + j);
        }
      }
      count += _mm_popcnt_u32(result);
    }

    #if WRITE_VECTOR == 1
    for(; i < s_a; i++){
      C[i] = A[i];
      for(size_t j = 0; j < BITS_PER_WORD; j++){
        if((C[i] >> j) % 2){
          C[count++] = (BITS_PER_WORD*i + j);
        }
      }
    }
    #endif 

    const double density = ((count > 0) ? ((double)count/(C[count]-C[0])) : 0.0);
    return Set<uinteger>(C_in.data,count,small_length,density,common::UINTEGER);
  }
}

#endif
