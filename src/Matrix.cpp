#include "Matrix.hpp"
//Undirected Graphs
Matrix::Matrix(vector< vector<unsigned int>*  > *g, size_t matrix_size_in, size_t cardinality_in, 
  bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), 
  unordered_map<unsigned int,unsigned int> *external_ids_in, common::type t_in){
  
  array16::prepare_shuffling_dictionary16();
  hybrid::prepare_shuffling_dictionary();

  size_t *row_indicies_in = new size_t[matrix_size_in+1];
  unsigned int *row_lengths_in = new unsigned int[matrix_size_in];
  uint8_t *tmp_row_data = new uint8_t[cardinality_in*40]; 

  size_t new_cardinality = 0;
  size_t index = 0;

  for(size_t i = 0; i < matrix_size_in; ++i){
    if(nodeFilter(i)){
      row_indicies_in[i] = index;
      vector<unsigned int> *row = g->at(i);
      unsigned int *selected_row = new unsigned int[row->size()];
      size_t new_size = 0;
      for(size_t j = 0; j < row->size(); ++j) {
        if(nodeFilter(row->at(j)) && edgeFilter(i,row->at(j))){
          new_cardinality++;
          selected_row[new_size++] = row->at(j);
        } 
      }
      row_lengths_in[i] = new_size;
      if(new_size > 0){
        common::type row_type = Matrix::get_array_type(t_in,selected_row,new_size,matrix_size_in);
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
Matrix::Matrix(vector< vector<unsigned int>*  > *out_nbrs,vector< vector<unsigned int>*  > *in_nbrs, size_t matrix_size_in, size_t cardinality_in, 
  bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), unordered_map<unsigned int,unsigned int> *external_ids_in, common::type t_in){
  array16::prepare_shuffling_dictionary16();
  hybrid::prepare_shuffling_dictionary();  

  size_t *row_indicies_in = new size_t[matrix_size_in+1];
  unsigned int *row_lengths_in = new unsigned int[matrix_size_in];
  uint8_t *tmp_row_data = new uint8_t[cardinality_in*40]; 

  size_t new_cardinality = 0;
  size_t index = 0;
  size_t nbr_i = 0;

  for(size_t i = 0; i < matrix_size_in; ++i) {
    row_indicies_in[i] = index;
    if(nbr_i < out_nbrs->size() && out_nbrs->at(nbr_i)->at(0) == i){
      vector<unsigned int> *hood = out_nbrs->at(nbr_i);
      size_t node = hood->at(0);
      if(nodeFilter(node)){
        unsigned int *filtered_hood = new unsigned int[hood->size()-1];
        size_t filter_index = 0;
        for(size_t j = 1; j < hood->size(); ++j) {
          if(nodeFilter(hood->at(j)) && edgeFilter(i,hood->at(j))){
            new_cardinality++;
            filtered_hood[filter_index++] = hood->at(j);
          } 
        }
        row_lengths_in[i] = filter_index;
        const common::type row_type = Matrix::get_array_type(t_in,filtered_hood,filter_index,matrix_size_in); //depends on above array being set
        index = uint_array::preprocess(tmp_row_data,index,filtered_hood,filter_index,matrix_size_in,row_type);
        delete[] filtered_hood;
      }
      nbr_i++;
    } else{
      row_lengths_in[i] = 0;
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
  uint8_t *tmp_col_data = new uint8_t[cardinality_in*40]; 

  for(size_t i = 0; i < matrix_size_in; ++i) {
    col_indicies_in[i] = index;
    if(nbr_i < in_nbrs->size() && in_nbrs->at(nbr_i)->at(0) == i){
      vector<unsigned int> *hood = in_nbrs->at(nbr_i);
      size_t node = hood->at(0);
      if(nodeFilter(node)){
        unsigned int *filtered_hood = new unsigned int[hood->size()-1];
        size_t filter_index = 0;
        size_t prev = 10000000000;
        size_t num_hit = 0;

        for(size_t j = 1; j < hood->size(); ++j) {
          if(nodeFilter(hood->at(j)) && edgeFilter(i,hood->at(j))){
            new_cardinality++;
            if(prev >= (hood->at(j)-3)){
              num_hit++;
            }
            filtered_hood[filter_index++] = hood->at(j);
            prev = hood->at(j);
          } 
        }
        col_lengths_in[i] = filter_index;
        const common::type col_type = Matrix::get_array_type(t_in,filtered_hood,filter_index,matrix_size_in); //depends on above array being set
        index = uint_array::preprocess(tmp_col_data,index,filtered_hood,filter_index,matrix_size_in,col_type);
        delete[] filtered_hood;
      }
      nbr_i++;
    } else{
      col_lengths_in[i] = 0;
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

  symmetric = false;
  external_ids = external_ids_in;

  column_indicies = col_indicies_in;
  column_lengths = col_lengths_in;
  column_data = col_data_in;
}

void Matrix::print_rows(unsigned int i, unsigned int j, string filename){
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

void Matrix::print_column(unsigned int i, string filename){
  ofstream myfile;
  myfile.open(filename);

  myfile << "COL: " << i << endl;
  size_t start = column_indicies[i];
  size_t end = column_indicies[i+1];
  size_t card = column_lengths[i];
  uint_array::print_data(column_data+start,end-start,card,t,myfile);
}

void Matrix::print_data(string filename){
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
/*
void Matrix::createN2X(){
  n2x = new vector< vector<unsigned int>*  >(matrix_size);
  //n2x_counts = new vector< vector<unsigned int>*  >(matrix_size);

  //parallel
  for(size_t i = 0; i < matrix_size; i++){
    size_t start = row_indicies[i];
    size_t end = row_indicies[i+1];
    size_t card = row_lengths[i];
    const common::type row_type = (common::type) row_types[i];

    unsigned int *union_data = new unsigned int[matrix_size];
    unsigned int *prev_union = new unsigned int[matrix_size];
    size_t union_size = 0;

    unsigned int *neighbors = new unsigned int[card];
    uint_array::get_a32(neighbors,row_data+start,end-start,card,row_type);
    for(size_t j = 0; j < card; j++){
      unsigned int nbr = neighbors[j];
      size_t start2 = row_indicies[nbr];
      size_t end2 = row_indicies[nbr+1];
      size_t card2 = row_lengths[nbr];
      const common::type row_type2 = (common::type) row_types[nbr];

      unsigned int *next_input2 = new unsigned int[card2];
      uint_array::get_a32(next_input2,row_data+start2,end2-start2,card2,row_type2);

      union_size = array32::set_union(union_data,prev_union,next_input2,union_size,card2);
      unsigned int *tmp = prev_union;
      prev_union = union_data;
      union_data = tmp;
      delete[] next_input2;
    }
    union_data = prev_union;
    
    ///count up how many times it appears
    //this isn't right actually need back edges to list it.
    vector<unsigned int> *un_count = new vector<unsigned int>(union_data,union_data+union_size);
    for(size_t j = 0; j < card; j++){
      unsigned int nbr = neighbors[j];
      size_t start2 = row_indicies[nbr];
      size_t end2 = row_indicies[nbr+1];
      size_t card2 = row_lengths[nbr];
      const common::type row_type2 = (common::type) row_types[nbr];
      
      unsigned int *next_input2 = new unsigned int[card2];
      uint_array::get_a32(next_input2,row_data+start2,end2-start2,card2,row_type2);

      size_t i_a=0, i_b= 0;
      size_t s_a, s_b;
      s_a = union_size;
      s_b = card2;
      unsigned int *A = union_data;
      unsigned int *B = next_input2;
      bool notFinished = i_a < s_a  && i_b < s_b;
      while(notFinished){
        while(notFinished && B[i_b] < A[i_a]){
          ++i_b;
          notFinished = i_b < s_b;
        }
        if(notFinished && A[i_a] == B[i_b]){
          un_count->at(i_a)++;
        }
        ++i_a;
        notFinished = notFinished && i_a < s_a;
      }
    }
    vector<unsigned int> *un = new vector<unsigned int>(union_data,union_data+union_size);
    n2x->at(i) = un;
    //n2x_counts->at(i) = un_count;
    union_size = 0;
  }
  //intersect n2x across all sets and take output of each +1 in incrementer array
}
*/
