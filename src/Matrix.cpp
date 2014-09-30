#include "Matrix.hpp"

//Undirected Graphs
Matrix Matrix::buildSymetric(vector< vector<unsigned int>*  > *g, size_t matrix_size_in, size_t cardinality_in, 
  bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), common::type t_in){
  array16::prepare_shuffling_dictionary16();

  size_t *row_indicies_in = new size_t[matrix_size_in+1];
  unsigned int *row_lengths_in = new unsigned int[matrix_size_in];
  uint8_t *row_types_in = new uint8_t[matrix_size_in];
  uint8_t *tmp_row_data = new uint8_t[cardinality_in*40]; 

  size_t new_cardinality = 0;
  size_t index = 0;

  for(size_t i = 0; i < matrix_size_in; ++i) {
    if(nodeFilter(i)){
      row_indicies_in[i] = index;
      vector<unsigned int> *hood = g->at(i);
      hood->erase(hood->begin()); //we know the ID here equals the index-undirected is easy.
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
  }

  uint8_t *row_data_in = new uint8_t[index];
  cout << "ROW DATA SIZE (Bytes): " << index << endl;
  std::copy(tmp_row_data,tmp_row_data+index,row_data_in);
  row_indicies_in[matrix_size_in] = index;

  return Matrix(matrix_size_in,new_cardinality,t_in,row_indicies_in,row_lengths_in,row_types_in,row_data_in);
}
//Directed Graph
Matrix Matrix::buildAsymetric(vector< vector<unsigned int>*  > *out_nbrs,vector< vector<unsigned int>*  > *in_nbrs, size_t matrix_size_in, size_t cardinality_in, 
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

  return Matrix(matrix_size_in,new_cardinality,t_in,
    row_indicies_in,row_lengths_in,row_types_in,row_data_in,
    col_indicies_in,col_lengths_in,col_types_in,col_data_in);
}

void Matrix::print_rows(unsigned int i, unsigned int j){
  ofstream myfile;
  myfile.open("debug.txt");

  cout << "ROW: " << i << endl;
  size_t start = row_indicies[i];
  size_t end = row_indicies[i+1];
  size_t card = row_lengths[i];
  common::type row_type = (common::type) row_types[i];
  uint_array::print_data(row_data+start,end-start,card,row_type,myfile);

  cout << "ROW: " << j << endl;
  start = row_indicies[j];
  end = row_indicies[j+1];
  card = row_lengths[j];
  cout << "start: " << start << " end: " << end << endl;
  row_type = (common::type) row_types[j];
  uint_array::print_data(row_data+start,end-start,card,row_type,myfile);
}

void Matrix::print_data(string filename){
  ofstream myfile;
  myfile.open(filename);

  //Printing out neighbors
  cout << "Writing matrix row_data to file: " << filename << endl;
	for(size_t i = 0; i < matrix_size; i++){
		myfile << "ROW: " << i << endl;
		size_t start = row_indicies[i];
		size_t end = row_indicies[i+1];
    size_t card = row_lengths[i];
    const common::type row_type = (common::type) row_types[i];

		uint_array::print_data(row_data+start,end-start,card,row_type,myfile);
	}
  myfile << endl;
  //Printing in neighbors
  if(!symmetric){
    for(size_t i = 0; i < matrix_size; i++){
      myfile << "COLUMN: " << i << endl;
      size_t start = column_indicies[i];
      size_t end = column_indicies[i+1];
      size_t card = column_lengths[i];
      const common::type col_type = (common::type) column_types[i];
      uint_array::print_data(column_data+start,end-start,card,col_type,myfile);
    }
  }

  myfile.close();
}
