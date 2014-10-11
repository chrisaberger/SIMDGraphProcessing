// class templates
#include <stdio.h>
#include <stdlib.h>
#include "UnsignedIntegerArray.hpp"
#include <set>
#include <vector>

//sparsity = length/max
void create_synthetic_array(unsigned int *data, size_t length, unsigned int max){
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
  size_t a_size = 1000000;
  size_t b_size = 1000000;
  unsigned int max1 = 0x0f00000;
  unsigned int max2 = 0xff0000;


  unsigned int *a_data = new unsigned int[a_size];
  unsigned int *b_data = new unsigned int[b_size];
  unsigned int *result = new unsigned int[a_size+b_size];

  create_synthetic_array(a_data,a_size,max1);
  create_synthetic_array(b_data,b_size,max2);
  //print_arrays(a_data,b_data,a_size,b_size);
  //read_arrays(a_data,b_data,a_size,b_size);

  ofstream myfile;
  vector<unsigned int> *r = new vector<unsigned int>();
  r->reserve(a_size+b_size);
  std::vector<unsigned int>::iterator itv;
  size_t count;
  size_t j;

  //set<unsigned int> x;
  //set<unsigned int> *y = new set<unsigned int>();

  ///////////////////////////////////////////////////////////////////////////////////////
  common::startClock();
  count  = array32::set_union(result,a_data,b_data,a_size,b_size);
  common::stopClock("simd");
  
  myfile.open("simd_union.txt");
  for(size_t i = 0; i < count; i++){
    myfile << "union[" << i << "]: " << result[i] << endl;
  }
  myfile.close();

  common::startClock();
  itv = std::set_union(&a_data[0],&a_data[0] + a_size, &b_data[0], &b_data[0] + b_size, r->begin());
  itv = std::unique(r->begin(),itv);
  common::stopClock("std");

  myfile.open("std_union.txt");

  j = 0;
  for(std::vector<unsigned int>::iterator iter=r->begin(); iter!=itv;++iter) {    
    myfile << "union[" << j++ << "]: " << (*iter)<< endl;
  }
  myfile.close();
  ///////////////////////////////////////////////////////////////////////////////////////

  common::startClock();
  itv = std::set_difference(&a_data[0],&a_data[0] + a_size, &b_data[0], &b_data[0] + b_size, r->begin());
  common::stopClock("std diff");

  myfile.open("std_difference.txt");

  j = 0;
  for(std::vector<unsigned int>::iterator iter=r->begin(); iter!=itv;++iter) {    
    myfile << "difference[" << j++ << "]: " << (*iter)<< endl;
  }
  myfile.close();

  
  common::startClock();
  count = array32::set_difference(result,a_data,b_data,a_size,b_size);
  common::stopClock("simd diff");
  
  myfile.open("simd_difference.txt");
  for(size_t i = 0; i < count; i++){
    myfile << "difference[" << i << "]: " << result[i] << endl;
  }
  myfile.close();

  return 0;
}
