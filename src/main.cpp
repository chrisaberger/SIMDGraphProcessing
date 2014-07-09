// class templates
#include "Common.hpp"

using namespace std;

int main (int argc, char* argv[]) { 
  if(argc != 4){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of files> <# of threads>" << endl;
    exit(0);
  } 
  omp_set_num_threads(atoi(argv[2]));        
  /*
  unsigned short *test = new unsigned short[4096];
  for(size_t i = 0; i < 257; ++i){
    test[i] = i;
  }

  createBitSet(test,257);

  for(size_t i = 4096; i < 10000; ++i){
    addToBitSet(i,test);
  }

  //addToBitSet(1,test);
  //addToBitSet(3,test);
  long count = andCardinalityInRange(test,test,256);
  cout << "Count: " << count << endl;

  for(size_t i = 0; i < 20; ++i){
    cout << "Index: " << i << " Data: " << test[i] << endl;
  }
  */
  VectorGraph *vg = ReadFile(argv[1],atoi(argv[2]));
  CompressedGraph *graph = createCompressedGraph(vg);
  cout << "COMPRESSED EDGE BYTES: " << ((graph->edge_array_length * 16)/8)+((graph->num_nodes*32)/8) << endl;

  startClock();


  long triangles = graph->countTriangles();
  cout << "Triangles: " << triangles << endl;
  stopClock("COMPRESSED APPLICATION");

  return 0;
}
