#ifndef _UNION_H_
#define _UNION_H_

namespace ops{
  //this method is untested as of right now....
 inline void set_union(Set<bitset> *A_in, Set<bitset> *B_in){
    if(A_in->number_of_bytes > 0 && B_in->number_of_bytes > 0){
      const uint32_t *a_index = (uint32_t*) A_in->data;
      const uint32_t *b_index = (uint32_t*) B_in->data;
      
      const uint64_t * const A = (uint64_t*)(A_in->data+sizeof(uint64_t));
      const uint64_t * const B = (uint64_t*)(B_in->data+sizeof(uint64_t));
      const size_t s_a = ((A_in->number_of_bytes-sizeof(uint64_t))/sizeof(uint64_t));
      const size_t s_b = ((B_in->number_of_bytes-sizeof(uint64_t))/sizeof(uint64_t));

      const bool a_big = a_index[0] > b_index[0];
      const uint64_t start_index = (a_big) ? a_index[0] : b_index[0];
      const uint64_t a_start_index = (a_big) ? 0:(b_index[0]-a_index[0]);
      const uint64_t b_start_index = (a_big) ? (a_index[0]-b_index[0]):0;

      const uint64_t end_index = ((a_index[0]+s_a) > (b_index[0]+s_b)) ? (b_index[0]+s_b):(a_index[0]+s_a);
      const uint64_t total_size = (start_index > end_index) ? 0:(end_index-start_index);

      //16 uint16_ts
      //8 ints
      //4 longs
      size_t i = 0;
      #if VECTORIZE == 1
      while((i+3) < total_size){
        const __m256 a1 = _mm256_loadu_ps((const float*)&A[i+a_start_index]);
        const __m256 a2 = _mm256_loadu_ps((const float*)&B[i+b_start_index]);
        const __m256 r = _mm256_or_ps(a2, a1);

        uint64_t tmp[4];
        _mm256_storeu_ps((float*)&A[i+a_start_index], r);

        i += 4;
      }
      #endif

      for(; i < total_size; i++){
        A[i+a_start_index] |= B[i+b_start_index];
      }
    }
  }
  inline void set_union(Set<bitset> *A_in,Set<pshort> *B_in){
    uint64_t* A = (uint64_t*)(A_in->data+sizeof(uint64_t));
    const uint32_t * const s_index_p = (uint32_t*)A_in->data;
    const uint32_t start_index = (A_in->number_of_bytes > 0) ? s_index_p[0]:0;

    B_in->foreach( [&A_in,&A,start_index] (uint32_t cur){
      const size_t word_index = bitset::word_index(cur);
      if(!(A[word_index-start_index] & ((uint64_t)1 << (cur % BITS_PER_WORD))))
        __sync_fetch_and_or(&A[word_index-start_index],((uint64_t) 1 << (cur % BITS_PER_WORD)));
    });
  }
  inline void set_union(Set<pshort> *A_in, Set<bitset> *B_in){
    set_union(B_in,A_in);
  }
  inline void set_union(Set<bitset> *A_in,Set<uinteger> *B_in){
    uint64_t* A = (uint64_t*)(A_in->data+sizeof(uint64_t));

    const uint32_t * const s_index_p = (uint32_t*)A_in->data;
    const uint32_t start_index = (A_in->number_of_bytes > 0) ? s_index_p[0]:0;
    size_t new_count = 0;
    B_in->foreach( [&A_in,&A,start_index,&new_count] (uint32_t cur){
      const size_t word_index = bitset::word_index(cur);
      A[word_index-start_index] |= ((uint64_t) 1 << (cur % BITS_PER_WORD));
    });
  }
  inline void set_union(Set<uinteger> *A_in,Set<bitset> *B_in){
    set_union(B_in,A_in);
  }
  inline void set_union(Set<bitset> *A_in,Set<variant> *B_in){
    uint64_t* A = (uint64_t*)(A_in->data+sizeof(uint64_t));
    const uint32_t * const s_index_p = (uint32_t*)A_in->data;
    const uint32_t start_index = (A_in->number_of_bytes > 0) ? s_index_p[0]:0;

    B_in->foreach( [&A_in,&A,start_index] (uint32_t cur){
      const size_t word_index = bitset::word_index(cur);
      if(!(A[word_index-start_index] & ((uint64_t)1 << (cur % BITS_PER_WORD))))
        __sync_fetch_and_or(&A[word_index-start_index],((uint64_t) 1 << (cur % BITS_PER_WORD)));
    });
  }
  inline void set_union(Set<variant> *A_in,Set<bitset> *B_in){
    set_union(B_in,A_in);
  }
  inline void set_union(Set<bitset> *A_in,Set<bitpacked> *B_in){
    uint64_t* A = (uint64_t*)(A_in->data+sizeof(uint64_t));
    const uint32_t * const s_index_p = (uint32_t*)A_in->data;
    const uint32_t start_index = (A_in->number_of_bytes > 0) ? s_index_p[0]:0;

    B_in->foreach( [&A_in,&A,start_index] (uint32_t cur){
      const size_t word_index = bitset::word_index(cur);
      const uint64_t old_value = A[word_index-start_index];
      if(!(old_value & ((uint64_t)1 << (cur % BITS_PER_WORD))))
        __sync_fetch_and_or(&A[word_index-start_index],((uint64_t) 1 << (cur % BITS_PER_WORD)));
      A_in->cardinality += (A[word_index-start_index]!=old_value);
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
