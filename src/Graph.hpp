#ifndef GRAPH_H
#define GRAPH_H

#include "Common.hpp"
#include <omp.h>
#include <unordered_map>
#include <xmmintrin.h>
#include <cstring>
#include <immintrin.h>
#include <unordered_map>


#define SHORTS_PER_REG 8

using namespace std;

int getBit(int value, int position) {
  return ( ( value & (1 << position) ) >> position);
}


// a simple implementation, we don't care about performance here
//static __m128i shuffle_mask16[16]; // precomputed dictionary
// a simple implementation, we don't care about performance here
/*
void prepare_shuffling_dictionary() {
    for(int i = 0; i < 16; i++) {
        int counter = 0;
        char permutation[16];
        memset(permutation, 0xFF, sizeof(permutation));
        for(char b = 0; b < 4; b++) {
            if(getBit(i, b)) {
                permutation[counter++] = 4*b;
                permutation[counter++] = 4*b + 1;
                permutation[counter++] = 4*b + 2;
                permutation[counter++] = 4*b + 3;
            }
        }
        __m128i mask = _mm_loadu_si128((const __m128i*)permutation);
        shuffle_mask16[i] = mask;
    }
}
*/
inline size_t intersect_vector16(const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
  
  size_t count = 0;
  size_t i_a = 0, i_b = 0;

  size_t st_a = (s_a / SHORTS_PER_REG) * SHORTS_PER_REG;
  size_t st_b = (s_b / SHORTS_PER_REG) * SHORTS_PER_REG;
  //scout << "Sizes:: " << st_a << " " << st_b << endl;
 
  while(i_a < st_a && i_b < st_b ) {
    __m128i v_a = _mm_loadu_si128((__m128i*)&A[i_a]);
    __m128i v_b = _mm_loadu_si128((__m128i*)&B[i_b]);    
    unsigned short a_max = _mm_extract_epi16(v_a, SHORTS_PER_REG-1);
    unsigned short b_max = _mm_extract_epi16(v_b, SHORTS_PER_REG-1);
    
    __m128i res_v = _mm_cmpestrm(v_b, SHORTS_PER_REG, v_a, SHORTS_PER_REG,
            _SIDD_UWORD_OPS|_SIDD_CMP_EQUAL_ANY|_SIDD_BIT_MASK);
    int r = _mm_extract_epi32(res_v, 0);
    //__m128i p = _mm_shuffle_epi8(v_a, shuffle_mask16[r]);
    //_mm_storeu_si128((__m128i*)&C[count], p);
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
  /*
  while(i_a < s_a && i_b < s_b) {
    if(A[i_a] < B[i_b]) {
      i_a++;
    } else if(B[i_b] < A[i_a]) {
      i_b++;
    } else {
      ++count;//C[count++] = A[i_a];
      i_a++; i_b++;
    }
  }
  */
  return count;
}
// A, B - partitioned operands
inline size_t intersect_partitioned(const unsigned short *A, const unsigned short *B, const size_t s_a, const size_t s_b) {
  size_t i_a = 0, i_b = 0;
  size_t counter = 0;

  while(i_a < s_a && i_b < s_b) {
    //cout << "looping" << endl;
    if(A[i_a] < B[i_b]) {
      i_a += A[i_a + 1] + 2;
    } else if(B[i_b] < A[i_a]) {
      i_b += B[i_b + 1] + 2;
    } else {
      unsigned short partition_size = intersect_vector16(&A[i_a + 2], &B[i_b + 2],A[i_a + 1], B[i_b + 1]);
      counter += partition_size;
      i_a += A[i_a + 1] + 2;
      i_b += B[i_b + 1] + 2;
    }
  }

  return counter;
}
void print_partition(unsigned short *A, size_t s_a){
  for(size_t i = 0; i < s_a; i++){
    unsigned int prefix = (A[i] << 16);
    unsigned short size = A[i+1];
    cout << "size: " << size << endl;
    i += 2;
    size_t inner_end = i+size;
    while(i < inner_end){
      unsigned int tmp = prefix | A[i];
      cout << prefix << " " << tmp << endl;
      ++i;
    }
    i--;
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
        R[counter++] = clow;
        partition_length++;
      } else { // start new partition
        R[counter++] = chigh; // partition prefix
        R[counter++] = 0;     // reserve place for partition size
        R[counter++] = clow;  // write the first element
        R[partition_size_position] = partition_length;
        //cout << "setting: " << partition_size_position << " to: " << partition_length << endl;
        partition_length = 1; // reset counters
        partition_size_position = counter - 2;
        high = chigh;
      }
  }
  R[partition_size_position] = partition_length;

  return counter;
}
struct CompressedGraph {
  const size_t num_nodes;
  const size_t num_edges;
  const int *nodes;
  unsigned short *edges;
  const unordered_map<size_t,size_t> *external_ids;
  CompressedGraph(  
    const size_t num_nodes_in, 
    const size_t num_edges_in, 
    const int *nodes_in,
    unsigned short *edges_in,
    const unordered_map<size_t,size_t> *external_ids_in): 
    num_nodes(num_nodes_in), 
    num_edges(num_edges_in),
    nodes(nodes_in), 
    edges(edges_in),
    external_ids(external_ids_in){}


    inline long countTriangles(int numThreads){
      cout << "COMPRESSED EDGE BYTES: " << (num_edges * 16)/8 << endl;

      long count = 0;
      for(size_t i = 0; i < num_nodes; ++i){
        size_t start1 = nodes[i];
        const int start1F = start1;
        
        size_t end1 = 0;
        if(i+1 < num_nodes) end1 = nodes[i+1];
        else end1 = num_edges;
        size_t len1 = end1-start1;

        size_t j = start1;

        while(j < end1){
          unsigned int prefix = (edges[j] << 16);
          size_t inner_end = j+edges[j+1]+2;
          j += 2;

          bool notFinished = edges[j-2] <= (i >> 16);

          while(j < inner_end && notFinished){
            size_t cur = prefix | edges[j];
            //finished = i < cur;

            size_t start2 = nodes[cur];
            //cout << "start: " << start2 << " num_edges: " << num_edges << endl;
            size_t end2 = 0;
            if(cur+1 < num_nodes) end2 = nodes[cur+1];
            else end2 = num_edges;
            size_t len2 = end2-start2;

            notFinished = cur < i;
            count += notFinished * intersect_partitioned(edges+start1F,edges+start2,len1,len2);
            ++j;
          }
          j = inner_end;
        }
      }
      return count;
    }
};
static inline CompressedGraph* createCompressedGraph (VectorGraph *vg) {
  int *nodes = new int[vg->num_nodes];
  unsigned short *edges = new unsigned short[vg->num_edges*8];
  size_t num_nodes = vg->num_nodes;
  const unordered_map<size_t,size_t> *external_ids = vg->external_ids;

  cout  << "Num nodes: " << vg->num_nodes << endl;

  size_t index = 0;
  for(size_t i = 0; i < vg->num_nodes; ++i){
    vector<size_t> *hood = vg->neighborhoods->at(i);

    int *tmp_hood = new int[hood->size()];
    for(size_t j = 0; j < hood->size(); ++j) {
      tmp_hood[j] = hood->at(j);
    }
    nodes[i] = index;
    index = partition(tmp_hood,hood->size(),edges,index);
    delete[] tmp_hood;
    //hood->clear();
    //delete hood;
  }

  //delete vg;

  return new CompressedGraph(num_nodes,index,nodes,edges,external_ids);
}

#endif
