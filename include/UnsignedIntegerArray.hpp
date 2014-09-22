#ifndef UNSIGNEDINTEGERARRAY_H
#define UNSIGNEDINTEGERARRAY_H

#include "uint_array/uint_array.hpp"

class UnsignedIntegerArray{
	public:
    common::type t;
  	size_t cardinality;
    size_t length;
  	uint8_t *data;

    UnsignedIntegerArray(unsigned int *data_in, size_t length_in, common::type t_in);
    ~UnsignedIntegerArray(){
      delete[] data;
    }

    void print_data(string file);
};

#endif
