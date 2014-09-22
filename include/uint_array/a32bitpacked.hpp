#include "common.hpp"
#include "variant_delta.hpp" //really just need variant but this helps for a32 so no circular dep

namespace a32bitpacked {
  inline __m128i simd_bit_unpack(const uint8_t bits_used, size_t &bit_i, uint8_t *data, size_t &data_i){
    //cout << "Bits used: " << (uint)bits_used << endl;
    unsigned int mask32 = (long)((long)1 << (long)bits_used)-1;
    //cout << "mASK: " << hex << mask32 << dec << endl;
    __m128i mask = _mm_set1_epi32(mask32);
    if(bit_i+bits_used <= 32){
      __m128i data_register = _mm_loadu_si128((__m128i*)&data[data_i]);

      //uint32_t *t = (uint32_t*) &data_register;
      //cout << "1LOADING Values[" << data_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;
     
      __m128i result = _mm_srli_epi32(data_register,bit_i);
      result = _mm_and_si128(result,mask);
      bit_i += bits_used;

      return result;
    }
    else if(bit_i < 32){
      __m128i data_register = _mm_loadu_si128((__m128i*)&data[data_i]);

      //uint32_t *t = (uint32_t*) &data_register;
      //cout << "2LOADING Values[" << data_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;

      __m128i cur_lower = _mm_srli_epi32(data_register,bit_i);
      data_register = _mm_loadu_si128((__m128i*)&data[data_i+INTS_PER_REG*4]);

      //t = (uint32_t*) &data_register;
      //cout << "22LOADING Values[" << data_i+INTS_PER_REG*4 << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;


      __m128i cur_upper = _mm_slli_epi32(data_register,(32-bit_i));
      __m128i result = _mm_and_si128(_mm_or_si128(cur_upper,cur_lower),mask);

      bit_i = bits_used-(32-bit_i);      
      data_i += INTS_PER_REG*4;

      return result;
    } else{
      bit_i = 0;
      data_i += INTS_PER_REG*4;

      __m128i data_register = _mm_loadu_si128((__m128i*)&data[data_i]);

      //uint32_t *t = (uint32_t*) &data_register;
      //cout << "3LOADING Values[" << data_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;

      __m128i result = _mm_srli_epi32(data_register,bit_i);
      result = _mm_and_si128(result,mask);
      bit_i += bits_used;

      return result;
    }
  }
  inline size_t simd_bit_pack(const uint8_t bits_used, unsigned int *data, size_t data_index, size_t num_simd_packed, uint8_t *result_in, size_t result_i){
    size_t num_packed = 0;
    size_t bit_i = 0;
    size_t data_i = data_index;

    __m128i data_register;
    __m128i packed_register = _mm_set1_epi32(0);
    while(num_packed < num_simd_packed){
      while(bit_i+bits_used <= 32 && data_i < num_simd_packed){
        //cout << "1bit_i: " << bit_i << endl;
        data_register = _mm_loadu_si128((__m128i*)&data[data_i]);

        //cout << "Data[" << data_i << "]: " << data[data_i] << endl;
        //uint32_t *t = (uint32_t*) &data_register;
        //cout << "11LOADING Values[" << data_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;
     
        __m128i cur = _mm_slli_epi32(data_register,bit_i);
        packed_register = _mm_or_si128(cur,packed_register);

        data_i += INTS_PER_REG;
        bit_i += bits_used;
      }
      if(bit_i < 32 && data_i < num_simd_packed){
        //cout << "SHAREDbit_i: " << bit_i << endl;
        data_register = _mm_loadu_si128((__m128i*)&data[data_i]);

        /*
        cout << "Data[" << data_i << "]: " << data[data_i] << endl;
        uint32_t *t = (uint32_t*) &data_register;
        cout << "12LOADING Values[" << data_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;
        */

        __m128i cur = _mm_slli_epi32(data_register,bit_i);
        packed_register = _mm_or_si128(cur,packed_register);

        //uint32_t *t = (uint32_t*) &packed_register;
        //cout << "STORING Values[" << result_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;
     
        _mm_storeu_si128((__m128i*)&result_in[result_i],packed_register);
        num_packed = data_i;
        packed_register = _mm_set1_epi32(0);
        packed_register =_mm_srli_epi32(data_register,(32-bit_i));

        data_i += INTS_PER_REG;
        bit_i = bits_used-(32-bit_i);
      } else{
        //cout << "endbit_i: " << bit_i << endl;

        //uint32_t *t = (uint32_t*) &packed_register;
        //cout << "END STORING Values[" << result_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;
     
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

      unsigned int *data = data_in;
      size_t num_simd_packed = (length/INTS_PER_REG)*INTS_PER_REG*(length >= INTS_PER_REG*2);
      const uint8_t bits_used = (unsigned int)log2(data_in[length-1])+1;
      result_in[result_i++] = bits_used;
      //cout << "bits_used: " << (uint) result_in[0] << endl;
      //cout << "sending in: " << data[data_i] << endl;
      //cout << " num_simd_packed: " << num_simd_packed << endl;

      //cout << data_i << endl << endl;
      result_i = simd_bit_pack(bits_used,data,data_i,num_simd_packed,result_in,result_i);
      data_i = num_simd_packed;
      
      //cout << "starting encode at: " << result_i << " data elem: " << data_i << " result: " << result_i << endl;
      result_i = variant::variant_encode(data,data_i,length,result_in,result_i);

      return result_i;
    } else{
      return 0;
    }
  }
  inline void print_data(uint8_t *data, const size_t length, const size_t cardinality, std::ofstream &file){
    (void)length;
    
    if(cardinality != 0){
      //cout << "bits_used: " << (uint)bits_used << endl;
      size_t data_i = 1;
      size_t num_decoded = 0;
      size_t num_simd_packed = (cardinality/INTS_PER_REG)*INTS_PER_REG*(cardinality >= INTS_PER_REG*2);
      const uint8_t bits_used = data[0];
      size_t bit_i = 0;
      cout << "num decoded: " << num_decoded << " num_simd_packed: " << num_simd_packed << endl;
      while(num_decoded < num_simd_packed){
       // cout << "num_decoded: " << num_decoded << endl;
        __m128i packed_data;//simd_bit_unpack(bits_used,bit_i,data,data_i);

        unsigned int mask32 = (long)((long)1 << (long)bits_used)-1;
        //cout << "mASK: " << hex << mask32 << dec << endl;
        __m128i mask = _mm_set1_epi32(mask32);
        if(bit_i+bits_used <= 32){
          __m128i data_register = _mm_loadu_si128((__m128i*)&data[data_i]);

          //uint32_t *t = (uint32_t*) &data_register;
          //cout << "1LOADING Values[" << data_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;
         
          __m128i result = _mm_srli_epi32(data_register,bit_i);
          result = _mm_and_si128(result,mask);
          bit_i += bits_used;

          packed_data = result;
        }
        else if(bit_i < 32){
          __m128i data_register = _mm_loadu_si128((__m128i*)&data[data_i]);

          //uint32_t *t = (uint32_t*) &data_register;
          //cout << "2LOADING Values[" << data_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;

          __m128i cur_lower = _mm_srli_epi32(data_register,bit_i);
          data_register = _mm_loadu_si128((__m128i*)&data[data_i+INTS_PER_REG*4]);

          //t = (uint32_t*) &data_register;
          //cout << "22LOADING Values[" << data_i+INTS_PER_REG*4 << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;


          __m128i cur_upper = _mm_slli_epi32(data_register,(32-bit_i));
          __m128i result = _mm_and_si128(_mm_or_si128(cur_upper,cur_lower),mask);

          bit_i = bits_used-(32-bit_i);      
          data_i += INTS_PER_REG*4;

          packed_data = result;
        } else{
          bit_i = 0;
          data_i += INTS_PER_REG*4;

          __m128i data_register = _mm_loadu_si128((__m128i*)&data[data_i]);

          //uint32_t *t = (uint32_t*) &data_register;
          //cout << "3LOADING Values[" << data_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;

          __m128i result = _mm_srli_epi32(data_register,bit_i);
          result = _mm_and_si128(result,mask);
          bit_i += bits_used;

          packed_data = result;
        }


        //Apply Function (intersection)
        uint32_t *t = (uint32_t*) &packed_data;
        file << " Data: " << t[0] << endl;
        file << " Data: " << t[1] << endl;
        file << " Data: " << t[2] << endl;
        file << " Data: " << t[3] << endl;
        //

        //cout << "bit_i: " << bit_i << " data_i: " << data_i << endl;

        num_decoded += INTS_PER_REG;
      }
      if(num_simd_packed > 0){
        data_i += INTS_PER_REG*4;
      }

      variant::print_data(&data[data_i],length-data_i,cardinality-num_decoded,file);
    }
  }
} 