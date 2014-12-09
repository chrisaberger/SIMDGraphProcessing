#include <stdio.h>
#include <stdlib.h>
#include "MutableGraph.hpp"

int main (int argc, char* argv[]) {
  if(argc != 3){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <input edgeList> <output file>" << endl;
    exit(0);
  }
  MutableGraph *inputGraph = MutableGraph::undirectedFromEdgeList(argv[1]);
  cout << "Loaded edge list" << endl;

  string outfile = argv[2];
  outfile.append("/u_bfs.bin");
  inputGraph->reorder_bfs();
  inputGraph->writeUndirectedToBinary(outfile);

  outfile = argv[2];
  outfile.append("/u_degree.bin");
  inputGraph->reorder_by_degree();
  inputGraph->writeUndirectedToBinary(outfile);

  outfile = argv[2];
  outfile.append("/u_rev_degree.bin");
  inputGraph->reorder_by_rev_degree();
  inputGraph->writeUndirectedToBinary(outfile);

  outfile = argv[2];
  outfile.append("/u_strong_run.bin");
  inputGraph->reorder_strong_run();
  inputGraph->writeUndirectedToBinary(outfile);

  outfile = argv[2];
  outfile.append("/u_random.bin");
  inputGraph->reorder_random();
  inputGraph->writeUndirectedToBinary(outfile);
  
  return 0;
}
