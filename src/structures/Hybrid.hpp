#include "Common.hpp"
#include <x86intrin.h>
#include "Array16.hpp"
#include "Array32.hpp"
#include "Bitset.hpp"

using namespace std;

namespace hybrid {
  //untested
  inline size_t intersect_a16_bs(unsigned short *C, const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
    //cout << "16 & BS" << endl;
    size_t count = 0;
    for(size_t i = 0; i < s_a; i++){
      unsigned int prefix = (A[i] << 16);
      unsigned short size = A[i+1];
      i += 2;

      size_t inner_end = i+size;
      while(i < inner_end){
        unsigned int cur = prefix | A[i];
        if(bitset::is_set(cur,B)){
          #if WRITE_VECTOR == 1
          C[count*2] = cur;
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
  inline size_t intersect_a32_bs(unsigned short *C, const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
    //cout << "32 & BS" << endl;
    size_t count = 0;
    unsigned int *A_32 = (unsigned int*) A;
    size_t a_size = s_a/2;

    for(size_t i = 0; i < a_size; i++){
      unsigned int cur = A_32[i];
      if(bitset::is_set(cur,B)){
        #if WRITE_VECTOR == 1
        C[count*2] = cur;
        #endif
        count++;
      }
    }
    return count;
  }
	inline size_t intersect_a32_a16(unsigned short *C, const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
    //cout << "32 & 16" << endl;
    size_t a_i = 0;
    size_t b_i = 0;
    size_t count = 0;

    unsigned int *C_32 = (unsigned int*) C;
    C_32 = C_32; //to get rid of compiler warning
    unsigned int *A_32 = (unsigned int*) A;

    size_t a_size = s_a/2;
    bool not_finished = a_i < a_size && b_i < s_b;

    while(not_finished){
      unsigned int prefix = (B[b_i] << 16);
      unsigned short b_size = B[b_i+1];
      unsigned int cur_match = A_32[a_i];
      size_t inner_end = b_i+b_size+2;
      //cout << endl;
      //cout << "Bi: " << b_i << " Bsize: " << b_size << " InnerEnd: " << inner_end << endl;

      if(prefix < (cur_match & 0xFFFF0000)){
        //cout << "1" << endl;
        b_i = inner_end;
        not_finished = b_i < s_b;
      } else if(prefix > cur_match){
        //cout << prefix << " " << cur_match << endl;
        //cout << "2" << endl;
        a_i++;
        not_finished = a_i < a_size;
      } else{
        //cout << "3" << endl;
        b_i += 2;
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

            unsigned short a_max = _mm_extract_epi16(v_a, SHORTS_PER_REG-1);
            unsigned short b_max = _mm_extract_epi16(v_b, SHORTS_PER_REG-1);
            
            __m128i res_v = _mm_cmpestrm(v_b, SHORTS_PER_REG, v_a, SHORTS_PER_REG,
                    _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
            unsigned int r = _mm_extract_epi32(res_v, 0);

            #if WRITE_VECTOR == 1
            __m128i p = _mm_shuffle_epi8(v_a, array16::shuffle_mask16[r]);
            uint16_t *t = (uint16_t*) &p;
            for(size_t wi=0;wi<SHORTS_PER_REG;wi++){
              C_32[count+wi] = prefix | (unsigned int)t[wi]; 
            }
           #endif

            count += _mm_popcnt_u32(r);
            
            a_i += (a_max <= b_max) * SHORTS_PER_REG;
            a_continue = (a_i+SHORTS_PER_REG) < a_size && (A_32[a_i+SHORTS_PER_REG-1] & 0xFFFF0000) == prefix;
            b_i += (a_max >= b_max) * SHORTS_PER_REG;
          }
          #endif

          unsigned int cur = prefix | B[b_i];
          while(cur_match < cur && (a_i+1) < a_size){
            cur_match = A_32[++a_i];
          }
          if(cur_match == cur){
            //place data
            #if WRITE_VECTOR == 1
            C_32[count] = cur;
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
    }
    return count;
	}
} 