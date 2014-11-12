#ifndef TABLE_H
#define TABLE_H

class Table{
  public:
    size_t num_tuples;
    size_t num_threads;

    size_t *table_size;
    unsigned int **table_pointers; //num_tuples*num_threads
    unsigned int *tuple; //num_tuples*num_threads


    size_t *thread_index;

    Table(size_t num_tuples_in, size_t num_threads_in, size_t cardinality){
      num_tuples = num_tuples_in;
      num_threads = num_threads_in;
      table_pointers = new unsigned int*[num_tuples*num_threads];
      tuple = new unsigned int[num_tuples*num_threads];
      table_size = new size_t[num_threads];

      for(size_t i = 0; i < num_threads; i++){
        table_size[i] = 0;
      }
      for(size_t i = 0; i < (num_threads*num_tuples); i++){
        table_pointers[i] = new unsigned int[(40*cardinality)/num_threads];
      }
    }

    ~Table(){
      delete[] tuple;
      for(size_t i = 0; i < num_threads; i++){
        delete[] table_pointers[i];
      }
    }

    void print_data(string filename);
};

void Table::print_data(string filename){
  ofstream myfile;
  myfile.open(filename);

  cout << "Writing table to: " << filename << endl;

  for(size_t t = 0; t < num_threads; t++){
    size_t t_size = table_size[t];
    for(size_t i = 0; i < t_size; i++){
      for(size_t j = 0; j < num_tuples; j++){
        unsigned int *column = table_pointers[t*num_tuples+j];
        myfile << column[i] << "\t";
      }
      myfile << endl;
    }
  }
}

#endif