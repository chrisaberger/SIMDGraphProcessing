#ifndef GRAPH_H
#define GRAPH_H

#include <omp.h>
#include <unordered_map>
#include <xmmintrin.h>
#include <cstring>
#include <immintrin.h>
#include <unordered_map>

#define SHORTS_PER_REG 8

using namespace std;
long numSets = 0;
long numSetsCompressed = 0;

static __m256i pr_mask[8]; // precomputed dictionary

// a simple implementation, we don't care about performance here
void prepare_pr_mask() {
  for(size_t i = 0; i < 8; i++) {
    
    int permutation[8];
    for(size_t b = 0; b < 8; b++) {
      permutation[b] = 0;
      if(i == b){
        permutation[b] = 0x80000000; 
      }
    }
    __m256i mask = _mm256_load_si256((const __m256i*)permutation);
    pr_mask[i] = mask;
  }
}
 
int getBit(int value, int position) {
    return ( ( value & (1 << position) ) >> position);
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

// A, B - partitioned operands
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
      unsigned short partition_size;
      //If we are not in the range of the limit we don't need to worry about it.
      if(A[i_a] < limPrefix && B[i_b] < limPrefix){
        partition_size = simd_intersect_vector16(10000000,&A[i_a + 2], &B[i_b + 2],A[i_a + 1], B[i_b + 1]);
      } else {
        partition_size = simd_intersect_vector16(limLower,&A[i_a + 2], &B[i_b + 2],A[i_a + 1], B[i_b + 1]);
      }
      counter += partition_size;
      i_a += A[i_a + 1] + 2;
      i_b += B[i_b + 1] + 2;
      notFinished = i_a < s_a && i_b < s_b && A[i_a] <= lim && B[i_b] <= limPrefix;
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
      numSets++;
      if(partition_length > 4096)
        numSetsCompressed++;
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
  const size_t edge_array_length;
  const unsigned int *nbr_lengths;
  const size_t *nodes;
  const unsigned short *edges;
  const unordered_map<size_t,size_t> *external_ids;
  CompressedGraph(  
    const size_t num_nodes_in, 
    const size_t num_edges_in,
    const size_t edge_array_length_in,
    const unsigned int *nbrs_lengths_in, 
    const size_t *nodes_in,
    const unsigned short *edges_in,
    const unordered_map<size_t,size_t> *external_ids_in): 
      num_nodes(num_nodes_in), 
      num_edges(num_edges_in),
      edge_array_length(edge_array_length_in),
      nbr_lengths(nbrs_lengths_in),
      nodes(nodes_in), 
      edges(edges_in),
      external_ids(external_ids_in){}
    inline size_t getEndOfNeighborhood(const size_t node){
      size_t end = 0;
      if(node+1 < num_nodes) end = nodes[node+1];
      else end = edge_array_length;
      return end;
    }
    inline void traverseInnerPartition(size_t &i, size_t &prefix, size_t &end){
      const size_t header_length = 2;
      const size_t start = i;
      prefix = edges[i++]; //need
      const size_t len = edges[i++];
      end = start+len+header_length; //need
    }
    inline void printGraph(){
      for(size_t i = 0; i < num_nodes; ++i){
        size_t start1 = nodes[i];
        
        size_t end1 = 0;
        if(i+1 < num_nodes) end1 = nodes[i+1];
        else end1 = edge_array_length;

        size_t j = start1;
        cout << "Node: " << i <<endl;

        while(j < end1){
          unsigned int prefix = (edges[j] << 16);
          size_t inner_end = j+edges[j+1]+2;
          j += 2;
          while(j < inner_end){
            size_t cur = prefix | edges[j];
            cout << "nbr: " << cur << endl;
            ++j;
          }
        }
        cout << endl;
      }
    }
    inline double pagerank(){
      prepare_pr_mask();
      float *pr = new float[num_nodes];
      float *oldpr = new float[num_nodes];
      const double damp = 0.85;
      const int maxIter = 100;
      const double threshold = 0.0001;

      #pragma omp parallel for default(none) schedule(static,150) shared(pr,oldpr)
      for(size_t i=0; i < num_nodes; ++i){
        oldpr[i] = 1.0/num_nodes;
      }
      
      int iter = 0;
      double delta = 1000000000000.0;
      float totalpr = 0.0;
      while(delta > threshold && iter < maxIter){
        totalpr = 0.0;
        delta = 0.0;
        //#pragma omp parallel for default(none) shared(pr,oldpr,pr_mask) schedule(static,150) reduction(+:delta) reduction(+:totalpr)
        for(size_t i = 0; i < num_nodes; ++i){
          const size_t start1 = nodes[i];
          const size_t end1 = getEndOfNeighborhood(i);
          
          //float *tmp_degree_holder = new float[len];
          //size_t k = 0;

          size_t j = start1;
          float sum = 0.0;
          while(j < end1){
          size_t prefix, inner_end;
          traverseInnerPartition(j,prefix,inner_end);
            while(j < inner_end){
              if(j+7 < inner_end){
                float tmp_pr_holder[8];
                float tmp_deg_holder[8];
                for(size_t k = 0; k < 8; k++){
                  size_t cur = (prefix << 16) | edges[j+k];
                  tmp_pr_holder[k] = oldpr[cur];
                  tmp_deg_holder[k] = nbr_lengths[cur];
                }
                __m256 pr = _mm256_load_ps(tmp_pr_holder);
                __m256 deg = _mm256_load_ps(tmp_deg_holder);
                __m256 d = _mm256_div_ps(pr, deg);
                _mm256_storeu_ps(tmp_pr_holder, d);
                for(size_t o = 0; o < 8; o++){
                  sum += tmp_pr_holder[o]; //should be auto vectorized.
                }
                j += 8;
                //const size_t len2 = nbr_lengths[cur];
                //sum += oldpr[cur]/len2;
              }
              else{
                const size_t cur = (prefix << 16) | edges[j];
                const size_t len2 = nbr_lengths[cur];
                sum += oldpr[cur]/len2;
                ++j;
              }

              /*
              __m256 v_a = _mm256_maskload_ps((const float*)&oldpr[cur],pr_mask[0]);
              __m256 v_b = _mm256_maskload_ps((const float*)&oldpr[cur],pr_mask[1]);

              _mm256_storeu_ps(&tmp_pr_holder[0], v_a);

              
              for(size_t o = 0; o < 8; o++){
                cout << "i: " << o << " data: " << tmp_pr_holder[o] << endl;
              }
              */

              //tmp_pr_holder[k] = oldpr[cur];
              //tmp_degree_holder[k] = len2;
              //++k;
            }
          }
          /*
          for(k = 0; k < len; k++){
            sum += tmp_pr_holder[k]/tmp_degree_holder[k];
          }
          delete[] tmp_pr_holder;
          delete[] tmp_degree_holder;
          */

          pr[i] = ((1.0-damp)/num_nodes) + damp * sum;
          delta += abs(pr[i]-oldpr[i]);
          totalpr += pr[i];
        }

        float *tmp = oldpr;
        oldpr = pr;
        pr = tmp;
        ++iter;
      }
      pr = oldpr;
    
      cout << "Iter: " << iter << endl;
      /*
      for(size_t i=0; i < num_nodes; ++i){
        cout << "Node: " << i << " PR: " << pr[i] << endl;
      }
      */

      return totalpr;
    }   
    inline long countTriangles(){
      long count = 0;
      #pragma omp parallel for default(none) schedule(static,150) reduction(+:count)   
      for(size_t i = 0; i < num_nodes; ++i){
        const size_t start1 = nodes[i];
        const size_t end1 = getEndOfNeighborhood(i);
        const size_t len1 = end1-start1;

        size_t j = start1; //need to skip size area
        while(j < end1){
          size_t prefix, inner_end;
          traverseInnerPartition(j,prefix,inner_end);

          bool notFinished = (i >> 16) >= prefix;
          while(j < inner_end && notFinished){
            const size_t cur = (prefix << 16) | edges[j];
            
            const size_t start2 = nodes[cur];
            const size_t end2 = getEndOfNeighborhood(cur);
            const size_t len2 = end2-start2;

            notFinished = i > cur; //has to be reverse cause cutoff could
            //be in the middle of a partition.
            if(notFinished){
              long ncount = intersect_partitioned(cur,edges+start1,edges+start2,len1,len2);
              count += ncount;
            }
            ++j;
          }
          j = inner_end;
        }
      }
      return count;
    }
};

#endif
