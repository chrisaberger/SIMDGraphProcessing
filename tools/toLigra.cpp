#include <stdio.h>
#include <stdlib.h>
#include "MutableGraph.hpp"

int main (int argc, char* argv[]) {
  if(argc != 3){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <input edgeList> <output file>" << endl;
    exit(0);
  }
  MutableGraph *inputGraph = MutableGraph::directedFromEdgeList(argv[1]);
  cout << "Loaded edge list" << endl;
  
  inputGraph->writeDirectedToLigra(argv[2]);
  
  return 0;
}
