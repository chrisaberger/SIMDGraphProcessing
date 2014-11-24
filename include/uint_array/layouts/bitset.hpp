#include "common.hpp"

#define BITS_PER_WORD 8
#define ADDRESS_BITS_PER_WORD 3

namespace bitset {
	inline size_t word_index(uint32_t bit_index){
  	return bit_index >> ADDRESS_BITS_PER_WORD;
	}
	inline int get_bit(uint32_t value, uint32_t position) {
    return ( ( value & (1 << position) ) >> position);
	}
	inline bool is_set(uint32_t index, const uint8_t *in_array){
  	return (in_array[word_index(index)] & (1 << (index%BITS_PER_WORD)));
	}
	inline size_t preprocess(uint8_t *R, uint32_t *A, size_t s_a){
		uint32_t max = A[s_a-1];
		size_t num_words = word_index(max);
		if(s_a != 0){
			num_words++;
		}

	  size_t i = 0;
	  while(i<s_a){
	    uint32_t cur = A[i];
	    size_t word = word_index(cur);
	    uint8_t set_value = 1 << (cur % BITS_PER_WORD);
	    bool same_word = true;
	    ++i;
	    while(i<s_a && same_word){
	      if(word_index(A[i])==word){
	        cur = A[i];
	        set_value |= (1 << (cur%BITS_PER_WORD));
	        ++i;
	      } else same_word = false;
	    }
	    R[word] = set_value; 
	  }
	  return num_words;
	}
	inline size_t intersect(uint8_t *C_in, uint8_t *A, uint8_t *B, const size_t s_a, const size_t s_b) {
    #if WRITE_VECTOR == 0
    (void) C_in;
    #endif

    #if WRITE_VECTOR == 1
    size_t *C_size = (size_t*)&C_in[1];
    C_in[0] = common::BITSET;
    uint8_t *C = (uint8_t*)&C_in[sizeof(size_t)+1];
    #endif

	  long count = 0l;
	  uint8_t *small = A;
	  size_t small_length = s_a;
	  uint8_t *large = B;
	  if(s_a > s_b){
	  	large = A;
	  	small = B;
	  	small_length = s_b;
	  }

	  //16 unsigned shorts
	  //8 ints
	  //4 longs
	  size_t i = 0;
	  
    #if VECTORIZE == 1
	  while((i+15) < small_length){
	    __m128i a1 = _mm_loadu_si128((const __m128i*)&A[i]);
	    __m128i a2 = _mm_loadu_si128((const __m128i*)&B[i]);
	    
	    __m128i r = _mm_and_si128(a1, a2);
      
      #if WRITE_VECTOR == 1
	    _mm_storeu_si128((__m128i*)&C[i], r);
	   	#endif
	    
	    unsigned long l = _mm_extract_epi64(r,0);
	    count += _mm_popcnt_u64(l);
	    l = _mm_extract_epi64(r,1);
	    count += _mm_popcnt_u64(l);

	    i += 8;
	  }
	  #endif

	  for(; i < small_length; i++){
	  	unsigned short result = small[i] & large[i];

	  	#if WRITE_VECTOR == 1
	  	C[i] = result;
	   	#endif
	  	
	  	count += _mm_popcnt_u32(result);
	  }

    #if WRITE_VECTOR == 1
    C_size[0] = i*sizeof(short);
    #endif

	  return count;
	}
	template<typename T> 
  inline T sum(std::function<T(uint32_t,uint32_t)> function,uint32_t col,uint8_t *data, size_t length){
    T result = (T) 0;
    for(size_t i = 0; i < length; i++){
    	uint8_t cur_word = data[i];
    	for(size_t j = 0; j < BITS_PER_WORD; j++){
    		if((cur_word >> j) % 2){
    			uint32_t cur = BITS_PER_WORD*i + j;
    			result += function(col,cur);
    		}
    	}
    }
    return result;
  }
  inline void decode(uint32_t *result, uint8_t *A, size_t s_a){
    size_t count = 0;
    size_t i = 0;
    while(count < s_a){
      uint8_t cur_word = A[i];
      for(size_t j = 0; j < BITS_PER_WORD; j++){
        if((cur_word >> j) % 2)
          result[count++] = BITS_PER_WORD*i + j ;
      }
      i++;
    }
  }
  inline void print_data(uint8_t *A, size_t s_a, ofstream &file){
  	cout << "Size: " << s_a << endl;
    for(size_t i = 0; i < s_a; i++){
    	uint8_t cur_word = A[i];
    	for(size_t j = 0; j < BITS_PER_WORD; j++){
    		if((cur_word >> j) % 2)
    			file << " Data: " << BITS_PER_WORD*i + j << endl;
    	}
    }
  }
} 