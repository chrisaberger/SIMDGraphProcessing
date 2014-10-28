#include "CSR_Matrix.hpp"
//Undirected Graphs
CSR_Matrix::CSR_Matrix(vector< vector<unsigned int>*  > *g, size_t matrix_size_in, size_t cardinality_in, 
  std::function<bool(unsigned int)> node_selection,std::function<bool(unsigned int,unsigned int)> edge_selection, 
  unordered_map<unsigned int,unsigned int> *external_ids_in, common::type t_in){
  
  array16::prepare_shuffling_dictionary16();
  hybrid::prepare_shuffling_dictionary();

  size_t *row_indicies_in = new size_t[matrix_size_in+1];
  unsigned int *row_lengths_in = new unsigned int[matrix_size_in];
  uint8_t *tmp_row_data = new uint8_t[matrix_size_in*25000]; 

  cout << "Number of edges: " << cardinality_in << endl;

  size_t new_cardinality = 0;
  size_t index = 0;

  for(size_t i = 0; i < matrix_size_in; ++i){
    if(node_selection(i)){
      row_indicies_in[i] = index;
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
      if(new_size > 0){
        common::type row_type = uint_array::get_array_type(t_in,selected_row,new_size,matrix_size_in);
        index = uint_array::preprocess(tmp_row_data,index,selected_row,new_size,matrix_size_in,row_type);
      }
      delete[] selected_row;
    } else{
      row_lengths_in[i] = 0;
    }
  }

  cout << "ROW DATA SIZE (Bytes): " << index << endl;
  uint8_t *row_data_in = new uint8_t[index];
  std::copy(tmp_row_data,tmp_row_data+index,row_data_in);
  row_indicies_in[matrix_size_in] = index;

  matrix_size = matrix_size_in;
  cardinality = cardinality_in;
  t = t_in;

  row_indicies = row_indicies_in;
  row_lengths = row_lengths_in;
  row_data = row_data_in;
  
  symmetric = true;
  external_ids = external_ids_in;

  column_indicies = row_indicies_in;
  column_lengths = row_lengths_in;
  column_data = row_data_in;
}
//Directed Graph
CSR_Matrix::CSR_Matrix(vector< vector<unsigned int>*  > *out_nbrs,vector< vector<unsigned int>*  > *in_nbrs, size_t matrix_size_in, size_t cardinality_in, 
  std::function<bool(unsigned int)> node_selection,std::function<bool(unsigned int,unsigned int)> edge_selection, 
  unordered_map<unsigned int,unsigned int> *external_ids_in, common::type t_in){

  array16::prepare_shuffling_dictionary16();
  hybrid::prepare_shuffling_dictionary();  

  cout << "Number of edges: " << cardinality_in << endl;

  size_t *row_indicies_in = new size_t[matrix_size_in+1];
  unsigned int *row_lengths_in = new unsigned int[matrix_size_in];
  uint8_t *tmp_row_data = new uint8_t[cardinality_in*40]; 
  size_t *col_indicies_in = new size_t[matrix_size_in+1];
  unsigned int *col_lengths_in = new unsigned int[matrix_size_in];
  uint8_t *tmp_col_data = new uint8_t[cardinality_in*40]; 

  size_t new_cardinality = 0;
  size_t index_o = 0;
  size_t index_i = 0;

  for(size_t i = 0; i < matrix_size_in; ++i) {
    row_indicies_in[i] = index_o;
    col_indicies_in[i] = index_i;

    vector<unsigned int> *row = out_nbrs->at(i);
    vector<unsigned int> *col = in_nbrs->at(i);
    if(node_selection(i)){
      unsigned int *filtered_row = new unsigned int[row->size()];      
      size_t new_row_size = 0;
      for(size_t j = 0; j < row->size(); ++j) {
        if(node_selection(row->at(j)) && edge_selection(i,row->at(j))){
          new_cardinality++;
          filtered_row[new_row_size++] = row->at(j);
        } 
      }

      row_lengths_in[i] = new_row_size;
      const common::type row_type = uint_array::get_array_type(t_in,filtered_row,new_row_size,matrix_size_in); //depends on above array being set
      index_o = uint_array::preprocess(tmp_row_data,index_o,filtered_row,new_row_size,matrix_size_in,row_type);
      delete[] filtered_row;

      unsigned int *filtered_col = new unsigned int[col->size()];
      size_t new_col_size = 0;
      for(size_t j = 0; j < col->size(); ++j) {
        if(node_selection(col->at(j)) && edge_selection(i,col->at(j))){
          new_cardinality++;
          filtered_col[new_col_size++] = col->at(j);
        } 
      }

      col_lengths_in[i] = new_col_size;
      const common::type col_type = uint_array::get_array_type(t_in,filtered_col,new_col_size,matrix_size_in); //depends on above array being set
      index_i = uint_array::preprocess(tmp_col_data,index_i,filtered_col,new_col_size,matrix_size_in,col_type);
      delete[] filtered_col;
    }
  }

  uint8_t *row_data_in = new uint8_t[index_o];
  cout << "ROW DATA SIZE (Bytes): " << index_o << endl;
  std::copy(tmp_row_data,tmp_row_data+index_o,row_data_in);
  row_indicies_in[matrix_size_in] = index_o;

  uint8_t *col_data_in = new uint8_t[index_i];
  cout << "COLUMN DATA SIZE (Bytes): " << index_i << endl;
  std::copy(tmp_col_data,tmp_col_data+index_i,col_data_in);
  col_indicies_in[matrix_size_in] = index_i;

  matrix_size = matrix_size_in;
  cardinality = cardinality_in;
  t = t_in;
  
  row_indicies = row_indicies_in;
  row_lengths = row_lengths_in;
  row_data = row_data_in;

  symmetric = false;
  external_ids = external_ids_in;

  column_indicies = col_indicies_in;
  column_lengths = col_lengths_in;
  column_data = col_data_in;
}

void CSR_Matrix::print_rows(unsigned int i, unsigned int j, string filename){
  ofstream myfile;
  myfile.open(filename);

  myfile << "ROW: " << i << endl;
  size_t start = row_indicies[i];
  size_t end = row_indicies[i+1];
  size_t card = row_lengths[i];
  uint_array::print_data(row_data+start,end-start,card,t,myfile);

  myfile << "ROW: " << j << endl;
  start = row_indicies[j];
  end = row_indicies[j+1];
  card = row_lengths[j];
  uint_array::print_data(row_data+start,end-start,card,t,myfile);
}

void CSR_Matrix::print_column(unsigned int i, string filename){
  ofstream myfile;
  myfile.open(filename);

  myfile << "COL: " << i << endl;
  size_t start = column_indicies[i];
  size_t end = column_indicies[i+1];
  size_t card = column_lengths[i];
  uint_array::print_data(column_data+start,end-start,card,t,myfile);
}

void CSR_Matrix::print_data(string filename){
  ofstream myfile;
  myfile.open(filename);

  //Printing out neighbors
  cout << "Writing matrix row_data to file: " << filename << endl;
	for(size_t i = 0; i < matrix_size; i++){
		myfile << "ROW: " << i <<  " LEN: " << row_lengths[i] << endl;
    size_t card = row_lengths[i];
    if(card > 0){
      size_t start = row_indicies[i];
      size_t end = row_indicies[i+1];
      uint_array::print_data(row_data+start,end-start,card,t,myfile);
    }
	}
  myfile << endl;
  //Printing in neighbors
  if(!symmetric){
    for(size_t i = 0; i < matrix_size; i++){
      myfile << "COLUMN: " << i << " LEN: " << column_lengths[i] << endl;
      size_t card = column_lengths[i];
      if(card > 0){
        size_t start = column_indicies[i];
        size_t end = column_indicies[i+1];
        uint_array::print_data(column_data+start,end-start,card,t,myfile);
      }
    }
  }

  myfile.close();
}