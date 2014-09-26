#include "common.hpp"
#include "variant_delta.hpp" //really just need variant but this helps for a32 so no circular dep

namespace a32bitpacked {
  static size_t global_bit_i = 0;
  static size_t global_data_i = 1;
  
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
  inline size_t preprocess(uint8_t *result_in, unsigned int *data_in, size_t length){
    if(length > 0){
      unsigned int *data = data_in;
      const size_t num_simd_packed = (length/INTS_PER_REG)*INTS_PER_REG*(length >= INTS_PER_REG*2);
      const uint8_t bits_used = (unsigned int)log2(data_in[length-1])+1;
      result_in[0] = bits_used;

      size_t result_i = simd_bit_pack(bits_used,data,0,num_simd_packed,result_in,1);
      cout << "Encoding at: " << result_i << endl;
      result_i = variant::variant_encode(data,num_simd_packed,length,result_in,result_i);

      return result_i;
    } else{
      return 0;
    }
  }
  inline void print_data(uint8_t *data, const size_t length, const size_t cardinality, std::ofstream &file){
    (void)length;
    
    if(cardinality != 0){
      size_t num_decoded = 0;
      size_t num_simd_packed = (cardinality/INTS_PER_REG)*INTS_PER_REG*(cardinality >= INTS_PER_REG*2);
      const uint8_t bits_used = data[0];
      size_t bit_i = 0;
    
      unsigned int mask32 = (long)((long)1 << (long)bits_used)-1;
      __m128i mask = _mm_set1_epi32(mask32);
      
      unsigned int *integer_data = (unsigned int*)(data+1); 
      size_t integer_data_i = 0;
      bool incr_data_i = false;

      __m128i data_register;
      while(num_decoded < num_simd_packed){
        data_register = _mm_loadu_si128((__m128i*)&integer_data[integer_data_i]);
        while(bit_i+bits_used <= 32){
          __m128i result = _mm_srli_epi32(data_register,bit_i);
          result = _mm_and_si128(result,mask);
          bit_i += bits_used;

          //Apply Function (intersection)
          uint32_t *t = new uint32_t[4];
          _mm_storeu_si128((__m128i*)&t[0],result);

          file << " Data: " << t[0] << endl;
          file << " Data: " << t[1] << endl;
          file << " Data: " << t[2] << endl;
          file << " Data: " << t[3] << endl;

          num_decoded += INTS_PER_REG;
        }
        if(bit_i < 32){
          __m128i cur_lower = _mm_srli_epi32(data_register,bit_i);
          data_register = _mm_loadu_si128((__m128i*)&integer_data[integer_data_i+INTS_PER_REG]);

          __m128i cur_upper = _mm_slli_epi32(data_register,(32-bit_i));
          __m128i result = _mm_and_si128(_mm_or_si128(cur_upper,cur_lower),mask);

          bit_i = bits_used-(32-bit_i);      

          //Apply Function (intersection)
          uint32_t *t = (uint32_t*) &result;
          file << " Data: " << t[0] << endl;
          file << " Data: " << t[1] << endl;
          file << " Data: " << t[2] << endl;
          file << " Data: " << t[3] << endl;

          num_decoded += INTS_PER_REG;
          integer_data_i += INTS_PER_REG;
          incr_data_i = true;
        } else{
          bit_i = 0;
          integer_data_i += INTS_PER_REG;
          incr_data_i = false;
        }
      }
      size_t data_i = 1 + ((integer_data_i+INTS_PER_REG*incr_data_i) << 2);
      cout << "Decoding at: " << data_i << endl;
      variant::print_data(&data[data_i],length-data_i,cardinality-num_decoded,file);
    }
  }
} 