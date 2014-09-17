#include "Common.hpp"
#include <x86intrin.h>
#include "Array16.hpp"
#include "Array32.hpp"
#include "Bitset.hpp"
#include "DeltaCompA32.hpp"

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
    size_t a_i = 0;
    size_t b_i = 0;
    size_t count = 0;

    unsigned int *C_32 = (unsigned int*) C;
    C_32 = C_32; //to get rid of compiler warning
    unsigned int *A_32 = (unsigned int*) A;

    size_t a_size = s_a/2;
    size_t b_size = s_b;
    bool not_finished = a_i < a_size && b_i < b_size;

    while(not_finished){
      unsigned int prefix = (B[b_i] << 16);
      unsigned short b_inner_size = B[b_i+1];
      unsigned int cur_match = A_32[a_i];
      size_t inner_end = b_i+b_inner_size+2;
      //cout << endl;
      //cout << "Bi: " << b_i << " Bsize: " << b_size << " InnerEnd: " << inner_end << endl;

      if(prefix < (cur_match & 0xFFFF0000)){
        //cout << "1" << endl;
        b_i = inner_end;
        not_finished = b_i < b_size;
      } else if(prefix > cur_match){
        //cout << prefix << " " << cur_match << endl;
        //cout << "2" << endl;
        a_i++;
        not_finished = a_i < a_size;
      } else{
        //cout << "3" << endl;
        b_i += 2;

        #if VECTORIZE == 1
        bool a_continue = (a_i+SHORTS_PER_REG) < a_size && (A_32[a_i+SHORTS_PER_REG-1] & 0xFFFF0000) == prefix;
        size_t st_b = (b_inner_size / SHORTS_PER_REG) * SHORTS_PER_REG;
        size_t i_b = 0;
        while(a_continue && i_b < st_b) {
          __m128i v_a_1_32 = _mm_loadu_si128((__m128i*)&A_32[a_i]);
          __m128i v_a_2_32 = _mm_loadu_si128((__m128i*)&A_32[a_i+(SHORTS_PER_REG/2)]);

          __m128i v_a_1 = _mm_shuffle_epi8(v_a_1_32,_mm_set_epi8(0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x0D,0x0C,0x09,0x08,0x05,0x04,0x01,0x0));
          __m128i v_a_2 = _mm_shuffle_epi8(v_a_2_32,_mm_set_epi8(0x0D,0x0C,0x09,0x08,0x05,0x04,0x01,0x0,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80));
          
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
          _mm_storeu_si128((__m128i*)&C_32[count], p);
          
          //uint32_t *t = (uint32_t*) &p;
          //cout << "Data: " << t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;

          p = _mm_shuffle_epi8(v_a_2_32, array32::shuffle_mask32[r_upper]);
          _mm_storeu_si128((__m128i*)&C_32[count+_mm_popcnt_u32(r_lower)], p);
          C_32[count] = A_32[a_i];
          #endif

          count += _mm_popcnt_u32(r);
          a_i += (a_max <= b_max) * SHORTS_PER_REG;
          a_continue = (a_i+SHORTS_PER_REG) < a_size && (A_32[a_i+SHORTS_PER_REG-1] & 0xFFFF0000) == prefix;
          i_b += (a_max >= b_max) * SHORTS_PER_REG;
        }
        #endif

        bool notFinished = a_i < a_size  && i_b < b_inner_size && (A_32[a_i] & 0xFFFF0000) == prefix;
        while(notFinished){
          while(notFinished && (unsigned int)(prefix | B[i_b+b_i]) < A_32[a_i]){
            ++i_b;
            notFinished = i_b < b_inner_size;
          }
          if(notFinished && A_32[a_i] == (unsigned int)(prefix | B[i_b+b_i])){
            #if WRITE_VECTOR == 1
            C_32[count] = A_32[a_i];
            #endif
            ++count;
          }
          ++a_i;
          notFinished = notFinished && a_i < a_size && (A_32[a_i] & 0xFFFF0000) == prefix;
        }
        b_i = inner_end;
        not_finished = a_i < a_size && b_i < b_size;
      }
    }
    return count;
  }
} 