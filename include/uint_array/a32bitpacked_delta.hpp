#include "common.hpp"
#include "a32bitpacked.hpp"

namespace a32bitpacked_delta {
  static inline unsigned int produce_deltas(unsigned int *data_in, size_t length, unsigned int *data, size_t num_simd_packed){    
    size_t max = 0;
    unsigned int prev = 0;

    if(num_simd_packed > 0){
      //place first 4 elements->serves as intial delta for subtraction.
      __m128i prev_data_register = _mm_set1_epi32(data_in[0]); //set delta
      data[0] = data_in[0];

      for(size_t i = 1; i < num_simd_packed; i+=INTS_PER_REG){
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
      const size_t num_simd_packed = ((length-1)/INTS_PER_REG)*INTS_PER_REG*((length-1) >= INTS_PER_REG*2)+1;
      const unsigned int max = produce_deltas(data_in,length,data,num_simd_packed);
      const uint8_t bits_used = (unsigned int)log2(max)+1;
      result_in[0] = bits_used;

      size_t result_i = variant::variant_encode(data_in,0,1,result_in,1);
      cout << "First element ends at: " << result_i << endl;
      result_i = a32bitpacked::simd_bit_pack(bits_used,data,1,num_simd_packed,result_in,result_i);

      cout << "Encoding at: " << result_i << endl;
      result_i = variant::variant_encode(data,num_simd_packed,length,result_in,result_i);

      return result_i;
    } else{
      return 0;
    }
  }
  inline __m128i get_next_see(uint8_t *data,uint8_t bits_used,size_t data_i,__m128i prev_result, __m128i mask, size_t bit_i){
    //data_i
    //bit_i
    //bits_used
    if(bit_i+bits_used <= 32){
      __m128i data_register = _mm_loadu_si128((__m128i*)&data[data_i]);
      __m128i result = _mm_srli_epi32(data_register,bit_i);
      result = _mm_and_si128(result,mask);
      result = _mm_add_epi32(result,prev_result);      

      return result;
    } else if(bit_i < 32){
      __m128i data_register = _mm_loadu_si128((__m128i*)&data[data_i]);
      __m128i cur_lower = _mm_srli_epi32(data_register,bit_i);
      data_register = _mm_loadu_si128((__m128i*)&data[data_i+INTS_PER_REG*4]);

      __m128i cur_upper = _mm_slli_epi32(data_register,(32-bit_i));
      __m128i result = _mm_and_si128(_mm_or_si128(cur_upper,cur_lower),mask);
      result = _mm_add_epi32(result,prev_result);

      return result;
    } else{
      __m128i data_register = _mm_loadu_si128((__m128i*)&data[data_i+INTS_PER_REG*4]);
      __m128i result = _mm_srli_epi32(data_register,0);
      result = _mm_and_si128(result,mask);
      result = _mm_add_epi32(result,prev_result);
      
      return result;
    }
  }
  inline void increment_pointers(uint8_t bits_used,size_t &data_i, size_t &bit_i, bool last){
    if(bit_i+bits_used <= 32){
      bit_i += bits_used;
    } else if(bit_i < 32){
      bit_i = bits_used-(32-bit_i);      
      data_i += INTS_PER_REG*4 + last*16;
    } else{
      bit_i = 0;
      data_i += INTS_PER_REG*4*2;
    }
  }
  inline void print_data(uint8_t *data, const size_t length, const size_t cardinality, std::ofstream &file){
    (void)length;
    
    if(cardinality != 0){
      size_t num_simd_packed = ((cardinality-1)/INTS_PER_REG)*INTS_PER_REG*((cardinality-1) >= INTS_PER_REG*2) + 1;
      const uint8_t bits_used = data[0];

      unsigned int mask32 = (long)((long)1 << (long)bits_used)-1;
      __m128i mask = _mm_set1_epi32(mask32);

      size_t data_i = 1;
      unsigned int prev = variant::variant_decode(data,data_i);
      file << " Data: " << prev << endl;
      size_t num_decoded = 1;

      __m128i prev_result = _mm_set1_epi32(prev);

      unsigned int *integer_data = (unsigned int*)(data+data_i); 
      size_t integer_data_i = 0;
      bool incr_data_i = false;
      
      size_t bit_i = 0;
      __m128i data_register;
      cout << "num_simd_packed: " << num_simd_packed << endl;
      while(num_decoded < num_simd_packed){
        data_register = _mm_loadu_si128((__m128i*)&integer_data[integer_data_i]);
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
          data_register = _mm_loadu_si128((__m128i*)&integer_data[integer_data_i+INTS_PER_REG]);

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
          integer_data_i += INTS_PER_REG;
          incr_data_i = true;
        } else{
          bit_i = 0;
          integer_data_i += INTS_PER_REG;
          incr_data_i = false;
        }
      }
      cout << integer_data_i << " " << data_i << " " << incr_data_i << endl;
      data_i += ((integer_data_i+INTS_PER_REG*incr_data_i) << 2);
      cout << "Decoding at: " << data_i  << " Num decoded: " << num_decoded << " card: " << cardinality << endl;

      while(num_decoded < cardinality){
        unsigned int cur = variant::variant_decode(data,data_i)+prev;
        file << " Data: " << cur << endl;
        num_decoded++;
      }
    }
  }
}