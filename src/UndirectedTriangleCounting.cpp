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

  VectorGraph *vg = ReadFile(argv[1],atoi(argv[2]));
  cout << endl;

  startClock();
  CompressedGraph *graph = createCompressedGraph(vg);
  stopClock("COMPRESSED CREATION");
  
  startClock();
  long triangles = graph->countTrianglesV16();
  cout << "Triangles: " << triangles << endl;
  stopClock("V16");

  startClock();
   triangles = graph->countTrianglesN2X();
  cout << "Triangles: " << triangles << endl;
  stopClock("N2X");
  
  cout << endl;

  startClock();
  CSRGraph *graph2 = createCSRGraph(vg);
  stopClock("CSR CREATION");
    
  startClock();
  long triangles2 = graph2->countTrianglesCSR();
  cout << "Triangles: " << triangles2 << endl;
  stopClock("CSR APPLICATION");

  startClock();
   triangles2 = graph2->countTrianglesV32();
  cout << "Triangles: " << triangles2 << endl;
  stopClock("V32");

  return 0;
}
