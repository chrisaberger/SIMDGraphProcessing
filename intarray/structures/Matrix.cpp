#include "Matrix.hpp"

Matrix::Matrix(VectorGraph *vg, bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), common::type t_in){
  t = t_in;
  num_columns = vg->num_nodes;
  indicies = new size_t[num_columns+1];
  row_lengths = new unsigned int[num_columns];

  unsigned short *tmp_data = new unsigned short[vg->num_edges*3]; 

	size_t new_cardinality = 0;
  size_t index = 0;
  for(size_t i = 0; i < vg->num_nodes; ++i) {
  	indicies[i] = index;
  	if(nodeFilter(i)){
	    vector<unsigned int> *hood = vg->neighborhoods->at(i);
	    unsigned int *filtered_hood = new unsigned int[hood->size()];
	    size_t filter_index = 0;
	    for(size_t j = 0; j < hood->size(); ++j) {
	    	if(nodeFilter(hood->at(j)) && edgeFilter(i,hood->at(j))){
	    		new_cardinality++;
	    		filtered_hood[filter_index++] = hood->at(j);
	    	}
	    }
      //size_t start_index = index;
      row_lengths[i] = filter_index;
			index = integerarray::preprocess(tmp_data,index,filtered_hood,filter_index, t);
      delete[] filtered_hood;
  	}
  }
  data = new unsigned short[index];
  cout << "Data Length (Bytes): " << index/2 << endl;
  std::copy(tmp_data,tmp_data+index,data);
  cardinality = new_cardinality;
  indicies[num_columns] = index;
}

template<typename T> 
T Matrix::foreach_column(T (Matrix::*rowfunction)(unsigned int,T (*f)(unsigned int,unsigned int)), T (*f)(unsigned int,unsigned int)) {
	T reducer = (T) 0;
  #pragma omp parallel for default(none) shared(f,rowfunction) schedule(static,150) reduction(+:reducer) 
  for(size_t i = 0; i < num_columns; i++){
		reducer += (this->*rowfunction)(i,f);
	}
  return reducer;
}

template<typename T> 
T Matrix::for_row(unsigned int col,T (*function)(unsigned int,unsigned int)){
	size_t start = indicies[col];
	size_t end = indicies[col+1];
  unsigned int length = row_lengths[col];
	return integerarray::foreach(function,col,data+start,end-start,length,t); //function(data(i))
}
inline size_t Matrix::row_intersect(unsigned short *R, unsigned int i, unsigned int j){
  size_t i_start = indicies[i];
  size_t i_end = indicies[i+1];

  size_t j_start = indicies[j];
  size_t j_end = indicies[j+1];

  unsigned int i_size = row_lengths[i];
  unsigned int j_size = row_lengths[j];

  long ncount = integerarray::intersect(R,data+i_start,data+j_start,i_end-i_start,j_end-j_start,i_size,j_size,t);
  return ncount;
}
void Matrix::print_columns(unsigned int i, unsigned int j){
  cout << "COLUMN: " << i << endl;
  size_t start = indicies[i];
  size_t end = indicies[i+1];
  unsigned int size = row_lengths[i];
  integerarray::print_data(t,data+start,end-start,size);

  cout << "COLUMN: " << j << endl;
  start = indicies[j];
  end = indicies[j+1];
  size = row_lengths[j];
  integerarray::print_data(t,data+start,end-start,size);
}
void Matrix::print_matrix(){
	for(size_t i = 0; i < num_columns; i++){
		cout << "COLUMN: " << i << endl;
		size_t start = indicies[i];
		size_t end = indicies[i+1];
    unsigned int size = row_lengths[i];
		integerarray::print_data(t,data+start,end-start,size);
	}
}
