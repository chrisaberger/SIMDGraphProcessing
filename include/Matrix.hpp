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
  	uint8_t *row_data; 

    /*
    Stores in neighbors.
    */
    size_t *column_indicies;
    unsigned int *column_lengths;
    uint8_t *column_data;

    vector< vector<unsigned int>*  > *n2x;    
    vector< vector<unsigned int>*  > *n2x_counts;
    unordered_map<unsigned int,unsigned int> *external_ids;

    void createN2X();

    //Constructor symmetric 
    Matrix(vector< vector<unsigned int>*  > *g, size_t matrix_size_in, size_t cardinality_in, 
      bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), 
      unordered_map<unsigned int,unsigned int> *external_ids_in,common::type t_in);

    //Constructor asymmetric 
    Matrix(vector< vector<unsigned int>*  > *out_nbrs, vector< vector<unsigned int>*  > *in_nbrs, 
      size_t matrix_size_in, size_t cardinality_in, 
      bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), 
      unordered_map<unsigned int,unsigned int> *external_ids_in, common::type t_in);

    ~Matrix(){
      delete[] row_indicies;
      delete[] row_lengths;
      delete[] row_data;

      if(!symmetric){
        delete[] column_indicies;
        delete[] column_lengths;
        delete[] column_data;
      }
    }
    static Matrix buildAsymetric(vector< vector<unsigned int>*  > *out_nbrs, vector< vector<unsigned int>*  > *in_nbrs, size_t matrix_size_in, size_t cardinality_in, 
        bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), unordered_map<unsigned int,unsigned int> *external_ids_in, common::type t_in);
    static Matrix buildSymetric(vector< vector<unsigned int>*  > *g, size_t matrix_size_in, size_t cardinality_in, 
        bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), unordered_map<unsigned int,unsigned int> *external_ids_in, common::type t_in);
    
    //Simple methods to give type of row or column
    static common::type get_array_type(common::type t_stat,unsigned int *r_data, size_t len, size_t m_size);
    static common::type get_hybrid_array_type(unsigned int *r_data, size_t len, size_t m_size);

    //Mainly for debug
    void print_data(string filename);

    //Some accessors.  Right now these are for rows but the same thing can be done for columns.
    //Currently out neighbors but easy to apply to in neighbors.
    void print_rows(unsigned int i, unsigned int j, string filename);
    template<typename T> T map_columns_pr(T (Matrix::*rowfunction)(unsigned int,T*), T *mapped_data, T *old_data);
    template<typename T> T sum_over_rows_in_column_pr(unsigned int c, T *old_data);

    template<typename T> T map_columns(T (Matrix::*rowfunction)(unsigned int,T*), T *mapped_data, T *old_data);
    template<typename T> T sum_over_rows_in_column(unsigned int c, T *old_data);

    template<typename T> T sum_over_rows(T (Matrix::*function)(unsigned int,T (*f)(unsigned int,unsigned int,unsigned int*)), T (*f)(unsigned int,unsigned int,unsigned int*));
    template<typename T> T sum_over_columns_in_row(unsigned int c,T (*function)(unsigned int,unsigned int,unsigned int*));        
    size_t row_intersect(uint8_t *R, unsigned int i, unsigned int j, unsigned int *decoded_a);
};

inline size_t Matrix::row_intersect(uint8_t *R, unsigned int i, unsigned int j, unsigned int *decoded_a){
  size_t i_start = row_indicies[i];
  size_t i_end = row_indicies[i+1];

  size_t j_start = row_indicies[j];
  size_t j_end = row_indicies[j+1];

  size_t card_a = 0;
  size_t card_b = 0;

  #if COMPRESSION == 1
  card_a = row_lengths[i];
  card_b = row_lengths[j];
  #endif

  long ncount = 0;
  if((i_end-i_start) > 0 && (j_end-j_start) > 0){
    ncount = uint_array::intersect(R,row_data+i_start,row_data+j_start,i_end-i_start,j_end-j_start,card_a,card_b,t,decoded_a);
  }

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
  } else if(row_size >= 12){
    return common::A32BITPACKED;
  } 
  else if(row_size < 12 && row_size > 4){
    return common::VARIANT;
  } 
  else{
    return common::ARRAY32;
  }
}

template<typename T> 
T Matrix::sum_over_rows(T (Matrix::*rowfunction)(unsigned int,T (*f)(unsigned int,unsigned int,unsigned int*)), T (*f)(unsigned int,unsigned int,unsigned int*)) {
  T reducer = (T) 0;
  //#pragma omp parallel for default(none) shared(f,rowfunction) schedule(static,150) reduction(+:reducer) 
  for(size_t i = 0; i < matrix_size; i++){
    reducer += (this->*rowfunction)(i,f);
  }
  return reducer;
}

template<typename T> 
T Matrix::sum_over_columns_in_row(unsigned int row,T (*function)(unsigned int,unsigned int,unsigned int*)){
  size_t start = row_indicies[row];
  size_t end = row_indicies[row+1];
  size_t card = 0;

  unsigned int *decoded_a = row_lengths;
  #if COMPRESSION == 1
  card = row_lengths[row];
  decoded_a = new unsigned int[card];
  #endif
  
  T result = (T) 0;
  if((end-start) > 0){
    result = uint_array::sum_decoded(function,row,row_data+start,end-start,card,t,decoded_a);
  }

  #if COMPRESSION == 1
  delete[] decoded_a;
  #endif

  return result;
}

template<typename T> 
T Matrix::map_columns(T (Matrix::*rowfunction)(unsigned int,T *old_data), T *new_data, T *old_data) {
  T diff = (T) 0;
  //#pragma omp parallel for default(none) shared(rowfunction,new_data,old_data) schedule(static,150) reduction(+:diff,+:sum) 
  for(size_t i = 0; i < matrix_size; i++){
    new_data[i] = ((this->*rowfunction)(i,old_data));
    //diff += new_data[i]-old_data[i];
  }
  return diff;
}

template<typename T> 
T Matrix::sum_over_rows_in_column(unsigned int col,T *old_data){
  const size_t start = column_indicies[col];
  const size_t end = column_indicies[col+1];

  #if COMPRESSION == 1
  const size_t card = column_lengths[col];
  #else 
  const size_t card = 0;
  #endif

  T result = (T) 0;
  if((end-start) > 0){
    result = uint_array::sum(column_data+start,end-start,card,t,old_data,row_lengths);
  }

  return result;
}

template<typename T> 
T Matrix::map_columns_pr(T (Matrix::*rowfunction)(unsigned int,T *old_data), T *new_data, T *old_data) {
  T diff = (T) 0;
  //#pragma omp parallel for default(none) shared(rowfunction,new_data,old_data) schedule(static,150) reduction(+:diff,+:sum) 
  for(size_t i = 0; i < matrix_size; i++){
    new_data[i] = 0.85*((this->*rowfunction)(i,old_data))+(0.15f/matrix_size);
    diff += new_data[i]-old_data[i];
  }
  return diff;
}

template<typename T> 
T Matrix::sum_over_rows_in_column_pr(unsigned int col,T *old_data){
  const size_t start = column_indicies[col];
  const size_t end = column_indicies[col+1];

  #if COMPRESSION == 1
  const size_t card = column_lengths[col];
  #else 
  const size_t card = 0;
  #endif
  
  T result = (T) 0;
  if((end-start) > 0){
    result = uint_array::sum_pr(column_data+start,end-start,card,t,old_data,row_lengths);
  }

  return result;
}

#endif
