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
  inline long countTriangles(int numThreads) const{
    std::cout << "Number of threads: " << numThreads << std::endl;
    cout << "CSR EDGE BYTES: " << (num_edges * 32)/8 << endl;

    omp_set_num_threads(numThreads);
    
    long result = 0l;
    #pragma omp parallel for default(none) schedule(static,150) reduction(+:result)        
    for(size_t i=0; i < num_nodes; i++){
      size_t start,end;
      getRange(i,start,end);
      for(size_t j = start; j < end; ++j) {
        if(i < edges[j]) result += intersectStandard(i,edges[j]);
      }
    }
    return result;
  }
  inline long intersectStandard(const size_t node1, const size_t node2) const{
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

void printCSRGraph(CSRGraph *graph) {
  for(size_t i = 0; i < graph->num_nodes; ++i) {
    cout << "Neighborhood: " << graph->nodes[i] << endl;
    size_t start,end;
    graph->getRange(i,start,end);
    for(size_t j = start; j < end; ++j) {
      cout << graph->edges[j] << " ";
    }
    cout << endl;
  }
  cout << endl;
}

CSRGraph* createCSRGraph(VectorGraph *vg){
  size_t *nodes = new size_t[vg->num_nodes];
  const size_t num_nodes = vg->num_nodes;
  const size_t num_edges = vg->num_edges;
  size_t *edges = new size_t[num_edges]; 
  const unordered_map<size_t,size_t> *external_ids = vg->external_ids;
 

  size_t index = 0;
  for(size_t i = 0; i < vg->num_nodes; ++i) {
    nodes[i] = index;
    vector<size_t> *hood = vg->neighborhoods->at(i);
    for(size_t j = 0; j < hood->size(); ++j) {
      edges[index++] = hood->at(j); 
    }
    //delete hood;
  }

  //delete vg;

  cout << "Num nodes: " << num_nodes << " Num edges: " << num_edges << endl;

  return new CSRGraph(num_nodes,num_edges,nodes,edges,external_ids);
}
#endif
