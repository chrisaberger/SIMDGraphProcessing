#include <stdio.h>
#include <stdlib.h>
#include "Matrix.hpp"
#include "MutableGraph.hpp"

inline bool myNodeSelection(uint32_t node){
  (void)node;
  return true;
}
inline bool myEdgeSelection(uint32_t node, uint32_t nbr){
  (void)node;
  (void)nbr;
  return true;
}

int main (int argc, char* argv[]) {
  MutableGraph *inputGraph = MutableGraph::syntheticUndirected(100,100);
  cout << "Loaded edge list" << endl;
  
  Matrix *m = new Matrix(inputGraph->out_neighborhoods,inputGraph->in_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &myNodeSelection,&myEdgeSelection,inputGraph->external_ids,common::ARRAY32);
  
  m->print_data("output.txt");

  return 0;
}
