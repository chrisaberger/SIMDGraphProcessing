// class templates
#include <stdio.h>
#include <stdlib.h>
#include "UnsignedIntegerArray.hpp"
#include <set>
#include <vector>

#define UNION 1
#define DIFFERENCE 1
#define PRINT 0

//sparsity = length/max
void create_synthetic_array(uint32_t *data, size_t length, uint32_t max){
  cout << "creating synthetic array" << endl;
  if(length > 0){
    set<uint32_t> x;
    x.insert(max);
    for(size_t data_i = 1; data_i < length; data_i++){
      uint32_t rand_num = rand() % max;
      while(x.find(rand_num) != x.end()){
        rand_num = rand() % max;
      }
      x.insert(rand_num);
    }
    size_t data_i = 0;
    for(set<uint32_t>::iterator iter=x.begin(); iter!=x.end();++iter) {    
      data[data_i++] = (*iter);
    }
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
  size_t a_size = 400000;
  size_t b_size = 700000;
  uint32_t max1 = 5000000;
  uint32_t max2 = 6000000;

  uint32_t num_times = 1000;

  uint32_t *a_data = new uint32_t[a_size];
  uint32_t *b_data = new uint32_t[b_size];
  uint32_t *result = new uint32_t[a_size+b_size];

  common::startClock();
  create_synthetic_array(a_data,a_size,max1);
  create_synthetic_array(b_data,b_size,max2);
  #if PRINT == 1
  print_arrays(a_data,b_data,a_size,b_size);
  #endif 

  //read_arrays(a_data,b_data,a_size,b_size);
  common::stopClock("CREATING ARRAYS");

  ofstream myfile;
  std::vector<uint32_t>::iterator itv;
  size_t count;

  ///////////////////////////////////////////////////////////////////////////////////////
  #if UNION == 1
  common::startClock();
  for(size_t i = 0; i < num_times; i++){
    count  = array32::set_union_std(result,a_data,b_data,a_size,b_size);
  }
  common::stopClock("STD UNION");  
  
  #if PRINT == 1
  myfile.open("std_union.txt");
  for(size_t i = 0; i < count; i++){
    myfile << "union[" << i << "]: " << result[i] << endl;
  }
  myfile.close();
  #endif

  common::startClock();
  for(size_t i = 0; i < num_times; i++){
    count = array32::set_union(result,a_data,b_data,a_size,b_size);
  }
  common::stopClock("SIMD UNION");

  #if PRINT == 1
  myfile.open("simd_union.txt");
  for(size_t i = 0; i < count; i++){
    myfile << "union[" << i << "]: " << result[i] << endl;
  }
  myfile.close();
  #endif

  #endif
  ///////////////////////////////////////////////////////////////////////////////////////
  #if DIFFERENCE == 1
  common::startClock();
  for(size_t i = 0; i < num_times; i++){
    count  = array32::set_difference_std(result,a_data,b_data,a_size,b_size);
  }
  common::stopClock("STD DIFFERENCE");  
  
  #if PRINT == 1
  myfile.open("std_difference.txt");
  for(size_t i = 0; i < count; i++){
    myfile << "difference[" << i << "]: " << result[i] << endl;
  }
  myfile.close();
  #endif

  common::startClock();
  for(size_t i = 0; i < num_times; i++){
    count = array32::set_difference(result,a_data,b_data,a_size,b_size);
  }
  common::stopClock("SIMD DIFFERENCE");
  
  #if PRINT == 1
  myfile.open("simd_difference.txt");
  for(size_t i = 0; i < count; i++){
    myfile << "difference[" << i << "]: " << result[i] << endl;
  }
  myfile.close();
  #endif

  #endif

  return 0;
}
