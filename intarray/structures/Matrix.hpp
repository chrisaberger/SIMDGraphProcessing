#ifndef MATRIX_H
#define MATRIX_H

#include "IntegerArray.cpp"
#include "VectorGraph.hpp"

using namespace std;

class Matrix{
	private:


	public:
  	size_t num_columns;  //number of nodes
  	size_t cardinality;  //number of edges
  	common::type t;

  	size_t *indicies; //formerly nodes
  	unsigned short *data; //formerly edges
    Matrix(VectorGraph *vg,bool (*function1)(unsigned int),bool (*function2)(unsigned int,unsigned int),common::type t);
    void print_matrix();
    template<typename T> 
    T foreach_column(T (Matrix::*function)(unsigned int,T (*f)(unsigned int,unsigned int,Matrix*)), T (*f)(unsigned int,unsigned int,Matrix*));
    template<typename T> 
    T for_row(unsigned int c,T (*function)(unsigned int,unsigned int,Matrix*));
};

#endif
