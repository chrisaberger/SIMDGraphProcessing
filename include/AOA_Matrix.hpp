#ifndef AOA_MATRIX_H
#define AOA_MATRIX_H

#include "UnsignedIntegerArray.hpp"
#include "MutableGraph.hpp"

class AOA_Matrix{
  public:
    size_t matrix_size;  //number of nodes, number of columns = number of rows
    size_t cardinality;  //number of edges
    size_t max_nbrhood_size;
    common::type t; //representation of matrix
    bool symmetric; //undirected?

    /*
    Stores out neighbors.
    */
    uint32_t *row_lengths; 
    uint8_t **row_arrays;

    /*
    Stores in neighbors.
    */
    uint32_t *column_lengths;
    uint8_t **column_arrays;

    const unordered_map<uint64_t,uint32_t> *external_ids;
    uint64_t *id_map;
    uint32_t *node_attributes;
    vector< vector<uint32_t>*  > *out_edge_attributes;
    vector< vector<uint32_t>*  > *in_edge_attributes;

    AOA_Matrix(size_t matrix_size_in,
      size_t cardinality_in,
      size_t max_nbrhood_size_in,
      common::type t_in, 
      bool symmetric_in, 
      uint32_t *row_lengths_in,
      uint8_t **row_arrays_in, 
      uint32_t *column_lengths_in, 
      uint8_t **column_arrays_in, 
      uint64_t *id_map_in,
      uint32_t *node_attributes_in,
      vector< vector<uint32_t>*  > *out_edge_attributes_in,
      vector< vector<uint32_t>*  > *in_edge_attributes_in):
        matrix_size(matrix_size_in),
        cardinality(cardinality_in),
        max_nbrhood_size(max_nbrhood_size_in),
        t(t_in),
        symmetric(symmetric_in),
        row_lengths(row_lengths_in),
        row_arrays(row_arrays_in),
        column_lengths(column_lengths_in),
        column_arrays(column_arrays_in),
        id_map(id_map_in),
        node_attributes(node_attributes_in),
        out_edge_attributes(out_edge_attributes_in),
        in_edge_attributes(in_edge_attributes_in){}

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

    void *parallel_constructor(void *);

    static AOA_Matrix* from_symmetric(MutableGraph *inputGraph,
      const std::function<bool(uint32_t,uint32_t)> node_selection,
      const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection, 
      const common::type t_in);

    static AOA_Matrix* from_asymmetric(MutableGraph *inputGraph,
      const std::function<bool(uint32_t)> node_selection,
      const std::function<bool(uint32_t,uint32_t)> edge_selection, 
      const common::type t_in);

    UnsignedIntegerArray* get_distinct_neighbors(UnsignedIntegerArray *result,UnsignedIntegerArray *frontier,UnsignedIntegerArray *tmp);
    size_t row_intersect(uint8_t *R, uint32_t i, uint32_t j, uint32_t *decoded_a);
    size_t buffer_intersect(uint8_t *R, uint32_t j, uint8_t *A, uint32_t card_a);

    template<typename T> 
    T map_columns(std::function<T(uint32_t, std::function<T(uint32_t)>)> rowfunction, std::function<T(uint32_t)> f, T *new_data);
    template<typename T> 
    T map_columns_pr(std::function<T(uint32_t, std::function<T(uint32_t)>)> rowfunction, std::function<T(uint32_t)> f, T *new_data, T *old_data);
    template<typename T> 
    T sum_over_rows(std::function<T(uint32_t, uint32_t*, std::function<T(uint32_t,uint32_t,uint32_t*)>)> rowfunction, std::function<T(uint32_t,uint32_t,uint32_t*)> f);
    
    template<typename T> 
    T sum_over_columns_in_row(uint32_t col, uint32_t *decoded, std::function<T(uint32_t,uint32_t)> f);
    template<typename T> 
    T sum_over_rows_in_column(uint32_t row,std::function<T(uint32_t)> f);
        
    void print_data(string filename);

};

inline UnsignedIntegerArray* AOA_Matrix::get_distinct_neighbors(UnsignedIntegerArray *result, UnsignedIntegerArray *frontier, UnsignedIntegerArray *tmp){
  //set_union(uint32_t *C, const uint32_t *A, const uint32_t *B, size_t s_a, size_t s_b);
  uint32_t *data = (uint32_t*)frontier->data;

  if(frontier->length > 0){
    size_t card = row_lengths[data[0]];
    if(card > 0){
      uint32_t *cur_row = (uint32_t*) row_arrays[data[0]];
      result->length = cur_row[0]/4;
      std::copy((uint8_t*)&cur_row[1],(uint8_t*)&cur_row[1+cur_row[0]],result->data);
    }
  }
  
  for(size_t i=1; i<frontier->length; i++){
    size_t card = row_lengths[data[i]];
    if(card > 0){
      uint32_t *size_ptr = (uint32_t*) row_arrays[data[i]];
      tmp->length = array32::set_union((uint32_t*)tmp->data,(uint32_t*)result->data,(uint32_t*)(row_arrays[data[i]]+4),result->length,size_ptr[0]/4);
      UnsignedIntegerArray *tmp2 = result;
      result = tmp;
      tmp = tmp2;

    }
  }

  /*
  uint32_t *rdata = (uint32_t*) result->data;
  for(size_t i = 0; i < result->length; i++){
    cout << "distinct neighbors: " << i << " data: " << rdata[i] << endl;
  }
  */
  return result;
}

inline size_t AOA_Matrix::row_intersect(uint8_t *R, uint32_t i, uint32_t j, uint32_t *decoded_a){
  size_t card_a = row_lengths[i];
  size_t card_b = row_lengths[j];
  long ncount = 0;

  if(card_a > 0 && card_b > 0){
    ncount = uint_array::intersect(R,row_arrays[i],row_arrays[j],card_a,card_b,t,decoded_a);  
  }
  return ncount;
}

inline size_t AOA_Matrix::buffer_intersect(uint8_t *R, uint32_t j, uint8_t *A, uint32_t card_a){
  size_t card_b = row_lengths[j];
  long ncount = 0;

  if(card_a > 0 && card_b > 0){
    ncount = uint_array::intersect(R,A,row_arrays[j],card_a,card_b,t,(uint32_t*)A); //last variable is foo wont be used
  }
  return ncount;
}

template<typename T> 
T AOA_Matrix::map_columns(std::function<T(uint32_t, std::function<T(uint32_t)>)> rowfunction, std::function<T(uint32_t)> f, T *new_data){
  T diff = (T) 0;
  #pragma omp parallel for default(none) shared(f,new_data,rowfunction) schedule(dynamic) reduction(+:diff) 
  for(size_t i = 0; i < matrix_size; i++){
    new_data[i] = rowfunction(i,f);
  }
  return diff;
}

template<typename T> 
T AOA_Matrix::map_columns_pr(std::function<T(uint32_t, std::function<T(uint32_t)>)> rowfunction, std::function<T(uint32_t)> f, T *new_data, T *old_data){
  T diff = (T) 0;
  #pragma omp parallel for default(none) shared(rowfunction,old_data,f,new_data) schedule(dynamic) reduction(+:diff) 
  for(size_t i = 0; i < matrix_size; i++){
    new_data[i] = 0.85* rowfunction(i,f) +(0.15f/matrix_size);
    diff += new_data[i]-old_data[i];
  }
  return diff;
}

template<typename T> 
T AOA_Matrix::sum_over_columns_in_row(uint32_t row, uint32_t *decoded, std::function<T(uint32_t,uint32_t)> f){    
  T result = (T) 0;
  size_t card = row_lengths[row];
  if(card > 0){
    result = uint_array::sum(f,row,row_arrays[row],card,t,decoded);
  }
  return result;
}
template<typename T> 
T AOA_Matrix::sum_over_rows_in_column(uint32_t col,std::function<T(uint32_t)> f){    
  T result = (T) 0;
  size_t card = column_lengths[col];
  if(card > 0){
    result = uint_array::sum(f,column_arrays[col],card,t);
  }
  return result;
}

#endif
