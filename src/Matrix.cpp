#include "Matrix.hpp"

Matrix::Matrix(vector< vector<unsigned int>*  > *g, size_t num_rows, size_t cardinality, bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), common::type t_in):
  num_rows(0),  //number of nodes
  cardinality(0),  //number of edges
  t(common::ARRAY32),
  indicies(new size_t[0]), //formerly nodes
  row_lengths(new unsigned int[0]),
  row_types(new unsigned char[0]),
  data(new uint8_t[0]) //formerly edges
{
  array16::prepare_shuffling_dictionary16();
  
  t = t_in;
  indicies = new size_t[num_rows+1];
  row_lengths = new unsigned int[num_rows];
  row_types = new unsigned char[num_rows];

  uint8_t *tmp_data = new uint8_t[cardinality*3]; 

  size_t new_cardinality = 0;
  size_t index = 0;
  for(size_t i = 0; i < num_rows; ++i) {
    indicies[i] = index;
    if(nodeFilter(i)){
      vector<unsigned int> *hood = g->at(i);
      unsigned int *filtered_hood = new unsigned int[hood->size()];
      size_t filter_index = 0;
      for(size_t j = 0; j < hood->size(); ++j) {
        if(nodeFilter(hood->at(j)) && edgeFilter(i,hood->at(j))){
          new_cardinality++;
          filtered_hood[filter_index++] = hood->at(j);
        }
      }
      row_lengths[i] = filter_index;
      const common::type row_type = get_row_type(i,filtered_hood); //depends on above array being set
      row_types[i] = row_type;
      index = uint_array::preprocess(tmp_data,index,filtered_hood,filter_index,row_type);
      delete[] filtered_hood;
    }
  }
  data = new uint8_t[index];
  cout << "Data Length (Bytes): " << index << endl;
  std::copy(tmp_data,tmp_data+index,data);
  cardinality = new_cardinality;
  indicies[num_rows] = index;
}

void Matrix::print_rows(unsigned int i, unsigned int j){
  cout << "ROW: " << i << endl;
  size_t start = indicies[i];
  size_t end = indicies[i+1];
  size_t card = row_lengths[i];
  common::type row_type = (common::type) row_types[i];
  uint_array::print_data(data+start,end-start,card,row_type);

  cout << "ROW: " << j << endl;
  start = indicies[j];
  end = indicies[j+1];
  card = row_lengths[j];
  cout << "start: " << start << " end: " << end << endl;
  row_type = (common::type) row_types[j];
  uint_array::print_data(data+start,end-start,card,row_type);
}

void Matrix::print_matrix(){
	for(size_t i = 0; i < num_rows; i++){
		cout << "ROW: " << i << endl;
		size_t start = indicies[i];
		size_t end = indicies[i+1];
    size_t card = row_lengths[i];
    const common::type row_type = (common::type) row_types[i];
		uint_array::print_data(data+start,end-start,card,row_type);
	}
}
