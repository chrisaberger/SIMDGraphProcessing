#ifndef INTEGERARRAY_H
#define INTEGERARRAY_H

#include <omp.h>
#include "Array16.hpp"
#include "Array32.hpp"
#include "Common.hpp"

using namespace std;

class IntegerArray{
	private:
		unsigned short *data;
		size_t length;
		size_t physical_size;
		common::type t;
	 
	public:
	  IntegerArray(unsigned int *data, size_t length, common::type t);
	  void print_data();
	  template<typename T>
	  T reduce(T (*function)(T,T), T zero);
};

#endif
