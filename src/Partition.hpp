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
#include <limits>
#include "BitSet.hpp"

#define WORDS_IN_BS 4096
#define SHORTS_PER_REG 8
#define BITS_PER_WORD 16
#define ADDRESS_BITS_PER_WORD 4

using namespace std;
// a simple implementation, we don't care about performance here

static __m128i shuffle_mask16[256]; // precomputed dictionary

inline int getBitSD(unsigned int value, unsigned int position) {
  return ( ( value & (1 << position) ) >> position);
}

inline void prepare_shuffling_dictionary16() {
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

inline long intersect_bitsets(unsigned short *C,const unsigned short *in_array1,const unsigned short *in_array2){
  long count = 0l;
  size_t smallLength = wordIndex(65535);

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
  return count;
}

long numSets = 0;
long numSetsCompressed = 0;

inline size_t simd_intersect_bitset_and_set(unsigned short *C, const unsigned short *A, const unsigned short *B, const size_t s_b) {
  size_t count = 0;
  bool notFinished = true;
  for(size_t i_b=0;i_b<s_b && notFinished;++i_b){
    unsigned short cur = B[i_b];
    if(isSet(cur,A)){
      C[count] = cur;
      ++count;       
    } 
  }
  return count;
}

inline size_t simd_intersect_vector16(unsigned short *C, const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
  size_t count = 0;
  size_t i_a = 0, i_b = 0;

  size_t st_a = (s_a / SHORTS_PER_REG) * SHORTS_PER_REG;
  size_t st_b = (s_b / SHORTS_PER_REG) * SHORTS_PER_REG;
  //scout << "Sizes:: " << st_a << " " << st_b << endl;
 
  while(i_a < st_a && i_b < st_b) {
    __m128i v_a = _mm_loadu_si128((__m128i*)&A[i_a]);
    __m128i v_b = _mm_loadu_si128((__m128i*)&B[i_b]);    
    unsigned short a_max = _mm_extract_epi16(v_a, SHORTS_PER_REG-1);
    unsigned short b_max = _mm_extract_epi16(v_b, SHORTS_PER_REG-1);
    
    __m128i res_v = _mm_cmpestrm(v_b, SHORTS_PER_REG, v_a, SHORTS_PER_REG,
            _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
    unsigned int r = _mm_extract_epi32(res_v, 0);
    //cout << "Mask: " << hex << val[7] << " " << val[6] << " " << val[5] << " " << val[4] << " " << val[3] << " " << val[2] << " " << val[1] << " " << val[0] << " "  << dec << endl;

    __m128i p = _mm_shuffle_epi8(v_a, shuffle_mask16[r]);
    _mm_storeu_si128((__m128i*)&C[count], p);

    count += _mm_popcnt_u32(r);
    
    i_a += (a_max <= b_max) * SHORTS_PER_REG;
    i_b += (a_max >= b_max) * SHORTS_PER_REG;
  }
  // intersect the tail using scalar intersection
  //...

  bool notFinished = i_a < s_a  && i_b < s_b;
  while(notFinished){
    while(notFinished && B[i_b] < A[i_a]){
      ++i_b;
      notFinished = i_b < s_b;
    }
    if(notFinished && A[i_a] == B[i_b]){
     ++count;
    }
    ++i_a;
    notFinished = notFinished && i_a < s_a;
  }

  return count;
}
inline size_t intersect_partitioned(unsigned short *C,const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
  size_t i_a = 0, i_b = 0;
  size_t counter = 0;
  size_t count = 0;
  bool notFinished = i_a < s_a && i_b < s_b;

  //cout << lim << endl;
  while(notFinished) {
    //size_t limLower = limLowerHolder;
    //cout << "looping" << endl;
    if(A[i_a] < B[i_b]) {
      i_a += A[i_a + 1] + 2;
      notFinished = i_a < s_a;
    } else if(B[i_b] < A[i_a]) {
      i_b += B[i_b + 1] + 2;
      notFinished = i_b < s_b;
    } else {
      unsigned short partition_size = 0;
      //If we are not in the range of the limit we don't need to worry about it.
      if(A[i_a+1] <= WORDS_IN_BS && B[i_b+1] <= WORDS_IN_BS){
        C[counter++] = A[i_a]; // write partition prefix
        //cout << "1Counter: " << counter << endl;
        partition_size = simd_intersect_vector16(&C[counter+1],&A[i_a + 2], &B[i_b + 2],A[i_a + 1], B[i_b + 1]);
        C[counter++] = partition_size; // write partition size
        i_a += A[i_a + 1] + 2;
        i_b += B[i_b + 1] + 2;      
      }else if(A[i_a+1] > WORDS_IN_BS && B[i_b+1] > WORDS_IN_BS){
        C[counter++] = A[i_a]; // write partition prefix
        //cout << "2Counter: " << counter << endl;
        partition_size = intersect_bitsets(&C[counter+1],&A[i_a+2],&B[i_b+2]);
        C[counter++] = partition_size; // write partition size

        i_a += WORDS_IN_BS + 2;
        i_b += WORDS_IN_BS + 2;   
      } else if(A[i_a+1] > WORDS_IN_BS && B[i_b+1] <= WORDS_IN_BS){
        C[counter++] = A[i_a]; // write partition prefix
        //cout << "3Counter: " << counter << endl;
        partition_size += simd_intersect_bitset_and_set(&C[counter+1],&A[i_a + 2], &B[i_b + 2], B[i_b + 1]);
        C[counter++] = partition_size; // write partition size

        i_a += WORDS_IN_BS + 2;
        i_b += B[i_b + 1] + 2;      
      } else{
        C[counter++] = A[i_a]; // write partition prefix
        //cout << "4Counter: " << counter << endl;
        partition_size += simd_intersect_bitset_and_set(&C[counter+1],&B[i_b + 2], &A[i_a + 2], A[i_a + 1]);
        C[counter++] = partition_size; // write partition size

        i_b += WORDS_IN_BS + 2;   
        i_a += A[i_a + 1] + 2;
      }

      count += partition_size;
      counter += partition_size;
      notFinished = i_a < s_a && i_b < s_b;
    }
  }
  return count;
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
        cout << "Nbr: " << tmp << endl;
        ++i;
      }
      i--;
    }
  }
}

// A - sorted array
// s_a - size of A
// R - partitioned sorted array
inline size_t partition(int *A, size_t s_a, unsigned short *R, size_t index, size_t up, size_t lp) {
  unsigned short high = 0;
  size_t partition_length = 0;
  size_t partition_size_position = index+1;
  size_t counter = index;

  unsigned int upper_mask = 0;
  for(size_t i=0;i<up;i++){
    upper_mask = (upper_mask << 1) | 0x01;
  }
  upper_mask = upper_mask << lp;

  unsigned int lower_mask = 0;
  for(size_t i=0;i<lp;i++){
    lower_mask = (lower_mask << 1) | 0x01;
  }

  //cout << hex << upper_mask << " " << lower_mask << dec << endl;

  for(size_t p = 0; p < s_a; p++) {
    unsigned short chigh = (A[p] & upper_mask) >> lp; // upper dword
    unsigned short clow = A[p] & lower_mask;   // lower dword
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