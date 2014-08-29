#ifndef CSRGRAPH_H
#define CSRGRAPH_H

#include <omp.h>
#include <unordered_map>

using namespace std;
inline size_t intersect_vector(const unsigned int *A, const unsigned int *B, size_t s_a, size_t s_b, unsigned int *C) {
  size_t count = 0;
  size_t i_a = 0, i_b = 0;

  // trim lengths to be a multiple of 4
  size_t st_a = (s_a / 4) * 4;
  size_t st_b = (s_b / 4) * 4;

  while(i_a < st_a && i_b < st_b) {
    //[ load segments of four 32-bit elements
    __m128i v_a = _mm_loadu_si128((__m128i*)&A[i_a]);
    __m128i v_b = _mm_loadu_si128((__m128i*)&B[i_b]);
    //]

    //[ move pointers
    unsigned int a_max = _mm_extract_epi32(v_a, 3);
    unsigned int b_max = _mm_extract_epi32(v_b, 3);
    i_a += (a_max <= b_max) * 4;
    i_b += (a_max >= b_max) * 4;
    //]

    //[ compute mask of common elements
    unsigned int cyclic_shift = _MM_SHUFFLE(0,3,2,1);
    __m128i cmp_mask1 = _mm_cmpeq_epi32(v_a, v_b);    // pairwise comparison
    v_b = _mm_shuffle_epi32(v_b, cyclic_shift);       // shuffling
    __m128i cmp_mask2 = _mm_cmpeq_epi32(v_a, v_b);    // again...
    v_b = _mm_shuffle_epi32(v_b, cyclic_shift);
    __m128i cmp_mask3 = _mm_cmpeq_epi32(v_a, v_b);    // and again...
    v_b = _mm_shuffle_epi32(v_b, cyclic_shift);
    __m128i cmp_mask4 = _mm_cmpeq_epi32(v_a, v_b);    // and again.
    __m128i cmp_mask = _mm_or_si128(
            _mm_or_si128(cmp_mask1, cmp_mask2),
            _mm_or_si128(cmp_mask3, cmp_mask4)
    ); // OR-ing of comparison masks
    // convert the 128-bit mask to the 4-bit mask
    unsigned int mask = _mm_movemask_ps((__m128)cmp_mask);
    //]
    /*
    //[ copy out common elements
    __m128i p = _mm_shuffle_epi8(v_a, shuffle_mask[mask]);
    _mm_storeu_si128((__m128i*)&C[count], p);
    */
    count += _mm_popcnt_u32(mask); // a number of elements is a weight of the mask
    //]
  }

  // intersect the tail using scalar intersection
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
struct CSRGraph {
  const size_t num_nodes;
  const size_t num_edges;
  const size_t *nodes;
  const unsigned int *edges;
  const std::unordered_map<unsigned int,unsigned int> *external_ids;
  CSRGraph( const size_t num_nodes_in, 
    const size_t num_edges_in, 
    const size_t *nodes_in, 
    const unsigned int *edges_in, 
    const std::unordered_map<unsigned int,unsigned int> *external_ids_in): 
      num_nodes(num_nodes_in), 
      num_edges(num_edges_in), 
      nodes(nodes_in),edges(edges_in), 
      external_ids(external_ids_in){}
  
  inline void print_graph() {
    for(size_t i = 0; i < num_nodes; ++i) {
      cout << "Node: " << i << endl;
      for(size_t j=nodes[i];j<nodes[i+1];++j) {
        cout << "Nbr: " << edges[j] << endl;
      }
      cout << endl;
    }
  }
  inline double pagerank() const{
    //std::cout << "Number of threads: " << numThreads << std::endl;

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
      #pragma omp parallel for default(none) shared(pr,oldpr) schedule(static,150) reduction(+:delta) reduction(+:totalpr)
      for(size_t i=0; i < num_nodes; ++i){
        float sum = 0.0;
        for(size_t j = nodes[i]; j < nodes[i+1]; ++j) {
          sum += oldpr[edges[j]]/(nodes[edges[j]+1]-nodes[edges[j]]);
        }
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

  inline void test() const{
    /*
    size_t i = 373;
    cout << "Node: " << i << endl;
    size_t start,end;
    getRange(i,start,end);
    for(size_t j = start; j < end; ++j) {
      cout << "nbr: " << edges[j] << endl;
    }
    i = 5660;
    cout << "Node: " << i << endl;
    getRange(i,start,end);
    for(size_t j = start; j < end; ++j) {
      cout << "nbr: " << edges[j] << endl;
    }
    */
    long output = intersectStandard(373,5660);
    cout << "OUTPUT: " << output << endl;
  }

  inline long countTriangles() const{
    long result = 0l;
    //#pragma omp parallel for default(none) schedule(static,150) reduction(+:result)        
    for(size_t i=0; i < num_nodes; i++){
      unsigned int *resultV = new unsigned int[10];
      for(size_t j=nodes[i]; j<nodes[i+1]; ++j) {
        long ncount = intersect_vector(&edges[nodes[i]],&edges[nodes[edges[j]]],nodes[i+1]-nodes[i],nodes[edges[j]+1]-nodes[edges[j]],resultV);
        result += ncount;
      }
    }
    return result;
  }
  inline long intersectStandard(const size_t node1, const size_t node2) const{
    //cout << "Intersecting: " << node1 << " " << node2 << endl;
    size_t i = nodes[node1];
    size_t endI = nodes[node1+1];
    size_t j = nodes[node2];
    size_t endJ = nodes[node2+1];

    size_t lim = node1;

    long count = 0l;

    bool notFinished = i < endI  && j < endJ && edges[i] < lim && edges[j] < lim;
    while(notFinished){
      while(notFinished && edges[j] < edges[i]){
        j++;
        notFinished = j < endJ && edges[j] < lim;
      }
      if(notFinished && edges[i] == edges[j]){
       ++count;
      }
      ++i;
      notFinished = notFinished && i < endI && edges[i] < lim;
    }
    //cout << "count: " << count << endl;
    return count;
  }

  /*
  inline long intersectGallop(const size_t lim, size_t i, const size_t endI, size_t j, const size_t endJ) const{
    long count = 0l;

    bool notFinished = i < endI  && j < endJ && edges[i] < lim && edges[j] < lim;
    while (notFinished) {
      if (edges[i] == edges[j]) { 
        ++count;
        ++i;
        ++j;
        notFinished = i < endI  && j < endJ && edges[i] < lim && edges[j] < lim;
      }
      else if (edges[i] < edges[j]) { 
        i = gallop(i,edges,endI,lim,edges[j]); 
        notFinished = i < endI  && edges[i] < lim;
      }
      else { 
        j = gallop(j,edges,endJ,lim,edges[i]);  
        notFinished = j < endJ  && edges[j] < lim;
      }
    }
    return count;
  }

  // this is a standard implementation of galloping
  inline size_t gallop(size_t start, const size_t __restrict__ * v, const size_t end, const size_t lim, const size_t t) const {
    size_t stepSize = 1;
    while (start < end && v[start] < lim && v[start] < t) {
      if (start + stepSize < end && v[start+stepSize] < t){
        start += stepSize;
        stepSize *= 2;
      } else {
        start += 1;
        stepSize = 1;
      }
    }
    return start;
  }
  */
};

#endif
