#ifndef TABLE_H
#define TABLE_H

template<class T>
class Table{
  public:
    size_t num_columns;
    size_t column_size;
    size_t cardinality;

    T **data;
    T *tuple;

    size_t *padding;
    Table(const size_t num_columns_in, const size_t column_size_in){
      num_columns = num_columns_in;
      data = new T*[num_columns];
      tuple = new T[num_columns];
      cardinality = 0;
      column_size = column_size_in;
      for(size_t i = 0; i < num_columns_in; i++){
        data[i] = new T[column_size]; //(40*cardinality)/num_threads
      }
      padding = new size_t[PADDING];
    }

    ~Table(){}

    template<class R>
    void write_table(Set<R> c, T* id_map){
      const size_t offset = cardinality;
      cardinality += c.cardinality;
      for(size_t i=0;i<(num_columns-1);i++){
        T *cur_column = data[i]+offset;
        for(size_t j=0;j<c.cardinality;j++){
          cur_column[j] = tuple[i];
        }
      }
      T *cur_column = data[num_columns-1]+offset;
      c.foreach([num_columns,cur_column,data,id_map](uint32_t data){
        cur_column[num_columns-1] = id_map[data];
      });
    }

    void print_data(ofstream &file){
      for(size_t i=0;i<cardinality;i++){
        for(size_t j=0;j<num_columns;j++){
          T *cur_column = data[j];
          file << cur_column[i] << "\t";
        }
        file << endl;
      }
    }

};

template<class T>
class ParallelTable{
  public:
    Table<T> **table;
    size_t num_threads;
    size_t num_columns;
    size_t column_size;

    size_t cardinality;

    ParallelTable(size_t num_threads_in, size_t num_columns_in, size_t column_size_in){
      num_threads = num_threads_in;
      num_columns = num_columns_in;
      column_size = column_size_in;
      table = new Table<T>*[num_threads];
      cardinality = 0;
    }
    ~ParallelTable(){}

    void print_data(string filename){
      cout << "Printing data to table: " << filename << endl;
      ofstream myfile;
      myfile.open(filename);
      for(size_t i=0; i<num_threads;i++){
        table[i]->print_data(myfile);
      }
    }

    void allocate(size_t tid){
      table[tid] = new Table<T>(num_columns,column_size);
    }
};

#endif