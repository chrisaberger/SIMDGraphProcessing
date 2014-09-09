#include "Common.hpp"
#include <x86intrin.h>
#include "Array16.hpp"
#include "Array32.hpp"

using namespace std;

namespace hybrid {
  static __m128i shuffle_mask16[256]; // precomputed dictionary
  
  static inline int getBitSD(unsigned int value, unsigned int position) {
    return ( ( value & (1 << position) ) >> position);
  }

  static inline void prepare_shuffling_dictionary16() {
    //Number of bits that can possibly be set are the lower 8
    for(unsigned int i = 0; i < 256; i++) { // 2^8 possibilities we need to store masks for
      unsigned int counter = 0;
      unsigned char permutation[16];
      memset(permutation, 0xFF, sizeof(permutation));
      for(unsigned char b = 0; b < 8; b++) { //Check each possible bit that can be set 1-by-1
        if(getBitSD(i, b)) {
          permutation[counter++] = 2*b; //tell us byte offset to get from comparison vector
          permutation[counter++] = 2*b + 1; //tess us byte offset to get from comparison vector
        }
      }
      __m128i mask = _mm_loadu_si128((const __m128i*)permutation);
      shuffle_mask16[i] = mask;
    }
  }

	#define SHORTS_PER_REG 8

  inline common::type select_type(size_t s_a){
    if(s_a > 16){
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
      unsigned short b_size = B[b_i++];
      unsigned int cur_match = A_32[a_i];
      size_t inner_end = b_i+b_size;


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
          #if VECTORIZE == 1

          bool a_continue = (a_i+SHORTS_PER_REG) < a_size && (A_32[a_i+SHORTS_PER_REG-1] & 0xFFFF0000) == prefix;
          size_t st_b = (b_size / SHORTS_PER_REG) * SHORTS_PER_REG;   
          while(a_continue && b_i < st_b) {
            unsigned short *a_loader = new unsigned short[SHORTS_PER_REG];
            for(size_t ali = 0; ali < SHORTS_PER_REG; ali++){
              a_loader[ali] = A_32[a_i+ali] & 0x0000FFFF;
            }
            __m128i v_a = _mm_loadu_si128((__m128i*)&a_loader[0]);
            __m128i v_b = _mm_loadu_si128((__m128i*)&B[b_i]);    
            delete[] a_loader;

            /*
            uint16_t *t = (uint16_t*) &v_a;
            cout << "Data: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << " " << t[4] << " " << t[5] << " " << t[6] << " " << t[7] << endl;
            t = (uint16_t*) &v_b;
            cout << "Data: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << " " << t[4] << " " << t[5] << " " << t[6] << " " << t[7] << endl;    
            */
            unsigned short a_max = _mm_extract_epi16(v_a, SHORTS_PER_REG-1);
            unsigned short b_max = _mm_extract_epi16(v_b, SHORTS_PER_REG-1);
            
            __m128i res_v = _mm_cmpestrm(v_b, SHORTS_PER_REG, v_a, SHORTS_PER_REG,
                    _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
            unsigned int r = _mm_extract_epi32(res_v, 0);

            #if WRITE_VECTOR == 1
            __m128i p = _mm_shuffle_epi8(v_a, shuffle_mask16[r]);
            _mm_storeu_si128((__m128i*)&C[count], p);
           #endif

            count += _mm_popcnt_u32(r);
            
            a_i += (a_max <= b_max) * SHORTS_PER_REG;
            a_continue = (a_i+SHORTS_PER_REG) < a_size && (A_32[a_i+SHORTS_PER_REG-1] & 0xFFFF0000) == prefix;
            b_i += (a_max >= b_max) * SHORTS_PER_REG;
          }
          #endif

          unsigned int cur = prefix | B[b_i];
          //cout << cur << " " << cur_match << " " << a_i << " " << a_size << endl;
          while(cur_match < cur && (a_i+1) < a_size){
            cur_match = A_32[++a_i];
            //cout << "inner: " << cur_match << endl;
          }
          if(cur_match == cur){
            //place data
            #if WRITE_VECTOR == 1
            C[count] = cur;
            #endif
            cur_match = A_32[++a_i];
            count++;
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