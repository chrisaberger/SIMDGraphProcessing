#ifndef TABLE_H
#define TABLE_H

class Table{
  public:
    size_t num_tuples;
    size_t num_threads;

    unsigned int **table_pointers; //num_tuples*num_threads
    unsigned int *tuple; //num_tuples*num_threads


    size_t *thread_index;

    Table(size_t num_tuples_in, size_t num_threads_in, size_t cardinality){
      num_tuples = num_tuples_in;
      num_threads = num_threads_in;
      table_pointers = new unsigned int*[num_tuples*num_threads];
      tuple = new unsigned int[num_tuples*num_threads];

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
};

#endif