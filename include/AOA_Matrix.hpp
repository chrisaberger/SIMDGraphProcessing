#ifndef AOA_MATRIX_H
#define AOA_MATRIX_H

#include "uint_array/uint_array.hpp"

class AOA_Matrix{
  public:
    size_t matrix_size;  //number of nodes, number of columns = number of rows
    size_t cardinality;  //number of edges
    common::type t; //representation of matrix
    bool symmetric; //undirected?

    /*
    Stores out neighbors.
    */
    unsigned int *row_lengths; 
    uint8_t **row_arrays;

    /*
    Stores in neighbors.
    */
    unsigned int *column_lengths;
    uint8_t **column_arrays;

    const unordered_map<unsigned int,unsigned int> *external_ids;

    ~AOA_Matrix(){
      #pragma omp parallel for default(none)
      for(size_t i = 0; i < matrix_size; i++){
        delete[] row_arrays[i];
      }
      delete[] row_arrays;
      delete[] row_lengths;

      if(!symmetric){
        delete[] column_lengths;
        #pragma omp parallel for default(none)
        for(size_t i = 0; i < matrix_size; i++){
          delete[] row_arrays[i];
        }
      }
    }

    AOA_Matrix(const vector< vector<unsigned int>*  > *g,const size_t matrix_size_in,const size_t cardinality_in, 
      const std::function<bool(unsigned int)> node_selection,const std::function<bool(unsigned int,unsigned int)> edge_selection, 
      const unordered_map<unsigned int,unsigned int> *external_ids_in,const common::type t_in);


    size_t row_intersect(uint8_t *R, unsigned int i, unsigned int j, unsigned int *decoded_a);
    
    template<typename T> 
    T sum_over_rows(std::function<T(unsigned int, std::function<T(unsigned int,unsigned int,unsigned int*)>)> rowfunction, std::function<T(unsigned int,unsigned int,unsigned int*)> f);
    
    template<typename T> 
    T sum_over_columns_in_row(unsigned int row,std::function<T(unsigned int,unsigned int,unsigned int*)> f);
        
    void print_data(string filename);

};

inline size_t AOA_Matrix::row_intersect(uint8_t *R, unsigned int i, unsigned int j, unsigned int *decoded_a){
  size_t card_a = row_lengths[i];
  size_t card_b = row_lengths[j];
  long ncount = 0;

  if(card_a > 0 && card_b > 0){
    unsigned int *i_size_ptr = (unsigned int*) row_arrays[i];
    unsigned int *j_size_ptr = (unsigned int*) row_arrays[j];

    ncount = uint_array::intersect(R,row_arrays[i]+4,row_arrays[j]+4,i_size_ptr[0],j_size_ptr[0],card_a,card_b,t,decoded_a);
  }
  return ncount;
}

template<typename T> 
T AOA_Matrix::sum_over_rows(std::function<T(unsigned int, std::function<T(unsigned int,unsigned int,unsigned int*)>)> rowfunction, std::function<T(unsigned int,unsigned int,unsigned int*)> f){
  T reducer = (T) 0;
  #pragma omp parallel for default(none) shared(f,rowfunction) schedule(dynamic) reduction(+:reducer) 
  for(size_t i = 0; i < matrix_size; i++){
    reducer += (rowfunction)(i,f);
  }
  return reducer;
}

template<typename T> 
T AOA_Matrix::sum_over_columns_in_row(unsigned int row,std::function<T(unsigned int,unsigned int,unsigned int*)> f){    
  T result = (T) 0;
  size_t card = row_lengths[row];
  if(card > 0){
    unsigned int *size_ptr = (unsigned int*) row_arrays[row];
    unsigned int *decoded_a = row_lengths;
    result = uint_array::sum(f,row,row_arrays[row]+4,size_ptr[0],card,t,decoded_a);
  }
  return result;
}

#endif