#include "Common.hpp"
#include <x86intrin.h>
#include <math.h>

using namespace std;

#define DELTA 0

namespace deltacompa32 {
  inline size_t encode_array(unsigned short *result_in, size_t index, unsigned int *data_in, size_t length){
    if(length > 0){
      size_t bit_i = 0;
      size_t data_i = 0;
      size_t num_packed = 0;
      unsigned int *result = (unsigned int*)(result_in+index);

      #if DELTA == 1
        unsigned int *simd_data = data_in;
        size_t max = 0;
        size_t num_simd_packed = 0; 
        size_t result_i = 2;

        //place first element->serves as intial delta for subtraction.
        result[1] = data_in[0];
        num_simd_packed = (unsigned int)((length-1)/INTS_PER_REG) * INTS_PER_REG * (length >= 8);
        
        if(num_simd_packed > 0){
          __m128i prev_data_register = _mm_set1_epi32(data_in[0]); //set delta
          simd_data = new unsigned int[num_simd_packed+1];
          for(size_t i = 1; i < num_simd_packed; i+=INTS_PER_REG){
            __m128i data_register = _mm_loadu_si128((__m128i*)&data_in[i]);
            __m128i dt = _mm_sub_epi32(data_register,prev_data_register);
            //Find max difference (will tell the # of bits to represent value needed)
            uint32_t *ot = (uint32_t*) &dt;
            for(size_t j =0; j<4; j++){
              if(ot[j] > max){
                max = ot[j];
              }
            }
            //Store the deltas
            _mm_storeu_si128((__m128i*)&simd_data[i],dt);
            prev_data_register = data_register;
          }
        }

        const unsigned int bits_used = (unsigned int)log2(max)+1;
        result[0] = bits_used;
      #else
        size_t result_i = 1;
        unsigned int *simd_data = data_in;
        const unsigned int bits_used = (unsigned int)log2(data_in[length-1])+1;
        size_t num_simd_packed = (unsigned int)(length /INTS_PER_REG) * INTS_PER_REG;

        result[0] = bits_used;
      #endif 

      #if VECTORIZE == 1
      __m128i data_register;
      __m128i packed_register = _mm_set1_epi32(0);
      while(num_packed < num_simd_packed){
        while(bit_i+bits_used <= 32 && data_i < num_simd_packed){
          data_register = _mm_loadu_si128((__m128i*)&simd_data[data_i]);

          __m128i cur = _mm_slli_epi32(data_register,bit_i);
          packed_register = _mm_or_si128(cur,packed_register);

          data_i += INTS_PER_REG;
          bit_i += bits_used;
        }
        if(bit_i < 32 && data_i < num_simd_packed){
          data_register = _mm_loadu_si128((__m128i*)&simd_data[data_i]);

          __m128i cur = _mm_slli_epi32(data_register,bit_i);
          packed_register = _mm_or_si128(cur,packed_register);

          //uint32_t *t = (uint32_t*) &result;
          //cout << "STORING Values[" << result_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;
       
          _mm_storeu_si128((__m128i*)&result[result_i],packed_register);
          num_packed = data_i;

          packed_register = _mm_set1_epi32(0);
          packed_register =_mm_srli_epi32(data_register,(32-bit_i));

          data_i += INTS_PER_REG;
          bit_i = bits_used-(32-bit_i);
        } else{
          //uint32_t *t = (uint32_t*) &result;
          //cout << "STORING Values[" << result_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;
       
          _mm_storeu_si128((__m128i*)&result[result_i], packed_register);
          num_packed = data_i;
          packed_register = _mm_set1_epi32(0);
          bit_i = 0;
        }
        result_i += INTS_PER_REG;
      }
      #endif

      #if DELTA == 1
      if(num_simd_packed >= 0){
        delete[] simd_data;
      }
      data_i++;  //compesate for difference as delta placed first element for delta.
      #endif
      
      //Implement Variant
      while(data_i < length){
        result[result_i++] = data_in[data_i++];
      }
      return result_i*2 + index;
    } else{
      return index;
    }
  }
  inline void decode_array(unsigned int *data, const size_t length, const size_t cardinality){
    size_t bit_i = 0;
    size_t num_simd_packed = (cardinality /INTS_PER_REG) * INTS_PER_REG * (cardinality >= 8);
    unsigned int bits_used = data[0];
    size_t data_incrementer = 0;

    #if DELTA == 1
      __m128i prev_result = _mm_set1_epi32(data[1]);
      size_t num_decoded = 1;
      
      //Apply function
      cout << " Data: " << data[1] << endl;
      //

      size_t data_i = 2;
    #else
      size_t num_decoded = 0;
      size_t data_i = 1; 
    #endif


    #if VECTORIZE == 1

    __m128i mask = _mm_set1_epi32((1 << bits_used)-1);
    __m128i data_register;
    while(num_decoded < num_simd_packed){
      //cout << "Bit Index: " << bit_i << endl;
      while(bit_i+bits_used <= 32 && num_decoded < num_simd_packed){
        data_register = _mm_loadu_si128((__m128i*)&data[data_i]);
        data_incrementer = 0;

        __m128i result = _mm_srli_epi32(data_register,bit_i);
        result = _mm_and_si128(result,mask);

        #if DELTA == 1
        result = _mm_add_epi32(result,prev_result);
        prev_result = result;
        #endif

        //Apply Function (intersection)
        uint32_t *t = (uint32_t*) &result;
        cout << " Data: " << t[0] << endl;
        cout << " Data: " << t[1] << endl;
        cout << " Data: " << t[2] << endl;
        cout << " Data: " << t[3] << endl;
        //

        num_decoded += INTS_PER_REG;
        bit_i += bits_used;
      }
      //cout << "Bit Index: " << bit_i << endl << endl;;
      if(bit_i < 32 && num_decoded < num_simd_packed){
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
        cout << " Data: " << t[0] << endl;
        cout << " Data: " << t[1] << endl;
        cout << " Data: " << t[2] << endl;
        cout << " Data: " << t[3] << endl;
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
    #endif

    data_i += data_incrementer;
    while(num_decoded < cardinality){
      cout << " Data: " << data[data_i] << endl;
      data_i++; num_decoded++;
    }
  }
} 