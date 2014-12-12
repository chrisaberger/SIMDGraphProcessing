#ifndef SparseMatrix_H
#define SparseMatrix_H

#include "MutableGraph.hpp"
#include "set/ops.hpp"
#include "ParallelBuffer.hpp"

template<class T,class R>
class SparseMatrix{
  public:
    size_t matrix_size;  //number of nodes, number of columns = number of rows
    size_t cardinality;  //number of edges
    size_t row_total_bytes_used; // the size of all edges combined
    size_t col_total_bytes_used; // the size of all edges combined
    size_t max_nbrhood_size;
    bool symmetric; //undirected?

    /*
    Stores out neighbors.
    */
    uint32_t *row_lengths; 
    uint8_t **row_arrays;

    /*
    Stores in neighbors.
    */
    uint32_t *column_lengths;
    uint8_t **column_arrays;

    uint64_t *id_map;
    uint32_t *node_attributes;
    vector< vector<uint32_t>*  > *out_edge_attributes;
    vector< vector<uint32_t>*  > *in_edge_attributes;

    SparseMatrix(size_t matrix_size_in,
      size_t cardinality_in,
      size_t row_total_bytes_used_in,
      size_t col_total_bytes_used_in,
      size_t max_nbrhood_size_in,
      bool symmetric_in, 
      uint32_t *row_lengths_in,
      uint8_t **row_arrays_in, 
      uint32_t *column_lengths_in, 
      uint8_t **column_arrays_in, 
      uint64_t *id_map_in,
      uint32_t *node_attributes_in,
      vector< vector<uint32_t>*  > *out_edge_attributes_in,
      vector< vector<uint32_t>*  > *in_edge_attributes_in):
        matrix_size(matrix_size_in),
        cardinality(cardinality_in),
        row_total_bytes_used(row_total_bytes_used_in),
        col_total_bytes_used(col_total_bytes_used_in),
        max_nbrhood_size(max_nbrhood_size_in),
        symmetric(symmetric_in),
        row_lengths(row_lengths_in),
        row_arrays(row_arrays_in),
        column_lengths(column_lengths_in),
        column_arrays(column_arrays_in),
        id_map(id_map_in),
        node_attributes(node_attributes_in),
        out_edge_attributes(out_edge_attributes_in),
        in_edge_attributes(in_edge_attributes_in){}

    ~SparseMatrix(){
      #pragma omp parallel for default(none)
      for(size_t i = 0; i < matrix_size; i++){
        delete[] row_arrays[i];
      }
      delete[] row_arrays;
      delete[] row_lengths;

      if(!symmetric){
        delete[] column_lengths;
        #pragma omp parallel for default(none)
        for(size_t i = 0; i < matrix_size; i++){
          delete[] row_arrays[i];
        }
      }
    }

    SparseMatrix* clone_on_node(int node);
    void *parallel_constructor(void *);

    static SparseMatrix* from_symmetric_graph(MutableGraph *inputGraph,
      const std::function<bool(uint32_t,uint32_t)> node_selection,
      const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection,
      const size_t num_threads);

    static SparseMatrix* from_symmetric_attribute_graph(MutableGraph *inputGraph,
      const std::function<bool(uint32_t,uint32_t)> node_selection,
      const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection);

    static SparseMatrix* from_asymmetric_graph(MutableGraph *inputGraph,
      const std::function<bool(uint32_t,uint32_t)> node_selection,
      const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection, const size_t num_threads);

    uint32_t get_max_row_id();
    uint32_t get_internal_id(uint64_t external_id);
    Set<T> get_row(uint32_t row);
    Set<T> get_column(uint32_t column);
    Set<R> get_decoded_row(uint32_t row, uint32_t *decoded_a);
    void print_data(string filename);
};

template<class T,class R>
inline uint32_t SparseMatrix<T,R>::get_max_row_id(){
  size_t max = 0;
  uint32_t max_id = 0;
  for(size_t i=0; i<matrix_size; i++){
    if(row_lengths[i] > max){
      max_id = i;
      max = row_lengths[i];
    }
  }
  cout << "MAXIMUM NODE: " << id_map[max_id] << " INTERNAL ID: " << max_id << endl; 
  return max_id;
}

template<class T,class R>
inline uint32_t SparseMatrix<T,R>::get_internal_id(uint64_t external_id){
  for(size_t i=0; i<matrix_size; i++){
    if(id_map[i] == external_id)
      return i;
  }
  return 0;
}

template<class T,class R>
inline Set<T> SparseMatrix<T,R>::get_row(uint32_t row){
  size_t card = row_lengths[row];
  return Set<T>::from_flattened(row_arrays[row],card);
}
template<class T,class R>
inline Set<T> SparseMatrix<T,R>::get_column(uint32_t column){
  size_t card = column_lengths[column];
  return Set<T>::from_flattened(column_arrays[column],card);
}
/*
This function decodes the variant and bitpacked types into UINTEGER arrays.
This function is not necessary if these types are not used (thus the pragma.)
*/
template<class T,class R>
inline Set<R> SparseMatrix<T,R>::get_decoded_row(uint32_t row, uint32_t *buffer){
  size_t card = row_lengths[row];
  #if COMPRESSION == 1
  Set<T> row_set = Set<T>::from_flattened(row_arrays[row],card);
  if(row_set.type == common::VARIANT || row_set.type == common::BITPACKED){
    return row_set.decode(buffer);
  }
  return Set<R>(row_set);
  #else
  (void) buffer;
  return Set<R>::from_flattened(row_arrays[row],card);
  #endif
}

template<class T, class R>
void SparseMatrix<T,R>::print_data(string filename){
  ofstream myfile;
  myfile.open(filename);

  //Printing out neighbors
  cout << "Writing matrix row_data to file: " << filename << endl;
  for(size_t i = 0; i < matrix_size; i++){
    size_t card = row_lengths[i];
    myfile << "External ID: " << id_map[i] << " ROW: " << i << " LEN: " << row_lengths[i] << endl;
    Set<T> row = Set<T>::from_flattened(row_arrays[i],card);
    row.foreach( [&myfile] (uint32_t data){
      myfile << " DATA: " << data << endl;
    });
  }
  myfile << endl;
  //Printing in neighbors
  if(!symmetric){
    for(size_t i = 0; i < matrix_size; i++){
      myfile << "External ID: " << id_map[i] << " COLUMN: " << i << " LEN: " << column_lengths[i] << endl;
      size_t card = column_lengths[i];
      Set<T> col = Set<T>::from_flattened(column_arrays[i],card);
      col.foreach( [&myfile] (uint32_t data){
        myfile << " DATA: " << data << endl;
      });
    }
  }

  myfile.close();
}
/////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
inline pair<size_t,size_t> pack_attribute_data(const uint32_t i,
  const vector<uint32_t> *node_attr, const vector<uint32_t> *edge_attr,
  const vector<uint32_t> *neighborhood, uint32_t *selected_neighborhood,
  const uint32_t *old2newids, vector<vector<uint32_t>*> *edge_attributes_in,
  uint8_t *data, size_t index,
  const std::function<bool(uint32_t,uint32_t)> node_selection,
  const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection){

  size_t new_size = 0;
  vector<uint32_t> *new_edge_attribute = new vector<uint32_t>();
  for(size_t j = 0; j < neighborhood->size(); ++j) {
    if(node_selection(neighborhood->at(j),node_attr->at(neighborhood->at(j))) && edge_selection(i,neighborhood->at(j),edge_attr->at(j))){
      selected_neighborhood[new_size++] = old2newids[neighborhood->at(j)];
      new_edge_attribute->push_back(edge_attr->at(j));
    } 
  }
  edge_attributes_in->push_back(new_edge_attribute);
  index += Set<T>::flatten_from_array(data+index,selected_neighborhood,new_size);
  return make_pair(index,new_size);
}
template<class T>
inline pair<size_t,size_t> pack_data(const uint32_t i,
  const vector<uint32_t> *neighborhood, uint32_t *selected_neighborhood, 
  uint8_t *data, size_t index,
  const std::function<bool(uint32_t,uint32_t)> node_selection,
  const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection){

  size_t new_size = 0;
  for(size_t j = 0; j < neighborhood->size(); ++j) {
    if(node_selection(neighborhood->at(j),0) && edge_selection(i,neighborhood->at(j),0)){
      selected_neighborhood[new_size++] = neighborhood->at(j);
    } 
  }
  index += Set<T>::flatten_from_array(data+index,selected_neighborhood,new_size);
  return make_pair(index,new_size);
}
//Constructors
template<class T,class R>
SparseMatrix<T,R>* SparseMatrix<T,R>::from_symmetric_attribute_graph(MutableGraph* inputGraph,
  const std::function<bool(uint32_t,uint32_t)> node_selection,
  const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection){
  
  const size_t matrix_size_in = inputGraph->num_nodes;
  const size_t cardinality_in = inputGraph->num_edges;
  const vector<uint32_t> *node_attr = inputGraph->node_attr;
  const vector<vector<uint32_t>*> *edge_attr = inputGraph->out_edge_attributes;

  ops::prepare_shuffling_dictionary16();

  uint8_t **row_arrays_in = new uint8_t*[matrix_size_in];
  uint32_t *row_lengths_in = new uint32_t[matrix_size_in];

  uint32_t *old2newids = new uint32_t[matrix_size_in];
  size_t new_num_nodes = 0;

  //Filter out nodes.
  for(size_t i = 0; i < matrix_size_in; ++i){
    if(node_selection(i,node_attr->at(i))){
      old2newids[i] = new_num_nodes;
      new_num_nodes++;
    } else{
      old2newids[i] = -1;
    }
  }

  uint32_t *node_attributes_in;
  vector<vector<uint32_t>*> *edge_attributes_in = new vector<vector<uint32_t>*>();
  node_attributes_in= new uint32_t[new_num_nodes];
  edge_attributes_in->reserve(cardinality_in);

  uint64_t *new_imap = new uint64_t[new_num_nodes];  
  size_t new_cardinality = 0;
  size_t total_bytes_used = 0;

  size_t alloc_size = cardinality_in*sizeof(int)*4;//sizeof(size_t)*(cardinality_in/omp_get_num_threads());
  if(alloc_size < new_num_nodes){
    alloc_size = new_num_nodes;
  }

  #pragma omp parallel default(shared) reduction(+:total_bytes_used) reduction(+:new_cardinality)
  {
    uint8_t *row_data_in = new uint8_t[alloc_size];
    uint32_t *selected_row = new uint32_t[new_num_nodes];
    size_t index = 0;
    #pragma omp for schedule(static)
    for(size_t i = 0; i < matrix_size_in; ++i){
      if(old2newids[i] != -1){
        node_attributes_in[old2newids[i]] = node_attr->at(i);
        new_imap[old2newids[i]] = inputGraph->id_map->at(i);
        row_arrays_in[old2newids[i]] = &row_data_in[index];

        pair<size_t,size_t> index_size = pack_attribute_data<T>(i,node_attr,edge_attr,inputGraph->out_neighborhoods,selected_row,
          old2newids,edge_attributes_in,row_lengths_in,row_arrays_in,row_data_in,
          index,node_selection,edge_selection);
        index = index_size.first;
        row_lengths_in[old2newids[i]] = index_size.second;
        new_cardinality += row_lengths_in[old2newids[i]];
      }
    }
    delete[] selected_row;
    total_bytes_used += index;
    row_data_in = (uint8_t*) realloc((void *) row_data_in, index*sizeof(uint8_t));
  }
  delete[] old2newids;

  cout << "Number of edges: " << new_cardinality << endl;
  cout << "ROW DATA SIZE (Bytes): " << total_bytes_used << endl;

  return new SparseMatrix(new_num_nodes,new_cardinality,
    total_bytes_used,0,inputGraph->max_nbrhood_size,true,
    row_lengths_in,row_arrays_in,row_lengths_in,row_arrays_in,new_imap,
    node_attributes_in,edge_attributes_in,edge_attributes_in);
}
//Constructors
template<class T,class R>
SparseMatrix<T,R>* SparseMatrix<T,R>::from_symmetric_graph(MutableGraph* inputGraph,
  const std::function<bool(uint32_t,uint32_t)> node_selection,
  const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection,
  const size_t num_threads){
  
  (void) num_threads;
  const size_t matrix_size_in = inputGraph->num_nodes;
  const size_t cardinality_in = inputGraph->num_edges;

  ops::prepare_shuffling_dictionary16();

  uint8_t **row_arrays_in = new uint8_t*[matrix_size_in];
  uint32_t *row_lengths_in = new uint32_t[matrix_size_in];

  size_t new_cardinality = 0;
  size_t total_bytes_used = 0;

  size_t alloc_size = (cardinality_in*sizeof(uint32_t)*2)/num_threads;

  ParallelBuffer<uint8_t>row_data_buffer(num_threads,alloc_size);
  ParallelBuffer<uint32_t>selected_data_buffer(num_threads,alloc_size);
  size_t *indices = new size_t[num_threads];
  size_t *cardinalities= new size_t[num_threads];
  
  common::par_for_range(num_threads, 0, matrix_size_in, 100,
    //////////////////////////////////////////////////////////
    [&selected_data_buffer,&row_data_buffer,&indices,&cardinalities](size_t tid){
      indices[tid] = 0;
      cardinalities[tid] = 0;
      row_data_buffer.allocate(tid);
      selected_data_buffer.allocate(tid);
    },
    /////////////////////////////////////////////////////////////
    [&cardinalities,&row_data_buffer,&selected_data_buffer,&row_arrays_in,&indices,&row_lengths_in,&inputGraph,&node_selection,&edge_selection]
    (size_t tid, size_t i) {
      uint8_t * const row_data_in = row_data_buffer.data[tid];
      uint32_t * const selected_row = selected_data_buffer.data[tid];

      row_arrays_in[i] = &row_data_in[indices[tid]];  
      pair<size_t,size_t> index_size = pack_data<T>(i,inputGraph->out_neighborhoods->at(i),
        selected_row,row_data_in,indices[tid],node_selection,edge_selection); 
      indices[tid] = index_size.first;
      row_lengths_in[i] = index_size.second;
      cardinalities[tid] += row_lengths_in[i];
    },
    /////////////////////////////////////////////////////////////
    [&row_data_buffer,&selected_data_buffer,&cardinalities,&new_cardinality,&total_bytes_used,&indices](size_t tid){
      selected_data_buffer.unallocate(tid);
      new_cardinality += cardinalities[tid];
      total_bytes_used += indices[tid];
      row_data_buffer.data[tid] = (uint8_t*) realloc((void *) row_data_buffer.data[tid], indices[tid]*sizeof(uint8_t));  
    }
  );
  delete[] indices;
  delete[] cardinalities;

  cout << "Number of edges: " << new_cardinality << endl;
  cout << "ROW DATA SIZE (Bytes): " << total_bytes_used << endl;

  return new SparseMatrix(matrix_size_in,new_cardinality,total_bytes_used,
    0,inputGraph->max_nbrhood_size,true,
    row_lengths_in,row_arrays_in,
    row_lengths_in,row_arrays_in,
    inputGraph->id_map->data(),NULL,NULL,NULL);
}

//Directed Graph
template<class T,class R>
SparseMatrix<T,R>* SparseMatrix<T,R>::from_asymmetric_graph(MutableGraph* inputGraph,
  const std::function<bool(uint32_t,uint32_t)> node_selection,
  const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection, const size_t num_threads){
  
  const size_t matrix_size_in = inputGraph->num_nodes;
  const size_t cardinality_in = inputGraph->num_edges;

  ops::prepare_shuffling_dictionary16();

  uint8_t **row_arrays_in = new uint8_t*[matrix_size_in];
  uint32_t *row_lengths_in = new uint32_t[matrix_size_in];
  uint8_t **col_arrays_in = new uint8_t*[matrix_size_in];
  uint32_t *col_lengths_in = new uint32_t[matrix_size_in];

  size_t new_cardinality = 0;
  size_t row_total_bytes_used = 0;
  size_t col_total_bytes_used = 0;

  size_t alloc_size = (cardinality_in*sizeof(uint32_t)*2)/num_threads;

  ParallelBuffer<uint8_t>row_data_buffer(num_threads,alloc_size);
  ParallelBuffer<uint8_t>col_data_buffer(num_threads,alloc_size);

  ParallelBuffer<uint32_t>selected_data_buffer(num_threads,alloc_size);
  size_t *row_indices = new size_t[num_threads];
  size_t *col_indices = new size_t[num_threads];
  size_t *cardinalities= new size_t[num_threads];
  
  common::par_for_range(num_threads, 0, matrix_size_in, 100,
    //////////////////////////////////////////////////////////
    [&selected_data_buffer,&row_data_buffer,&col_data_buffer,&row_indices,&col_indices,&cardinalities](size_t tid){
      row_indices[tid] = 0;
      col_indices[tid] = 0;
      cardinalities[tid] = 0;
      row_data_buffer.allocate(tid);
      col_data_buffer.allocate(tid);
      selected_data_buffer.allocate(tid);
    },
    /////////////////////////////////////////////////////////////
    [&cardinalities,&row_data_buffer,&col_data_buffer,&selected_data_buffer,
      &row_arrays_in,&col_arrays_in,
      &row_indices,&col_indices,
      &row_lengths_in,&col_lengths_in,
      &inputGraph,&node_selection,&edge_selection]
    (size_t tid, size_t i) {
      uint32_t * const selected_data = selected_data_buffer.data[tid];

      uint8_t * const row_data_in = row_data_buffer.data[tid];
      row_arrays_in[i] = &row_data_in[row_indices[tid]];  
      pair<size_t,size_t> index_size = pack_data<T>(i,inputGraph->out_neighborhoods->at(i),
        selected_data,row_data_in,row_indices[tid],node_selection,edge_selection); 
      row_indices[tid] = index_size.first;
      row_lengths_in[i] = index_size.second;
      cardinalities[tid] += row_lengths_in[i];

      uint8_t * const col_data_in = col_data_buffer.data[tid];
      col_arrays_in[i] = &col_data_in[col_indices[tid]];  
      index_size = pack_data<T>(i,inputGraph->in_neighborhoods->at(i),
        selected_data,col_data_in,col_indices[tid],node_selection,edge_selection); 
      col_indices[tid] = index_size.first;
      col_lengths_in[i] = index_size.second;
      cardinalities[tid] += col_lengths_in[i];
    },
    /////////////////////////////////////////////////////////////
    [&row_data_buffer,&col_data_buffer,&selected_data_buffer,&cardinalities,
      &new_cardinality,&row_total_bytes_used,&col_total_bytes_used,
      &row_indices,&col_indices]
    (size_t tid){
      selected_data_buffer.unallocate(tid);
      new_cardinality += cardinalities[tid];
      row_total_bytes_used += row_indices[tid];
      col_total_bytes_used += col_indices[tid];
      row_data_buffer.data[tid] = (uint8_t*) realloc((void *) row_data_buffer.data[tid], row_indices[tid]*sizeof(uint8_t));  
      col_data_buffer.data[tid] = (uint8_t*) realloc((void *) col_data_buffer.data[tid], col_indices[tid]*sizeof(uint8_t));  
    }
  );
  delete[] row_indices;
  delete[] col_indices;
  delete[] cardinalities;

  cout << "Number of edges: " << new_cardinality << endl;
  cout << "ROW DATA SIZE (Bytes): " << row_total_bytes_used << endl;
  cout << "COL DATA SIZE (Bytes): " << col_total_bytes_used << endl;

  return new SparseMatrix(matrix_size_in,new_cardinality,row_total_bytes_used,
    col_total_bytes_used,inputGraph->max_nbrhood_size,false,
    row_lengths_in,row_arrays_in,
    col_lengths_in,col_arrays_in,
    inputGraph->id_map->data(),NULL,NULL,NULL);
}

#endif
