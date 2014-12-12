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
  string outfile = argv[2];
  outfile.append("/u_the_game.bin");
  inputGraph->reorder_by_the_game();
  inputGraph->writeUndirectedToBinary(outfile);
  cout << "The game generated" << endl;

  MutableGraph *inputGraph2 = MutableGraph::undirectedFromEdgeList(argv[1]);
  outfile = argv[2];
  outfile.append("/u_bfs.bin");
  inputGraph2->reorder_bfs();
  inputGraph2->writeUndirectedToBinary(outfile);
  cout << "BFS generated" << endl;

  MutableGraph *inputGraph3 = MutableGraph::undirectedFromEdgeList(argv[1]);
  outfile = argv[2];
  outfile.append("/u_degree.bin");
  inputGraph3->reorder_by_degree();
  inputGraph3->writeUndirectedToBinary(outfile);
  cout << "Degree generated" << endl;

  MutableGraph *inputGraph4 = MutableGraph::undirectedFromEdgeList(argv[1]);
  outfile = argv[2];
  outfile.append("/u_rev_degree.bin");
  inputGraph4->reorder_by_rev_degree();
  inputGraph4->writeUndirectedToBinary(outfile);
  cout << "Rev degree generated" << endl;

  MutableGraph *inputGraph5 = MutableGraph::undirectedFromEdgeList(argv[1]);
  outfile = argv[2];
  outfile.append("/u_strong_run.bin");
  inputGraph5->reorder_strong_run();
  inputGraph5->writeUndirectedToBinary(outfile);
  cout << "Strong run generated" << endl;

  MutableGraph *inputGraph6 = MutableGraph::undirectedFromEdgeList(argv[1]);
  outfile = argv[2];
  outfile.append("/u_random.bin");
  inputGraph6->reorder_random();
  inputGraph6->writeUndirectedToBinary(outfile);
  cout << "Random generated" << endl;

  return 0;
}
