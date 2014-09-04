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
  	short *data; //formerly edges
    Matrix(VectorGraph *vg,bool (*function1)(int),bool (*function2)(int,int),common::type t);
    void print_matrix();
    template<typename T> 
    void foreach_column(void (Matrix::*function)(int,T (*f)(int,int,Matrix*)), T (*f)(int,int,Matrix*));
    template<typename T> 
    void for_row(int c,T (*function)(int,int,Matrix*));
};

#endif
