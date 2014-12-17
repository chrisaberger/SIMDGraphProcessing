#ifndef _UNION_H_
#define _UNION_H_

namespace ops{
 inline void set_union(Set<bitset> *A_in, Set<bitset> *B_in){
    uint8_t * const A = A_in->data;
    const uint8_t * const B = B_in->data;
    const size_t s_a = A_in->number_of_bytes;
    const size_t s_b = B_in->number_of_bytes;

    const uint8_t *small = (s_a > s_b) ? B : A;
    const size_t small_length = (s_a > s_b) ? s_b : s_a;
    const uint8_t *large = (s_a <= s_b) ? B : A;

    //16 uint16_ts
    //8 ints
    //4 longs
    size_t i = 0;

    #if VECTORIZE == 1
    for(; (i + 15) < small_length; i += 16) {
      __m128i a1 = _mm_lddqu_si128((const __m128i*)&A[i]);
      const __m128i a2 = _mm_lddqu_si128((const __m128i*)&B[i]);
      a1 = _mm_or_si128(a1, a2);

      const uint64_t l = _mm_extract_epi64(a1,0);
      const uint64_t l2 = _mm_extract_epi64(a1,1);
        __sync_fetch_and_or((uint64_t*)&A[i],l);
        __sync_fetch_and_or((uint64_t*)&A[i+8],l2);
    }
    #endif

    for(; i < small_length; i++){
      const uint8_t result = small[i] | large[i];
      __sync_fetch_and_or(&A[i],result);
    }
  }
  inline void set_union(Set<bitset> *A_in,Set<pshort> *B_in){
    uint64_t* A = (uint64_t*)(A_in->data);
    B_in->foreach( [&A] (uint32_t cur){
      const size_t word_index = bitset::word_index(cur);
      __sync_fetch_and_or(&A[word_index],((uint64_t) 1 << (cur % BITS_PER_WORD)));
    });
  }
  inline void set_union(Set<pshort> *A_in, Set<bitset> *B_in){
    set_union(B_in,A_in);
  }
  inline void set_union(Set<bitset> *A_in,Set<uinteger> *B_in){
    uint64_t* A = (uint64_t*)(A_in->data);
    B_in->foreach( [&A] (uint32_t cur){
      const size_t word_index = bitset::word_index(cur);

#ifdef ENABLE_ATOMIC_UNION
      __sync_fetch_and_or(&A[word_index], ((uint64_t) 1 << (cur % BITS_PER_WORD)));
#else
      A[word_index] |= ((uint64_t) 1 << (cur % BITS_PER_WORD));
#endif
    });
  }
  inline void set_union(Set<uinteger> *A_in,Set<bitset> *B_in){
    set_union(B_in,A_in);
  }
  inline void set_union(Set<bitset> *A_in,Set<variant> *B_in){
    uint64_t* A = (uint64_t*)(A_in->data);
    B_in->foreach( [&A] (uint32_t cur){
      const size_t word_index = bitset::word_index(cur);
      __sync_fetch_and_or(&A[word_index],((uint64_t) 1 << (cur % BITS_PER_WORD)));
    });
  }
  inline void set_union(Set<variant> *A_in,Set<bitset> *B_in){
    set_union(B_in,A_in);
  }
  inline void set_union(Set<bitset> *A_in,Set<bitpacked> *B_in){
    uint64_t* A = (uint64_t*)(A_in->data);
    B_in->foreach( [&A] (uint32_t cur){
      const size_t word_index = bitset::word_index(cur);
      __sync_fetch_and_or(&A[word_index],((uint64_t) 1 << (cur % BITS_PER_WORD)));
    });
  }
  inline void set_union(Set<bitpacked> *A_in,Set<bitset> *B_in){
    return set_union(B_in,A_in);
  }
  inline void set_union(Set<bitset> *A_in,Set<hybrid> *B_in){
    switch(B_in->type){
      case common::UINTEGER:
        set_union(A_in,(Set<uinteger>*)B_in);
      break;
      case common::PSHORT:
        set_union(A_in,(Set<pshort>*)B_in);
      break;
      case common::BITSET:
        set_union(A_in,(Set<bitset>*)B_in);
      break;
      case common::VARIANT:
        set_union(A_in,(Set<variant>*)B_in);
      break;
      case common::BITPACKED:
        set_union(A_in,(Set<bitpacked>*)B_in);
      break;
    }
  }
}

#endif
