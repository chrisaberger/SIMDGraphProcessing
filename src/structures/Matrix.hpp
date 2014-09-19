#ifndef MATRIX_H
#define MATRIX_H

#include "IntegerArray.cpp"
#include "VectorGraph.hpp"

using namespace std;

class Matrix{
	public:
  	size_t num_rows;  //number of nodes
  	size_t cardinality;  //number of edges
  	common::type t;

  	size_t *indicies; //formerly nodes
    unsigned int *row_lengths;
    unsigned char *row_types;
  	uint8_t *data; //formerly edges
    Matrix(VectorGraph *vg,bool (*function1)(unsigned int),bool (*function2)(unsigned int,unsigned int),common::type t);
    common::type get_row_type(unsigned int r, unsigned int *row);
    common::type get_hybrid_row_type(unsigned int r, unsigned int *row);
    void print_matrix();
    void print_rows(unsigned int i, unsigned int j);
    template<typename T> 
    T foreach_row(T (Matrix::*function)(unsigned int,T (*f)(unsigned int,unsigned int)), T (*f)(unsigned int,unsigned int));
    template<typename T> 
    T for_row(unsigned int c,T (*function)(unsigned int,unsigned int));
    size_t row_intersect(uint8_t *R, unsigned int i, unsigned int j);
};

#endif
