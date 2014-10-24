#include <stdio.h>
#include <stdlib.h>
#include "MutableGraph.hpp"
#include "Matrix.hpp"

int main (int argc, char* argv[]) {
  if(argc != 3){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <input edgeList> <output file>" << endl;
    exit(0);
  }
  MutableGraph *inputGraph = MutableGraph::undirectedFromEdgeList(argv[1],1);
  cout << "Loaded edge list" << endl;
  
  inputGraph->writeUndirectedToBinary(argv[2]);
  
  /*
  inputGraph = MutableGraph::undirectedFromBinary("output.bin");
  cout << "Loaded edge list" << endl;
  
  Matrix *m = new Matrix(inputGraph->out_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &myNodeSelection,&myEdgeSelection,inputGraph->external_ids,common::ARRAY32);
  
  m->print_data("output.txt");
  */
  return 0;
}
