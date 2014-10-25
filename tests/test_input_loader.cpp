#include <stdio.h>
#include <stdlib.h>
#include "Matrix.hpp"
#include "MutableGraph.hpp"

inline bool myNodeSelection(unsigned int node){
  (void)node;
  return true;
}
inline bool myEdgeSelection(unsigned int node, unsigned int nbr){
  (void)node;
  (void)nbr;
  return true;
}

int main (int argc, char* argv[]) {
  MutableGraph *inputGraph = MutableGraph::directedFromBinary(argv[1]);
  cout << "Loaded edge list" << endl;
  
  Matrix *m = new Matrix(inputGraph->out_neighborhoods,inputGraph->in_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &myNodeSelection,&myEdgeSelection,inputGraph->external_ids,common::ARRAY32);
  
  m->print_data("output.txt");

  return 0;
}
