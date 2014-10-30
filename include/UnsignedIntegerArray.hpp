#ifndef UNSIGNEDINTEGERARRAY_H
#define UNSIGNEDINTEGERARRAY_H

#include "uint_array/uint_array.hpp"

class UnsignedIntegerArray{
	public:
    uint8_t *data;
    size_t length;
    size_t cardinality;
    common::type t;

    UnsignedIntegerArray(uint8_t *data_in, 
      size_t length_in,
      size_t card_in, 
      common::type t_in):
        data(data_in),
        length(length_in),
        cardinality(card_in),
        t(t_in){}

    ~UnsignedIntegerArray(){
      delete[] data;
    }

    void print_data(string file);
    void reset();

    static void swap(UnsignedIntegerArray *a,UnsignedIntegerArray *b);
    static UnsignedIntegerArray* fromRange(size_t start, size_t end);
    static UnsignedIntegerArray* fromArray(unsigned int *data_in, size_t length_in, common::type t_in);
    static UnsignedIntegerArray* alloc(size_t size);

};

#endif
