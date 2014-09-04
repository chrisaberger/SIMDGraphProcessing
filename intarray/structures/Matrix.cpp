#include "Matrix.hpp"

Matrix::Matrix(VectorGraph *vg, bool (*nodeFilter)(int), bool (*edgeFilter)(int,int), common::type t_in){
  t = t_in;
  num_columns = vg->num_nodes;
  indicies = new size_t[num_columns+1];

  short *tmp_data = new short[vg->num_edges*3]; 

	size_t new_cardinality = 0;
  size_t index = 0;
  for(size_t i = 0; i < vg->num_nodes; ++i) {
  	indicies[i] = index;
  	if(nodeFilter(i)){
	    vector<unsigned int> *hood = vg->neighborhoods->at(i);
	    int *filtered_hood = new int[hood->size()];
	    size_t filter_index = 0;
	    for(size_t j = 0; j < hood->size(); ++j) {
	    	if(nodeFilter(hood->at(j)) && edgeFilter(i,hood->at(j))){
	    		new_cardinality++;
	    		filtered_hood[filter_index++] = hood->at(j);
	    	}
	    }
      //size_t start_index = index;
			index = integerarray::preprocess(tmp_data,index,filtered_hood,filter_index, t);
      delete[] filtered_hood;
  	}
  }
  data = new short[index];
  std::copy(tmp_data,tmp_data+index,data);
  cardinality = new_cardinality;
  indicies[num_columns] = index;
}

template<typename T> 
void Matrix::foreach_column(void (Matrix::*rowfunction)(int,T (*f)(int,int,Matrix*)), T (*f)(int,int,Matrix*)) {
	for(size_t i = 0; i < num_columns; i++){
		(this->*rowfunction)(i,f);
	}
}

template<typename T> 
void Matrix::for_row(int col,T (*function)(int,int,Matrix*)){
	size_t start = indicies[col];
	size_t end = indicies[col+1];
	integerarray::foreach(function,col,this,data+start,end-start,t); //function(data(i))
}

void Matrix::print_matrix(){
	for(size_t i = 0; i < num_columns; i++){
		cout << "COLUMN: " << i << endl;
		size_t start = indicies[i];
		size_t end = indicies[i+1];
		integerarray::print_data(t,data+start,end-start);
	}
}
