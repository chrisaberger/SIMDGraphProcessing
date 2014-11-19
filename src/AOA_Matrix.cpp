#include "AOA_Matrix.hpp"

AOA_Matrix* AOA_Matrix::from_symmetric(const vector< vector<uint32_t>*  > *g,const size_t matrix_size_in,const size_t cardinality_in, const size_t max_nbrhood_size_in,
  const std::function<bool(uint32_t)> node_selection,const std::function<bool(uint32_t,uint32_t)> edge_selection, 
  const unordered_map<uint64_t,uint32_t> *external_ids_in, const common::type t_in){
  
  array16::prepare_shuffling_dictionary16();
  hybrid::prepare_shuffling_dictionary();

  uint8_t **row_arrays_in = new uint8_t*[matrix_size_in];
  uint32_t *row_lengths_in = new uint32_t[matrix_size_in];

  cout << "Number of edges: " << cardinality_in << endl;

  size_t new_cardinality = 0;
  size_t total_bytes_used = 0;
    
  size_t alloc_size = sizeof(uint32_t)*(cardinality_in/omp_get_num_threads());
  if(alloc_size < matrix_size_in){
    alloc_size = matrix_size_in;
  }
  #pragma omp parallel default(shared) reduction(+:total_bytes_used) reduction(+:new_cardinality)
  {
    uint8_t *row_data_in = new uint8_t[alloc_size];
    uint32_t *selected_row = new uint32_t[matrix_size_in];
    size_t index = 0;
    
    #pragma omp for schedule(static,100)
    for(size_t i = 0; i < matrix_size_in; ++i){
      if(node_selection(i)){
        vector<uint32_t> *row = g->at(i);
        size_t new_size = 0;
        for(size_t j = 0; j < row->size(); ++j) {
          if(node_selection(row->at(j)) && edge_selection(i,row->at(j))){
            new_cardinality++;
            selected_row[new_size++] = row->at(j);
          } 
        }
      
        row_lengths_in[i] = new_size;
        row_arrays_in[i] = &row_data_in[index];

        if(new_size > 0){
          common::type row_type = uint_array::get_array_type(t_in,selected_row,new_size,matrix_size_in);
          index = uint_array::preprocess(row_data_in,index,selected_row,new_size,row_type);
        }
        new_cardinality += new_size;
      } else{
        row_lengths_in[i] = 0;
      }
    }
    delete[] selected_row;
    total_bytes_used += index;
    row_data_in = (uint8_t*) realloc((void *) row_data_in, index*sizeof(uint8_t));
  }

  cout << "ROW DATA SIZE (Bytes): " << total_bytes_used << endl;

  return new AOA_Matrix(
        matrix_size_in,
        new_cardinality,
        total_bytes_used,
        0,
        max_nbrhood_size_in,
        t_in,
        true,
        row_lengths_in,
        row_arrays_in,
        row_lengths_in,
        row_arrays_in,
        external_ids_in);
}

AOA_Matrix* AOA_Matrix::from_asymmetric(vector< vector<uint32_t>*  > *out_nbrs,vector< vector<uint32_t>*  > *in_nbrs,size_t max_nbrhood_size_in,
  const size_t matrix_size_in,const size_t cardinality_in, 
  const std::function<bool(uint32_t)> node_selection,
  const std::function<bool(uint32_t,uint32_t)> edge_selection, 
  const unordered_map<uint64_t,uint32_t> *external_ids_in, 
  const common::type t_in){
  
  array16::prepare_shuffling_dictionary16();
  hybrid::prepare_shuffling_dictionary();

  uint8_t **row_arrays_in = new uint8_t*[matrix_size_in];
  uint32_t *row_lengths_in = new uint32_t[matrix_size_in];
  uint8_t **col_arrays_in = new uint8_t*[matrix_size_in];
  uint32_t *col_lengths_in = new uint32_t[matrix_size_in];

  cout << "Number of edges: " << cardinality_in << endl;

  size_t new_cardinality = 0;
  size_t row_total_bytes_used = 0;
  size_t col_total_bytes_used = 0;
    
  size_t alloc_size = (sizeof(uint32_t)+2)*(cardinality_in/omp_get_num_threads());
  if(alloc_size <= matrix_size_in*6){
    alloc_size = matrix_size_in*10;
  }
  #pragma omp parallel default(shared) reduction(+:row_total_bytes_used) reduction(+:new_cardinality)
  {
    uint8_t *row_data_in = new uint8_t[alloc_size];
    uint8_t *col_data_in = new uint8_t[alloc_size];

    uint32_t *selected = new uint32_t[matrix_size_in];
    size_t row_index = 0;
    size_t col_index = 0;

    #pragma omp for schedule(static,100)
    for(size_t i = 0; i < matrix_size_in; ++i){
      if(node_selection(i)){
        vector<uint32_t> *row = out_nbrs->at(i);
        size_t new_row_size = 0;
        for(size_t j = 0; j < row->size(); ++j) {
          if(node_selection(row->at(j)) && edge_selection(i,row->at(j))){
            new_cardinality++;
            selected[new_row_size++] = row->at(j);
          } 
        }
      
        row_lengths_in[i] = new_row_size;
        row_arrays_in[i] = &row_data_in[row_index];

        if(new_row_size > 0){
          common::type row_type = uint_array::get_array_type(t_in,selected,new_row_size,matrix_size_in);
          row_index = uint_array::preprocess(row_data_in,row_index,selected,new_row_size,row_type);
        }
        new_cardinality += new_row_size;
        ////////////////////////////////////////////////////////////////////////////////////////////
        vector<uint32_t> *col = in_nbrs->at(i);
        size_t new_col_size = 0;
        for(size_t j = 0; j < col->size(); ++j) {
          if(node_selection(col->at(j)) && edge_selection(i,col->at(j))){
            new_cardinality++;
            selected[new_col_size++] = col->at(j);
          } 
        }
      
        col_lengths_in[i] = new_col_size;
        col_arrays_in[i] = &col_data_in[col_index];

        if(new_col_size > 0){
          common::type col_type = uint_array::get_array_type(t_in,selected,new_col_size,matrix_size_in);
          col_index = uint_array::preprocess(col_data_in,col_index,selected,new_col_size,col_type);
        }
        new_cardinality += new_col_size;
      } else{
        row_lengths_in[i] = 0;
        col_lengths_in[i] = 0;
      }
    }
    delete[] selected;

    row_total_bytes_used += row_index;
    col_total_bytes_used += col_index;
    col_data_in = (uint8_t*) realloc((void *) col_data_in, col_index*sizeof(uint8_t));
    row_data_in = (uint8_t*) realloc((void *) row_data_in, row_index*sizeof(uint8_t));
  }

  cout << "ROW DATA SIZE (Bytes): " << row_total_bytes_used << endl;
  cout << "COLUMN DATA SIZE (Bytes): " << col_total_bytes_used << endl;

  return new AOA_Matrix(
        matrix_size_in,
        new_cardinality,
        row_total_bytes_used,
        col_total_bytes_used,
        max_nbrhood_size_in,
        t_in,
        false,
        row_lengths_in,
        row_arrays_in,
        col_lengths_in,
        col_arrays_in,
        external_ids_in);
}

void AOA_Matrix::print_data(string filename){
  ofstream myfile;
  myfile.open(filename);

  //Printing out neighbors
  cout << "Writing matrix row_data to file: " << filename << endl;
  for(size_t i = 0; i < matrix_size; i++){
    myfile << "ROW: " << i <<  " LEN: " << row_lengths[i] << endl;
    size_t card = row_lengths[i];
    if(card > 0){
      uint_array::print_data(row_arrays[i],card,t,myfile);
    }
  }
  myfile << endl;
  //Printing in neighbors
  if(!symmetric){
    for(size_t i = 0; i < matrix_size; i++){
      myfile << "COLUMN: " << i << " LEN: " << column_lengths[i] << endl;
      size_t card = column_lengths[i];
      if(card > 0){
        uint_array::print_data(column_arrays[i],card,t,myfile);
      }
    }
  }

  myfile.close();
}

// XXX: This code only works for a32 for now
AOA_Matrix* AOA_Matrix::clone_on_node(int node) {
   numa_run_on_node(node);
   numa_set_preferred(node);

   size_t matrix_size = this->matrix_size;
   uint64_t lengths_size = matrix_size * sizeof(uint32_t);
   uint32_t* cloned_row_lengths =
      (uint32_t*)numa_alloc_onnode(lengths_size, node);
   std::copy(this->row_lengths, this->row_lengths + matrix_size + 1,
         cloned_row_lengths);

   uint8_t** cloned_row_arrays =
      (uint8_t**) numa_alloc_onnode(matrix_size * sizeof(uint8_t*) * 4, node);
   uint8_t* neighborhood =
      (uint8_t*) numa_alloc_onnode(this->cardinality * sizeof(uint32_t) * 4, node);
   for(uint64_t i = 0; i < matrix_size; i++) {
      uint32_t row_length = cloned_row_lengths[i];
      size_t num_bytes = row_length * 4 + 1;
      std::copy(this->row_arrays[i], this->row_arrays[i] + num_bytes, neighborhood);
      cloned_row_arrays[i] = (uint8_t*) neighborhood;
      neighborhood += num_bytes;
   }

   uint32_t* cloned_column_lengths = NULL;
   uint8_t** cloned_column_arrays = NULL;
   /*
   if(this->symmetric) {
      cloned_column_lengths = (uint32_t*) numa_alloc_onnode(lengths_size, node);
      std::copy(column_lengths, column_lengths + matrix_size, cloned_column_lengths);

      cloned_column_arrays =
         (uint8_t**) numa_alloc_onnode(col_total_bytes_used, node);
      for(uint64_t i = 0; i < matrix_size; i++) {
         uint32_t col_length = cloned_column_lengths[i];
         uint8_t* neighborhood =
            (uint8_t*) numa_alloc_onnode(col_length * sizeof(uint8_t), node);
         std::copy(this->column_arrays[i], this->column_arrays[i] + col_length, neighborhood);
         cloned_column_arrays[i] = neighborhood;
      }
   }
   */

   std::cout << "Target node: " << node << std::endl;
   std::cout << "Node of row_lengths: " << common::find_memory_node_for_addr(cloned_row_lengths) << std::endl;
   std::cout << "Node of row_arrays: " << common::find_memory_node_for_addr(cloned_row_arrays) << std::endl;
   std::cout << "Node of neighborhood: " << common::find_memory_node_for_addr(neighborhood) << std::endl;

   return new AOA_Matrix(
         matrix_size,
         this->cardinality,
         this->row_total_bytes_used,
         this->col_total_bytes_used,
         this->max_nbrhood_size,
         this->t,
         this->symmetric,
         cloned_row_lengths,
         cloned_row_arrays,
         cloned_column_lengths,
         cloned_column_arrays,
         external_ids);
}

