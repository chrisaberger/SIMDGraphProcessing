#ifndef MATRIX_H
#define MATRIX_H

#include "uint_array/uint_array.hpp"
#include "VectorGraph.hpp"

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
    ~Matrix(){
      delete[] indicies;
      delete[] row_lengths;
      delete[] row_types;
      delete[] data;
    }

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

inline size_t Matrix::row_intersect(uint8_t *R, unsigned int i, unsigned int j){
  size_t i_start = indicies[i];
  size_t i_end = indicies[i+1];

  size_t j_start = indicies[j];
  size_t j_end = indicies[j+1];

  long ncount;
  #if HYBRID_LAYOUT == 1
  const common::type t1 = (common::type) row_types[i];
  const common::type t2 = (common::type) row_types[j];
  if(t1 == t2){
    ncount = uint_array::intersect(R,data+i_start,data+j_start,i_end-i_start,j_end-j_start,t1);
  } else{
    ncount = uint_array::intersect(R,data+i_start,data+j_start,i_end-i_start,j_end-j_start,t1,t2);
  }
  #else 
    ncount = uint_array::intersect(R,data+i_start,data+j_start,i_end-i_start,j_end-j_start,t);
  #endif

  return ncount;
}

inline common::type Matrix::get_row_type(unsigned int r, unsigned int *row_data){
  #if HYBRID_LAYOUT == 1
  if(t == common::HYBRID){
    return get_hybrid_row_type(r,row_data);
  } else{
    return t;
  }
  #else
  return t;
  #endif
}

inline common::type Matrix::get_hybrid_row_type(unsigned int r, unsigned int *row_data){
  size_t row_size = row_lengths[r];
  double sparsity = (double) row_size/num_rows;
  //unsigned int num_bins = ((row_data[row_size-1] >> 16) - (row_data[0] >> 16) + 1) ;
  //cout << sparsity << endl;
  if( sparsity > (double) 1/32 ){
    //cout << "BITSET" << endl;
    return common::BITSET;
  } 
  //size/num_bins = average # per bin, > 8 say yes
  else if(row_size != 0 && 
    (row_size/((row_data[row_size-1] >> 16) - (row_data[0] >> 16) + 1)) > 8){
    //cout << "ARRAY16" << endl;
    return common::ARRAY16;
  } else{
    return common::ARRAY32;
  }
}

template<typename T> 
T Matrix::foreach_row(T (Matrix::*rowfunction)(unsigned int,T (*f)(unsigned int,unsigned int)), T (*f)(unsigned int,unsigned int)) {
  T reducer = (T) 0;
  //#pragma omp parallel for default(none) shared(f,rowfunction) schedule(static,150) reduction(+:reducer) 
  for(size_t i = 0; i < num_rows; i++){
    reducer += (this->*rowfunction)(i,f);
  }
  return reducer;
}

template<typename T> 
T Matrix::for_row(unsigned int col,T (*function)(unsigned int,unsigned int)){
  size_t start = indicies[col];
  size_t end = indicies[col+1];
  const common::type row_type = (common::type) row_types[col];
  return uint_array::foreach(function,col,data+start,end-start,row_type);
}

#endif
