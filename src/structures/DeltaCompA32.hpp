#include "Common.hpp"
#include "Array32.hpp"
#include <x86intrin.h>
#include <math.h>

using namespace std;

namespace deltacompa32 {
  inline size_t encode_array(unsigned int *data, unsigned int length, unsigned int *result){
    const unsigned int bits_used = ceil(log2(data[length-1]));
    cout << "Bits used: " << bits_used << endl;

    result[0] = bits_used;
    result = &result[1];

    size_t bit_i = 0;
    size_t data_i = 0;
    size_t result_i = 0;
    
    size_t num_simd_packed = (length /INTS_PER_REG) * INTS_PER_REG;
    size_t num_packed = 0;

    __m128i data_register;
    __m128i prev_data_register = _mm_set1_epi32(0);
    __m128i packed_register = _mm_set1_epi32(0);
    while(num_packed < num_simd_packed){
      while(bit_i+bits_used <= 32 && data_i < num_simd_packed){
        data_register = _mm_loadu_si128((__m128i*)&data[data_i]);
        __m128i cur = _mm_slli_epi32(data_register,bit_i);
        packed_register = _mm_or_si128(cur,packed_register);

        data_i += INTS_PER_REG;
        bit_i += bits_used;
        prev_data_register = data_register;
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
        prev_data_register = data_register;
      } else{
        _mm_storeu_si128((__m128i*)&result[result_i], packed_register);
        num_packed = data_i;
        packed_register = _mm_set1_epi32(0);
        bit_i = 0;
      }
      result_i += INTS_PER_REG;
    }

    cout << "DATA_I: " << data_i << " Length: " << length << " RESULT_I: " << result_i << endl;
    while(data_i < length){
      result[result_i++] = data[data_i++]; 
    }
    prev_data_register = prev_data_register;
    return result_i;
  }
  inline void decode_array(unsigned int *data, const size_t length, const size_t cardinality){
    unsigned int bits_used = data[0];
    data = &data[1];

    __m128i mask = _mm_set1_epi32((1 << bits_used)-1);
    
    size_t bit_i = 0;
    size_t num_decoded = 0;
    size_t data_i = 0;
    size_t num_packed = (cardinality /INTS_PER_REG) * INTS_PER_REG;

    __m128i data_register;
    __m128i prev_result = _mm_set1_epi32(0);

    while(num_decoded < num_packed){
      cout << "Bit Index: " << bit_i << endl;
      while(bit_i+bits_used <= 32 && num_decoded < num_packed){
        data_register = _mm_loadu_si128((__m128i*)&data[data_i]);
        
        uint32_t *t = (uint32_t*) &data_register;
        cout << "LOADED Values[" << data_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;

        __m128i cur = _mm_srli_epi32(data_register,bit_i);
        __m128i result = _mm_and_si128(cur,mask);

        uint32_t *tt = (uint32_t*) &result;
        cout << "1Decoded Values[" << num_decoded << "]: " << tt[0] << " " << tt[1] << " " << tt[2] << " " << tt[3] << endl;

        num_decoded += INTS_PER_REG;
        bit_i += bits_used;
        prev_result = result;
      }
      cout << "Bit Index: " << bit_i << endl;
      if(bit_i < 32 && num_decoded < num_packed){
        __m128i cur_lower = _mm_srli_epi32(data_register,bit_i);
        data_register = _mm_loadu_si128((__m128i*)&data[data_i+INTS_PER_REG]);

        uint32_t *tt = (uint32_t*) &data_register;
        cout << "LOADED Values[" << data_i+INTS_PER_REG << "]: " << tt[0] << " " << tt[1] << " " << tt[2] << " " << tt[3] << endl;

        __m128i cur_upper = _mm_slli_epi32(data_register,(32-bit_i));

        __m128i result = _mm_or_si128(cur_upper,cur_lower);
        
        uint32_t *t = (uint32_t*) &result;
        cout << "2Decoded Values[" << num_decoded << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;

        num_decoded += INTS_PER_REG;
        bit_i = bits_used-(32-bit_i);
        prev_result = result;
      } else{
        bit_i = 0;
      }
      data_i += INTS_PER_REG;
    }
    cout << "Data_I: " << data_i << " Length: " << length << endl;
    while(num_decoded < cardinality){
      cout << "Decoded Value: " << data[data_i] << endl;
      data_i++; num_decoded++;
    }
  }
} 