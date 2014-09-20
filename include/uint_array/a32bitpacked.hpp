#include "common.hpp"

namespace a32bitpacked {
  inline size_t variant_encode(unsigned int *data, size_t length, uint8_t *result, size_t result_i){
    size_t data_i = 0;
    while(data_i < length){
      unsigned int cur = data[data_i++];
      unsigned int bytes_needed = (((unsigned int)log2(cur)+1)/7)+1; //1 bit is reserved for continue
      size_t bytes_set = 0;
      while(bytes_set < bytes_needed){
        uint8_t continue_bit = 0x00;
        if(bytes_set+1 < bytes_needed){
          continue_bit = 0x01;
        }
        result[result_i++] = ((cur & 0xFFFFFF80) << 1) | continue_bit;
        bytes_set++; 
      }
    }
    return result_i;
  }
  inline unsigned int produce_deltas(unsigned int *data_in, size_t length, unsigned int *data, size_t num_simd_packed){
    size_t max = 0;
    
    #if VECTORIZE == 1
    if(num_simd_packed > 0){
      //place first 4 elements->serves as intial delta for subtraction.
      __m128i prev_data_register = _mm_set1_epi32(data_in[0]); //set delta

      for(size_t i = INTS_PER_REG; i < num_simd_packed; i+=INTS_PER_REG){
        __m128i data_register = _mm_loadu_si128((__m128i*)&data_in[i]);
        __m128i dt = _mm_sub_epi32(data_register,prev_data_register);
        //Find max difference (will tell the # of bits to represent value needed)
        uint32_t *ot = (uint32_t*) &dt;
        for(size_t j = 0; j < INTS_PER_REG; j++){
          if(ot[j] > max){
            max = ot[j];
          }
        }
        //Store the deltas
        _mm_storeu_si128((__m128i*)&data[i],dt);
        prev_data_register = data_register;
      }
    }
    #endif

    unsigned int prev = 0;
    if(num_simd_packed != 0){
      prev = data_in[num_simd_packed-1];
    }

    for(size_t i = num_simd_packed; i < length; i++){
      unsigned int cur = data_in[i] - prev;
      data[i] = cur;
      if(cur > max){
        max = cur;
      }
    }

    return max;
  }
  inline size_t simd_bit_pack(const uint8_t bits_used, unsigned int *data, size_t num_simd_packed, uint8_t *result_in, size_t result_i){
    size_t num_packed = 0;
    size_t bit_i = 0;
    size_t data_i = 1;

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

        //uint32_t *t = (uint32_t*) &result;
        //cout << "STORING Values[" << result_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;
     
        _mm_storeu_si128((__m128i*)&result_in[result_i],packed_register);
        num_packed = data_i;

        packed_register = _mm_set1_epi32(0);
        packed_register =_mm_srli_epi32(data_register,(32-bit_i));

        data_i += INTS_PER_REG;
        bit_i = bits_used-(32-bit_i);
      } else{
        //uint32_t *t = (uint32_t*) &result;
        //cout << "STORING Values[" << result_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;
     
        _mm_storeu_si128((__m128i*)&result_in[result_i], packed_register);
        num_packed = data_i;
        packed_register = _mm_set1_epi32(0);
        bit_i = 0;
      }
      result_i += INTS_PER_REG*4;
    }
    return result_i;
  }
  inline size_t preprocess(uint8_t *result_in, unsigned int *data_in, size_t length){
    if(length > 0){
      size_t data_i = 0;
      size_t result_i = 0;
      size_t num_simd_packed = (length/INTS_PER_REG)*INTS_PER_REG*(length > INTS_PER_REG*2);

      #if DELTA == 1
        data_i++;
        result_i = variant_encode(data_in,1,result_in,result_i);
        unsigned int *data = new unsigned int[length];
        unsigned int max = produce_deltas(data_in,length,data,num_simd_packed);
        const uint8_t bits_used = (unsigned int)log2(max)+1;
        result_in[0] = bits_used;
      #else
        unsigned int *data = data_in;
        const uint8_t bits_used = (unsigned int)log2(data_in[length-1])+1;
        result_in[0] = bits_used;
      #endif 

      #if VECTORIZE == 1
      result_i = simd_bit_pack(bits_used,&data[data_i],num_simd_packed,result_in,result_i);
      data_i = num_simd_packed;
      #endif
      
      variant_encode(&data[data_i],length-data_i,result_in,result_i);

      #if DELTA == 1
      delete[] data;
      #endif

      return result_i;
    } else{
      return 0;
    }
  }
  inline void print_data(uint8_t *data, const size_t length, const size_t cardinality){
    /*
    size_t bit_i = 0;
    size_t num_simd_packed = (unsigned int)(length /INTS_PER_REG)*INTS_PER_REG*(length > INTS_PER_REG*2);
    uint8_t bits_used = data[0];
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
    */
  }
} 