
#include "UnsignedIntegerArray.hpp"

UnsignedIntegerArray::UnsignedIntegerArray(unsigned int *data_in, size_t length_in, common::type t_in){
  t = t_in;
  data = new uint8_t[length_in*10*4];
  cardinality = length_in;

  length = uint_array::preprocess(data,0,data_in,length_in,0xffffffff,t);

  /*
  data = new uint8_t[length];
  std::copy(tmp_data,tmp_data+length,data);
  delete[] tmp_data;
  */
}

void UnsignedIntegerArray::print_data(string file){
  ofstream myfile;
  myfile.open(file);
  uint_array::print_data(data,length,cardinality,t,myfile);
}