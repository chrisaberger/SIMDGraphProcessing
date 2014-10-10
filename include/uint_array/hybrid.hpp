#include "bitset.hpp"
#include "array32.hpp"
#include "array16.hpp"
#include "a32bitpacked.hpp"

namespace hybrid {
  //untested
  inline size_t intersect_a16_bs(unsigned int *C, const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
    (void)C;
    (void)s_b;

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
          C[count] = cur;
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
  inline size_t intersect_a32_bs(unsigned int *C, const unsigned int *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
    (void)C;
    (void)s_b;
    
    //cout << "32 & BS" << endl;
    size_t count = 0;
    for(size_t i = 0; i < s_a; i++){
      unsigned int cur = A[i];
      if(bitset::is_set(cur,B)){
        #if WRITE_VECTOR == 1
        C[count] = cur;
        #endif
        count++;
      }
    }
    return count;
  }
  inline size_t intersect_a32_a16(unsigned int *C, const unsigned int *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
    #if WRITE_VECTOR == 0
    (void)C;
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
    return count;
  }
} 