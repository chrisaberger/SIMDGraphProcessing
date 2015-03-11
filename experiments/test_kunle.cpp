#define WRITE_VECTOR 1

#include <stdio.h>
#include <stdlib.h>
#include "SparseMatrix.hpp"
#include "MutableGraph.hpp"

#define BUF_SIZE 1000000

int main (int argc, char* argv[]) {

  common::alloc_scratch_space(BUF_SIZE,1);

  uint8_t *data1 = new uint8_t[BUF_SIZE];
  uint8_t *data2 = new uint8_t[BUF_SIZE];

  size_t array_size = 9;
  uint32_t input_array1[] = {0,257,513,800,1300,1600,1900,2200,2500}; //new uint32_t[array_size];
  uint32_t input_array2[] = {0,257,513,850,1350,1600,1900,2250,2500};

  cout << "attempting to build" << endl;
  Set<bitset_new> A = Set<bitset_new>::from_array(data1,input_array1,array_size);
  Set<bitset_new> B = Set<bitset_new>::from_array(data2,input_array2,array_size);

  A.foreach([](uint32_t dat){
    cout << "A data: " << dat << endl;
  });

  uint8_t* set_c_buffer = new uint8_t[BUF_SIZE];
  Set<bitset_new> C(set_c_buffer);

  C = ops::set_intersect(&C,&A,&B);
  cout << "Intersection: " << C.cardinality << endl;;

  C.foreach([](uint32_t dat){
    cout << "C data: " << dat << endl;
  });
  
  return 0;
}
