#ifndef MATRIX_H
#define MATRIX_H

#include "uint_array/uint_array.hpp"

class Matrix {
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

  static void* operator new(std::size_t sz) {
    return ::operator new(sz);
  }

  static void* operator new(std::size_t sz, int32_t node) {
    numa_set_preferred(node);
    return numa_alloc_onnode(sz, node);
  }

  static void operator delete(void* ptr) {
  }
   //Undirected Graphs
   Matrix(int node, vector< vector<unsigned int>*  > *g, size_t matrix_size_in, size_t cardinality_in, 
     bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), common::type t_in){
     array16::prepare_shuffling_dictionary16();

     /*
     size_t *row_indicies_in = (size_t*) common::allocate_local(matrix_size_in + 1, sizeof(size_t), node);
     unsigned int *row_lengths_in = (unsigned int*) common::allocate_local(matrix_size_in, sizeof(unsigned int), node);
     uint8_t *row_types_in = (uint8_t*) common::allocate_local(matrix_size_in, sizeof(uint8_t), node);
     */

     void* data = common::allocate_local(matrix_size_in + 1, sizeof(size_t) + sizeof(unsigned int) + sizeof(uint8_t), node);
     size_t *row_indicies_in = (size_t*)data;
     unsigned int *row_lengths_in = (unsigned int*)(row_indicies_in + matrix_size_in + 1);
     uint8_t *row_types_in = (uint8_t*)(row_lengths_in + matrix_size_in + 1);
 
     uint8_t *tmp_row_data = (uint8_t*) calloc(cardinality_in*40, sizeof(uint8_t));

     size_t new_cardinality = 0;
     size_t index = 0;

     row_indicies_in[0] = 5;
     for(size_t i = 0; i < matrix_size_in; ++i) {
       if(nodeFilter(i)){
         row_indicies_in[i] = index;
         vector<unsigned int> *hood = g->at(i);
         size_t filtered_hood_size = hood->size() - 1;
         unsigned int *filtered_hood = (unsigned int*) calloc(filtered_hood_size, sizeof(unsigned int)); //(unsigned int*) common::allocate_local(filtered_hood_size, sizeof(unsigned int), node);
         size_t filter_index = 0;
         for(size_t j = 1; j < hood->size(); ++j) {
           if(nodeFilter(hood->at(j)) && edgeFilter(i,hood->at(j))){
             new_cardinality++;
             filtered_hood[filter_index++] = hood->at(j);
           }
         }
         row_lengths_in[i] = filter_index;
         const common::type row_type = Matrix::get_array_type(t_in,filtered_hood,filter_index,matrix_size_in); //depends on above array being set
         row_types_in[i] = row_type;

         index = uint_array::preprocess(tmp_row_data,index,filtered_hood,filter_index,row_type);
         //common::free_local(filtered_hood, filtered_hood_size, sizeof(unsigned int));
         free(filtered_hood);
       }
     }

     uint8_t *row_data_in = (uint8_t*) common::allocate_local(index, sizeof(uint8_t), node);
     cout << "ROW DATA SIZE (Bytes): " << index << endl;
     std::copy(tmp_row_data,tmp_row_data+index,row_data_in);
     row_indicies_in[matrix_size_in] = index;

     matrix_size = matrix_size_in;
     cardinality = cardinality_in;
     t = t_in;

     row_indicies = row_indicies_in;
     row_lengths = row_lengths_in;
     row_data = row_data_in;
     row_types = row_types_in;

     symmetric = true;

     column_indicies = row_indicies_in;
     column_lengths = row_lengths_in;
     column_types = row_types_in;
     column_data = row_data_in;

     cout << "-----" << node << "------" << endl;
     cout << common::find_memory_node_for_addr(row_indicies_in) << endl;
     cout << common::find_memory_node_for_addr(row_lengths) << endl;
     cout << common::find_memory_node_for_addr(this) << endl;
     cout << "----" << endl;


     //return Matrix(matrix_size_in,new_cardinality,t_in,row_indicies_in,row_lengths_in,row_types_in,row_data_in);
   }

   //Directed Graph
   Matrix(vector< vector<unsigned int>*  > *out_nbrs,vector< vector<unsigned int>*  > *in_nbrs, size_t matrix_size_in, size_t cardinality_in, 
     bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), common::type t_in){
     array16::prepare_shuffling_dictionary16();

     size_t *row_indicies_in = new size_t[matrix_size_in+1];
     unsigned int *row_lengths_in = new unsigned int[matrix_size_in];
     uint8_t *row_types_in = new unsigned char[matrix_size_in];
     uint8_t *tmp_row_data = new uint8_t[cardinality_in*40]; 

     size_t new_cardinality = 0;
     size_t index = 0;
     size_t nbr_i = 0;

     for(size_t i = 0; i < matrix_size_in; ++i) {
       row_indicies_in[i] = index;
       if(nbr_i < out_nbrs->size() && out_nbrs->at(nbr_i)->at(0) == i){
         vector<unsigned int> *hood = out_nbrs->at(i);
         size_t node = hood->at(0);
         hood->erase(hood->begin());

         if(nodeFilter(node)){
           unsigned int *filtered_hood = new unsigned int[hood->size()];
           size_t filter_index = 0;
           for(size_t j = 0; j < hood->size(); ++j) {
             if(nodeFilter(hood->at(j)) && edgeFilter(i,hood->at(j))){
               new_cardinality++;
               filtered_hood[filter_index++] = hood->at(j);
             }
           }
           row_lengths_in[i] = filter_index;
           const common::type row_type = Matrix::get_array_type(t_in,filtered_hood,filter_index,matrix_size_in); //depends on above array being set
           row_types_in[i] = row_type;
           index = uint_array::preprocess(tmp_row_data,index,filtered_hood,filter_index,row_type);
           delete[] filtered_hood;
         }
         nbr_i++;
       } else{
         row_lengths_in[i] = 0;
         row_types_in[i] = common::ARRAY32;
       } 
     }

     uint8_t *row_data_in = new uint8_t[index];
     cout << "ROW DATA SIZE (Bytes): " << index << endl;
     std::copy(tmp_row_data,tmp_row_data+index,row_data_in);
     row_indicies_in[matrix_size_in] = index;

     index = 0;
     nbr_i = 0;
     size_t *col_indicies_in = new size_t[matrix_size_in+1];
     unsigned int *col_lengths_in = new unsigned int[matrix_size_in];
     uint8_t *col_types_in = new uint8_t[matrix_size_in];
     uint8_t *tmp_col_data = new uint8_t[cardinality_in*40]; 
     for(size_t i = 0; i < matrix_size_in; ++i) {
       col_indicies_in[i] = index;
       if(nbr_i < out_nbrs->size() && in_nbrs->at(nbr_i)->at(0) == i){
         vector<unsigned int> *hood = in_nbrs->at(i);
         size_t node = hood->at(0);
         hood->erase(hood->begin());

         if(nodeFilter(node)){
           unsigned int *filtered_hood = new unsigned int[hood->size()];
           size_t filter_index = 0;
           for(size_t j = 0; j < hood->size(); ++j) {
             if(nodeFilter(hood->at(j)) && edgeFilter(i,hood->at(j))){
               new_cardinality++;
               filtered_hood[filter_index++] = hood->at(j);
             } 
           }
           col_lengths_in[i] = filter_index;
           const common::type col_type = Matrix::get_array_type(t_in,filtered_hood,filter_index,matrix_size_in); //depends on above array being set
           col_types_in[i] = col_type;
           index = uint_array::preprocess(tmp_col_data,index,filtered_hood,filter_index,col_type);
           delete[] filtered_hood;
         }
         nbr_i++;
       } else{
         col_lengths_in[i] = 0;
         col_types_in[i] = common::ARRAY32;
       } 
     }

     uint8_t *col_data_in = new uint8_t[index];
     cout << "COLUMN DATA SIZE (Bytes): " << index << endl;
     std::copy(tmp_col_data,tmp_col_data+index,col_data_in);
     col_indicies_in[matrix_size_in] = index;

     matrix_size = matrix_size_in;
     cardinality = cardinality_in;
     t = t_in;
     
     row_indicies = row_indicies_in;
     row_lengths = row_lengths_in;
     row_data = row_data_in;
     row_types = row_types_in;

     symmetric = false;

     column_indicies = col_indicies_in;
     column_lengths = col_lengths_in;
     column_types = col_types_in;
     column_data = col_data_in;
   }
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

    //Mainly for debug
    void print_data(string filename);

    //Some accessors.  Right now these are for rows but the same thing can be done for columns.
    //Currently out neighbors but easy to apply to in neighbors.
    void print_rows(unsigned int i, unsigned int j, string filename);

    template<typename T> 
    void test(std::function<T(unsigned int, unsigned int)> fun) {
       fun(0,0);
    }

   inline size_t row_intersect(uint8_t *R, unsigned int i, unsigned int j){
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

   inline common::type get_array_type(common::type t_stat,unsigned int *r_data, size_t len, size_t size){
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

   inline common::type get_hybrid_array_type(unsigned int *r_data, size_t row_size, size_t matrix_size){
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

   template<typename T> 
   T reduce_row(
         std::function<T(unsigned int, std::function<T(unsigned int,unsigned int)>)> rowfunction,
         std::function<T(unsigned int,unsigned int)> f) {
     T reducer = (T) 0;
     //#pragma omp parallel for default(none) shared(f,rowfunction) schedule(static,150) reduction(+:reducer) 
     for(size_t i = 0; i < matrix_size; i++){
       reducer += rowfunction(i,f);
     }
     return reducer;
   }

   template<typename T> 
   void part_reduce_row_omp(
       std::function<T(unsigned int, std::function<T(unsigned int,unsigned int)>)> rowfunction,
       std::function<T(unsigned int,unsigned int)> f,
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

   template<typename T> 
   void part_reduce_row(
       std::function<T(unsigned int, std::function<T(unsigned int,unsigned int)>)> rowfunction,
       std::function<T(unsigned int,unsigned int)> f,
       int thread_id, int num_threads) {
     for(size_t i = thread_id; i < matrix_size; i += num_threads){
        rowfunction(i,f);
     }
   }

   template<typename T> 
   T reduce_column_in_row(
         unsigned int row,
         std::function<T(unsigned int,unsigned int)> f) {
     size_t start = row_indicies[row];
     size_t end = row_indicies[row+1];
     size_t card = row_lengths[row];

     #if HYBRID_LAYOUT == 1
     const common::type row_type = (common::type) row_types[row];
     #else
     const common::type row_type = (common::type) t;
     #endif
     return uint_array::reduce(f,row,row_data+start,end-start,card,row_type);
   }

   template<typename T> 
   T part_reduce_column_in_row(
         unsigned int row,
         std::function<T(unsigned int,unsigned int)> f) {
     size_t start = row_indicies[row];
     size_t end = row_indicies[row+1];
     size_t card = row_lengths[row];

     #if HYBRID_LAYOUT == 1
     const common::type row_type = (common::type) row_types[row];
     #else
     const common::type row_type = (common::type) t;
     #endif
     return uint_array::reduce(f,row,row_data+start,end-start,card,row_type);
   }
};



#endif
