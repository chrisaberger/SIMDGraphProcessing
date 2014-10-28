#ifndef CSR_MATRIX_H
#define CSR_MATRIX_H

#include "uint_array/uint_array.hpp"

class CSR_Matrix{
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
    
    unordered_map<unsigned int,unsigned int> *external_ids;

    //Constructor symmetric 
    CSR_Matrix(vector< vector<unsigned int>*  > *g, size_t matrix_size_in, size_t cardinality_in, 
      std::function<bool(unsigned int)> node_selection,std::function<bool(unsigned int,unsigned int)> edge_selection, 
      unordered_map<unsigned int,unsigned int> *external_ids_in,common::type t_in);

    //Constructor asymmetric 
    CSR_Matrix(vector< vector<unsigned int>*  > *out_nbrs, vector< vector<unsigned int>*  > *in_nbrs, 
      size_t matrix_size_in, size_t cardinality_in, 
      std::function<bool(unsigned int)> node_selection,std::function<bool(unsigned int,unsigned int)> edge_selection, 
      unordered_map<unsigned int,unsigned int> *external_ids_in, common::type t_in);

    ~CSR_Matrix(){
      delete[] row_indicies;
      delete[] row_lengths;
      delete[] row_data;

      if(!symmetric){
        delete[] column_indicies;
        delete[] column_lengths;
        delete[] column_data;
      }
    }
    
    //Mainly for debug
    void print_data(string filename);
    void print_rows(unsigned int i, unsigned int j, string filename);
    void print_column(unsigned int i, string filename);

    //Some accessors.  Right now these are for rows but the same thing can be done for columns.
    //Currently out neighbors but easy to apply to in neighbors.
    template<typename T> T sum_over_rows(std::function<T(unsigned int, std::function<T(unsigned int,unsigned int,unsigned int*)>)> rowfunction, std::function<T(unsigned int,unsigned int,unsigned int*)> f);
    template<typename T> T sum_over_columns_in_row(unsigned int c,std::function<T(unsigned int,unsigned int,unsigned int*)> f);        
    size_t row_intersect(uint8_t *R, unsigned int i, unsigned int j, unsigned int *decoded_a);

    template<typename T> void sum_over_rows_part(
        std::function<T(unsigned int, std::function<T(unsigned int,unsigned int,unsigned int*)>)> rowfunction,
        std::function<T(unsigned int,unsigned int,unsigned int*)> f,
        int thread_id, int num_threads);
    template<typename T> void sum_over_rows_part_omp(
        std::function<T(unsigned int, std::function<T(unsigned int,unsigned int,unsigned int*)>)> rowfunction,
        std::function<T(unsigned int,unsigned int,unsigned int*)> f,
        int node_id, int num_nodes);
};

inline size_t CSR_Matrix::row_intersect(uint8_t *R, unsigned int i, unsigned int j, unsigned int *decoded_a){
  size_t card_a = row_lengths[i];
  size_t card_b = row_lengths[j];
  long ncount = 0;

  if(card_a > 0 && card_b > 0){
    size_t i_start = row_indicies[i];
    size_t i_end = row_indicies[i+1];
    size_t j_start = row_indicies[j];
    size_t j_end = row_indicies[j+1];
    ncount = uint_array::intersect(R,row_data+i_start,row_data+j_start,i_end-i_start,j_end-j_start,card_a,card_b,t,decoded_a);
  }

  return ncount;
}

template<typename T> 
T CSR_Matrix::sum_over_rows(std::function<T(unsigned int, std::function<T(unsigned int,unsigned int,unsigned int*)>)> rowfunction, std::function<T(unsigned int,unsigned int,unsigned int*)> f){
  T reducer = (T) 0;
  #pragma omp parallel for default(none) shared(f,rowfunction) schedule(static,100) reduction(+:reducer) 
  for(size_t i = 0; i < matrix_size; i++){
    reducer += (rowfunction)(i,f);
  }
  return reducer;
}

template<typename T> 
T CSR_Matrix::sum_over_columns_in_row(unsigned int row,std::function<T(unsigned int,unsigned int,unsigned int*)> f){    
  T result = (T) 0;
  size_t card = row_lengths[row];
  if(card > 0){
    size_t start = row_indicies[row];
    size_t end = row_indicies[row+1];

    unsigned int *decoded_a = row_lengths;
    result = uint_array::sum(f,row,row_data+start,end-start,card,t,decoded_a);
  }
  return result;
}

template<typename T> 
void CSR_Matrix::sum_over_rows_part(
    std::function<T(unsigned int, std::function<T(unsigned int,unsigned int,unsigned int*)>)> rowfunction,
    std::function<T(unsigned int,unsigned int,unsigned int*)> f,
    int thread_id, int num_threads) {
  for(size_t i = thread_id; i < matrix_size; i += num_threads){
    rowfunction(i,f);
  }
}

template<typename T> 
void CSR_Matrix::sum_over_rows_part_omp(
    std::function<T(unsigned int, std::function<T(unsigned int,unsigned int,unsigned int*)>)> rowfunction,
    std::function<T(unsigned int,unsigned int,unsigned int*)> f,
    int node_id, int num_nodes) {
  size_t work_size = matrix_size / num_nodes;
  size_t start = node_id * work_size;
  // Make sure that we process all nodes
  size_t end = (node_id == num_nodes - 1) ? matrix_size : start + work_size;

#pragma omp parallel for default(none) shared(start,end,f,rowfunction) schedule(dynamic) num_threads(4)
  for(size_t i = start; i < end; i++){
    rowfunction(i,f);
  }
}

#endif
