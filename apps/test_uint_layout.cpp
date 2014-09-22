// class templates
#include "UnsignedIntegerArray.hpp"
#include <iostream>
#include <fstream>

//sparsity = length/max
void create_synthetic_array(unsigned int *data, size_t length, unsigned int max){
  if(length > 0){
    srand ( time(NULL) );
    data[0] = max;
    for(size_t data_i = 1; data_i < length; data_i++){
      data[data_i] = rand() % max;
    }
    std::sort(data,data+length);
  }
}

int main (int argc, char* argv[]) { 
  size_t a_size = 1000;
  unsigned int *data = new unsigned int[a_size];
  unsigned int max_size = 0xffffffff; 
  create_synthetic_array(data,a_size,max_size);

  ofstream myfile;
  myfile.open("reference.txt");
  for(size_t i = 0; i < a_size; i++){
    myfile << " Data: " << data[i] << endl;
  }

  UnsignedIntegerArray *my_data = new UnsignedIntegerArray(data,a_size,common::A32BITPACKED);
  
  //cout <<  endl << "A32BITPACKED BYTES: " << my_data->length << endl << endl;

  my_data->print_data();

  return 0;
}
