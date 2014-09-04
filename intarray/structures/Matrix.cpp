#include "Matrix.hpp"

Matrix::Matrix(VectorGraph *vg, bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), common::type t_in){
  t = t_in;
  num_columns = vg->num_nodes;
  indicies = new size_t[num_columns+1];

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
			index = integerarray::preprocess(tmp_data,index,filtered_hood,filter_index, t);
      delete[] filtered_hood;
  	}
  }
  data = new unsigned short[index];
  std::copy(tmp_data,tmp_data+index,data);
  cardinality = new_cardinality;
  indicies[num_columns] = index;
}

template<typename T> 
T Matrix::foreach_column(T (Matrix::*rowfunction)(unsigned int,T (*f)(unsigned int,unsigned int,Matrix*)), T (*f)(unsigned int,unsigned int,Matrix*)) {
	T reducer = (T) 0;
  #pragma omp parallel for default(none) shared(f,rowfunction) schedule(static,150) reduction(+:reducer) 
  for(size_t i = 0; i < num_columns; i++){
		reducer += (this->*rowfunction)(i,f);
	}
  return reducer;
}

template<typename T> 
T Matrix::for_row(unsigned int col,T (*function)(unsigned int,unsigned int,Matrix*)){
	size_t start = indicies[col];
	size_t end = indicies[col+1];
	return integerarray::foreach(function,col,this,data+start,end-start,t); //function(data(i))
}
void Matrix::print_matrix(){
	for(size_t i = 0; i < num_columns; i++){
		cout << "COLUMN: " << i << endl;
		size_t start = indicies[i];
		size_t end = indicies[i+1];
		integerarray::print_data(t,data+start,end-start);
	}
}
