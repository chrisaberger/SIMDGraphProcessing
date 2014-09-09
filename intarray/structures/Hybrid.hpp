#include "Common.hpp"
#include <x86intrin.h>
#include "Array16.hpp"
#include "Array32.hpp"

using namespace std;

#define VECTORIZE 1
#define WRITE_VECTOR 1

namespace hybrid {
	#define SHORTS_PER_REG 8

  inline common::type select_type(size_t s_a){
    if(s_a > 1){
      return common::ARRAY16;
    }else{
      return common::ARRAY32;
    }
  }

	inline size_t preprocess(unsigned short *R, size_t index, unsigned int *A, size_t s_a){
    common::type t = select_type(s_a);
    switch(t){
      case common::ARRAY32:
        index = array32::preprocess(R,index,A,s_a);
        break;
      case common::ARRAY16:
        index = array16::preprocess(R,index,A,s_a);
        break;
      case common::BITSET:
        break;
      default:
        break;
    }
    return index;
	}

	inline size_t intersect_a32_a16(unsigned short *C, const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
    size_t a_i = 0;
    size_t b_i = 0;
    size_t count = 0;

    unsigned int *A_32 = (unsigned int*) A;
    size_t a_size = s_a/2;
    bool not_finished = a_i < a_size && b_i < s_b;

    while(not_finished){
      unsigned int prefix = (B[b_i++] << 16);
      unsigned short size = B[b_i++];
      unsigned int cur_match = A_32[a_i];
      size_t inner_end = b_i+size;


      if(prefix < (cur_match & 0xFFFF0000)){
        //cout << "1" << endl;
        b_i = inner_end;
        not_finished = b_i < s_b;
      } else if(prefix > cur_match){
        //cout << "2" << endl;
        not_finished = ++a_i < a_size;
      } else{
        //cout << "3" << endl;
        while(not_finished){
          unsigned int cur = prefix | B[b_i];
          if(cur_match == cur){
            //place data
            #if WRITE_VECTOR == 1
            C[count] = cur;
            #endif
            count++;
            ++a_i;
          } 
          ++b_i;
          not_finished = a_i < a_size && b_i < inner_end;
        }
        b_i = inner_end;
        not_finished = a_i < a_size && b_i < s_b;
      }
      //cout << "end while" << endl;
    }
    return count;
	}
	inline size_t intersect(unsigned short *C,const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b, unsigned int ls_a, unsigned int ls_b) {
	  size_t count = 0;

    common::type t1 = select_type(ls_a);
    common::type t2 = select_type(ls_b);

    if(t1 == common::ARRAY32){
      if(t2 == common::ARRAY32){
        count = array32::intersect((unsigned int*)C,(unsigned int*)A,(unsigned int*)B,s_a/2,s_b/2);
      }
      else if(t2 == common::ARRAY16){
        count = intersect_a32_a16(C,A,B,s_a,s_b);
      }
    } else if(t1 == common::ARRAY16){
      if(t2 == common::ARRAY32){
        count = intersect_a32_a16(C,B,A,s_b,s_a);
      }
      else if(t2 == common::ARRAY16){
        count = array16::intersect(C,A,B,s_a,s_b);
      }
    }

	  return count;
	}
    //This forward reference is kind of akward but needed for Matrix traversals.
  template<typename T> 
  inline T foreach(T (*function)(unsigned int,unsigned int),unsigned int col,unsigned short *data, size_t length, unsigned int size){
    common::type t = select_type(size);
    switch(t){
      case common::ARRAY32:
        return array32::foreach(function,col,data,length);
        break;
      case common::ARRAY16:
        return array16::foreach(function,col,data,length);
        break;
      case common::BITSET:
        return (T) 0;
        break;
      default:
        return (T) 0;
        break;
    }
  }
  void print_data(unsigned short *A, size_t s_a, unsigned int size){
    common::type t = select_type(size);
    switch(t){
      case common::ARRAY32:
        array32::print_data(A,s_a);
        break;
      case common::ARRAY16:
        array16::print_data(A,s_a);
        break;
      case common::BITSET:
        break;
      default:
        break;
    }
  }
} 