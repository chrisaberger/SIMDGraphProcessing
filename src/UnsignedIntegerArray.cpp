#include "UnsignedIntegerArray.hpp"

void UnsignedIntegerArray::intersect(UnsignedIntegerArray *r,UnsignedIntegerArray *a,UnsignedIntegerArray *b){
  r->length = array32::intersect((unsigned int*) r->data, (unsigned int *)a->data, (unsigned int *)b->data, a->length, b->length);
}
void UnsignedIntegerArray::difference(UnsignedIntegerArray *r,UnsignedIntegerArray *a,UnsignedIntegerArray *b){
  r->length = array32::set_difference((unsigned int*) r->data, (unsigned int *)a->data, (unsigned int *)b->data, a->length, b->length);
}
void UnsignedIntegerArray::swap(UnsignedIntegerArray *a,UnsignedIntegerArray *b){
  UnsignedIntegerArray *tmp = a;
  a = b;
  b = tmp;
}

void UnsignedIntegerArray::reset(){
  length = 0;
  cardinality = 0;
}

UnsignedIntegerArray* UnsignedIntegerArray::fromRange(size_t start, size_t end){
  unsigned int *data_in = new unsigned int[end-start];

  for(size_t i=0; i<(end-start); i++){
    data_in[i] = start+i;
  }
  return new UnsignedIntegerArray((uint8_t*)data_in,(end-start)*4,(end-start)*4,common::ARRAY32);
}

UnsignedIntegerArray* UnsignedIntegerArray::fromArray(unsigned int *data_in, size_t length_in, common::type t_in){
  uint8_t *new_data = new uint8_t[length_in*10*4];
  size_t length = uint_array::preprocess(new_data,0,data_in,length_in,0xffffffff,t_in);

  return new UnsignedIntegerArray(new_data,length,length_in,t_in);
}

UnsignedIntegerArray* UnsignedIntegerArray::alloc(size_t size){
  uint8_t *data_in = new uint8_t[size*6];
  return new UnsignedIntegerArray(data_in,0,0,common::ARRAY32);
}

void UnsignedIntegerArray::print_data(string file){
  ofstream myfile;
  myfile.open(file);
  uint_array::print_data(data,length,cardinality,t,myfile);
}