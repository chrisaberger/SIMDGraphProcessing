#include <stdio.h>
#include <stdlib.h>
#include "MutableGraph.hpp"

void gen_ordering(
    char* argv[],
    string const& name,
    std::function<void(MutableGraph*)> reorder_graph) {
  MutableGraph *inputGraph = MutableGraph::undirectedFromEdgeList(argv[1]);
  string outfile = argv[2];
  outfile.append("/u_" + name + ".bin");
  auto startTime = common::startClock();
  reorder_graph(inputGraph);
  common::stopClock(name, startTime);
  inputGraph->writeUndirectedToBinary(outfile);
  cout << name << " generated" << endl;
}

int main (int argc, char* argv[]) {
  if(argc != 3){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <input edgeList> <output path>" << endl;
    exit(0);
  }

  gen_ordering(argv, "shingles", [](MutableGraph* g) { g->reorder_by_shingles(); });
  gen_ordering(argv, "the_game", [](MutableGraph* g) { g->reorder_by_the_game(); });
  gen_ordering(argv, "bfs", [](MutableGraph* g) { g->reorder_bfs(); });
  gen_ordering(argv, "degree", [](MutableGraph* g) { g->reorder_by_degree(); });
  gen_ordering(argv, "rev_degree", [](MutableGraph* g) { g->reorder_by_rev_degree(); });
  gen_ordering(argv, "strong_run", [](MutableGraph* g) { g->reorder_strong_run(); });
  gen_ordering(argv, "random", [](MutableGraph* g) { g->reorder_random(); });

  return 0;
}
