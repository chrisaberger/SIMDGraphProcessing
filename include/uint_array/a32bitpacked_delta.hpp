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
  inline __m128i simd_bit_unpack(const uint8_t bits_used, size_t &bit_i, uint8_t *data, size_t &data_i, __m128i data_register){
    //cout << "Bits used: " << (uint)bits_used << endl;
    unsigned int mask32 = (long)((long)1 << (long)bits_used)-1;
    //cout << "mASK: " << hex << mask32 << dec << endl;
    __m128i mask = _mm_set1_epi32(mask32);
    //cout << "Bit Index: " << bit_i << endl;
    if(bit_i+bits_used <= 32){
      //uint32_t *t = (uint32_t*) &data_register;
      //cout << "1LOADING Values[" << data_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;
     
      __m128i result = _mm_srli_epi32(data_register,bit_i);
      result = _mm_and_si128(result,mask);
      bit_i += bits_used;

      return result;
    }
    else if(bit_i < 32){
      //uint32_t *t = (uint32_t*) &data_register;
      //cout << "2LOADING Values[" << data_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;

      __m128i cur_lower = _mm_srli_epi32(data_register,bit_i);
      data_register = _mm_loadu_si128((__m128i*)&data[data_i+INTS_PER_REG*4]);

      ///t = (uint32_t*) &data_register;
      //cout << "22LOADING Values[" << data_i+INTS_PER_REG*4 << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;


      __m128i cur_upper = _mm_slli_epi32(data_register,(32-bit_i));
      __m128i result = _mm_and_si128(_mm_or_si128(cur_upper,cur_lower),mask);

      bit_i = bits_used-(32-bit_i);      
      data_i += INTS_PER_REG*4;

      return result;
    } else{
      bit_i = 0;
      data_i += INTS_PER_REG*4;

      //uint32_t *t = (uint32_t*) &data_register;
      //cout << "3LOADING Values[" << data_i << "]: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;

      __m128i result = _mm_srli_epi32(data_register,bit_i);
      result = _mm_and_si128(result,mask);
      bit_i += bits_used;

      return result;
    }
  }
  static inline size_t simd_bit_pack(const uint8_t bits_used, unsigned int *data, size_t data_index, size_t num_simd_packed, uint8_t *result_in, size_t result_i){
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
  static inline size_t preprocess(uint8_t *result_in, unsigned int *data_in, size_t length){
    if(length > 0){
      size_t data_i = 0;
      size_t result_i = 0;

      unsigned int *data = new unsigned int[length];
      size_t num_simd_packed = (((length-1)/INTS_PER_REG)*INTS_PER_REG*((length-1) >= INTS_PER_REG*2))+1;
      unsigned int max = produce_deltas(data_in,length,data,num_simd_packed);

      result_i = 1;
      const uint8_t bits_used = (unsigned int)log2(max)+1;
      result_in[0] = bits_used;

      result_i = variant::variant_encode(data_in,data_i++,1,result_in,result_i);

      //cout << "bits_used: " << (uint) result_in[0] << endl;
      //cout << "sending in: " << data[data_i] << endl;
      //cout << " num_simd_packed: " << num_simd_packed << endl;

      //cout << data_i << endl << endl;
      result_i = simd_bit_pack(bits_used,data,data_i,num_simd_packed,result_in,result_i);
      data_i = num_simd_packed;
      
      //cout << "starting encode at: " << result_i << " data elem: " << data_i << " result: " << result_i << endl;
      result_i = variant::variant_encode(data,data_i,length,result_in,result_i);

      delete[] data;

      return result_i;
    } else{
      return 0;
    }
  }
  static inline void print_data(uint8_t *data, const size_t length, const size_t cardinality, std::ofstream &file){
    (void)length;
    if(cardinality != 0){
      //cout << "bits_used: " << (uint)bits_used << endl;
      size_t data_i = 1;

      size_t num_decoded = 0;

      size_t num_simd_packed = ( ((cardinality-1)/INTS_PER_REG)*INTS_PER_REG*((cardinality-1) >= INTS_PER_REG*2))+1;
      unsigned int prev = variant::variant_decode(data,data_i);
      file << " Data: " << prev << endl;
      num_decoded++;


      uint8_t bits_used = data[0];
      __m128i prev_result = _mm_set1_epi32(prev);

      size_t bit_i = 0;
     // cout << "num decoded: " << num_decoded << " num_simd_packed: " << num_simd_packed << endl;
      while(num_decoded < num_simd_packed){
       // cout << "num_decoded: " << num_decoded << endl;
        __m128i data_register = _mm_loadu_si128((__m128i*)&data[data_i]);
        __m128i packed_data = simd_bit_unpack(bits_used,bit_i,data,data_i, data_register);

        packed_data = _mm_add_epi32(packed_data,prev_result);
        prev_result = packed_data;

        //Apply Function (intersection)
        uint32_t *t = (uint32_t*) &packed_data;
        file << " Data: " << t[0] << endl;
        file << " Data: " << t[1] << endl;
        file << " Data: " << t[2] << endl;
        file << " Data: " << t[3] << endl;
        //

        prev = t[3];

        //cout << "bit_i: " << bit_i << " data_i: " << data_i << endl;

        num_decoded += INTS_PER_REG;
      }
      if(num_simd_packed > 0){
        data_i += INTS_PER_REG*4;
      }

      //cout << "starting variant decode at: " << data_i << endl;
      while(num_decoded < cardinality){
        unsigned int cur = variant::variant_decode(data,data_i);
        //cout << "data_i: " << data_i << endl;
        //cout << "prev: " << prev << endl;
        cur += prev;
        prev = cur;

        file << " Data: " << cur << endl;
        num_decoded++;
      }
    }
  }
} 