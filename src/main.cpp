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

using namespace std;

int main (int argc, char* argv[]) {
  if(argc != 4){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of files> <# of threads>" << endl;
    exit(0);
  }
  /*
  int *foo = new int[10];
  //short *bar = new short[30];
  //short *result = new short[30];
  for(int i = 1; i <= 10; i++){
    foo[i] = i*10000;
  }

  unsigned short *bar = new unsigned short[10*4];
  size_t index = partition(foo,10,bar,0);
  
  cout << "Index: " << index << endl;
  for(size_t i = 0; i < 10*4; i++){
      
    unsigned int prefix = (bar[i] << 16);
    unsigned short size = bar[i+1];
    cout << "size: " << size << endl;
    i += 2;
    size_t inner_end = i+size;
    for(i; i < inner_end; ++i){
      unsigned int tmp = prefix | bar[i];
      cout << prefix << " " << tmp << endl;
    }
    i--;
  }
  partition(foo,10,bar,index);

  for(size_t i = 0; i < 10*4; i++){
    unsigned int prefix = (bar[i] << 16);
    unsigned short size = bar[i+1];
    cout << "size: " << size << endl;
    i += 2;
    size_t inner_end = i+size;
    for(i; i < inner_end; ++i){
      unsigned int tmp = prefix | bar[i];
      cout << prefix << " " << tmp << endl;
    }
    i--;
  }

  size_t count = 0;
  count += intersect_partitioned(&bar[0],bar+14,14,14);
  cout << count << endl;
  */

  startClock();
  VectorGraph *vg = ReadFile(argv[1],atoi(argv[2]));
  stopClock("INPUT");

  cout << endl;

  startClock();
  CompressedGraph *graph = createCompressedGraph(vg);
  stopClock("COMPRESSED CREATION");

  startClock();
  long triangles = graph->countTriangles(1);
  cout << "Triangles: " << triangles << endl;
  stopClock("COMPRESSED APPLICATION");

  cout << endl;

  startClock();
  CSRGraph *graph2 = createCSRGraph(vg);
  stopClock("CSR CREATION");

  startClock();
  long triangles2 = graph2->countTriangles(1);
  cout << "Triangles: " << triangles2 << endl;
  stopClock("CSR APPLICATION");

  return 0;
}
