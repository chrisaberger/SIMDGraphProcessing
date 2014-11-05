#include "common.hpp"

namespace array16 {  
  static __m128i shuffle_mask16[256]; // precomputed dictionary
  static inline int getBitSD(unsigned int value, unsigned int position) {
    return ( ( value & (1 << position) ) >> position);
  }
  static char max = char(0xff);
  static __m128i shuffle_mask16_low[16] = {        
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //0
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,1,0), //1
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,3,2), //2
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,3,2,max,max,1,0), //3
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,5,4), //4
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,5,4,max,max,1,0), //5
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,5,4,max,max,3,2), //6
    _mm_set_epi8(max,max,max,max,max,max,5,4,max,max,3,2,max,max,1,0), //7
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,7,6), //8
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,7,6,max,max,1,0), //9
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,7,6,max,max,3,2), //10
    _mm_set_epi8(max,max,max,max,max,max,7,6,max,max,3,2,max,max,0,1), //11
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,7,6,max,max,5,4), //12
    _mm_set_epi8(max,max,max,max,max,max,7,6,max,max,5,4,max,max,1,0), //13
    _mm_set_epi8(max,max,max,max,max,max,7,6,max,max,5,4,max,max,3,2), //14
    _mm_set_epi8(max,max,7,6,max,max,5,4,max,max,3,2,max,max,1,0) //15
  }; 
  static __m128i shuffle_mask16_high[16] = {        
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,max,max), //0
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,9,8), //1
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,11,10), //2
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,11,10,max,max,9,8), //3
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,13,12), //4
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,13,12,max,max,9,8), //5
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,13,12,max,max,11,10), //6
    _mm_set_epi8(max,max,max,max,max,max,13,12,max,max,11,10,max,max,9,8), //7
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,max,max,max,max,15,14), //8
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,15,14,max,max,9,8), //9
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,15,14,max,max,11,10), //10
    _mm_set_epi8(max,max,max,max,max,max,15,14,max,max,11,10,max,max,9,8), //11
    _mm_set_epi8(max,max,max,max,max,max,max,max,max,max,15,14,max,max,13,12), //12
    _mm_set_epi8(max,max,max,max,max,max,15,14,max,max,13,12,max,max,9,8), //13
    _mm_set_epi8(max,max,max,max,max,max,15,14,max,max,13,12,max,max,11,10), //14
    _mm_set_epi8(max,max,15,14,max,max,13,12,max,max,11,10,max,max,9,8) //15
  }; 

  static inline void prepare_shuffling_dictionary16() {
    //Number of bits that can possibly be set are the lower 8
    for(unsigned int i = 0; i < 256; i++) { // 2^8 possibilities we need to store masks for
      unsigned int counter = 0;
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
	inline size_t preprocess(unsigned short *R, unsigned int *A, size_t s_a){
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

	inline size_t simd_intersect_vector16(unsigned int prefix, unsigned int *C, const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
	  #if WRITE_VECTOR == 0
    (void)C;
    #endif
    
    size_t count = 0;
	  size_t i_a = 0, i_b = 0;

    #if VECTORIZE == 1
	  size_t st_a = (s_a / SHORTS_PER_REG) * SHORTS_PER_REG;
	  size_t st_b = (s_b / SHORTS_PER_REG) * SHORTS_PER_REG;
 
    __m128i prefix_reg = _mm_set1_epi32(prefix);

	  while(i_a < st_a && i_b < st_b) {
	    __m128i v_a = _mm_loadu_si128((__m128i*)&A[i_a]);
	    __m128i v_b = _mm_loadu_si128((__m128i*)&B[i_b]);    

	    unsigned short a_max = _mm_extract_epi16(v_a, SHORTS_PER_REG-1);
	    unsigned short b_max = _mm_extract_epi16(v_b, SHORTS_PER_REG-1);
	    
	   // __m128i res_v = _mm_cmpistrm(v_b, v_a,
	   //         _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
	    __m128i res_v = _mm_cmpestrm(v_b, SHORTS_PER_REG, v_a, SHORTS_PER_REG,
	            _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
	    
      unsigned int r = _mm_extract_epi32(res_v, 0);

      #if WRITE_VECTOR == 1
      unsigned int r_lower = r & 0x00ff;
	    __m128i p = _mm_shuffle_epi8(v_a, shuffle_mask16_low[r_lower]);
      p = _mm_or_si128(p,prefix_reg);
	    _mm_storeu_si128((__m128i*)&C[count], p);
      count += _mm_popcnt_u32(r_lower);

      unsigned int r_upper = ((r & 0xff00) >> 8);
      p = _mm_shuffle_epi8(v_a, shuffle_mask16_low[r_upper]);
      p = _mm_or_si128(p,prefix_reg);
      _mm_storeu_si128((__m128i*)&C[count], p);
      count += _mm_popcnt_u32(r_upper);
	   #else
      prefix_reg = prefix_reg;
	    count += _mm_popcnt_u32(r);
	   #endif

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
	      C[count] = (prefix | (unsigned int) A[i_a]);
        #endif
	      ++count;
	    }
	    ++i_a;
	    notFinished = notFinished && i_a < s_a;
	  }
	  return count;
	}
	inline size_t intersect(unsigned int *C,const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
	  size_t i_a = 0, i_b = 0;
	  size_t count = 0;
	  bool notFinished = i_a < s_a && i_b < s_b;

	  //cout << lim << endl;
	  while(notFinished) {
	    //size_t limLower = limLowerHolder;
	    //cout << "looping" << endl;
	    if(A[i_a] < B[i_b]) {
	      i_a += A[i_a + 1] + 2;
	      notFinished = i_a < s_a;
	    } else if(B[i_b] < A[i_a]) {
	      i_b += B[i_b + 1] + 2;
	      notFinished = i_b < s_b;
	    } else {
	      unsigned short partition_size = 0;
	      //If we are not in the range of the limit we don't need to worry about it.
        unsigned int prefix = ((unsigned int)(A[i_a])) << 16;
        partition_size = simd_intersect_vector16(prefix,&C[count],&A[i_a + 2],&B[i_b + 2],A[i_a + 1], B[i_b + 1]);
        i_a += A[i_a + 1] + 2;
	      i_b += B[i_b + 1] + 2;      

	      count += partition_size;
	      notFinished = i_a < s_a && i_b < s_b;
	    }
	  }
	  return count;
	}
  template<typename T> 
  inline T sum(std::function<T(unsigned int,unsigned int)> function,unsigned int col,unsigned short *data, size_t length){
    T result = (T) 0;
    for(size_t j = 0; j < length; ++j){
      const size_t header_length = 2;
      const size_t start = j;
      const size_t prefix = data[j++];
      const size_t len = data[j++];
      const size_t partition_end = start+header_length+len;

      //Traverse partition use prefix to get nbr id.
      for(;j < partition_end;++j){
        unsigned int cur = (prefix << 16) | data[j]; //neighbor node
        result += function(col,cur);
      }
      j = partition_end-1;   
    }
    return result;
  }
  inline void decode(unsigned int *result, unsigned short *A, size_t s_a){
    size_t count = 0;
    for(size_t i = 0; i < s_a; i++){
      unsigned int prefix = (A[i] << 16);
      unsigned short size = A[i+1];
      //cout << "size: " << size << endl;
      i += 2;

      size_t inner_end = i+size;
      while(i < inner_end){
        unsigned int tmp = prefix | A[i];
        result[count++] = tmp; 
        ++i;
      }
      i--;
    }
  }
  inline void print_data(unsigned short *A, size_t s_a, std::ofstream &file){
  	//cout << "LEN: " << s_a << endl;
    for(size_t i = 0; i < s_a; i++){
      unsigned int prefix = (A[i] << 16);
      unsigned short size = A[i+1];
      //cout << "size: " << size << endl;
      i += 2;

      size_t inner_end = i+size;
      while(i < inner_end){
        unsigned int tmp = prefix | A[i];
        file << " Data: " << tmp << endl;
        ++i;
      }
      i--;
    }
  }
} 