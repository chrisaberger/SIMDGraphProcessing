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

  MutableGraph inputGraph = MutableGraph::undirectedFromEdgeList(argv[1],atoi(argv[2]));
  cout << "Loaded edge list" << endl;
  
  Matrix m = Matrix::buildSymetric(inputGraph.out_neighborhoods,
    inputGraph.num_nodes,inputGraph.num_edges,
    &myNodeSelection,&myEdgeSelection,common::ARRAY32);
  
  m.print_matrix("output.txt");

  return 0;
}
