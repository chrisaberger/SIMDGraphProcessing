#ifndef MATRIX_H
#define MATRIX_H

#include "uint_array/uint_array.hpp"

class Matrix{
	public:
  	size_t matrix_size;  //number of nodes, number of columns = number of rows
  	size_t cardinality;  //number of edges
  	common::type t; //representation of matrix
    bool symmetric; //undirected?

    /*
    Stores out neighbors.
    */
  	size_t *row_indicies; 
    unsigned int *row_lengths;
    uint8_t *row_types;
  	uint8_t *row_data; 

    /*
    Stores in neighbors.
    */
    size_t *column_indicies;
    unsigned int *column_lengths;
    uint8_t *column_types;
    uint8_t *column_data;

    Matrix(vector< vector<unsigned int>*  > *g, size_t matrix_size_in, size_t cardinality_in, 
      bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), common::type t_in);

    Matrix(vector< vector<unsigned int>*  > *out_nbrs,
      vector< vector<unsigned int>*  > *in_nbrs, size_t matrix_size_in, size_t cardinality_in, 
      bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), common::type t_in);

    ~Matrix(){
      delete[] row_indicies;
      delete[] row_lengths;
      delete[] row_types;
      delete[] row_data;

      if(!symmetric){
        delete[] column_indicies;
        delete[] column_lengths;
        delete[] column_types;
        delete[] column_data;
      }
    }
    //Constructors
    static Matrix buildAsymetric(vector< vector<unsigned int>*  > *out_nbrs, vector< vector<unsigned int>*  > *in_nbrs, size_t matrix_size_in, size_t cardinality_in, 
        bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), common::type t_in);
    static Matrix buildSymetric(vector< vector<unsigned int>*  > *g, size_t matrix_size_in, size_t cardinality_in, 
        bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), common::type t_in);
    
    //Simple methods to give type of row or column
    static common::type get_array_type(common::type t_stat,unsigned int *r_data, size_t len, size_t m_size);
    static common::type get_hybrid_array_type(unsigned int *r_data, size_t len, size_t m_size);

    //Mainly for debug
    void print_data(string filename);

    //Some accessors.  Right now these are for rows but the same thing can be done for columns.
    //Currently out neighbors but easy to apply to in neighbors.
    void print_rows(unsigned int i, unsigned int j, string filename);

    template<typename T, typename U> 
    T reduce_row(U env, T (Matrix::*function)(U, unsigned int,T (*f)(U, unsigned int,unsigned int)), T (*f)(U, unsigned int,unsigned int));

    template<typename T, typename U> 
    T reduce_column_in_row(U env, unsigned int c,T (*function)(U, unsigned int,unsigned int));

    size_t row_intersect(uint8_t *R, unsigned int i, unsigned int j);
};

inline size_t Matrix::row_intersect(uint8_t *R, unsigned int i, unsigned int j){
  size_t i_start = row_indicies[i];
  size_t i_end = row_indicies[i+1];
  size_t card_a = row_lengths[i];


  size_t j_start = row_indicies[j];
  size_t j_end = row_indicies[j+1];
  size_t card_b = row_lengths[j];


  long ncount;
  #if HYBRID_LAYOUT == 1
  const common::type t1 = (common::type) row_types[i];
  const common::type t2 = (common::type) row_types[j];
  if(t1 == t2){
    ncount = uint_array::intersect(R,row_data+i_start,row_data+j_start,i_end-i_start,j_end-j_start,card_a,card_b,t1);
  } else{
    ncount = uint_array::intersect(R,row_data+i_start,row_data+j_start,i_end-i_start,j_end-j_start,t1,t2);
  }
  #else 
    ncount = uint_array::intersect(R,row_data+i_start,row_data+j_start,i_end-i_start,j_end-j_start,card_a,card_b,t);
  #endif

  return ncount;
}

inline common::type Matrix::get_array_type(common::type t_stat,unsigned int *r_data, size_t len, size_t size){
  #if HYBRID_LAYOUT == 1
  if(t_stat == common::HYBRID){
    return Matrix::get_hybrid_array_type(r_data,len,size);
  } else{
    return t_stat;
  }
  #else
  (void)r_data;
  (void)len;
  (void)size;
  return t_stat;
  #endif
}

inline common::type Matrix::get_hybrid_array_type(unsigned int *r_data, size_t row_size, size_t matrix_size){
  double sparsity = (double) row_size/matrix_size;
  if( sparsity > (double) 1/32 ){
    return common::BITSET;
  } 
  //size/num_bins = average # per bin, > 8 say yes
  else if(row_size != 0 && 
    (row_size/((r_data[row_size-1] >> 16) - (r_data[0] >> 16) + 1)) > 8){
    return common::ARRAY16;
  } else{
    return common::ARRAY32;
  }
}

template<typename T, typename U> 
T Matrix::reduce_row(U env, T (Matrix::*rowfunction)(U, unsigned int,T (*f)(U, unsigned int,unsigned int)), T (*f)(U, unsigned int,unsigned int)) {
  T reducer = (T) 0;
  //#pragma omp parallel for default(none) shared(f,rowfunction) schedule(static,150) reduction(+:reducer) 
  for(size_t i = 0; i < matrix_size; i++){
    reducer += (this->*rowfunction)(env,i,f);
  }
  return reducer;
}

template<typename T, typename U>
T Matrix::reduce_column_in_row(U env, unsigned int row,T (*function)(U, unsigned int,unsigned int)){
  size_t start = row_indicies[row];
  size_t end = row_indicies[row+1];
  size_t card = row_lengths[row];

  #if HYBRID_LAYOUT == 1
  const common::type row_type = (common::type) row_types[row];
  #else
  const common::type row_type = (common::type) t;
  #endif
  
  return uint_array::reduce(env,function,row,row_data+start,end-start,card,row_type);
}

#endif
