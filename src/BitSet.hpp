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

#define WORDS_IN_BS 4096
#define SHORTS_PER_REG 8
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
inline void printBitSet(unsigned short prefix,size_t size, const unsigned short *in_array){
  for(size_t i=0;i<size;i++){
    for(size_t j=0;j<16;j++){
      if((in_array[i] >> j) % 2){
        unsigned short cur = j + i*16;
        cout << "Nbr: " << cur << endl;
      }
    }
  }
}
inline bool getBitSetBit(unsigned short index, const unsigned short *in_array){
  return in_array[wordIndex(index)] & (1 << (index%BITS_PER_WORD));
}
inline bool isSet(unsigned short index, const unsigned short *in_array){
  return (in_array[wordIndex(index)] & (1 << (index%16)));
}
int getBit(int value, int position) {
    return ( ( value & (1 << position) ) >> position);
}

#endif