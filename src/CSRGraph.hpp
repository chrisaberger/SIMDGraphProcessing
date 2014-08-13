#ifndef CSRGRAPH_H
#define CSRGRAPH_H

#include <omp.h>
#include <unordered_map>

using namespace std;

struct CSRGraph {
  const size_t num_nodes;
  const size_t num_edges;
  const size_t *nodes;
  const size_t *edges;
  const std::unordered_map<size_t,size_t> *external_ids;
  CSRGraph( const size_t num_nodes_in, 
    const size_t num_edges_in, 
    const size_t *nodes_in, 
    const size_t *edges_in, 
    const std::unordered_map<size_t,size_t> *external_ids_in): 
      num_nodes(num_nodes_in), 
      num_edges(num_edges_in), 
      nodes(nodes_in),edges(edges_in), 
      external_ids(external_ids_in){}

  void getRange(const size_t node,size_t &start, size_t &end) const{
    start = nodes[node];
    if(node+1 >= num_nodes) end = num_edges;
    else end = nodes[node+1];
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
        size_t start,end;
        getRange(i,start,end);
        float sum = 0.0;
        for(size_t j = start; j < end; ++j) {
          size_t start2,end2;
          getRange(edges[j],start2,end2);
          sum += oldpr[edges[j]]/(end2-start2);
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
      size_t start,end;
      getRange(i,start,end);
      for(size_t j = start; j < end; ++j) {
        if(i < edges[j]) 
          result += intersectStandard(i,edges[j]);
        /*
        //Debug when comparing to compressed will compare in the same order.
        if(i > edges[j]){ 
          cout << "Intersecting: " << i << " with " << edges[j] << endl;
          long output = intersectStandard(edges[j],i);
          result += output;
          cout << "OUTPUT: " << output << endl;
        }
        */
      }
    }
    return result;
  }
  inline long intersectStandard(const size_t node1, const size_t node2) const{
    //cout << "Intersecting: " << node1 << " " << node2 << endl;
    size_t i, endI, j, endJ;
    getRange(node1,i,endI);
    getRange(node2,j,endJ);
    size_t lim = node1;

    long count = 0l;

    bool notFinished = i < endI  && j < endJ && edges[i] < lim && edges[j] < lim;
    while(notFinished){
      while(notFinished && edges[j] < edges[i]){
        j++;
        notFinished = j < endJ && edges[j] < lim;
      }
      if(notFinished && edges[i] == edges[j]){
        cout << "MATCH: " << edges[i] << " " << edges[j] << endl;
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

void printGraph(CSRGraph *graph) {
  for(size_t i = 0; i < graph->num_nodes; ++i) {
    cout << "Node: " << i << endl;
    size_t start,end;
    graph->getRange(i,start,end);
    for(size_t j = start; j < end; ++j) {
      cout << "nbr: " << graph->edges[j] << endl;
    }
    cout << endl;
  }
}
#endif
