#include "Common.hpp"
#include <x86intrin.h>

using namespace std;

#define BITS_PER_WORD 16
#define ADDRESS_BITS_PER_WORD 4

namespace bitset {
	inline size_t word_index(unsigned int bit_index){
  	return bit_index >> ADDRESS_BITS_PER_WORD;
	}
	inline int get_bit(unsigned int value, unsigned int position) {
    return ( ( value & (1 << position) ) >> position);
	}
	inline bool is_set(unsigned int index, const unsigned short *in_array){
  	return (in_array[word_index(index)] & (1 << (index%16)));
	}
	inline size_t preprocess(unsigned short *R, size_t index, unsigned int *A, size_t s_a){
		unsigned int max = A[s_a-1];
		size_t num_words = word_index(max);
		cout << "Max Word Index: " << num_words << endl;
		if(s_a != 0){
			num_words++;
		}

	  size_t i = 0;
	  while(i<s_a){
	    unsigned int cur = A[i];
	    cout << "Input Cur: " << cur << endl;
	    size_t word = word_index(cur);
	    unsigned short set_value = 1 << (cur % BITS_PER_WORD);
	    bool same_word = true;
	    ++i;
	    while(i<s_a && same_word){
	      if(word_index(A[i])==word){
	        cur = A[i];
	       	cout << "Input Cur: " << cur << endl;
	        set_value |= (1 << (cur%BITS_PER_WORD));
	        ++i;
	      } else same_word = false;
	    }
	    cout << "Setting word: " << word << " Value: " << set_value << endl;
	    R[index+word] = set_value; 
	  }
	  cout << "Length: " << index+num_words << endl;
	  return index + num_words;
	}
	inline size_t intersect(unsigned short *C, unsigned short *A, unsigned short *B, const size_t s_a, const size_t s_b) {
	  long count = 0l;
	  unsigned short *small = A;
	  size_t small_length = s_a;
	  unsigned short *large = B;
	  if(s_a > s_b){
	  	large = A;
	  	small = B;
	  	small_length = s_b;
	  }

	  //16 unsigned shorts
	  //8 ints
	  //4 longs
	  size_t i = 0;

	  while((i+7) < small_length){
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
	  for(; i < small_length; i++){
	  	unsigned short result = small[i] & large[i];

	  	#if WRITE_VECTOR == 1
	  	C[i] = result;
	   	#endif
	  	
	  	count += _mm_popcnt_u32(result);
	  }
	  return count;
	}
	template<typename T> 
  inline T foreach(T (*function)(unsigned int,unsigned int),unsigned int col,unsigned short *data, size_t length){
    T result = (T) 0;
    cout << "Column: " << col << endl;
    for(size_t i = 0; i < length; i++){
    	cout << i << " len: " << length << endl; 
    	unsigned short cur_word = data[i];
    	cout << "Cur word: " << cur_word << endl;
    	for(size_t j = 0; j < BITS_PER_WORD; j++){
    		if((cur_word >> j) % 2){
    			unsigned int cur = BITS_PER_WORD*i + j;
    			cout << "Cur: " << cur << endl;
    			result += function(col,cur);
    		}
    	}
    }
    return result;
  }
  void print_data(unsigned short *A, size_t s_a){
  	cout << "Size: " << s_a << endl;
    for(size_t i = 0; i < s_a; i++){
    	unsigned short cur_word = A[i];
    	for(size_t j = 0; j < BITS_PER_WORD; j++){
    		if((cur_word >> j) % 2)
    			cout << "Data: " << BITS_PER_WORD*i + j << endl;
    	}
    }
  }
} 