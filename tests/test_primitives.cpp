// class templates
#include <stdio.h>
#include <stdlib.h>
#include "SparseMatrix.hpp"
#include <set>
#include <vector>

#define UNION 1
#define DIFFERENCE 0
#define PRINT 0

//sparsity = length/max
void create_synthetic_array(uint32_t *data, size_t length, uint32_t max){
  cout << "creating synthetic array" << endl;
  uint32_t prev = 0;
  for(size_t data_i = 0; data_i < length; data_i++){
    uint32_t rand_num = rand() % max +1;
    prev += rand_num;
    data[data_i] = prev;
  }
}
void print_arrays(uint32_t *a_data, uint32_t *b_data, size_t a_size, size_t b_size){
  ofstream myfile;
  cout << "Printing arrays to files: " << "array1.txt" << " " << "array2.txt" << endl;
  myfile.open("array1.txt");
  for(size_t i = 0; i < a_size; i++){
    myfile << a_data[i] << endl;
  }
  myfile.close();

  myfile.open("array2.txt");
  for(size_t i = 0; i < b_size; i++){
    myfile << b_data[i] << endl;
  }
  myfile.close();
}
void read_arrays(uint32_t *a_data, uint32_t *b_data, size_t a_size, size_t b_size){
  (void) a_size; (void) b_size;
  cout << "Reading arrays from files: " << "array1.txt" << " " << "array2.txt" << endl;
  ofstream myfile;
  uint32_t n,i=0;
  ifstream read("array1.txt");
  while(read>>n){
    a_data[i]=n;
    i++;
  }

  i=0;
  ifstream read2("array2.txt");
  while(read2>>n){
    b_data[i]=n;
    i++;
  }
}

int main (int argc, char* argv[]) {
  srand ( time(NULL) );
  size_t a_size = 40000000;
  size_t b_size = 70000000;
  uint32_t max1 = 500;
  uint32_t max2 = 600;

  uint32_t num_times = 1;

  uint32_t *a_data = new uint32_t[a_size];
  uint8_t *a_preproccessed = new uint8_t[a_size*sizeof(size_t)];

  uint32_t *b_data = new uint32_t[b_size];
  uint8_t *b_preproccessed = new uint8_t[b_size*sizeof(size_t)];

  uint8_t *result = new uint8_t[(a_size+b_size)*sizeof(size_t)];

  common::startClock();
  create_synthetic_array(a_data,a_size,max1);
  create_synthetic_array(b_data,b_size,max2);
  //read_arrays(a_data,b_data,a_size,b_size);


  #if PRINT == 1
  print_arrays(a_data,b_data,a_size,b_size);
  #endif 

  Set<uinteger> set_a = Set<uinteger>(a_preproccessed,a_data,a_size);
  Set<uinteger> set_b = Set<uinteger>(b_preproccessed,b_data,b_size);


  return 0;
}
