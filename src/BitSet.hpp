#ifndef BITSET_H
#define BITSET_H
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <stdlib.h>     /* atoi */
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <x86intrin.h>

#define BITS_PER_WORD 16
#define ADDRESS_BITS_PER_WORD 4

using namespace std;

inline size_t wordIndex(unsigned short bitIndex){
  return bitIndex >> ADDRESS_BITS_PER_WORD;
}
inline void addToBitSet(unsigned short bitIndex, unsigned short *in_array){
  size_t word = wordIndex(bitIndex);
  in_array[word] |= (1 << (bitIndex % BITS_PER_WORD));
}

inline void createBitSet(unsigned short *in_array, const size_t len){
  unsigned short *tmp = new unsigned short[len];

  //Parallel copy & clear to tmp array.
  #pragma omp parallel for default(none) shared(tmp,in_array)
  for(size_t j=0;j<len;++j){
    tmp[j] = in_array[j];
    in_array[j] = 0;
  }

  size_t i = 0;
  while(i<len){
    unsigned short cur = tmp[i];
    size_t word = wordIndex(cur);
    unsigned short setValue = 1 << (cur % BITS_PER_WORD);
    bool sameWord = true;
    ++i;
    while(i<len && sameWord){
      if(wordIndex(tmp[i])==word){
        cur = tmp[i];
        setValue |= (1 << (cur%BITS_PER_WORD));
        ++i;
      } else sameWord = false;
    }
    in_array[word] = setValue; 
  }
  delete [] tmp;
}
inline long andCardinalityInRange(unsigned short *in_array1, unsigned short *in_array2, size_t max){
  long count = 0l;
  size_t smallLength = wordIndex(max);

  //16 unsigned shorts
  //8 ints
  //4 longs

  size_t i = 0;
  while((i+15) < smallLength){
    __m256i a1 = _mm256_load_si256((const __m256i*)&in_array1[i]);
    __m256i a2 = _mm256_load_si256((const __m256i*)&in_array2[i]);
    __m256i r = _mm256_castps_si256(
        _mm256_and_ps(_mm256_castsi256_ps(a1), _mm256_castsi256_ps(a2)));
    for(size_t j=0; j<4;++j){
      count += _mm_popcnt_u64(_mm_extract_epi64(_mm256_extractf128_si256(r,(j<1)*1),j%2));
    }
    i += 16;
  }
  for(;i < smallLength;++i){
    count += _mm_popcnt_u32(in_array1[i] & in_array2[i]);
  }
  max &= 0x000F; //0x00f mask off index
  unsigned short mask = 0;
  for(i=0;i<max;++i){
    mask |= (1 << (i%BITS_PER_WORD));
  }
  count += _mm_popcnt_u32(in_array1[smallLength] & in_array2[smallLength] & mask);
  return count;
}
#endif