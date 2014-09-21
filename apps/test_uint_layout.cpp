// class templates
#include "UnsignedIntegerArray.hpp"
#include <iostream>
#include <fstream>

//sparsity = length/max
void create_synthetic_array(unsigned int *data, size_t length, double sparsity){
  if(length > 0){
    data[0] = length/sparsity;
    unsigned int max = data[0];
    for(size_t data_i = 1; data_i < length; data_i++){
      data[data_i] = rand() % max;
    }
    std::sort(data,data+length);
  }
}

int main (int argc, char* argv[]) { 
  size_t a_size = 10;
  unsigned int *data = new unsigned int[a_size];
  double sparsity = 0.001; 
  create_synthetic_array(data,a_size,sparsity);
  

  for(size_t i = 0; i < a_size; i++){
    cout << "Data: " << data[i] << endl;
  }

  UnsignedIntegerArray *my_data = new UnsignedIntegerArray(data,a_size,common::A32BITPACKED);
  cout << "A32BITPACKED BYTES: " << my_data->length << endl;
  cout << endl;
  my_data->print_data();

  return 0;
}
