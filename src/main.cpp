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

  unsigned short *test = new unsigned short[4096];
  for(size_t i = 0; i < 4096; ++i){
    test[i] = i*2;
  }

  //createBitSet(test,4096);

  unsigned short *test2 = new unsigned short[2048];
  for(size_t i = 0; i < 2048; ++i){
    test2[i] = i*2;
  }

  unsigned short *result = new unsigned short[2048];

  prepare_shuffling_dictionary();
  long count = simd_intersect_vector16(result,test,test2,4096,2048);

  for(size_t i=0; i < (size_t) count; i++){
    cout << "Index: " << i << " Data: " << result[i] << endl;
  }

  cout << "Count: " << count << endl;
  /*
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
  
  VectorGraph *vg = ReadFile(argv[1],atoi(argv[2]));
  CompressedGraph *graph = createCompressedGraph(vg);
  //CSRGraph *graph2 = createCSRGraph(vg);
  //printGraph(graph2);
  //graph->printGraph();
  //cout << "COMPRESSED EDGE BYTES: " << ((graph->edge_array_length * 16)/8)+((graph->num_nodes*32)/8) << endl;

  startClock();

  //long triangles = graph->countTriangles();
  //cout << endl << "starting 2" << endl;
  long triangles = graph->countTriangles();

  cout << "Triangles: " << triangles << endl;
  stopClock("COMPRESSED APPLICATION");
  */
  return 0;
}
