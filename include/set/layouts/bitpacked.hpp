/*

THIS CLASS IMPLEMENTS THE FUNCTIONS ASSOCIATED WITH A PREFIX SHORT SET LAYOUT.

*/

#include "common.hpp"
#include "variant.hpp"

class bitpacked{
  public:
    static common::type get_type();
    static size_t build(uint8_t *r_in, const uint32_t *data, const size_t length);
    static size_t build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length);
    static tuple<size_t,size_t,common::type> get_flattened_data(const uint8_t *set_data, const size_t cardinality);

    static size_t get_num_simd_packed(size_t cardinality);
    static size_t simd_bit_pack(const uint8_t bits_used, const uint32_t *data, size_t data_index, size_t num_simd_packed, uint8_t *result_in, size_t result_i);
    static uint32_t produce_deltas(const uint32_t *data_in, size_t length, uint32_t *data, size_t num_simd_packed);

    static void foreach_until(const std::function <bool (uint32_t)>& f,
      const uint8_t *data_in, 
      const size_t cardinality, 
      const size_t number_of_bytes,
      const common::type type);
    static void foreach(const std::function <void (uint32_t)>& f,
      const uint8_t *data_in, 
      const size_t cardinality, 
      const size_t number_of_bytes,
      const common::type type);
};

inline common::type bitpacked::get_type(){
  return common::BITPACKED;
}
inline size_t bitpacked::get_num_simd_packed(size_t cardinality){
  return ((cardinality/INTS_PER_REG)*INTS_PER_REG*(cardinality >= INTS_PER_REG*2));
}
inline size_t bitpacked::simd_bit_pack(const uint8_t bits_used, const uint32_t *data, size_t data_index, size_t num_simd_packed, uint8_t *result_in, size_t result_i){
  size_t num_packed = 0;
  size_t bit_i = 0;
  size_t data_i = data_index;
  __m128i packed_register = _mm_set1_epi32(0);

  uint32_t *result = (uint32_t*)(result_in+result_i);
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
inline uint32_t bitpacked::produce_deltas(const uint32_t *data_in, size_t length, uint32_t *data, size_t num_simd_packed){    
  size_t max = 0;
  uint32_t prev = 0;
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
    uint32_t cur = data_in[i] - prev;
    //cout  << "Writing delta index: " << i << "  " << cur << endl;
    data[i] = cur;
    prev = data_in[i];
  }
  return max;
}
inline size_t bitpacked::build(uint8_t *r_in, const uint32_t *A, const size_t s_a){
  if(s_a > 0){
    uint32_t *data = new uint32_t[s_a];
    const size_t num_simd_packed = get_num_simd_packed(s_a);
    const uint32_t max = produce_deltas(A,s_a,data,num_simd_packed);
    const uint8_t bits_used = (uint32_t)log2(max)+1;
    r_in[0] = bits_used;

    size_t result_i = variant::encode(A,0,1,r_in,1);
    //cout << "First element ends at: " << result_i << endl;
    result_i = simd_bit_pack(bits_used,data,0,num_simd_packed,r_in,result_i);

    //cout << "Encoding at: " << result_i << endl;
    result_i = variant::encode(data,num_simd_packed,s_a,r_in,result_i);

    return result_i;
  } else{
    return 0;
  }
}
//Nothing is different about build flattened here. The number of bytes
//can be infered from the type. This gives us back a true CSR representation.
inline size_t bitpacked::build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length){
  if(length > 0){
    size_t *size_ptr = (size_t*) r_in;
    size_t num_bytes = build(r_in+sizeof(size_t),data,length);
    size_ptr[0] = num_bytes;
    return num_bytes+sizeof(size_t);
  } else{
    return 0;
  }
}

inline tuple<size_t,size_t,common::type> bitpacked::get_flattened_data(const uint8_t *set_data, const size_t cardinality){
  if(cardinality > 0){
    const size_t *size_ptr = (size_t*) set_data;
    return make_tuple(sizeof(size_t),size_ptr[0],common::BITPACKED);
  } else{
    return make_tuple(0,0,common::BITPACKED);
  }
}
inline void bitpacked::foreach_until(const std::function <bool (uint32_t)>& f, 
  const uint8_t *A_in, 
  const size_t cardinality, 
  const size_t number_of_bytes, 
  const common::type type){
  (void) number_of_bytes; (void) type;

  size_t data_i = 1;
  const uint8_t bits_used = A_in[0];
  size_t num_decoded = 0;
  uint32_t prev = variant::decode(A_in,data_i);

  size_t num_simd_packed = get_num_simd_packed(cardinality);
  if(num_simd_packed > 0){
    uint32_t mask32 = (long)((long)1 << (long)bits_used)-1;
    __m128i mask = _mm_set1_epi32(mask32);
    __m128i prev_result = _mm_set1_epi32(prev);
    
    bool incr_data_i = false;
    size_t bit_i = 0;
    __m128i data_register;
    //cout << "num_simd_packed: " << num_simd_packed << endl;
    while(num_decoded < num_simd_packed){
      data_register = _mm_loadu_si128((__m128i*)&A_in[data_i]);
      while(bit_i+bits_used <= 32 && num_decoded < num_simd_packed){
        __m128i result = _mm_srli_epi32(data_register,bit_i);
        result = _mm_and_si128(result,mask);
        result = _mm_add_epi32(result,prev_result);
        prev_result = result;
        
        bit_i += bits_used;

        //Apply Function (intersection)
        if(f(_mm_extract_epi32(result,0)))
          goto DONE;
        if(f(_mm_extract_epi32(result,1)))
          goto DONE;
        if(f(_mm_extract_epi32(result,2)))
          goto DONE;
        if(f(_mm_extract_epi32(result,3)))
          goto DONE;

        prev = _mm_extract_epi32(result,3);

        num_decoded += INTS_PER_REG;
      }
      if(bit_i < 32 && num_decoded < num_simd_packed){
        __m128i cur_lower = _mm_srli_epi32(data_register,bit_i);
        data_register = _mm_loadu_si128((__m128i*)&A_in[data_i+INTS_PER_REG*4]);

        __m128i cur_upper = _mm_slli_epi32(data_register,(32-bit_i));
        __m128i result = _mm_and_si128(_mm_or_si128(cur_upper,cur_lower),mask);

        result = _mm_add_epi32(result,prev_result);
        prev_result = result;

        bit_i = bits_used-(32-bit_i);      

        //Apply Function (intersection)
        if(f(_mm_extract_epi32(result,0)))
          goto DONE;
        if(f(_mm_extract_epi32(result,1)))
          goto DONE;
        if(f(_mm_extract_epi32(result,2)))
          goto DONE;
        if(f(_mm_extract_epi32(result,3)))
          goto DONE;

        prev = _mm_extract_epi32(result,3);

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
    uint32_t cur = variant::decode(A_in,data_i)+prev;
    if(f(cur))
      goto DONE;
    prev = cur;
    num_decoded++;
  }
  DONE: ;
}
inline void bitpacked::foreach(const std::function <void (uint32_t)>& f, 
  const uint8_t *A_in, 
  const size_t cardinality, 
  const size_t number_of_bytes, 
  const common::type type){
  (void) number_of_bytes; (void) type;

  size_t data_i = 1;
  const uint8_t bits_used = A_in[0];
  size_t num_decoded = 0;
  uint32_t prev = variant::decode(A_in,data_i);

  size_t num_simd_packed = get_num_simd_packed(cardinality);
  if(num_simd_packed > 0){
    uint32_t mask32 = (long)((long)1 << (long)bits_used)-1;
    __m128i mask = _mm_set1_epi32(mask32);
    __m128i prev_result = _mm_set1_epi32(prev);
    
    bool incr_data_i = false;
    size_t bit_i = 0;
    __m128i data_register;
    //cout << "num_simd_packed: " << num_simd_packed << endl;
    while(num_decoded < num_simd_packed){
      data_register = _mm_loadu_si128((__m128i*)&A_in[data_i]);
      while(bit_i+bits_used <= 32 && num_decoded < num_simd_packed){
        __m128i result = _mm_srli_epi32(data_register,bit_i);
        result = _mm_and_si128(result,mask);
        result = _mm_add_epi32(result,prev_result);
        prev_result = result;
        
        bit_i += bits_used;

        //Apply Function (intersection)
        f(_mm_extract_epi32(result,0));
        f(_mm_extract_epi32(result,1));
        f(_mm_extract_epi32(result,2));
        f(_mm_extract_epi32(result,3));

        prev = _mm_extract_epi32(result,3);

        num_decoded += INTS_PER_REG;
      }
      if(bit_i < 32 && num_decoded < num_simd_packed){
        __m128i cur_lower = _mm_srli_epi32(data_register,bit_i);
        data_register = _mm_loadu_si128((__m128i*)&A_in[data_i+INTS_PER_REG*4]);

        __m128i cur_upper = _mm_slli_epi32(data_register,(32-bit_i));
        __m128i result = _mm_and_si128(_mm_or_si128(cur_upper,cur_lower),mask);

        result = _mm_add_epi32(result,prev_result);
        prev_result = result;

        bit_i = bits_used-(32-bit_i);      

        //Apply Function (intersection)
        f(_mm_extract_epi32(result,0));
        f(_mm_extract_epi32(result,1));
        f(_mm_extract_epi32(result,2));
        f(_mm_extract_epi32(result,3));

        prev = _mm_extract_epi32(result,3);

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
    uint32_t cur = variant::decode(A_in,data_i)+prev;
    f(cur);
    prev = cur;
    num_decoded++;
  }
}
