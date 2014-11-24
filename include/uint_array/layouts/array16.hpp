#include "common.hpp"

namespace array16 {  
  static __m128i shuffle_mask16[256]; // precomputed dictionary
  static inline int getBitSD(uint32_t value, uint32_t position) {
    return ( ( value & (1 << position) ) >> position);
  }
  static inline void prepare_shuffling_dictionary16() {
    //Number of bits that can possibly be set are the lower 8
    for(uint32_t i = 0; i < 256; i++) { // 2^8 possibilities we need to store masks for
      uint32_t counter = 0;
      unsigned char permutation[16];
      memset(permutation, 0xFF, sizeof(permutation));
      for(unsigned char b = 0; b < 8; b++) { //Check each possible bit that can be set 1-by-1
        if(getBitSD(i, b)) {
          permutation[counter++] = 2*b; //tell us byte offset to get from comparison vector
          permutation[counter++] = 2*b + 1; //tells us byte offset to get from comparison vector
        }
      }
      __m128i mask = _mm_loadu_si128((const __m128i*)permutation);
      shuffle_mask16[i] = mask;
    }
  }
	inline size_t preprocess(unsigned short *R, uint32_t *A, size_t s_a){
	  unsigned short high = 0;
	  size_t partition_length = 0;
	  size_t partition_size_position = 1;
	  size_t counter = 0;
	  for(size_t p = 0; p < s_a; p++) {
	    unsigned short chigh = (A[p] & 0xFFFF0000) >> 16; // upper dword
	    unsigned short clow = A[p] & 0x0FFFF;   // lower dword
	    if(chigh == high && p != 0) { // add element to the current partition
	      partition_length++;
	      R[counter++] = clow;
	    }else{ // start new partition
	      R[counter++] = chigh; // partition prefix
	      R[counter++] = 0;     // reserve place for partition size
	      R[counter++] = clow;  // write the first element
	      R[partition_size_position] = partition_length;

	      partition_length = 1; // reset counters
	      partition_size_position = counter - 2;
	      high = chigh;
	    }
	  }
	  R[partition_size_position] = partition_length;
	  return counter*2;
	}

	inline size_t simd_intersect_vector16(unsigned short *C, const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
	  #if WRITE_VECTOR == 0
    (void)C;
    #endif
    
    size_t count = 0;
	  size_t i_a = 0, i_b = 0;

    #if VECTORIZE == 1
	  size_t st_a = (s_a / SHORTS_PER_REG) * SHORTS_PER_REG;
	  size_t st_b = (s_b / SHORTS_PER_REG) * SHORTS_PER_REG;
 
	  while(i_a < st_a && i_b < st_b) {
	    __m128i v_a = _mm_loadu_si128((__m128i*)&A[i_a]);
	    __m128i v_b = _mm_loadu_si128((__m128i*)&B[i_b]);    

	    unsigned short a_max = _mm_extract_epi16(v_a, SHORTS_PER_REG-1);
	    unsigned short b_max = _mm_extract_epi16(v_b, SHORTS_PER_REG-1);
	    
	   // __m128i res_v = _mm_cmpistrm(v_b, v_a,
	   //         _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
	    __m128i res_v = _mm_cmpestrm(v_b, SHORTS_PER_REG, v_a, SHORTS_PER_REG,
	            _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
	    uint32_t r = _mm_extract_epi32(res_v, 0);

      #if WRITE_VECTOR == 1
	    __m128i p = _mm_shuffle_epi8(v_a, shuffle_mask16[r]);
	    _mm_storeu_si128((__m128i*)&C[count], p);
	   #endif

	    count += _mm_popcnt_u32(r);
	    
	    i_a += (a_max <= b_max) * SHORTS_PER_REG;
	    i_b += (a_max >= b_max) * SHORTS_PER_REG;
	  }
    #endif

	  // intersect the tail using scalar intersection
	  //...
	  bool notFinished = i_a < s_a  && i_b < s_b;
	  while(notFinished){
	    while(notFinished && B[i_b] < A[i_a]){
	      ++i_b;
	      notFinished = i_b < s_b;
	    }
	    if(notFinished && A[i_a] == B[i_b]){
        #if WRITE_VECTOR == 1
	      C[count] = A[i_a];
        #endif
	      ++count;
	    }
	    ++i_a;
	    notFinished = notFinished && i_a < s_a;
	  }
	  return count;
	}  
  inline size_t simd_union_vector16(unsigned short *C, const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
    #if WRITE_VECTOR == 0
    (void)C;
    #endif
    
    size_t count = 0;
    size_t i_a = 0, i_b = 0;

    /*
    #if VECTORIZE == 1
    size_t st_a = (s_a / SHORTS_PER_REG) * SHORTS_PER_REG;
    size_t st_b = (s_b / SHORTS_PER_REG) * SHORTS_PER_REG;
 
    while(i_a < st_a && i_b < st_b) {
      __m128i v_a = _mm_loadu_si128((__m128i*)&A[i_a]);
      __m128i v_b = _mm_loadu_si128((__m128i*)&B[i_b]);    

      unsigned short a_max = _mm_extract_epi16(v_a, SHORTS_PER_REG-1);
      unsigned short b_max = _mm_extract_epi16(v_b, SHORTS_PER_REG-1);
      
     // __m128i res_v = _mm_cmpistrm(v_b, v_a,
     //         _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
      __m128i res_v = _mm_cmpestrm(v_b, SHORTS_PER_REG, v_a, SHORTS_PER_REG,
              _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);

      uint32_t r = _mm_extract_epi32(res_v, 0);

      uint32_t left_cyclic_shift = _MM_SHUFFLE(2,1,0,3);

      // x x x v = l1 (v = valid data, x = tbd)
      // v x x x = h1 
      __m128i l_2 = _mm_min_epu32(_mm_shuffle_epi32(h_1,left_cyclic_shift),l_1);
      __m128i h_2 = _mm_max_epi16(h_1,_mm_shuffle_epi32(l_1,right_cyclic_shift));

      // x x v v = l1 (v = valid data, x = tbd)
      // v v x x = h2 

      left_cyclic_shift = _MM_SHUFFLE(1,0,3,2);
      right_cyclic_shift = left_cyclic_shift;
      __m128i l_3 = _mm_min_epu32(_mm_shuffle_epi32(h_2, left_cyclic_shift),l_2);
      __m128i h_3 = _mm_max_epi16(h_2,_mm_shuffle_epi32(l_2,right_cyclic_shift));

      left_cyclic_shift = _MM_SHUFFLE(0,3,2,1);
      right_cyclic_shift = _MM_SHUFFLE(2,1,0,3);

      __m128i l_4 = _mm_min_epu32(_mm_shuffle_epi32(h_3, left_cyclic_shift),l_3);
      __m128i h_4 = _mm_max_epi16(h_3,_mm_shuffle_epi32(l_3,right_cyclic_shift));
      _mm_storeu_si128((__m128i*)&C[count], l_4);
      _mm_storeu_si128((__m128i*)&C[count+INTS_PER_REG], h_4);

      size_t num_hit = _mm_popcnt_u32(r);
      if(a_max < b_max){
        i_a += num_hit;
        i_b += SHORTS_PER_REG;
      } else if(b_max < a_max){
        i_a += SHORTS_PER_REG;
        i_b += num_hit;
      } else{        
        i_a += SHORTS_PER_REG;
        i_b += SHORTS_PER_REG;
      }
      count += SHORTS_PER_REG-num_hit;

      i_a += (a_max <= b_max) * SHORTS_PER_REG;
      i_b += (a_max >= b_max) * SHORTS_PER_REG;
    }
    #endif
    */
    // intersect the tail using scalar intersection
    //...
    bool notFinished = i_a < s_a  && i_b < s_b;
    while(notFinished){
      while(notFinished && B[i_b] < A[i_a]){
        ++i_b;
        notFinished = i_b < s_b;
      }
      if(notFinished && A[i_a] == B[i_b]){
        #if WRITE_VECTOR == 1
        C[count] = A[i_a];
        #endif
        ++count;
      }
      ++i_a;
      notFinished = notFinished && i_a < s_a;
    }
    return count;
  }
	inline size_t set_union(uint8_t *C_in,const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
	  size_t i_a = 0, i_b = 0;
	  size_t counter = 0;
	  size_t count = 0;
	  bool notFinished = i_a < s_a && i_b < s_b;

    #if WRITE_VECTOR == 1
    size_t *C_size = (size_t*)&C_in[1];
    C_in[0] = common::ARRAY16;
    unsigned short *C = (unsigned short*)&C_in[sizeof(size_t)+1];
    #else
    unsigned short *C = (unsigned short*) C_in;
    #endif

	  cout << "ARRAY 16 UNION" << endl;
	  while(notFinished) {
	    //size_t limLower = limLowerHolder;
	    if(A[i_a] < B[i_b]) {
        std::copy(&A[i_a],&A[A[i_a+1]+2],&C[counter]);
        counter += A[i_a+1]+2;
        count += A[i_a];
	      i_a += A[i_a + 1] + 2;
	      notFinished = i_a < s_a;
	    } else if(B[i_b] < A[i_a]) {
        std::copy(&B[i_b],&B[B[i_b+1]+2],&C[counter]);
        counter += B[i_b+1]+2;
        count += B[i_b];
	      i_b += B[i_b + 1] + 2;
	      notFinished = i_b < s_b;
	    } else {
	      unsigned short partition_size = 0;
	      //If we are not in the range of the limit we don't need to worry about it.
	      #if WRITE_VECTOR == 1
        C[counter++] = A[i_a]; // write partition prefix
	      #endif
        partition_size = simd_union_vector16(&C[counter+1],&A[i_a + 2],&B[i_b + 2],A[i_a + 1], B[i_b + 1]);
	      #if WRITE_VECTOR == 1
        C[counter++] = partition_size; // write partition size
	      #endif
        i_a += A[i_a + 1] + 2;
	      i_b += B[i_b + 1] + 2;      

	      count += partition_size;
	      counter += partition_size;
	      notFinished = i_a < s_a && i_b < s_b;
	    }
	  }

    #if WRITE_VECTOR == 1
    C_size[0] = counter*sizeof(short);
    #endif

	  return count;
	}
  inline size_t intersect(uint8_t *C_in,const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
    size_t i_a = 0, i_b = 0;
    size_t counter = 0;
    size_t count = 0;
    bool notFinished = i_a < s_a && i_b < s_b;

    #if WRITE_VECTOR == 1
    size_t *C_size = (size_t*)&C_in[1];
    C_in[0] = common::ARRAY16;
    unsigned short *C = (unsigned short*)&C_in[sizeof(size_t)+1];
    #else
    unsigned short *C = (unsigned short*) C_in;
    #endif

    //cout << lim << endl;
    while(notFinished) {
      //size_t limLower = limLowerHolder;
      if(A[i_a] < B[i_b]) {
        i_a += A[i_a + 1] + 2;
        notFinished = i_a < s_a;
      } else if(B[i_b] < A[i_a]) {
        i_b += B[i_b + 1] + 2;
        notFinished = i_b < s_b;
      } else {
        unsigned short partition_size = 0;
        //If we are not in the range of the limit we don't need to worry about it.
        #if WRITE_VECTOR == 1
        C[counter++] = A[i_a]; // write partition prefix
        #endif
        partition_size = simd_intersect_vector16(&C[counter+1],&A[i_a + 2],&B[i_b + 2],A[i_a + 1], B[i_b + 1]);
        #if WRITE_VECTOR == 1
        C[counter++] = partition_size; // write partition size
        #endif
        i_a += A[i_a + 1] + 2;
        i_b += B[i_b + 1] + 2;      

        count += partition_size;
        counter += partition_size;
        notFinished = i_a < s_a && i_b < s_b;
      }
    }

    #if WRITE_VECTOR == 1
    C_size[0] = counter*sizeof(short);
    #endif

    return count;
  }
  template<typename T> 
  inline T sum(std::function<T(uint32_t,uint32_t)> function,uint32_t col,unsigned short *data, size_t length){
    T result = (T) 0;
    for(size_t j = 0; j < length; ++j){
      const size_t header_length = 2;
      const size_t start = j;
      const size_t prefix = data[j++];
      const size_t len = data[j++];
      const size_t partition_end = start+header_length+len;

      //Traverse partition use prefix to get nbr id.
      for(;j < partition_end;++j){
        uint32_t cur = (prefix << 16) | data[j]; //neighbor node
        result += function(col,cur);
      }
      j = partition_end-1;   
    }
    return result;
  }
  inline void decode(uint32_t *result, unsigned short *A, size_t s_a){
    size_t count = 0;
    size_t i = 0;
    while(count < s_a){
      uint32_t prefix = (A[i] << 16);
      unsigned short size = A[i+1];
      //cout << "size: " << size << endl;
      i += 2;

      size_t inner_end = i+size;
      while(i < inner_end){
        uint32_t tmp = prefix | A[i];
        result[count++] = tmp; 
        ++i;
      }
    }
  }
  inline void print_data(unsigned short *A, size_t s_a, std::ofstream &file){
  	//cout << "LEN: " << s_a << endl;
    for(size_t i = 0; i < s_a; i++){
      uint32_t prefix = (A[i] << 16);
      unsigned short size = A[i+1];
      //cout << "size: " << size << endl;
      i += 2;

      size_t inner_end = i+size;
      while(i < inner_end){
        uint32_t tmp = prefix | A[i];
        file << " Data: " << tmp << endl;
        ++i;
      }
      i--;
    }
  }
} 