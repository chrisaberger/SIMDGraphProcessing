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


int main (int argc, char* argv[]) {
  size_t a_size = 10340;
  hybrid::prepare_shuffling_dictionary();
  unsigned int *data = new unsigned int[a_size];
  unsigned int max_size = 4000000; 
  
  create_synthetic_array(data,a_size,max_size);

  /*
  ofstream myfile;
  myfile.open("reference.txt");
  for(size_t i = 0; i < a_size; i++){
    cout << " Data: " << data[i] << endl;
  }
  */

  uint8_t *result = new uint8_t[a_size*40];
  size_t length = hybrid::preprocess(result,data,a_size,max_size);

  float *compute_data = new float[max_size+1];
  for(size_t i = 0; i < max_size+1; i++){
    compute_data[i] = 1.0;
  }

  float sum = hybrid::sum(result,length,a_size,compute_data,(unsigned int *)result);
  cout << sum << endl;
  //uint32_t *tt = (uint32_t*) &data_register;
  //cout << "RRLOADING Values: " << (void *) my_data->data << endl;

  //a32bitpacked::print_incremental(my_data->data,my_data->length,my_data->cardinality);


  return 0;
}
