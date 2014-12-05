#ifndef _UNION_H_
#define _UNION_H_

namespace ops{
  inline Set<uinteger> set_difference(const Set<uinteger> &C_in, const Set<bitset> &A_in, const Set<bitset> &B_in){
    uint8_t * const C = C_in.data;
    const uint8_t * const A = A_in.data;
    const uint8_t * const B = B_in.data;
    const size_t s_a = A_in.number_of_bytes;
    const size_t s_b = B_in.number_of_bytes;

    #if WRITE_VECTOR == 0
    (void) C;
    #endif

    long count = 0l;
    const uint8_t *small = (s_a > s_b) ? B : A;
    const size_t small_length = (s_a > s_b) ? s_b : s_a;
    const uint8_t *large = (s_a <= s_b) ? B : A;

    //16 uint16_ts
    //8 ints
    //4 longs
    size_t i = 0;
    
    #if VECTORIZE == 1
    while((i+15) < small_length){
      __m128i a1 = _mm_loadu_si128((const __m128i*)&A[i]);
      __m128i a2 = _mm_loadu_si128((const __m128i*)&B[i]);
      
      __m128i r = _mm_andnot_si128(a2, a1);
      
      #if WRITE_VECTOR == 1
      _mm_storeu_si128((__m128i*)&C[i], r);
      #endif
      
      unsigned long l = _mm_extract_epi64(r,0);
      count += _mm_popcnt_u64(l);
      l = _mm_extract_epi64(r,1);
      count += _mm_popcnt_u64(l);

      i += 16;
    }
    #endif

    for(; i < small_length; i++){
      uint8_t result = small[i] | ~(large[i]);

      #if WRITE_VECTOR == 1
      C[i] = result;
      #endif
      
      count += _mm_popcnt_u32(result);
    }
    const double density = (count > 0) ? (sizeof(uint8_t)*(small_length-0))/count : 0.0;
    return Set<bitset>(C_in.data,count,small_length,density,common::BITSET);
  }
  /*
  inline Set<hybrid> set_intersect(const Set<hybrid> &C_in,const Set<hybrid> &A_in,const Set<hybrid> &B_in){
    switch (A_in.type) {
        case common::UINTEGER:
          switch (B_in.type) {
            case common::UINTEGER:
              return set_intersect((const Set<uinteger>&)C_in,(const Set<uinteger>&)A_in,(const Set<uinteger>&)B_in);
            break;
            case common::PSHORT:
              return set_intersect((const Set<uinteger>&)C_in,(const Set<uinteger>&)A_in,(const Set<pshort>&)B_in);
            break;
            case common::BITSET:
              return set_intersect((const Set<uinteger>&)C_in,(const Set<uinteger>&)A_in,(const Set<bitset>&)B_in);
            break;
            default:
            break;
          }
        break;
        case common::PSHORT:
          switch (B_in.type) {
            case common::UINTEGER:
              return set_intersect((const Set<uinteger>&)C_in,(const Set<uinteger>&)B_in,(const Set<pshort>&)A_in);
            break;
            case common::PSHORT:
              return set_intersect((const Set<pshort>&)C_in,(const Set<pshort>&)A_in,(const Set<pshort>&)B_in);
            break;
            case common::BITSET:
              return set_intersect((const Set<pshort>&)C_in,(const Set<pshort>&)A_in,(const Set<bitset>&)B_in);
            break;
            default:
            break;
          }
        break;
        case common::BITSET:
          switch (B_in.type) {
            case common::UINTEGER:
              return set_intersect((const Set<uinteger>&)C_in,(const Set<uinteger>&)B_in,(const Set<bitset>&)A_in);
            break;
            case common::PSHORT:
              return set_intersect((const Set<pshort>&)C_in,(const Set<pshort>&)B_in,(const Set<bitset>&)A_in);
            break;
            case common::BITSET:
              return set_intersect((const Set<bitset>&)C_in,(const Set<bitset>&)A_in,(const Set<bitset>&)B_in);
            break;
            default:
            break;
          }
        break;
        default:
        break;
    }

    cout << "ERROR" << endl;
    return C_in;
  }
  */
}

#endif
