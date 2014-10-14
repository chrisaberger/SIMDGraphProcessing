#include "Matrix.hpp"
//Undirected Graphs
Matrix::Matrix(vector< vector<unsigned int>*  > *g, size_t matrix_size_in, size_t cardinality_in, 
  bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), common::type t_in){
  array16::prepare_shuffling_dictionary16();

  size_t *row_indicies_in = new size_t[matrix_size_in+1];
  unsigned int *row_lengths_in = new unsigned int[matrix_size_in];
  uint8_t *row_types_in = new uint8_t[matrix_size_in];
  uint8_t *tmp_row_data = new uint8_t[cardinality_in*40]; 

  size_t new_cardinality = 0;
  size_t index = 0;

  for(size_t i = 0; i < matrix_size_in; ++i){
    if(nodeFilter(i)){
      row_indicies_in[i] = index;
      vector<unsigned int> *hood = g->at(i);
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
      row_types_in[i] = row_type;

      index = uint_array::preprocess(tmp_row_data,index,filtered_hood,filter_index,row_type);
      delete[] filtered_hood;
    }
  }

  uint8_t *row_data_in = new uint8_t[index];
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

  //return Matrix(matrix_size_in,new_cardinality,t_in,row_indicies_in,row_lengths_in,row_types_in,row_data_in);
}
//Directed Graph
Matrix::Matrix(vector< vector<unsigned int>*  > *out_nbrs,vector< vector<unsigned int>*  > *in_nbrs, size_t matrix_size_in, size_t cardinality_in, 
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

  cout << in_nbrs->at(1)->at(0) << endl;

  for(size_t i = 0; i < matrix_size_in; ++i) {
    col_indicies_in[i] = index;
    cout << nbr_i << " " << in_nbrs->size() <<" " << i << " " << in_nbrs->at(nbr_i)->at(0) << endl;
    if(nbr_i < in_nbrs->size() && in_nbrs->at(nbr_i)->at(0) == i){
      cout << "incrementer" << endl;
      nbr_i++;
      vector<unsigned int> *hood = out_nbrs->at(i);
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

        cout << "Node: " << i << " num_hit: " << num_hit << " deg: " << filter_index << endl;

        col_lengths_in[i] = filter_index;
        const common::type col_type = Matrix::get_array_type(t_in,filtered_hood,filter_index,matrix_size_in); //depends on above array being set
        col_types_in[i] = col_type;
        index = uint_array::preprocess(tmp_col_data,index,filtered_hood,filter_index,col_type);
        delete[] filtered_hood;
      }
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

void Matrix::print_rows(unsigned int i, unsigned int j, string filename){
  ofstream myfile;
  myfile.open(filename);

  myfile << "ROW: " << i << endl;
  size_t start = row_indicies[i];
  size_t end = row_indicies[i+1];
  size_t card = row_lengths[i];
  common::type row_type = (common::type) row_types[i];
  uint_array::print_data(row_data+start,end-start,card,row_type,myfile);

  myfile << "ROW: " << j << endl;
  start = row_indicies[j];
  end = row_indicies[j+1];
  card = row_lengths[j];
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
    /*
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
    */

    vector<unsigned int> *un = new vector<unsigned int>(union_data,union_data+union_size);
    n2x->at(i) = un;
    //n2x_counts->at(i) = un_count;
    union_size = 0;
  }
  //intersect n2x across all sets and take output of each +1 in incrementer array
}
