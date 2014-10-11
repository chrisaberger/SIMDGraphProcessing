// class templates
#include <stdio.h>
#include <stdlib.h>
#include "UnsignedIntegerArray.hpp"
#include <set>
#include <vector>

//sparsity = length/max
void create_synthetic_array(unsigned int *data, size_t length, unsigned int max){
  cout << "creating synthetic array" << endl;
  if(length > 0){
    set<unsigned int> x;
    x.insert(max);
    for(size_t data_i = 1; data_i < length; data_i++){
      unsigned int rand_num = rand() % max;
      while(x.find(rand_num) != x.end()){
        rand_num = rand() % max;
      }
      x.insert(rand_num);
    }
    size_t data_i = 0;
    for(set<unsigned int>::iterator iter=x.begin(); iter!=x.end();++iter) {    
      data[data_i++] = (*iter);
    }
  }
}
void print_arrays(unsigned int *a_data, unsigned int *b_data, size_t a_size, size_t b_size){
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
void read_arrays(unsigned int *a_data, unsigned int *b_data, size_t a_size, size_t b_size){
  (void) a_size; (void) b_size;
  cout << "Reading arrays from files: " << "array1.txt" << " " << "array2.txt" << endl;
  ofstream myfile;
  unsigned int n,i=0;
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
  size_t a_size = 100;
  size_t b_size = 100;
  unsigned int max1 = 1000;
  unsigned int max2 = 500;

  unsigned int num_times = 1;

  unsigned int *a_data = new unsigned int[a_size];
  unsigned int *b_data = new unsigned int[b_size];
  unsigned int *result = new unsigned int[a_size+b_size];

  common::startClock();
  //create_synthetic_array(a_data,a_size,max1);
  //create_synthetic_array(b_data,b_size,max2);
  //print_arrays(a_data,b_data,a_size,b_size);
  read_arrays(a_data,b_data,a_size,b_size);
  common::stopClock("CREATING ARRAYS");

  ofstream myfile;
  std::vector<unsigned int>::iterator itv;
  size_t count;

  ///////////////////////////////////////////////////////////////////////////////////////
  /*
  common::startClock();
  for(size_t i = 0; i < num_times; i++){
    count  = array32::set_union_std(result,a_data,b_data,a_size,b_size);
  }
  common::stopClock("STD");  
  
  myfile.open("std_union.txt");
  for(size_t i = 0; i < count; i++){
    myfile << "union[" << i << "]: " << result[i] << endl;
  }
  myfile.close();

  common::startClock();
  for(size_t i = 0; i < num_times; i++){
    count = array32::set_union(result,a_data,b_data,a_size,b_size);
  }
  common::stopClock("SIMD");

  
  myfile.open("simd_union.txt");
  for(size_t i = 0; i < count; i++){
    myfile << "union[" << i << "]: " << result[i] << endl;
  }
  myfile.close();
  */
  ///////////////////////////////////////////////////////////////////////////////////////
  common::startClock();
  for(size_t i = 0; i < num_times; i++){
    count  = array32::set_difference_std(result,a_data,b_data,a_size,b_size);
  }
  common::stopClock("STD");  
  
  myfile.open("std_difference.txt");
  for(size_t i = 0; i < count; i++){
    myfile << "difference[" << i << "]: " << result[i] << endl;
  }
  myfile.close();

  
  common::startClock();
  for(size_t i = 0; i < num_times; i++){
    count = array32::set_difference(result,a_data,b_data,a_size,b_size);
  }
  common::stopClock("simd diff");
  
  myfile.open("simd_difference.txt");
  for(size_t i = 0; i < count; i++){
    myfile << "difference[" << i << "]: " << result[i] << endl;
  }
  myfile.close();
  
  return 0;
}
