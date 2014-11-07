#include "bitset.hpp"
#include "array32.hpp"
#include "array16.hpp"
#include "a32bitpacked.hpp"

namespace hybrid {
  static __m256i load_mask_runs[256];
  static __m256i permutation_mask_runs[256];

  static inline int getBitH(unsigned int value, unsigned int position) {
    return ( ( value & (1 << position) ) >> position);
  }
  static inline void prepare_shuffling_dictionary() {
    //Number of bits that can possibly be set are the lower 8
    for(unsigned int i = 0; i < 256; i++) { // 2^8 possibilities we need to store masks for
      unsigned int counter = 0;
      unsigned int back_counter = 7;
      unsigned int permutation[8];
      unsigned int load_permutation[8];

      memset(load_permutation, 0, sizeof(load_permutation));
      for(unsigned int b = 0; b < 8; b++) { //Check each possible bit that can be set 1-by-1
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

  inline size_t preprocess(uint8_t *result_in, unsigned int *data_in, size_t length, size_t mat_size){
    size_t threshold = 6;
    size_t data_i = 0;
    size_t result_i = 4;
    unsigned int num_dense = 0;
    unsigned int *sparse_set = new unsigned int[length];
    size_t sparse_i = 0;

    while(data_i < length){
      //loop
      size_t start = data_i;
      unsigned int cur = data_in[data_i];
      uint8_t box_mask = 0;
      unsigned int num_in_box = 0;
      unsigned int dist = 0;
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
        unsigned int *dense_start = (unsigned int*)&result_in[result_i];
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

    unsigned int *num_dense_pointer = (unsigned int*) result_in;
    num_dense_pointer[0] = num_dense;

    return result_i;
  }
  inline float print_data(uint8_t *data, size_t length, size_t cardinality,ofstream &file){
    (void) cardinality;
    size_t data_i = 4;

    float result = 0.0;
    
    unsigned int *num_dense_pointer = (unsigned int*) data;
    const unsigned int num_dense = num_dense_pointer[0];

    __m256 avx_result = _mm256_set1_ps(0);
    for(size_t num_dense_processed = 0;num_dense_processed<num_dense;num_dense_processed++){
      unsigned int *cur_pointer = (unsigned int*) &data[data_i];
      unsigned int cur = cur_pointer[0];
      
      file << "Dense data: " << cur << endl;
      file << "Mask: " << hex << (unsigned int)data[data_i+4] << dec << endl;

      data_i += 5;
    }

    result = common::_mm256_reduce_add_ps(avx_result);
  
    unsigned int *data_32 = (unsigned int*) &data[data_i];
    size_t sparse_i = 0;
    for(;data_i<length;data_i+=4){
      file << "Sparse data: " << data_32[sparse_i++] << endl;
    }
    return result;
  }
  inline float sum_pr(uint8_t *data, size_t length, size_t cardinality,float *old_data, unsigned int *lengths){    
    (void) lengths; (void) cardinality;
    size_t data_i = 4;

    float result = 0.0;
    
    unsigned int *num_dense_pointer = (unsigned int*) data;
    const unsigned int num_dense = num_dense_pointer[0];

    __m256 avx_result = _mm256_set1_ps(0);
    for(size_t num_dense_processed = 0;num_dense_processed<num_dense;num_dense_processed++){
      unsigned int *cur_pointer = (unsigned int*) &data[data_i];
      unsigned int cur = cur_pointer[0];
      
      __m256 my_data = _mm256_maskload_ps(&old_data[cur],load_mask_runs[(unsigned int)data[data_i+4]]);
     // my_data = _mm256_mul_ps(my_data,_mm256_set1_ps(2.25));
      __m256 divisor = _mm256_cvtepi32_ps(_mm256_loadu_si256((__m256i*)&lengths[cur]));
      my_data = _mm256_div_ps(my_data,divisor);
      avx_result = _mm256_add_ps(my_data,avx_result);
      
      //my_data = _mm256_permutevar_ps(my_data,permutation_mask_runs[204]);
      data_i += 5;
    }

    result = common::_mm256_reduce_add_ps(avx_result);
  
    unsigned int *data_32 = (unsigned int*) &data[data_i];
    size_t sparse_i = 0;
    for(;data_i<length;data_i+=4){
      result += (old_data[data_32[sparse_i]]/lengths[data_32[sparse_i]]);
      sparse_i++;
    }
    return result;
  }
  inline float sum(uint8_t *data, size_t length, size_t cardinality,float *old_data, unsigned int *lengths){    
    (void) lengths; (void) cardinality;
    size_t data_i = 4;

    float result = 0.0;
    
    unsigned int *num_dense_pointer = (unsigned int*) data;
    const unsigned int num_dense = num_dense_pointer[0];

    __m256 avx_result = _mm256_set1_ps(0);
    for(size_t num_dense_processed = 0;num_dense_processed<num_dense;num_dense_processed++){
      unsigned int *cur_pointer = (unsigned int*) &data[data_i];
      unsigned int cur = cur_pointer[0];
      
      __m256 my_data = _mm256_maskload_ps(&old_data[cur],load_mask_runs[(unsigned int)data[data_i+4]]);
      //my_data = _mm256_mul_ps(my_data,_mm256_set1_ps(2.25));
      avx_result = _mm256_add_ps(my_data,avx_result);
      
      //my_data = _mm256_permutevar_ps(my_data,permutation_mask_runs[204]);
      data_i += 5;
    }

    result = common::_mm256_reduce_add_ps(avx_result);
  
    unsigned int *data_32 = (unsigned int*) &data[data_i];
    size_t sparse_i = 0;
    for(;data_i<length;data_i+=4){
      result += old_data[data_32[sparse_i++]];
    }
    return result;
  }
  inline size_t intersect_a16_bs(uint8_t *C_in, const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
    #if WRITE_VECTOR == 0
    (void) C;   
    #endif 

    #if WRITE_VECTOR == 1
    unsigned int *C_size = (unsigned int*)&C_in[0];
    C_in[4] = common::ARRAY32;
    unsigned int *C = (unsigned int*)&C_in[5];
    #endif


    size_t count = 0;
    for(size_t i = 0; i < s_a; i++){
      unsigned int prefix = (A[i] << 16);
      unsigned short size = A[i+1];
      i += 2;

      size_t inner_end = i+size;
      while(i < inner_end){
        unsigned int cur = prefix | A[i];
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

    #if WRITE_VECTOR == 1
    C_size[0] = count;
    #endif

    return count;
  }
  //untested
  inline size_t intersect_a32_bs(uint8_t *C_in, const unsigned int *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
    #if WRITE_VECTOR == 0
    (void) C;   
    #endif

    #if WRITE_VECTOR == 1
    unsigned int *C_size = (unsigned int*)&C_in[0];
    C_in[4] = common::ARRAY32;
    unsigned int *C = (unsigned int*)&C_in[5];
    #endif

    size_t count = 0;
    for(size_t i = 0; i < s_a; i++){
      unsigned int cur = A[i];
      if(bitset::word_index(cur) < s_b && bitset::is_set(cur,B)){
        #if WRITE_VECTOR == 1
        C[count] = cur;
        #endif
        count++;
      }
    }

    #if WRITE_VECTOR == 1
    C_size[0] = count;
    #endif

    return count;
  }
  inline size_t intersect_a32_a16(uint8_t *C_in, const unsigned int *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
    #if WRITE_VECTOR == 0
    (void)C;
    #endif

    #if WRITE_VECTOR == 1
    unsigned int *C_size = (unsigned int*)&C_in[0];
    C_in[4] = common::ARRAY32;
    unsigned int *C = (unsigned int*)&C_in[5];
    #endif

    size_t a_i = 0;
    size_t b_i = 0;
    size_t count = 0;

    bool not_finished = a_i < s_a && b_i < s_b;
    while(not_finished){
      unsigned int prefix = (B[b_i] << 16);
      unsigned short b_inner_size = B[b_i+1];
      unsigned int cur_match = A[a_i];
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
          unsigned int r = _mm_extract_epi32(res_v, 0);

          #if WRITE_VECTOR == 1
          unsigned int r_lower = r & 0x0F;
          unsigned int r_upper = (r & 0xF0) >> 4;
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
          while(notFinished && (unsigned int)(prefix | B[i_b+b_i]) < A[a_i]){
            ++i_b;
            notFinished = i_b < b_inner_size;
          }
          if(notFinished && A[a_i] == (unsigned int)(prefix | B[i_b+b_i])){
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

    #if WRITE_VECTOR == 1
    C_size[0] = count;
    #endif

    return count;
  }
} 