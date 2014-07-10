// class templates
#include "Common.hpp"

using namespace std;

int main (int argc, char* argv[]) {
  if(argc != 4){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of files> <# of threads>" << endl;
    exit(0);
  }
  
  cout << "Number of threads: " << atoi(argv[3]) << endl;
  omp_set_num_threads(atoi(argv[3]));  
  
  startClock();
  VectorGraph *vg = ReadFile(argv[1],atoi(argv[2]));
  stopClock("INPUT");

  cout << endl;

  startClock();
  CompressedGraph *graph = createCompressedGraph(vg);
  stopClock("COMPRESSED CREATION");
  cout << "COMPRESSED EDGE BYTES: " << ((graph->edge_array_length * 16)/8)+((graph->num_nodes*32)/8) << endl;
  startClock();

  double prc = graph->pagerank();
  cout << "Total pr: " << prc << endl;
  stopClock("COMPRESSED APPLICATION");

  cout << endl;

  startClock();
  CSRGraph *graph2 = createCSRGraph(vg);
  stopClock("CSR CREATION");
  cout << "CSR EDGE BYTES: " << (graph2->num_edges * 32)/8 << endl;
  startClock();

  double pr = graph2->pagerank();
  cout << "Total pr: " << pr << endl;
  stopClock("CSR APPLICATION");

  return 0;
}