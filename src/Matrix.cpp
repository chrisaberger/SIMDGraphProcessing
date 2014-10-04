#include "Matrix.hpp"

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
  cout << "Writing done" << endl;

  myfile.close();
}
