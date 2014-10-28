#include "AOA_Matrix.hpp"

AOA_Matrix::AOA_Matrix(const vector< vector<unsigned int>*  > *g,const size_t matrix_size_in,const size_t cardinality_in, 
  const std::function<bool(unsigned int)> node_selection,const std::function<bool(unsigned int,unsigned int)> edge_selection, 
  const unordered_map<unsigned int,unsigned int> *external_ids_in, const common::type t_in){
  
  array16::prepare_shuffling_dictionary16();
  hybrid::prepare_shuffling_dictionary();

  uint8_t **tmp_row_arrays = new uint8_t*[matrix_size_in+1];
  unsigned int *row_lengths_in = new unsigned int[matrix_size_in];

  cout << "Number of edges: " << cardinality_in << endl;

  size_t new_cardinality = 0;
  size_t total_bytes_used = 0;

  #pragma omp parallel for default(shared) schedule(dynamic) reduction(+:total_bytes_used) reduction(+:new_cardinality)
  for(size_t i = 0; i < matrix_size_in; ++i){
    size_t index = 0;
    if(node_selection(i)){
      vector<unsigned int> *row = g->at(i);
      unsigned int *selected_row = new unsigned int[row->size()];
      size_t new_size = 0;
      for(size_t j = 0; j < row->size(); ++j) {
        if(node_selection(row->at(j)) && edge_selection(i,row->at(j))){
          new_cardinality++;
          selected_row[new_size++] = row->at(j);
        } 
      }
      row_lengths_in[i] = new_size;
      
      uint8_t *tmp_row_data = new uint8_t[row->size()*6];
      unsigned int *len_info = (unsigned int*) tmp_row_data;
      if(new_size > 0){
        index+=4;
        common::type row_type = uint_array::get_array_type(t_in,selected_row,new_size,matrix_size_in);
        index = uint_array::preprocess(tmp_row_data,index,selected_row,new_size,matrix_size_in,row_type);
        len_info[0] = (unsigned int) (index-4);
      }
      uint8_t *row_data_in = new uint8_t[index];
      std::copy(tmp_row_data,tmp_row_data+index,row_data_in);
    
      total_bytes_used += index;
      tmp_row_arrays[i] = row_data_in;

      delete[] tmp_row_data;  
      delete[] selected_row;
    } else{
      row_lengths_in[i] = 0;
    }
  }

  matrix_size = matrix_size_in;
  cardinality = new_cardinality;
  t = t_in;
  symmetric = true;
  
  row_lengths = row_lengths_in;
  column_lengths = row_lengths_in;

  row_arrays = tmp_row_arrays;
  column_arrays = tmp_row_arrays;

  external_ids = external_ids_in;

  cout << "ROW DATA SIZE (Bytes): " << total_bytes_used << endl;
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
      unsigned int *size_ptr = (unsigned int*) row_arrays[i];
      uint_array::print_data(row_arrays[i]+4,size_ptr[0],card,t,myfile);
    }
  }
  myfile << endl;
  //Printing in neighbors
  if(!symmetric){
    for(size_t i = 0; i < matrix_size; i++){
      myfile << "COLUMN: " << i << " LEN: " << column_lengths[i] << endl;
      size_t card = column_lengths[i];
      if(card > 0){
        unsigned int *size_ptr = (unsigned int*) column_arrays[i];
        uint_array::print_data(column_arrays[i]+4,size_ptr[0],card,t,myfile);
      }
    }
  }

  myfile.close();
}