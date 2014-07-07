// class templates
#include <iostream>
#include <vector>
#include <iterator>
#include "ReadIn.cpp"
#include "Graph.hpp"
#include "CSRGraph.hpp"
#include "Common.hpp"
#include <algorithm>  // for std::find
#include <iostream>   // for std::cout
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */
#include <fstream>

using namespace std;

int main (int argc, char* argv[]) {
  if(argc != 4){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of files> <# of threads>" << endl;
    exit(0);
  }

  startClock();
  VectorGraph *vg = ReadFile(argv[1],atoi(argv[2]));
  stopClock("INPUT");

  cout << endl;

  startClock();
  CompressedGraph *graph = createCompressedGraph(vg);
  stopClock("COMPRESSED CREATION");
  cout << "COMPRESSED EDGE BYTES: " << (graph->num_edges * 16)/8 << endl;
  startClock();
  
  double prc = graph->pagerank();
  cout << "Total pr: " << prc << endl;
  stopClock("COMPRESSED APPLICATION");
  
  long triangles = graph->countTriangles(atoi(argv[3]));
  cout << "Triangles: " << triangles << endl;
  stopClock("COMPRESSED APPLICATION");
  cout << endl;

  startClock();
  CSRGraph *graph2 = createCSRGraph(vg);
  stopClock("CSR CREATION");
  cout << "CSR EDGE BYTES: " << (graph2->num_edges * 32)/8 << endl;
  startClock();
  double pr = graph2->pagerank(atoi(argv[3]));
  cout << "Total pr: " << pr << endl;
  stopClock("CSR APPLICATION");

  long triangles2 = graph2->countTriangles(atoi(argv[3]));
  cout << "Triangles: " << triangles2 << endl;
  stopClock("CSR APPLICATION");

  return 0;
}
