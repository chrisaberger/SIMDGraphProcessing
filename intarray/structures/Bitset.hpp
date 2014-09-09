#include "Common.hpp"
#include <x86intrin.h>

using namespace std;

namespace Bitset {
	inline size_t preprocess(unsigned short *R, size_t index, unsigned int *A, size_t s_a){
		prepare_shuffling_dictionary16();
	  unsigned short high = 0;
	  size_t partition_length = 0;
	  size_t partition_size_position = index+1;
	  size_t counter = index;
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
	  return counter;
	}
	inline size_t intersect(unsigned short *C,const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
	  size_t i_a = 0, i_b = 0;
	  size_t counter = 0;
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
	  return count;
	}
    //This forward reference is kind of akward but needed for Matrix traversals.
  template<typename T> 
  inline T foreach(T (*function)(unsigned int,unsigned int),unsigned int col,unsigned short *data, size_t length){
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
  void print_data(unsigned short *A, size_t s_a){
    for(size_t i = 0; i < s_a; i++){
      unsigned int prefix = (A[i] << 16);
      unsigned short size = A[i+1];
      cout << "size: " << size << endl;
      i += 2;

      size_t inner_end = i+size;
      while(i < inner_end){
        unsigned int tmp = prefix | A[i];
        cout << "Data: " << tmp << endl;
        ++i;
      }
      i--;
    }
  }

	template<typename T> 
	T reduce(unsigned short *data, size_t length,T (*function)(T,T), T zero){
		unsigned int *actual_data = (unsigned int*) data;
		T result = zero;
		for(size_t i = 0; i < length; i++){
			result = function(result,actual_data[i]);
		}	
		return result;
	}

} 