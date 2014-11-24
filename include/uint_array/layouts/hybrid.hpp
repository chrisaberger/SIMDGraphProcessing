#include "bitset.hpp"
#include "array32.hpp"
#include "array16.hpp"
#include "a32bitpacked.hpp"

/*

THIS CLASS IMPLEMENTS THE FUNCTIONS ASSOCIATED WITH AN UNCOMPRESSED SET LAYOUT.
QUITE SIMPLE THE LAYOUT JUST CONTAINS UNSIGNED INTEGERS IN THE SET.

*/


//Implement implicit type conversions from one templated type to another when 
//an op is called in the hybrid class.

#include "common.hpp"
class hybridC{
  public:
    static common::type get_type();
    static size_t build(uint8_t *r_in, const uint32_t *data, const size_t length);
    static size_t build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length);
    static tuple<uint8_t,size_t,size_t> get_flattened_data(const uint8_t *set_data, const size_t cardinality);
    static void foreach(const std::function <void (uint32_t)>& f,const uint8_t *data_in, const size_t cardinality, const size_t number_of_bytes, const common::type type);
};

inline common::type hybridC::get_type(){
  return common::HYBRID_PERF;
}
//Copies data from input array of ints to our set data r_in
inline size_t hybridC::build(uint8_t *r_in, const uint32_t *data, const size_t length){
  if(length > 0){
    uint32_t max_value = data[length-1];
    double sparsity = (double) length/max_value;
    if( sparsity > (double) 1/32 ){
      return uint32::build(r_in,data,length);
    } else if((length/((max_value >> 16) - (data[0] >> 16) + 1)) > 12){
      return uint32::build(r_in,data,length);
    } else {
      return uint32::build(r_in,data,length);
    }  
  }
  return 0;
}
//Nothing is different about build flattened here. The number of bytes
//can be infered from the type. This gives us back a true CSR representation.
inline size_t hybridC::build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length){
  if(length > 0){
    uint32_t max_value = data[length-1];
    double sparsity = (double) length/max_value;
    if( sparsity > (double) 1/32 ){
      r_in[0] = common::ARRAY32;
      size_t *size_ptr = (size_t*) &r_in[1];
      size_t byte_length = uint32::build(r_in+1+sizeof(size_t),data,length);
      size_ptr[0] = byte_length;
    } else if((length/((max_value >> 16) - (data[0] >> 16) + 1)) > 12){
      r_in[0] = common::ARRAY32;
      size_t *size_ptr = (size_t*) &r_in[1];
      size_t byte_length = uint32::build(r_in+1+sizeof(size_t),data,length);
      size_ptr[0] = byte_length;
    } else {
      r_in[0] = common::ARRAY32;
      size_t *size_ptr = (size_t*) &r_in[1];
      size_t byte_length = uint32::build(r_in+1+sizeof(size_t),data,length);
      size_ptr[0] = byte_length;
    }  
  }
  return 0;
}
inline tuple<uint8_t,size_t,size_t> hybridC::get_flattened_data(const uint8_t *set_data, const size_t cardinality){
  common::type type = (common::type) set_data[0];
  if(type == common::ARRAY32){
    return uint32::get_flattened_data(&set_data[1],cardinality);
  }else{
    return uint32::get_flattened_data(&set_data[1],cardinality);
  }
}

//Iterates over set applying a lambda.
inline void hybridC::foreach(const std::function <void (uint32_t)>& f, const uint8_t *data_in, 
  const size_t cardinality, const size_t number_of_bytes, const common::type type){

 (void) number_of_bytes; (void) type;
 uint32_t *data = (uint32_t*) data_in;
 for(size_t i=0; i<cardinality;i++){
  f(data[i]);
 }
}


namespace hybrid {
  static __m256i load_mask_runs[256];
  static __m256i permutation_mask_runs[256];

  static inline int getBitH(uint32_t value, uint32_t position) {
    return ( ( value & (1 << position) ) >> position);
  }
  static inline void prepare_shuffling_dictionary() {
    //Number of bits that can possibly be set are the lower 8
    for(uint32_t i = 0; i < 256; i++) { // 2^8 possibilities we need to store masks for
      uint32_t counter = 0;
      uint32_t back_counter = 7;
      uint32_t permutation[8];
      uint32_t load_permutation[8];

      memset(load_permutation, 0, sizeof(load_permutation));
      for(uint32_t b = 0; b < 8; b++) { //Check each possible bit that can be set 1-by-1
        if(getBitH(i, b)) {
          load_permutation[b] = 0xf0000000;
          permutation[counter++] = b;
        } else{
          permutation[back_counter--] = b;
        }
      }
      __m256i load_mask = _mm256_loadu_si256((const __m256i*)load_permutation);
      load_mask_runs[i] = load_mask;

      __m256i mask = _mm256_loadu_si256((const __m256i*)permutation);
      permutation_mask_runs[i] = mask;
    }
  }

  inline size_t preprocess(uint8_t *result_in, uint32_t *data_in, size_t length, size_t mat_size){
    size_t threshold = 6;
    size_t data_i = 0;
    size_t result_i = 4;
    uint32_t num_dense = 0;
    uint32_t *sparse_set = new uint32_t[length];
    size_t sparse_i = 0;

    while(data_i < length){
      //loop
      size_t start = data_i;
      uint32_t cur = data_in[data_i];
      uint8_t box_mask = 0;
      uint32_t num_in_box = 0;
      uint32_t dist = 0;
      while(dist < 8 && data_i < length && (cur+8 <= mat_size)){
        dist = (data_in[data_i]-cur);
        if(dist < 8){
          num_in_box++;
          box_mask |= (1 << dist);
          data_i += (dist < 8); 
        }
      }

      if(num_in_box >= threshold){
        //place in dense
        uint32_t *dense_start = (uint32_t*)&result_in[result_i];
        dense_start[0] = cur;
        result_i += 4;

        result_in[result_i++] = box_mask;
        num_dense++;
      } else{
        //place in sparse
        sparse_set[sparse_i++] = cur;
        data_i = start+1;
      }
    }

    uint8_t *sparse_set_bytes = (uint8_t *) sparse_set; 
    std::copy(&sparse_set_bytes[0],&sparse_set_bytes[sparse_i*4],&result_in[result_i]);
    result_i += (sparse_i*4);
    delete[] sparse_set;

    uint32_t *num_dense_pointer = (uint32_t*) result_in;
    num_dense_pointer[0] = num_dense;

    return result_i;
  }
  inline float print_data(uint8_t *data, size_t length, size_t cardinality,ofstream &file){
    (void) cardinality;
    size_t data_i = 4;

    float result = 0.0;
    
    uint32_t *num_dense_pointer = (uint32_t*) data;
    const uint32_t num_dense = num_dense_pointer[0];

    __m256 avx_result = _mm256_set1_ps(0);
    for(size_t num_dense_processed = 0;num_dense_processed<num_dense;num_dense_processed++){
      uint32_t *cur_pointer = (uint32_t*) &data[data_i];
      uint32_t cur = cur_pointer[0];
      
      file << "Dense data: " << cur << endl;
      file << "Mask: " << hex << (uint32_t)data[data_i+4] << dec << endl;

      data_i += 5;
    }

    result = common::_mm256_reduce_add_ps(avx_result);
  
    uint32_t *data_32 = (uint32_t*) &data[data_i];
    size_t sparse_i = 0;
    for(;data_i<length;data_i+=4){
      file << "Sparse data: " << data_32[sparse_i++] << endl;
    }
    return result;
  }
  inline float sum_pr(uint8_t *data, size_t length, size_t cardinality,float *old_data, uint32_t *lengths){    
    (void) lengths; (void) cardinality;
    size_t data_i = 4;

    float result = 0.0;
    
    uint32_t *num_dense_pointer = (uint32_t*) data;
    const uint32_t num_dense = num_dense_pointer[0];

    __m256 avx_result = _mm256_set1_ps(0);
    for(size_t num_dense_processed = 0;num_dense_processed<num_dense;num_dense_processed++){
      uint32_t *cur_pointer = (uint32_t*) &data[data_i];
      uint32_t cur = cur_pointer[0];
      
      __m256 my_data = _mm256_maskload_ps(&old_data[cur],load_mask_runs[(uint32_t)data[data_i+4]]);
     // my_data = _mm256_mul_ps(my_data,_mm256_set1_ps(2.25));
      __m256 divisor = _mm256_cvtepi32_ps(_mm256_loadu_si256((__m256i*)&lengths[cur]));
      my_data = _mm256_div_ps(my_data,divisor);
      avx_result = _mm256_add_ps(my_data,avx_result);
      
      //my_data = _mm256_permutevar_ps(my_data,permutation_mask_runs[204]);
      data_i += 5;
    }

    result = common::_mm256_reduce_add_ps(avx_result);
  
    uint32_t *data_32 = (uint32_t*) &data[data_i];
    size_t sparse_i = 0;
    for(;data_i<length;data_i+=4){
      result += (old_data[data_32[sparse_i]]/lengths[data_32[sparse_i]]);
      sparse_i++;
    }
    return result;
  }
  inline float sum(uint8_t *data, size_t length, size_t cardinality,float *old_data, uint32_t *lengths){    
    (void) lengths; (void) cardinality;
    size_t data_i = 4;

    float result = 0.0;
    
    uint32_t *num_dense_pointer = (uint32_t*) data;
    const uint32_t num_dense = num_dense_pointer[0];

    __m256 avx_result = _mm256_set1_ps(0);
    for(size_t num_dense_processed = 0;num_dense_processed<num_dense;num_dense_processed++){
      uint32_t *cur_pointer = (uint32_t*) &data[data_i];
      uint32_t cur = cur_pointer[0];
      
      __m256 my_data = _mm256_maskload_ps(&old_data[cur],load_mask_runs[(uint32_t)data[data_i+4]]);
      //my_data = _mm256_mul_ps(my_data,_mm256_set1_ps(2.25));
      avx_result = _mm256_add_ps(my_data,avx_result);
      
      //my_data = _mm256_permutevar_ps(my_data,permutation_mask_runs[204]);
      data_i += 5;
    }

    result = common::_mm256_reduce_add_ps(avx_result);
  
    uint32_t *data_32 = (uint32_t*) &data[data_i];
    size_t sparse_i = 0;
    for(;data_i<length;data_i+=4){
      result += old_data[data_32[sparse_i++]];
    }
    return result;
  }
  inline size_t intersect_a16_bs(uint8_t *C_in, const unsigned short *A, const uint8_t *B, const size_t s_a, const size_t s_b) {
    #if WRITE_VECTOR == 0
    (void) C_in;   
    #endif 

    #if WRITE_VECTOR == 1
    C_in[0] = common::ARRAY32;
    uint32_t *C = (uint32_t*)&C_in[1];
    #endif

    size_t count = 0;
    for(size_t i = 0; i < s_a; i++){
      uint32_t prefix = (A[i] << 16);
      unsigned short size = A[i+1];
      i += 2;

      size_t inner_end = i+size;
      while(i < inner_end){
        uint32_t cur = prefix | A[i];
        if(bitset::word_index(cur) < s_b && bitset::is_set(cur,B)){
          #if WRITE_VECTOR == 1
          C[count] = cur;
          #endif
          count++;
        }
        ++i;
      }
      i--;
    }
    return count;
  }
  //untested
  inline size_t intersect_a32_bs(uint8_t *C_in, const uint32_t *A, const uint8_t *B, const size_t s_a, const size_t s_b) {
    #if WRITE_VECTOR == 0
    C_in[0] = common::ARRAY32;
    (void) C_in;   
    #endif

    #if WRITE_VECTOR == 1
    uint32_t *C = (uint32_t*)&C_in[1];
    #endif

    size_t count = 0;
    for(size_t i = 0; i < s_a; i++){
      uint32_t cur = A[i];
      if(bitset::word_index(cur) < s_b && bitset::is_set(cur,B)){
        #if WRITE_VECTOR == 1
        C[count] = cur;
        #endif
        count++;
      }
    }

    return count;
  }
  inline size_t intersect_a32_a16(uint8_t *C_in, const uint32_t *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
    #if WRITE_VECTOR == 0
    (void)C_in;
    #endif

    #if WRITE_VECTOR == 1
    C_in[0] = common::ARRAY32;
    uint32_t *C = (uint32_t*)&C_in[1];
    #endif

    size_t a_i = 0;
    size_t b_i = 0;
    size_t count = 0;

    bool not_finished = a_i < s_a && b_i < s_b;
    while(not_finished){
      uint32_t prefix = (B[b_i] << 16);
      unsigned short b_inner_size = B[b_i+1];
      uint32_t cur_match = A[a_i];
      size_t inner_end = b_i+b_inner_size+2;
      //cout << endl;
      //cout << "Bi: " << b_i << " Bsize: " << s_b << " InnerEnd: " << inner_end << endl;

      if(prefix < (cur_match & 0xFFFF0000)){
        //cout << "1" << endl;
        b_i = inner_end;
        not_finished = b_i < s_b;
      } else if(prefix > cur_match){
        //cout << prefix << " " << cur_match << endl;
        //cout << "2" << endl;
        a_i++;
        not_finished = a_i < s_a;
      } else{
        //cout << "3" << endl;
        b_i += 2;
        size_t i_b = 0;

        #if VECTORIZE == 1
        bool a_continue = (a_i+SHORTS_PER_REG) < s_a && (A[a_i+SHORTS_PER_REG-1] & 0xFFFF0000) == prefix;
        size_t st_b = (b_inner_size / SHORTS_PER_REG) * SHORTS_PER_REG;
        while(a_continue && i_b < st_b) {
          __m128i v_a_1_32 = _mm_loadu_si128((__m128i*)&A[a_i]);
          __m128i v_a_2_32 = _mm_loadu_si128((__m128i*)&A[a_i+(SHORTS_PER_REG/2)]);

          __m128i v_a_1 = _mm_shuffle_epi8(v_a_1_32,_mm_set_epi8(uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x0D),uint8_t(0x0C),uint8_t(0x09),uint8_t(0x08),uint8_t(0x05),uint8_t(0x04),uint8_t(0x01),uint8_t(0x0)));
          __m128i v_a_2 = _mm_shuffle_epi8(v_a_2_32,_mm_set_epi8(uint8_t(0x0D),uint8_t(0x0C),uint8_t(0x09),uint8_t(0x08),uint8_t(0x05),uint8_t(0x04),uint8_t(0x01),uint8_t(0x0),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80),uint8_t(0x80)));
          
          __m128i v_a = _mm_or_si128(v_a_1,v_a_2);
            
          //uint16_t *t = (uint16_t*) &v_a;
          //cout << "Data: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << " " << t[4] << " " << t[5] << " " << t[6] << " " << t[7] << endl;

          __m128i v_b = _mm_loadu_si128((__m128i*)&B[b_i+i_b]);    

          unsigned short a_max = _mm_extract_epi16(v_a, SHORTS_PER_REG-1);
          unsigned short b_max = _mm_extract_epi16(v_b, SHORTS_PER_REG-1);
          
          __m128i res_v = _mm_cmpestrm(v_b, SHORTS_PER_REG, v_a, SHORTS_PER_REG,
                  _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
          uint32_t r = _mm_extract_epi32(res_v, 0);

          #if WRITE_VECTOR == 1
          uint32_t r_lower = r & 0x0F;
          uint32_t r_upper = (r & 0xF0) >> 4;
          __m128i p = _mm_shuffle_epi8(v_a_1_32, array32::shuffle_mask32[r_lower]);
          _mm_storeu_si128((__m128i*)&C[count], p);
          
          //uint32_t *t = (uint32_t*) &p;
          //cout << "Data: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;

          p = _mm_shuffle_epi8(v_a_2_32, array32::shuffle_mask32[r_upper]);
          _mm_storeu_si128((__m128i*)&C[count+_mm_popcnt_u32(r_lower)], p);
          C[count] = A[a_i];
          #endif

          count += _mm_popcnt_u32(r);
          a_i += (a_max <= b_max) * SHORTS_PER_REG;
          a_continue = (a_i+SHORTS_PER_REG) < s_a && (A[a_i+SHORTS_PER_REG-1] & 0xFFFF0000) == prefix;
          i_b += (a_max >= b_max) * SHORTS_PER_REG;
        }
        #endif

        bool notFinished = a_i < s_a  && i_b < b_inner_size && (A[a_i] & 0xFFFF0000) == prefix;
        while(notFinished){
          while(notFinished && (uint32_t)(prefix | B[i_b+b_i]) < A[a_i]){
            ++i_b;
            notFinished = i_b < b_inner_size;
          }
          if(notFinished && A[a_i] == (uint32_t)(prefix | B[i_b+b_i])){
            #if WRITE_VECTOR == 1
            C[count] = A[a_i];
            #endif
            ++count;
          }
          ++a_i;
          notFinished = notFinished && a_i < s_a && (A[a_i] & 0xFFFF0000) == prefix;
        }
        b_i = inner_end;
        not_finished = a_i < s_a && b_i < s_b;
      }
    }

    return count;
  }
} 