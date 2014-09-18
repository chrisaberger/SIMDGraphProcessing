#include "Common.hpp"
#include <x86intrin.h>
#include <math.h>

using namespace std;

#define DELTA 1

namespace deltacompa32 {
  inline size_t encode_array(size_t index, unsigned int *data_in, unsigned int length, unsigned int *result){
    size_t bit_i = 0;
    size_t data_i = 0;
    size_t num_packed = 0;

    #if DELTA == 1
    unsigned int *data = new unsigned int[length-1];
    __m128i prev_data_register;
    size_t max = 0;
    size_t num_simd_packed = 0; 
    
    if(length > 0){
      num_simd_packed = ((length-1)/INTS_PER_REG) * INTS_PER_REG;
      result[1] = data_in[0];
      prev_data_register = _mm_set1_epi32(data_in[0]);
      for(size_t i = 1; i < num_simd_packed; i+=4){
        __m128i data_register = _mm_loadu_si128((__m128i*)&data_in[i]);
        __m128i dt = _mm_sub_epi32(data_register,prev_data_register);
        uint32_t *ot = (uint32_t*) &dt;
        for(size_t j =0; j<4; j++){
          if(ot[j] > max){
            max = ot[j];
          }
        }
        _mm_storeu_si128((__m128i*)&data[i-1],dt);
        prev_data_register = data_register;
      }
    }
    const unsigned int bits_used = (unsigned int)log2(max)+1;

    result[0] = bits_used;
    size_t result_i = 2+index;
    #else
    unsigned int *data = data_in;
    const unsigned int bits_used = (unsigned int)log2(data[length-1])+1;
    size_t num_simd_packed = (length /INTS_PER_REG) * INTS_PER_REG;

    result[0] = bits_used;
    size_t result_i = 1+index;
    #endif 

    cout << "Bits used: " << bits_used << endl;

    __m128i data_register;
    __m128i packed_register = _mm_set1_epi32(0);
    while(num_packed < num_simd_packed){
      while(bit_i+bits_used <= 32 && data_i < num_simd_packed){
        data_register = _mm_loadu_si128((__m128i*)&data[data_i]);

        __m128i cur = _mm_slli_epi32(data_register,bit_i);
        packed_register = _mm_or_si128(cur,packed_register);

        data_i += INTS_PER_REG;
        bit_i += bits_used;
      }
      if(bit_i < 32 && data_i < num_simd_packed){
        data_register = _mm_loadu_si128((__m128i*)&data[data_i]);

        __m128i cur = _mm_slli_epi32(data_register,bit_i);
        packed_register = _mm_or_si128(cur,packed_register);

        _mm_storeu_si128((__m128i*)&result[result_i],packed_register);
        num_packed = data_i;

        packed_register = _mm_set1_epi32(0);
        packed_register =_mm_srli_epi32(data_register,(32-bit_i));

        data_i += INTS_PER_REG;
        bit_i = bits_used-(32-bit_i);
      } else{
        _mm_storeu_si128((__m128i*)&result[result_i], packed_register);
        num_packed = data_i;
        packed_register = _mm_set1_epi32(0);
        bit_i = 0;
      }
      result_i += INTS_PER_REG;
    }

    data_i++;
    while(data_i < length){
      cout << "Writing: " << data_i << " " << data_in[data_i] << endl;
      result[result_i++] = data_in[data_i++];
    }
    return result_i;
  }
  inline void decode_array(unsigned int *data, const size_t length, const size_t cardinality){
    size_t bit_i = 0;
    size_t num_packed = (cardinality /INTS_PER_REG) * INTS_PER_REG;
    unsigned int bits_used = data[0];
    #if DELTA == 1
    __m128i prev_result = _mm_set1_epi32(data[1]);
    size_t num_decoded = 1;

    cout << "Decoded Value: " << data[1] << endl;
    size_t data_i = 2;
    #else
    size_t num_decoded = 0;
    size_t data_i = 1; 
    #endif
    __m128i mask = _mm_set1_epi32((1 << bits_used)-1);

    __m128i data_register;
    size_t data_incrementer = 0;
    while(num_decoded < num_packed){
      cout << "Bit Index: " << bit_i << endl;
      while(bit_i+bits_used <= 32 && num_decoded < num_packed){
        data_register = _mm_loadu_si128((__m128i*)&data[data_i]);
        data_incrementer = 0;

        __m128i cur = _mm_srli_epi32(data_register,bit_i);
        __m128i result = _mm_and_si128(cur,mask);

        #if DELTA == 1
        result = _mm_add_epi32(result,prev_result);
        prev_result = result;
        #endif

        //Apply Function (intersection)
        uint32_t *t = (uint32_t*) &result;
        cout << "WRITTEN Values[" << num_decoded << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;
        //

        num_decoded += INTS_PER_REG;
        bit_i += bits_used;
      }
      cout << "Bit Index: " << bit_i << endl << endl;;
      if(bit_i < 32 && num_decoded < num_packed){
        __m128i cur_lower = _mm_srli_epi32(data_register,bit_i);

        data_register = _mm_loadu_si128((__m128i*)&data[data_i+INTS_PER_REG]);
        data_incrementer = 4;

        __m128i cur_upper = _mm_slli_epi32(data_register,(32-bit_i));
        __m128i result = _mm_and_si128(_mm_or_si128(cur_upper,cur_lower),mask);
      
        #if DELTA == 1
        result = _mm_add_epi32(result,prev_result);
        #endif

        //Apply Function (intersection)
        uint32_t *t = (uint32_t*) &result;
        cout << "WRITTEN Values[" << num_decoded << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;
        //

        num_decoded += INTS_PER_REG;
        bit_i = bits_used-(32-bit_i);
        
        #if DELTA == 1
        prev_result = result;
        #endif
     
      } else{
        bit_i = 0;
      }
      data_i += INTS_PER_REG;
    }

    data_i += data_incrementer;
    while(num_decoded < cardinality){
      cout << "Decoded Value: " << data[data_i] << endl;
      data_i++; num_decoded++;
    }
  }
} 