#ifndef PARTITION_H
#define PARTITION_H
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
inline long andCardinalityInRange(const unsigned short *in_array1,const unsigned short *in_array2, size_t max){
  long count = 0l;
  size_t smallLength = wordIndex(max);

  //16 unsigned shorts
  //8 ints
  //4 longs

  size_t i = 0;

  while((i+15) < smallLength){
    //cout <<" here1" << endl;
    __m256i a1 = _mm256_loadu_si256((const __m256i*)&in_array1[i]);
    //cout << "0.5" << endl;
    __m256i a2 = _mm256_loadu_si256((const __m256i*)&in_array2[i]);
    
    //cout << "0" << endl;
    __m256i r = _mm256_castps_si256(
        _mm256_and_ps(_mm256_castsi256_ps(a1), _mm256_castsi256_ps(a2)));

    //cout << "1" << endl;

    __m128i t = _mm256_extractf128_si256(r,0);
    unsigned long l = _mm_extract_epi64(t,0);
    count += _mm_popcnt_u64(l);
    l = _mm_extract_epi64(t,1);
    count += _mm_popcnt_u64(l);
    t = _mm256_extractf128_si256(r,1);
    l = _mm_extract_epi64(t,0);
    count += _mm_popcnt_u64(l);
    l = _mm_extract_epi64(t,1);
    count += _mm_popcnt_u64(l);

    i += 16;
  }
  for(;i < smallLength;++i){
    //cout << "here2" << endl;
    count += _mm_popcnt_u32(in_array1[i] & in_array2[i]);
  }
  max &= 0x000F; //0x00f mask off index
  unsigned short mask = 0;
  for(i=0;i<max;++i){
    //cout << "here3" << endl;
    mask |= (1 << (i%BITS_PER_WORD));
  }
  count += _mm_popcnt_u32(in_array1[smallLength] & in_array2[smallLength] & mask);
  return count;
}

long numSets = 0;
long numSetsCompressed = 0;

int getBit(int value, int position) {
    return ( ( value & (1 << position) ) >> position);
}
inline size_t simd_intersect_bitset_and_set(const size_t lim,const unsigned short *A, const unsigned short *B, const size_t s_b) {
  size_t count = 0;
  bool notFinished = true;
  for(size_t i_b=0;i_b<s_b && notFinished;++i_b){
    unsigned short cur = B[i_b];
    notFinished = cur < lim;
    if(notFinished && getBitSetBit(cur,A)) 
      ++count; 
  }
  return count;
}

inline size_t simd_intersect_vector16(const size_t lim,const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
  size_t count = 0;
  size_t i_a = 0, i_b = 0;

  size_t st_a = (s_a / SHORTS_PER_REG) * SHORTS_PER_REG;
  size_t st_b = (s_b / SHORTS_PER_REG) * SHORTS_PER_REG;
  //scout << "Sizes:: " << st_a << " " << st_b << endl;
 
  while(i_a < st_a && i_b < st_b && A[i_a+SHORTS_PER_REG-1] < lim && B[i_b+SHORTS_PER_REG-1] < lim) {
    __m128i v_a = _mm_loadu_si128((__m128i*)&A[i_a]);
    __m128i v_b = _mm_loadu_si128((__m128i*)&B[i_b]);    
    unsigned short a_max = _mm_extract_epi16(v_a, SHORTS_PER_REG-1);
    unsigned short b_max = _mm_extract_epi16(v_b, SHORTS_PER_REG-1);
    
    __m128i res_v = _mm_cmpestrm(v_b, SHORTS_PER_REG, v_a, SHORTS_PER_REG,
            _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
    int r = _mm_extract_epi32(res_v, 0);
    count += _mm_popcnt_u32(r);
    
    i_a += (a_max <= b_max) * SHORTS_PER_REG;
    i_b += (a_max >= b_max) * SHORTS_PER_REG;
  }
  // intersect the tail using scalar intersection
  //...

  bool notFinished = i_a < s_a  && i_b < s_b && A[i_a] < lim && B[i_b] < lim;
  while(notFinished){
    while(notFinished && B[i_b] < A[i_a]){
      ++i_b;
      notFinished = i_b < s_b && B[i_b] < lim;
    }
    if(notFinished && A[i_a] == B[i_b]){
     ++count;
    }
    ++i_a;
    notFinished = notFinished && i_a < s_a && A[i_a] < lim;
  }

  return count;
}
inline size_t intersect_partitioned(const size_t lim,const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
  size_t i_a = 0, i_b = 0;
  size_t counter = 0;
  size_t limPrefix = (lim & 0x0FFFF0000) >> 16;
  size_t limLower = lim & 0x0FFFF;
  bool notFinished = i_a < s_a && i_b < s_b && A[i_a] <= limPrefix && B[i_b] <= limPrefix;

  //cout << lim << endl;
  while(notFinished) {
    //size_t limLower = limLowerHolder;
    //cout << "looping" << endl;
    if(A[i_a] < B[i_b]) {
      i_a += A[i_a + 1] + 2;
      notFinished = i_a < s_a && A[i_a] <= limPrefix;
    } else if(B[i_b] < A[i_a]) {
      i_b += B[i_b + 1] + 2;
      notFinished = i_b < s_b && B[i_b] <= limPrefix;
    } else {
      unsigned short partition_size = 0;
      //If we are not in the range of the limit we don't need to worry about it.
      if(A[i_a+1] < WORDS_IN_BS && B[i_b+1] < WORDS_IN_BS){
        //cout << "1" << endl;
        if(A[i_a] < limPrefix && B[i_b] < limPrefix){
          partition_size = simd_intersect_vector16(10000000,&A[i_a + 2], &B[i_b + 2],A[i_a + 1], B[i_b + 1]);
        } else {
          partition_size = simd_intersect_vector16(limLower,&A[i_a + 2], &B[i_b + 2],A[i_a + 1], B[i_b + 1]);
        }  
        i_a += A[i_a + 1] + 2;
        i_b += B[i_b + 1] + 2;      
      }else if(A[i_a+1] > WORDS_IN_BS && B[i_b+1] > WORDS_IN_BS){
          //cout << "2" << endl;
          if(A[i_a] < limPrefix && B[i_b] < limPrefix)
            partition_size = andCardinalityInRange(&A[i_a+2],&B[i_b+2],65535);
          else
            partition_size = andCardinalityInRange(&A[i_a+2],&B[i_b+2],limLower);
        i_a += WORDS_IN_BS + 2;
        i_b += WORDS_IN_BS + 2;   
      } else if(A[i_a+1] > WORDS_IN_BS && B[i_b+1] < WORDS_IN_BS){
          if(A[i_a] < limPrefix && B[i_b] < limPrefix)
            partition_size += simd_intersect_bitset_and_set(10000000,&A[i_a + 2], &B[i_b + 2], B[i_b + 1]);
          else
            partition_size += simd_intersect_bitset_and_set(limLower,&A[i_a + 2], &B[i_b + 2], B[i_b + 1]);
        i_a += WORDS_IN_BS + 2;
        i_b += B[i_b + 1] + 2;      
      } else{
          if(A[i_a] < limPrefix && B[i_b] < limPrefix)
            partition_size += simd_intersect_bitset_and_set(10000000,&B[i_b + 2], &A[i_a + 2], A[i_a + 1]);
          else
            partition_size += simd_intersect_bitset_and_set(limLower,&B[i_b + 2], &A[i_a + 2], A[i_a + 1]);
        i_b += WORDS_IN_BS + 2;   
        i_a += A[i_a + 1] + 2;
      }

      counter += partition_size;
      notFinished = i_a < s_a && i_b < s_b && A[i_a] <= lim && B[i_b] <= limPrefix;
    }
  }
  return counter;
}
void print_partition(const unsigned short *A, const size_t s_a){
  for(size_t i = 0; i < s_a; i++){
    unsigned int prefix = (A[i] << 16);
    unsigned short size = A[i+1];
    //cout << "size: " << size << endl;
    i += 2;
    if(size > WORDS_IN_BS){
      printBitSet(prefix,WORDS_IN_BS,&A[i]);
      i += size;
    }
    else{
      size_t inner_end = i+size;
      while(i < inner_end){
        unsigned int tmp = prefix | A[i];
        cout << prefix << tmp << endl;
        ++i;
      }
      i--;
    }
  }
}

// A - sorted array
// s_a - size of A
// R - partitioned sorted array
inline size_t partition(int *A, size_t s_a, unsigned short *R, size_t index) {
  unsigned short high = 0;
  size_t partition_length = 0;
  size_t partition_size_position = index+1;
  size_t counter = index;
  for(size_t p = 0; p < s_a; p++) {
    unsigned short chigh = (A[p] & 0xFFFF0000) >> 16; // upper dword
    unsigned short clow = A[p] & 0x0FFFF;   // lower dword
    if(chigh == high && p != 0) { // add element to the current partition
      if(partition_length == WORDS_IN_BS){
        createBitSet(&R[counter-WORDS_IN_BS],WORDS_IN_BS);
      }
      partition_length++;
      if(partition_length > WORDS_IN_BS){
        addToBitSet(clow,&R[counter-WORDS_IN_BS]);
      } else{
        R[counter++] = clow;
      }
    }else{ // start new partition
      //cout << "New partition" << endl;
      R[counter++] = chigh; // partition prefix
      R[counter++] = 0;     // reserve place for partition size
      R[counter++] = clow;  // write the first element
      R[partition_size_position] = partition_length;
      numSets++;
      if(partition_length > WORDS_IN_BS){
        numSetsCompressed++;
      }
      //cout << "setting: " << partition_size_position << " to: " << partition_length << endl;
      partition_length = 1; // reset counters
      partition_size_position = counter - 2;
      high = chigh;
    }
  }
  if(partition_length > WORDS_IN_BS){
    numSetsCompressed++;
  }
  R[partition_size_position] = partition_length;
  //print_partition(R,s_a);
  return counter;
}
#endif