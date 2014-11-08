#include "common.hpp"
#include "variant.hpp"

namespace a32bitpacked {
  inline size_t get_num_simd_packed(size_t cardinality){
    return ((cardinality/INTS_PER_REG)*INTS_PER_REG*(cardinality >= INTS_PER_REG*2));
  }
  inline size_t simd_bit_pack(const uint8_t bits_used, unsigned int *data, size_t data_index, size_t num_simd_packed, uint8_t *result_in, size_t result_i){
    size_t num_packed = 0;
    size_t bit_i = 0;
    size_t data_i = data_index;
    __m128i packed_register = _mm_set1_epi32(0);

    unsigned int *result = (unsigned int*)(result_in+result_i);
    size_t result_integer_i = 0;
    while(num_packed < num_simd_packed){
      while(bit_i+bits_used <= 32 && data_i < num_simd_packed){
        //cout << "1bit_i: " << bit_i << endl;
        __m128i data_register = _mm_loadu_si128((__m128i*)&data[data_i]);
        __m128i cur = _mm_slli_epi32(data_register,bit_i);
        packed_register = _mm_or_si128(cur,packed_register);

        data_i += INTS_PER_REG;
        bit_i += bits_used;
      }
      //cout << "bit_i: " << bit_i << endl;
      if(bit_i < 32 && data_i < num_simd_packed){
        __m128i data_register = _mm_loadu_si128((__m128i*)&data[data_i]);

        __m128i cur = _mm_slli_epi32(data_register,bit_i);
        packed_register = _mm_or_si128(cur,packed_register);

        _mm_storeu_si128((__m128i*)&result[result_integer_i],packed_register);
  
        num_packed = data_i;
        packed_register = _mm_set1_epi32(0);
        packed_register =_mm_srli_epi32(data_register,(32-bit_i));

        data_i += INTS_PER_REG;
        bit_i = bits_used-(32-bit_i);
      } else{
        //uint32_t *t = (uint32_t*) &packed_register;
        //cout << "END STORING Values[" << result_integer_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;
        
        _mm_storeu_si128((__m128i*)&result[result_integer_i], packed_register);

        num_packed = data_i;
        packed_register = _mm_set1_epi32(0);
        bit_i = 0;
      }
      result_integer_i += INTS_PER_REG;
    }
    //cout << result_i << " " << result_integer_i << endl;
    return (result_i+(result_integer_i*4));
  }
  inline unsigned int produce_deltas(unsigned int *data_in, size_t length, unsigned int *data, size_t num_simd_packed){    
    size_t max = 0;
    unsigned int prev = 0;
    if(length > 0){
      prev = data_in[0];
    }

    if(num_simd_packed > 0){
      //place first 4 elements->serves as intial delta for subtraction.
      __m128i prev_data_register = _mm_set1_epi32(prev); //set delta

      for(size_t i = 0; i < num_simd_packed; i+=INTS_PER_REG){
        __m128i data_register = _mm_loadu_si128((__m128i*)&data_in[i]);
        __m128i deltas = _mm_sub_epi32(data_register,prev_data_register);
        _mm_storeu_si128((__m128i*)&data[i],deltas);
        prev_data_register = data_register;

        //uint32_t *t = (uint32_t*) &deltas;
        //cout << "STORING DELTAS Values[" << i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;
     
        //Find max difference (will tell the # of bits to represent value needed)
        uint32_t *ot = (uint32_t*) &deltas;
        for(size_t j = 0; j < INTS_PER_REG; j++){
          if(ot[j] > max){
            max = ot[j];
          }
        }
        //Store the deltas
      }
      prev = data_in[num_simd_packed-1];
    }

    for(size_t i = num_simd_packed; i < length; i++){
      unsigned int cur = data_in[i] - prev;
      //cout  << "Writing delta index: " << i << "  " << cur << endl;
      data[i] = cur;
      prev = data_in[i];
    }
    return max;
  }
    inline size_t preprocess(uint8_t *result_in, unsigned int *data_in, size_t length){
    if(length > 0){
      unsigned int *data = new unsigned int[length];
      const size_t num_simd_packed = get_num_simd_packed(length);
      const unsigned int max = produce_deltas(data_in,length,data,num_simd_packed);
      const uint8_t bits_used = (unsigned int)log2(max)+1;
      result_in[0] = bits_used;

      size_t result_i = variant::variant_encode(data_in,0,1,result_in,1);
      //cout << "First element ends at: " << result_i << endl;
      result_i = simd_bit_pack(bits_used,data,0,num_simd_packed,result_in,result_i);

      //cout << "Encoding at: " << result_i << endl;
      result_i = variant::variant_encode(data,num_simd_packed,length,result_in,result_i);

      return result_i;
    } else{
      return 0;
    }
  }
  inline __m128i get_next_see(uint8_t *data,uint8_t bits_used,size_t &data_i,__m128i prev_result, __m128i mask, size_t &bit_i, bool last){
    //data_i
    //bit_i
    //bits_used
    if(bit_i+bits_used <= 32){
      __m128i data_register = _mm_loadu_si128((__m128i*)&data[data_i]);
      __m128i result = _mm_srli_epi32(data_register,bit_i);
      result = _mm_and_si128(result,mask);
      result = _mm_add_epi32(result,prev_result);      

      bit_i += bits_used;

      return result;
    } else if(bit_i < 32){
      __m128i data_register = _mm_loadu_si128((__m128i*)&data[data_i]);
      __m128i cur_lower = _mm_srli_epi32(data_register,bit_i);
      data_register = _mm_loadu_si128((__m128i*)&data[data_i+INTS_PER_REG*4]);

      __m128i cur_upper = _mm_slli_epi32(data_register,(32-bit_i));
      __m128i result = _mm_and_si128(_mm_or_si128(cur_upper,cur_lower),mask);
      result = _mm_add_epi32(result,prev_result);

      bit_i = bits_used-(32-bit_i);      
      data_i += BYTES_PER_REG + last*BYTES_PER_REG;

      return result;
    } else{
      __m128i data_register = _mm_loadu_si128((__m128i*)&data[data_i+INTS_PER_REG*4]);
      __m128i result = _mm_srli_epi32(data_register,0);
      result = _mm_and_si128(result,mask);
      result = _mm_add_epi32(result,prev_result);
      
      bit_i = 0;
      data_i += BYTES_PER_REG*2;
      
      return result;
    }
  }
  inline void print_incremental(uint8_t *data, const size_t length, const size_t cardinality){
    (void)length;
    
    size_t data_i = 1;
    const uint8_t bits_used = data[0];
    size_t num_decoded = 0;
    unsigned int prev = variant::variant_decode(data,data_i);

    size_t num_simd_packed = get_num_simd_packed(cardinality);
    if(num_simd_packed > 0){
      unsigned int mask32 = (long)((long)1 << (long)bits_used)-1;
      __m128i mask = _mm_set1_epi32(mask32);
      __m128i prev_result = _mm_set1_epi32(prev);
      size_t bit_i = 0;
      cout << "num_simd_packed: " << num_simd_packed << endl;
      while(num_decoded < num_simd_packed){
        num_decoded += INTS_PER_REG;
        __m128i actual_data = get_next_see(data,bits_used,data_i,prev_result,mask,bit_i,num_decoded >= num_simd_packed);

        unsigned int uint_data = _mm_extract_epi32(actual_data, 0);
        cout << " Data: " << uint_data << endl;
        uint_data = _mm_extract_epi32(actual_data, 1);
        cout << " Data: " << uint_data << endl;
        uint_data = _mm_extract_epi32(actual_data, 2);
        cout << " Data: " << uint_data << endl;
        uint_data = _mm_extract_epi32(actual_data, 3);
        cout << " Data: " << uint_data << endl;
        prev = uint_data;

        prev_result = actual_data;
      }
    }
    //cout << "Decoding at: " << data_i  << " Num decoded: " << num_decoded << " card: " << cardinality << endl;

    while(num_decoded < cardinality){
      unsigned int cur = variant::variant_decode(data,data_i)+prev;
      cout << " Data: " << cur << endl;
      prev = cur;
      num_decoded++;
    }
  }
  template<typename T> 
  inline T sum(std::function<T(unsigned int,unsigned int,unsigned int*)> function,unsigned int col,uint8_t *data, size_t cardinality){
    T return_value = (T) 0;
    size_t data_i = 1;
    const uint8_t bits_used = data[0];
    size_t num_decoded = 0;
    unsigned int prev = variant::variant_decode(data,data_i);

    size_t num_simd_packed = get_num_simd_packed(cardinality);
    if(num_simd_packed > 0){
      unsigned int mask32 = (long)((long)1 << (long)bits_used)-1;
      __m128i mask = _mm_set1_epi32(mask32);
      __m128i prev_result = _mm_set1_epi32(prev);
      
      bool incr_data_i = false;
      size_t bit_i = 0;
      __m128i data_register;
      //cout << "num_simd_packed: " << num_simd_packed << endl;
      while(num_decoded < num_simd_packed){
        data_register = _mm_loadu_si128((__m128i*)&data[data_i]);
        while(bit_i+bits_used <= 32 && num_decoded < num_simd_packed){
          __m128i result = _mm_srli_epi32(data_register,bit_i);
          result = _mm_and_si128(result,mask);
          result = _mm_add_epi32(result,prev_result);
          prev_result = result;
          
          bit_i += bits_used;

          //Apply Function (intersection)
          uint32_t t = _mm_extract_epi32(result, 0);
          return_value += function(col,t);
          t = _mm_extract_epi32(result, 1);
          return_value += function(col,t);
          t = _mm_extract_epi32(result, 2);
          return_value += function(col,t);
          t = _mm_extract_epi32(result, 3);
          return_value += function(col,t);

          prev = t;

          num_decoded += INTS_PER_REG;
        }
        if(bit_i < 32 && num_decoded < num_simd_packed){
          __m128i cur_lower = _mm_srli_epi32(data_register,bit_i);
          data_register = _mm_loadu_si128((__m128i*)&data[data_i+INTS_PER_REG*4]);

          __m128i cur_upper = _mm_slli_epi32(data_register,(32-bit_i));
          __m128i result = _mm_and_si128(_mm_or_si128(cur_upper,cur_lower),mask);

          result = _mm_add_epi32(result,prev_result);
          prev_result = result;

          bit_i = bits_used-(32-bit_i);      

          //Apply Function (intersection)
          uint32_t t = _mm_extract_epi32(result, 0);
          return_value += function(col,t);
          t = _mm_extract_epi32(result, 1);
          return_value += function(col,t);
          t = _mm_extract_epi32(result, 2);
          return_value += function(col,t);
          t = _mm_extract_epi32(result, 3);
          return_value += function(col,t);

          prev = t;

          num_decoded += INTS_PER_REG;
          data_i += INTS_PER_REG*4;
          incr_data_i = true;
        } else{
          bit_i = 0;
          data_i += INTS_PER_REG*4;
          incr_data_i = false;
        }
      }
      data_i += INTS_PER_REG*incr_data_i*4;
    }
    //cout << "Decoding at: " << data_i  << " Num decoded: " << num_decoded << " card: " << cardinality << endl;
    //cout << "prev: " << prev << endl;

    while(num_decoded < cardinality){
      unsigned int cur = variant::variant_decode(data,data_i)+prev;
      return_value += function(col,cur);
      prev = cur;
      num_decoded++;
    }
    return return_value;
  }
  inline void decode(unsigned int *output,uint8_t *data, size_t cardinality){
    size_t output_i = 0;
    size_t data_i = 1;
    const uint8_t bits_used = data[0];
    size_t num_decoded = 0;
    unsigned int prev = variant::variant_decode(data,data_i);

    size_t num_simd_packed = get_num_simd_packed(cardinality);
    if(num_simd_packed > 0){
      unsigned int mask32 = (long)((long)1 << (long)bits_used)-1;
      __m128i mask = _mm_set1_epi32(mask32);
      __m128i prev_result = _mm_set1_epi32(prev);
      
      bool incr_data_i = false;
      size_t bit_i = 0;
      __m128i data_register;
      //cout << "num_simd_packed: " << num_simd_packed << endl;
      while(num_decoded < num_simd_packed){
        data_register = _mm_loadu_si128((__m128i*)&data[data_i]);
        while(bit_i+bits_used <= 32 && num_decoded < num_simd_packed){
          __m128i result = _mm_srli_epi32(data_register,bit_i);
          result = _mm_and_si128(result,mask);
          result = _mm_add_epi32(result,prev_result);
          prev_result = result;
          
          bit_i += bits_used;

          //Apply Function (intersection)
          _mm_storeu_si128((__m128i*)&output[output_i],result);
          prev = output[output_i+3];
          output_i += INTS_PER_REG;

          num_decoded += INTS_PER_REG;
        }
        if(bit_i < 32 && num_decoded < num_simd_packed){
          __m128i cur_lower = _mm_srli_epi32(data_register,bit_i);
          data_register = _mm_loadu_si128((__m128i*)&data[data_i+INTS_PER_REG*4]);

          __m128i cur_upper = _mm_slli_epi32(data_register,(32-bit_i));
          __m128i result = _mm_and_si128(_mm_or_si128(cur_upper,cur_lower),mask);

          result = _mm_add_epi32(result,prev_result);
          prev_result = result;

          bit_i = bits_used-(32-bit_i);      

          //Apply Function (intersection)
          _mm_storeu_si128((__m128i*)&output[output_i],result);
          prev = output[output_i+3];
          output_i += INTS_PER_REG;

          num_decoded += INTS_PER_REG;
          data_i += INTS_PER_REG*4;
          incr_data_i = true;
        } else{
          bit_i = 0;
          data_i += INTS_PER_REG*4;
          incr_data_i = false;
        }
      }
      data_i += INTS_PER_REG*incr_data_i*4;
    }
    //cout << "Decoding at: " << data_i  << " Num decoded: " << num_decoded << " card: " << cardinality << endl;
    //cout << "prev: " << prev << endl;

    while(num_decoded < cardinality){
      unsigned int cur = variant::variant_decode(data,data_i)+prev;
      output[output_i++] = cur;
      prev = cur;
      num_decoded++;
    }
  }
  inline void print_data(uint8_t *data, const size_t cardinality, std::ofstream &file){
    size_t data_i = 1;
    const uint8_t bits_used = data[0];
    size_t num_decoded = 0;
    unsigned int prev = variant::variant_decode(data,data_i);

    size_t num_simd_packed = get_num_simd_packed(cardinality);
    if(num_simd_packed > 0){
      unsigned int mask32 = (long)((long)1 << (long)bits_used)-1;
      __m128i mask = _mm_set1_epi32(mask32);
      __m128i prev_result = _mm_set1_epi32(prev);
      
      bool incr_data_i = false;
      size_t bit_i = 0;
      __m128i data_register;
      //cout << "num_simd_packed: " << num_simd_packed << endl;
      while(num_decoded < num_simd_packed){
        data_register = _mm_loadu_si128((__m128i*)&data[data_i]);
        while(bit_i+bits_used <= 32 && num_decoded < num_simd_packed){
          __m128i result = _mm_srli_epi32(data_register,bit_i);
          result = _mm_and_si128(result,mask);
          result = _mm_add_epi32(result,prev_result);
          prev_result = result;
          
          bit_i += bits_used;

          //Apply Function (intersection)
          uint32_t *t = new uint32_t[4];
          _mm_storeu_si128((__m128i*)&t[0],result);

          file << " Data: " << t[0] << endl;
          file << " Data: " << t[1] << endl;
          file << " Data: " << t[2] << endl;
          file << " Data: " << t[3] << endl;

          prev = t[3];

          num_decoded += INTS_PER_REG;
        }
        if(bit_i < 32 && num_decoded < num_simd_packed){
          __m128i cur_lower = _mm_srli_epi32(data_register,bit_i);
          data_register = _mm_loadu_si128((__m128i*)&data[data_i+INTS_PER_REG*4]);

          __m128i cur_upper = _mm_slli_epi32(data_register,(32-bit_i));
          __m128i result = _mm_and_si128(_mm_or_si128(cur_upper,cur_lower),mask);

          result = _mm_add_epi32(result,prev_result);
          prev_result = result;

          bit_i = bits_used-(32-bit_i);      

          //Apply Function (intersection)
          uint32_t *t = (uint32_t*) &result;
          file << " Data: " << t[0] << endl;
          file << " Data: " << t[1] << endl;
          file << " Data: " << t[2] << endl;
          file << " Data: " << t[3] << endl;

          prev = t[3];

          num_decoded += INTS_PER_REG;
          data_i += INTS_PER_REG*4;
          incr_data_i = true;
        } else{
          bit_i = 0;
          data_i += INTS_PER_REG*4;
          incr_data_i = false;
        }
      }
      data_i += INTS_PER_REG*incr_data_i*4;
    }
    //cout << "Decoding at: " << data_i  << " Num decoded: " << num_decoded << " card: " << cardinality << endl;
    //cout << "prev: " << prev << endl;

    while(num_decoded < cardinality){
      unsigned int cur = variant::variant_decode(data,data_i)+prev;
      file << " Data: " << cur << endl;
      prev = cur;
      num_decoded++;
    }
  }

}