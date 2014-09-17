#include "Matrix.hpp"

Matrix::Matrix(VectorGraph *vg, bool (*nodeFilter)(unsigned int), bool (*edgeFilter)(unsigned int,unsigned int), common::type t_in){
  array16::prepare_shuffling_dictionary16();
  
  t = t_in;
  num_rows = vg->num_nodes;
  indicies = new size_t[num_rows+1];
  row_lengths = new unsigned int[num_rows];
  row_types = new unsigned char[num_rows];

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
      row_lengths[i] = filter_index;
      const common::type row_type = get_row_type(i,filtered_hood); //depends on above array being set
      row_types[i] = row_type;
      index = integerarray::preprocess(tmp_data,index,filtered_hood,filter_index,row_type);
      delete[] filtered_hood;
  	}
  }
  data = new unsigned short[index];
  cout << "Data Length (Bytes): " << index/2 << endl;
  std::copy(tmp_data,tmp_data+index,data);
  cardinality = new_cardinality;
  indicies[num_rows] = index;
}

inline common::type Matrix::get_row_type(unsigned int r, unsigned int *row_data){
  #if HYBRID_LAYOUT == 1
  if(t == common::HYBRID){
    return get_hybrid_row_type(r,row_data);
  } else{
    return t;
  }
  #else
  return t;
  #endif
}

inline common::type Matrix::get_hybrid_row_type(unsigned int r, unsigned int *row_data){
  size_t row_size = row_lengths[r];
  double sparsity = (double) row_size/num_rows;
  //unsigned int num_bins = ((row_data[row_size-1] >> 16) - (row_data[0] >> 16) + 1) ;
  //cout << sparsity << endl;
  if( sparsity > (double) 1/32 ){
    //cout << "BITSET" << endl;
    return common::BITSET;
  } 
  //size/num_bins = average # per bin, > 8 say yes
  else if(row_size != 0 && 
    (row_size/((row_data[row_size-1] >> 16) - (row_data[0] >> 16) + 1)) > 8){
    //cout << "ARRAY16" << endl;
    return common::ARRAY16;
  } else{
    return common::ARRAY32;
  }
}

template<typename T> 
T Matrix::foreach_row(T (Matrix::*rowfunction)(unsigned int,T (*f)(unsigned int,unsigned int)), T (*f)(unsigned int,unsigned int)) {
	T reducer = (T) 0;
  #pragma omp parallel for default(none) shared(f,rowfunction) schedule(static,150) reduction(+:reducer) 
  for(size_t i = 0; i < num_rows; i++){
		reducer += (this->*rowfunction)(i,f);
	}
  return reducer;
}

template<typename T> 
T Matrix::for_row(unsigned int col,T (*function)(unsigned int,unsigned int)){
	size_t start = indicies[col];
	size_t end = indicies[col+1];
  const common::type row_type = (common::type) row_types[col];
	return integerarray::foreach(function,col,data+start,end-start,row_type);
}
inline size_t Matrix::row_intersect(unsigned short *R, unsigned int i, unsigned int j){
  size_t i_start = indicies[i];
  size_t i_end = indicies[i+1];

  size_t j_start = indicies[j];
  size_t j_end = indicies[j+1];

  long ncount;
  #if HYBRID_LAYOUT == 1
  const common::type t1 = (common::type) row_types[i];
  const common::type t2 = (common::type) row_types[j];
  if(t1 == t2){
    ncount = integerarray::intersect(R,data+i_start,data+j_start,i_end-i_start,j_end-j_start,t1);
  } else{
    ncount = integerarray::intersect(R,data+i_start,data+j_start,i_end-i_start,j_end-j_start,t1,t2);
  }
  #else 
    ncount = integerarray::intersect(R,data+i_start,data+j_start,i_end-i_start,j_end-j_start,t);
  #endif

  return ncount;
}
void Matrix::print_rows(unsigned int i, unsigned int j){
  cout << "ROW: " << i << endl;
  size_t start = indicies[i];
  size_t end = indicies[i+1];
  common::type row_type = (common::type) row_types[i];
  integerarray::print_data(data+start,end-start,row_type);

  cout << "ROW: " << j << endl;
  start = indicies[j];
  end = indicies[j+1];
  cout << "start: " << start << " end: " << end << endl;
  row_type = (common::type) row_types[j];
  integerarray::print_data(data+start,end-start,row_type);
}
void Matrix::print_matrix(){
	for(size_t i = 0; i < num_rows; i++){
		cout << "ROW: " << i << endl;
		size_t start = indicies[i];
		size_t end = indicies[i+1];
    const common::type row_type = (common::type) row_types[i];
		integerarray::print_data(data+start,end-start,row_type);
	}
}
