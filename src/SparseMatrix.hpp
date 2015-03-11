// XXX: Refactor to store type information separately from the flattened set
// and/or get rid of the flattened sets altogether and use Set instances.

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


    // Stores out neighbors.
    uint32_t *row_lengths;
    uint8_t **row_arrays;
    uint32_t *row_ranges;

    // Stores in neighbors.
    uint32_t *column_lengths;
    uint8_t **column_arrays;
    uint32_t *column_ranges;

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
      uint32_t *row_range_data_in, 
      uint32_t *column_lengths_in, 
      uint8_t **column_arrays_in, 
      uint32_t *column_range_data_in,
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
        row_ranges(row_range_data_in),
        column_lengths(column_lengths_in),
        column_arrays(column_arrays_in),
        column_ranges(column_range_data_in),
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

    static SparseMatrix* from_symmetric_noattribute_graph(MutableGraph *inputGraph,
      const std::function<bool(uint32_t,uint32_t)> node_selection,
      const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection,
      const size_t num_threads);

    static SparseMatrix* from_symmetric_attribute_graph(MutableGraph *inputGraph,
      const std::function<bool(uint32_t,uint32_t)> node_selection,
      const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection,
      const size_t num_threads);

    static SparseMatrix* from_asymmetric_graph(MutableGraph *inputGraph,
      const std::function<bool(uint32_t,uint32_t)> node_selection,
      const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection, 
      const size_t num_threads);

    static SparseMatrix* from_asymmetric_noattribute_graph(MutableGraph *inputGraph,
      const std::function<bool(uint32_t,uint32_t)> node_selection,
      const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection, 
      const size_t num_threads);

    static SparseMatrix* from_asymmetric_attribute_graph(MutableGraph *inputGraph,
      const std::function<bool(uint32_t,uint32_t)> node_selection,
      const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection, 
      const size_t num_threads);

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
    if(node_attributes != NULL)
      myfile << "Node Attribute: " << node_attributes[i] << endl;
    Set<T> row = Set<T>::from_flattened(row_arrays[i],card);
    size_t row_i = 0;
    row.foreach( [this, &myfile,&row_i,i] (uint32_t data){
      myfile << " DATA: " << data;
      if(this->out_edge_attributes != NULL)
        myfile << " Attribute: " << this->out_edge_attributes->at(i)->at(row_i++);
      myfile << endl;
    });
  }
  myfile << endl;
  //Printing in neighbors
  if(!symmetric){
    for(size_t i = 0; i < matrix_size; i++){
      myfile << "External ID: " << id_map[i] << " COLUMN: " << i << " LEN: " << column_lengths[i] << endl;
      if(node_attributes != NULL)
        myfile << "Node Attribute: " << node_attributes[i] << endl;
      size_t card = column_lengths[i];
      Set<T> col = Set<T>::from_flattened(column_arrays[i],card);
      size_t col_i = 0;
      col.foreach( [this,&myfile,&col_i,i] (uint32_t data){
        myfile << " DATA: " << data;
        if(this->in_edge_attributes != NULL)
          myfile << " Attribute: " << this->in_edge_attributes->at(i)->at(col_i++);
        myfile << endl;
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
  vector<uint32_t> *new_edge_attribute = edge_attributes_in->at(old2newids[i]);
  for(size_t j = 0; j < neighborhood->size(); ++j) {
    if(node_selection(neighborhood->at(j),node_attr->at(neighborhood->at(j))) && edge_selection(i,neighborhood->at(j),edge_attr->at(j))){
      selected_neighborhood[new_size++] = old2newids[neighborhood->at(j)];
      new_edge_attribute->push_back(edge_attr->at(j));
    } 
  }
  index += Set<T>::flatten_from_array(data+index,selected_neighborhood,new_size);
  return make_pair(index,new_size);
}

template<class T>
inline pair<size_t,size_t> pack_data(const uint32_t i,
  const vector<uint32_t> * const neighborhood, uint32_t * const selected_neighborhood, 
  uint8_t * const data, size_t index,
  const std::function<bool(uint32_t,uint32_t)> node_selection,
  const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection,
  uint32_t *range_data){

  size_t new_size = 0;
  for(size_t j = 0; j < neighborhood->size(); ++j) {
    if(node_selection(neighborhood->at(j),0) && edge_selection(i,neighborhood->at(j),0)){
      selected_neighborhood[new_size++] = neighborhood->at(j);
    }
  }
  range_data[i] = selected_neighborhood[new_size-1]-selected_neighborhood[0];
  index += Set<T>::flatten_from_array(data+index,selected_neighborhood,new_size);
  return make_pair(index,new_size);
}

template<class T,class R>
SparseMatrix<T,R>* SparseMatrix<T,R>::from_symmetric_graph(MutableGraph* inputGraph,
  const std::function<bool(uint32_t,uint32_t)> node_selection,
  const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection,
  const size_t num_threads){
  if(inputGraph->node_attr == NULL)
    return SparseMatrix<T,R>::from_symmetric_noattribute_graph(inputGraph,node_selection,edge_selection,num_threads);
  else
    return SparseMatrix<T,R>::from_symmetric_attribute_graph(inputGraph,node_selection,edge_selection,num_threads);
}
//Directed Graph
template<class T,class R>
SparseMatrix<T,R>* SparseMatrix<T,R>::from_asymmetric_graph(MutableGraph* inputGraph,
  const std::function<bool(uint32_t,uint32_t)> node_selection,
  const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection, const size_t num_threads){
  if(inputGraph->node_attr == NULL)
    return SparseMatrix<T,R>::from_asymmetric_noattribute_graph(inputGraph,node_selection,edge_selection,num_threads);
  else
    return SparseMatrix<T,R>::from_asymmetric_attribute_graph(inputGraph,node_selection,edge_selection,num_threads);
}
//Constructors
template<class T,class R>
SparseMatrix<T,R>* SparseMatrix<T,R>::from_symmetric_attribute_graph(MutableGraph* inputGraph,
  const std::function<bool(uint32_t,uint32_t)> node_selection,
  const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection,
  const size_t num_threads){

  const size_t matrix_size_in = inputGraph->num_nodes;
  const size_t cardinality_in = inputGraph->num_edges;
  const vector<uint32_t> *node_attr = inputGraph->node_attr;
  const vector<vector<uint32_t>*> *edge_attr = inputGraph->out_edge_attributes;

  ops::prepare_shuffling_dictionary16();

  uint8_t **row_arrays_in = new uint8_t*[matrix_size_in];
  uint32_t *row_lengths_in = new uint32_t[matrix_size_in];

  uint32_t *old2newids = new uint32_t[matrix_size_in];
  size_t new_num_nodes = 0;

  // Filter out nodes.
  vector<vector<uint32_t>*> *edge_attributes_in = new vector<vector<uint32_t>*>();
  for(size_t i = 0; i < matrix_size_in; ++i){
    if(node_selection(i,node_attr->at(i))){
      edge_attributes_in->push_back(new std::vector<uint32_t>());
      old2newids[i] = new_num_nodes++;
    } else{
      old2newids[i] = 0xFFFFFFFF;
    }
  }

  uint32_t *node_attributes_in = new uint32_t[new_num_nodes];
  uint64_t *new_imap = new uint64_t[new_num_nodes];
  size_t new_cardinality = 0;
  size_t total_bytes_used = 0;

  const size_t alloc_size = (cardinality_in*sizeof(int)*ALLOCATOR)/num_threads;

  ParallelBuffer<uint8_t> *row_data_buffer =
    new ParallelBuffer<uint8_t>(num_threads,alloc_size);
  ParallelBuffer<uint32_t> *selected_data_buffer =
    new ParallelBuffer<uint32_t>(num_threads,alloc_size);

  const size_t m = PADDING;
  size_t *indices = new size_t[num_threads * m];
  size_t *cardinalities= new size_t[num_threads * m];
  size_t *tid_alloc_size = new size_t[num_threads * m];

  double parallel_range = common::startClock();
  common::par_for_range(num_threads,0,matrix_size_in,128,
    //////////////////////////////////////////////////////////
    [tid_alloc_size,selected_data_buffer,row_data_buffer,indices,cardinalities](size_t tid){
      indices[tid * m] = 0;
      cardinalities[tid * m] = 0;
      row_data_buffer->allocate(tid);
      selected_data_buffer->allocate(tid);
      tid_alloc_size[tid * m] = 0;
    },
    /////////////////////////////////////////////////////////////
    [alloc_size,tid_alloc_size,node_attributes_in,old2newids,edge_attributes_in,node_attr,new_imap,edge_attr,cardinalities,row_data_buffer,selected_data_buffer,row_arrays_in,indices,row_lengths_in,inputGraph,node_selection,edge_selection]
    (size_t tid, size_t i) {
      if(old2newids[i] != 0xFFFFFFFF){
        uint8_t * const row_data_in = row_data_buffer->data[tid];
        uint32_t * const selected_data = selected_data_buffer->data[tid];
        node_attributes_in[old2newids[i]] = node_attr->at(i);
        new_imap[old2newids[i]] = inputGraph->id_map->at(i);
        row_arrays_in[old2newids[i]] = &row_data_in[tid_alloc_size[tid*m]];
        const pair<size_t,size_t> index_size = pack_attribute_data<T>(i,
          node_attr,edge_attr->at(i),inputGraph->out_neighborhoods->at(i),
          selected_data,old2newids,edge_attributes_in,
          row_data_in,tid_alloc_size[tid*m],node_selection,edge_selection); 
        indices[tid * m] += (index_size.first-tid_alloc_size[tid*m]);
        tid_alloc_size[tid*m] = index_size.first;
        row_lengths_in[old2newids[i]] = index_size.second;
        cardinalities[tid * m] += row_lengths_in[old2newids[i]];

        //realloc to larger size if within 95% of original allocation
        if(tid_alloc_size[tid*m] > (REALLOC_THRESHOLD*alloc_size)){
          row_data_buffer->data[tid] = (uint8_t*) realloc((void *) row_data_buffer->data[tid], tid_alloc_size[tid*m]*sizeof(uint8_t));  
          tid_alloc_size[tid*m] = 0;
          row_data_buffer->data[tid] = new uint8_t[alloc_size];  
        }
      }
    },
    /////////////////////////////////////////////////////////////
    [row_data_buffer,selected_data_buffer,cardinalities,&new_cardinality,&total_bytes_used,indices](size_t tid){
      selected_data_buffer->unallocate(tid);
      new_cardinality += cardinalities[tid * m];
      total_bytes_used += indices[tid * m];
    }
  );
  delete[] indices;
  delete[] cardinalities;
  common::stopClock("parallel section",parallel_range);

  cout << "Number of nodes: " << new_num_nodes << endl;
  cout << "Number of edges: " << new_cardinality << endl;
  cout << "ROW DATA SIZE (Bytes): " << total_bytes_used << endl;

  return new SparseMatrix(new_num_nodes,new_cardinality,
    total_bytes_used,0,inputGraph->max_nbrhood_size,true,
    row_lengths_in,row_arrays_in,NULL,row_lengths_in,row_arrays_in,NULL,new_imap,
    node_attributes_in,edge_attributes_in,edge_attributes_in);
}
//Constructors
template<class T,class R>
SparseMatrix<T,R>* SparseMatrix<T,R>::from_symmetric_noattribute_graph(MutableGraph* inputGraph,
  const std::function<bool(uint32_t,uint32_t)> node_selection,
  const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection,
  const size_t num_threads){
  const size_t matrix_size_in = inputGraph->num_nodes;
  const size_t cardinality_in = inputGraph->num_edges;

  ops::prepare_shuffling_dictionary16();

  uint8_t **row_arrays_in = new uint8_t*[matrix_size_in];
  uint32_t *row_lengths_in = new uint32_t[matrix_size_in];
  uint32_t *row_range_data = new uint32_t[matrix_size_in];

  size_t new_cardinality = 0;
  size_t total_bytes_used = 0;

  const size_t alloc_size = (cardinality_in*sizeof(uint32_t)*ALLOCATOR)/num_threads;

  ParallelBuffer<uint8_t> *row_data_buffer = new ParallelBuffer<uint8_t>(num_threads,alloc_size);
  ParallelBuffer<uint32_t> *selected_data_buffer = new ParallelBuffer<uint32_t>(num_threads,alloc_size);

  const size_t m = PADDING;
  size_t *indices = new size_t[num_threads * m];
  size_t *cardinalities= new size_t[num_threads * m];
  size_t *tid_alloc_size = new size_t[num_threads * m];

  double parallel_range = common::startClock();
  common::par_for_range(num_threads,0,matrix_size_in,256,
    //////////////////////////////////////////////////////////
    [alloc_size,tid_alloc_size,selected_data_buffer,row_data_buffer,indices,cardinalities](size_t tid){
      indices[tid * m] = 0;
      cardinalities[tid * m] = 0;
      row_data_buffer->allocate(tid);
      selected_data_buffer->allocate(tid);
      tid_alloc_size[tid * m] = 0;
    },
    /////////////////////////////////////////////////////////////
    [&row_range_data,alloc_size,tid_alloc_size,cardinalities,row_data_buffer,selected_data_buffer,row_arrays_in,indices,row_lengths_in,inputGraph,node_selection,edge_selection]
    (size_t tid, size_t i) {
      uint8_t * const row_data_in = row_data_buffer->data[tid];
      uint32_t * const selected_data = selected_data_buffer->data[tid];

      row_arrays_in[i] = &row_data_in[tid_alloc_size[tid*m]];
      const pair<size_t,size_t> index_size = pack_data<T>(
        i,
        inputGraph->out_neighborhoods->at(i),
        selected_data,
        row_data_in,
        tid_alloc_size[tid*m],
        node_selection, //[](uint32_t n, uint32_t a) -> bool { (void) n; (void) a; return true; },
        edge_selection, //[](uint32_t n1, uint32_t n2, uint32_t a) -> bool { (void) n1; (void) n2; (void) a; return true; },
        row_range_data);
      indices[tid * m] += (index_size.first-tid_alloc_size[tid*m]);
      tid_alloc_size[tid*m] = index_size.first;
      row_lengths_in[i] = index_size.second;
      cardinalities[tid * m] += row_lengths_in[i];

      //realloc to larger size if within 95% of original allocation
      if(tid_alloc_size[tid*m] > (REALLOC_THRESHOLD*alloc_size)){
        row_data_buffer->data[tid] = (uint8_t*) realloc((void *) row_data_buffer->data[tid], tid_alloc_size[tid*m]*sizeof(uint8_t));  
        tid_alloc_size[tid*m] = 0;
        row_data_buffer->data[tid] = new uint8_t[alloc_size];  
      }
    },
    /////////////////////////////////////////////////////////////
    [row_data_buffer,selected_data_buffer,cardinalities,&new_cardinality,&total_bytes_used,indices](size_t tid){
      selected_data_buffer->unallocate(tid);
      new_cardinality += cardinalities[tid * m];
      total_bytes_used += indices[tid * m];
    }
  );
  delete[] indices;
  delete[] cardinalities;
  common::stopClock("parallel section",parallel_range);

  cout << "Number of nodes: " << matrix_size_in << endl;
  cout << "Number of edges: " << new_cardinality << endl;
  cout << "ROW DATA SIZE (Bytes): " << total_bytes_used << endl;
  common::bits_per_edge = (8.0*(double)total_bytes_used)/new_cardinality;
  double meta_overhead = 4*(common::num_bs+common::num_pshort+common::num_v+common::num_bp);
  common::bits_per_edge_nometa = (8.0*(double)(total_bytes_used-meta_overhead))/new_cardinality;

  return new SparseMatrix(matrix_size_in,new_cardinality,total_bytes_used,
    0,inputGraph->max_nbrhood_size,true,
    row_lengths_in,row_arrays_in,row_range_data,
    row_lengths_in,row_arrays_in,row_range_data,
    inputGraph->id_map->data(),NULL,NULL,NULL);
}

//Directed Graph
template<class T,class R>
SparseMatrix<T,R>* SparseMatrix<T,R>::from_asymmetric_attribute_graph(MutableGraph* inputGraph,
  const std::function<bool(uint32_t,uint32_t)> node_selection,
  const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection, const size_t num_threads){
  
  const size_t matrix_size_in = inputGraph->num_nodes;
  const size_t cardinality_in = inputGraph->num_edges;

  cout << "Original nodes: " << matrix_size_in << " " << cardinality_in << endl;

 
  const vector<uint32_t> *node_attr = inputGraph->node_attr;
  const vector<vector<uint32_t>*> *out_edge_attr = inputGraph->out_edge_attributes;
  const vector<vector<uint32_t>*> *in_edge_attr = inputGraph->in_edge_attributes;

  ops::prepare_shuffling_dictionary16();

  size_t new_cardinality = 0;
  size_t row_total_bytes_used = 0;
  size_t col_total_bytes_used = 0;

  uint32_t *old2newids = new uint32_t[matrix_size_in];
  size_t new_num_nodes = 0;

  //Filter out nodes.
  vector<vector<uint32_t>*> *out_edge_attributes_in = new vector<vector<uint32_t>*>();
  vector<vector<uint32_t>*> *in_edge_attributes_in = new vector<vector<uint32_t>*>();
  for(size_t i = 0; i < matrix_size_in; ++i){
    if(node_selection(i,node_attr->at(i))){
      out_edge_attributes_in->push_back(new std::vector<uint32_t>());
      in_edge_attributes_in->push_back(new std::vector<uint32_t>());
      old2newids[i] = new_num_nodes++;
    } else{
      old2newids[i] = 0xFFFFFFFF;
    }
  }

  cout << "num nodes: " << new_num_nodes << endl;

  uint32_t *node_attributes_in = new uint32_t[new_num_nodes];
  uint8_t **row_arrays_in = new uint8_t*[new_num_nodes];
  uint32_t *row_lengths_in = new uint32_t[new_num_nodes];
  uint8_t **col_arrays_in = new uint8_t*[new_num_nodes];
  uint32_t *col_lengths_in = new uint32_t[new_num_nodes];

  uint64_t *new_imap = new uint64_t[new_num_nodes];  
  const size_t alloc_size = (cardinality_in*sizeof(uint32_t)*ALLOCATOR)/num_threads;

  ParallelBuffer<uint8_t> *row_data_buffer = new ParallelBuffer<uint8_t>(num_threads,alloc_size);
  ParallelBuffer<uint8_t> *col_data_buffer = new ParallelBuffer<uint8_t>(num_threads,alloc_size);
  ParallelBuffer<uint32_t> *selected_data_buffer = new ParallelBuffer<uint32_t>(num_threads,alloc_size);
  
  const size_t m = PADDING;  //Don't ask don't tell.
  size_t *row_indices = new size_t[num_threads*m];
  size_t *col_indices = new size_t[num_threads*m];
  size_t *cardinalities= new size_t[num_threads*m];
  size_t *row_tid_alloc_size = new size_t[num_threads * m];
  size_t *col_tid_alloc_size = new size_t[num_threads * m];

  common::par_for_range(num_threads,0,matrix_size_in,100,
    //////////////////////////////////////////////////////////
    [row_tid_alloc_size,col_tid_alloc_size,&selected_data_buffer,&row_data_buffer,&col_data_buffer,&row_indices,&col_indices,&cardinalities](size_t tid){
      row_indices[tid*m] = 0;
      col_indices[tid*m] = 0;
      cardinalities[tid*m] = 0;
      row_data_buffer->allocate(tid);
      col_data_buffer->allocate(tid);
      selected_data_buffer->allocate(tid);
      row_tid_alloc_size[tid * m] = 0;
      col_tid_alloc_size[tid * m] = 0;
    },
    /////////////////////////////////////////////////////////////
    [alloc_size,row_tid_alloc_size,col_tid_alloc_size,cardinalities,row_data_buffer,col_data_buffer,selected_data_buffer,node_attr,
      old2newids,new_imap,in_edge_attr,out_edge_attr,in_edge_attributes_in,out_edge_attributes_in,
      row_arrays_in,col_arrays_in,node_attributes_in,
      row_indices,col_indices,
      &row_lengths_in,&col_lengths_in,
      inputGraph,node_selection,edge_selection]
    (size_t tid, size_t i) {
      if(old2newids[i] != 0xFFFFFFFF){
        new_imap[old2newids[i]] = inputGraph->id_map->at(i);

        //////////////////////////////////////////////////////////////////
        uint8_t * const row_data_in = row_data_buffer->data[tid];
        uint32_t * const selected_data = selected_data_buffer->data[tid];
        node_attributes_in[old2newids[i]] = node_attr->at(i);
        row_arrays_in[old2newids[i]] = &row_data_in[row_tid_alloc_size[tid*m]];
        pair<size_t,size_t> index_size = pack_attribute_data<T>(i,
          node_attr,out_edge_attr->at(i),inputGraph->out_neighborhoods->at(i),
          selected_data,old2newids,out_edge_attributes_in,
          row_data_in,row_tid_alloc_size[tid*m],node_selection,edge_selection); 
        row_indices[tid * m] += (index_size.first-row_tid_alloc_size[tid*m]);
        row_tid_alloc_size[tid*m] = index_size.first;
        row_lengths_in[old2newids[i]] = index_size.second;
        //realloc to larger size if within 95% of original allocation
        if(row_tid_alloc_size[tid*m] > (REALLOC_THRESHOLD*alloc_size)){
          row_data_buffer->data[tid] = (uint8_t*) realloc((void *) row_data_buffer->data[tid], row_tid_alloc_size[tid*m]*sizeof(uint8_t));  
          row_tid_alloc_size[tid*m] = 0;
          row_data_buffer->data[tid] = new uint8_t[alloc_size];  
        }

        cardinalities[tid * m] += row_lengths_in[old2newids[i]];

        //////////////////////////////////////////////////////////////////
        uint8_t * const col_data_in = col_data_buffer->data[tid];
        col_arrays_in[old2newids[i]] = &col_data_in[col_tid_alloc_size[tid*m]];          
        index_size = pack_attribute_data<T>(i,
          node_attr,in_edge_attr->at(i),inputGraph->in_neighborhoods->at(i),
          selected_data,old2newids,in_edge_attributes_in,
          col_data_in,col_tid_alloc_size[tid*m],node_selection,edge_selection);
        col_indices[tid * m] += (index_size.first-col_tid_alloc_size[tid*m]);
        col_tid_alloc_size[tid*m] = index_size.first;
        col_lengths_in[old2newids[i]] = index_size.second;
        //realloc to larger size if within 95% of original allocation
        if(col_tid_alloc_size[tid*m] > (REALLOC_THRESHOLD*alloc_size)){
          col_data_buffer->data[tid] = (uint8_t*) realloc((void *) col_data_buffer->data[tid], col_tid_alloc_size[tid*m]*sizeof(uint8_t));  
          col_tid_alloc_size[tid*m] = 0;
          col_data_buffer->data[tid] = new uint8_t[alloc_size];  
        }

        cardinalities[tid*m] += col_lengths_in[old2newids[i]];
      }
    },
    ///////////////////////////////////////////////////////////////////////
    [&row_data_buffer,&col_data_buffer,&selected_data_buffer,&cardinalities,
      &new_cardinality,&row_total_bytes_used,&col_total_bytes_used,
      &row_indices,&col_indices]
    (size_t tid){
      selected_data_buffer->unallocate(tid);
      new_cardinality += cardinalities[tid*m];
      row_total_bytes_used += row_indices[tid*m];
      col_total_bytes_used += col_indices[tid*m];
    }
  );
  delete[] row_indices;
  delete[] col_indices;
  delete[] cardinalities;

  cout << "Number of edges: " << new_cardinality << endl;
  cout << "ROW DATA SIZE (Bytes): " << row_total_bytes_used << endl;
  cout << "COL DATA SIZE (Bytes): " << col_total_bytes_used << endl;

  return new SparseMatrix(new_num_nodes,new_cardinality,row_total_bytes_used,
    col_total_bytes_used,inputGraph->max_nbrhood_size,false,
    row_lengths_in,row_arrays_in,NULL,
    col_lengths_in,col_arrays_in,NULL,
    new_imap,node_attributes_in,out_edge_attributes_in,in_edge_attributes_in);
}

//Directed Graph
template<class T,class R>
SparseMatrix<T,R>* SparseMatrix<T,R>::from_asymmetric_noattribute_graph(MutableGraph* inputGraph,
  const std::function<bool(uint32_t,uint32_t)> node_selection,
  const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection, const size_t num_threads){

  const size_t matrix_size_in = inputGraph->num_nodes;
  const size_t cardinality_in = inputGraph->num_edges;

  ops::prepare_shuffling_dictionary16();

  uint8_t **row_arrays_in = new uint8_t*[matrix_size_in];
  uint32_t *row_lengths_in = new uint32_t[matrix_size_in];
  uint8_t **col_arrays_in = new uint8_t*[matrix_size_in];
  uint32_t *col_lengths_in = new uint32_t[matrix_size_in];
  uint32_t *row_range_data = new uint32_t[matrix_size_in];
  uint32_t *col_range_data = new uint32_t[matrix_size_in];

  size_t new_cardinality = 0;
  size_t row_total_bytes_used = 0;
  size_t col_total_bytes_used = 0;

  const size_t alloc_size = (cardinality_in*sizeof(uint32_t)*ALLOCATOR)/num_threads;

  ParallelBuffer<uint8_t> *row_data_buffer = new ParallelBuffer<uint8_t>(num_threads,alloc_size);
  ParallelBuffer<uint8_t> *col_data_buffer = new ParallelBuffer<uint8_t>(num_threads,alloc_size);
  ParallelBuffer<uint32_t> *selected_data_buffer = new ParallelBuffer<uint32_t>(num_threads,alloc_size);
  
  const size_t m = PADDING;
  size_t *row_indices = new size_t[num_threads*m];
  size_t *col_indices = new size_t[num_threads*m];
  size_t *cardinalities= new size_t[num_threads*m];
  size_t *row_tid_alloc_size = new size_t[num_threads * m];
  size_t *col_tid_alloc_size = new size_t[num_threads * m];

  common::par_for_range(num_threads,0,matrix_size_in,100,
    //////////////////////////////////////////////////////////
    [row_tid_alloc_size,col_tid_alloc_size,selected_data_buffer,row_data_buffer,col_data_buffer,row_indices,col_indices,cardinalities](size_t tid){
      row_indices[tid*m] = 0;
      col_indices[tid*m] = 0;
      cardinalities[tid*m] = 0;
      row_data_buffer->allocate(tid);
      col_data_buffer->allocate(tid);
      selected_data_buffer->allocate(tid);
      row_tid_alloc_size[tid * m] = 0;
      col_tid_alloc_size[tid * m] = 0;
    },
    /////////////////////////////////////////////////////////////
    [alloc_size,row_tid_alloc_size,col_tid_alloc_size,cardinalities,row_data_buffer,col_data_buffer,selected_data_buffer,
      &row_arrays_in,&col_arrays_in,
      &row_indices,&col_indices,
      &row_lengths_in,&col_lengths_in,
      &inputGraph,&node_selection,&edge_selection,&row_range_data,&col_range_data]
    (size_t tid, size_t i) {
      uint32_t * const selected_data = selected_data_buffer->data[tid];
      uint8_t * const row_data_in = row_data_buffer->data[tid];

      row_arrays_in[i] = &row_data_in[row_tid_alloc_size[tid*m]];  
      pair<size_t,size_t> index_size = pack_data<T>(i,inputGraph->out_neighborhoods->at(i),
        selected_data,row_data_in,row_tid_alloc_size[tid*m],node_selection,edge_selection,row_range_data); 
      //cout << index_size.first << endl;
      row_indices[tid * m] += (index_size.first-row_tid_alloc_size[tid*m]);
      row_tid_alloc_size[tid*m] = index_size.first;
      row_lengths_in[i] = index_size.second;
      cardinalities[tid*m] += row_lengths_in[i];

      //cout << row_tid_alloc_size[tid*m] << " " << row_indices[tid * m] << endl;
      //realloc to larger size if within 95% of original allocation
      if(row_tid_alloc_size[tid*m] > (REALLOC_THRESHOLD*alloc_size)){
        row_data_buffer->data[tid] = (uint8_t*) realloc((void *) row_data_buffer->data[tid], row_tid_alloc_size[tid*m]*sizeof(uint8_t));  
        row_tid_alloc_size[tid*m] = 0;
        row_data_buffer->data[tid] = new uint8_t[alloc_size];  
      }

      uint8_t * const col_data_in = col_data_buffer->data[tid];
      col_arrays_in[i] = &col_data_in[col_tid_alloc_size[tid*m]];  
      index_size = pack_data<T>(i,inputGraph->in_neighborhoods->at(i),
        selected_data,col_data_in,col_tid_alloc_size[tid*m],node_selection,edge_selection,col_range_data); 
      col_indices[tid * m] += (index_size.first-col_tid_alloc_size[tid*m]);
      col_tid_alloc_size[tid*m] = index_size.first;
      col_lengths_in[i] = index_size.second;
      cardinalities[tid*m] += col_lengths_in[i];

      //realloc to larger size if within 95% of original allocation
      if(col_tid_alloc_size[tid*m] > (REALLOC_THRESHOLD*alloc_size)){
        col_data_buffer->data[tid] = (uint8_t*) realloc((void *) col_data_buffer->data[tid], col_tid_alloc_size[tid*m]*sizeof(uint8_t));  
        col_tid_alloc_size[tid*m] = 0;
        col_data_buffer->data[tid] = new uint8_t[alloc_size];  
      }
    },
    /////////////////////////////////////////////////////////////
    [&row_data_buffer,&col_data_buffer,&selected_data_buffer,&cardinalities,
      &new_cardinality,&row_total_bytes_used,&col_total_bytes_used,
      &row_indices,&col_indices]
    (size_t tid){
      selected_data_buffer->unallocate(tid);
      new_cardinality += cardinalities[tid*m];
      row_total_bytes_used += row_indices[tid*m];
      col_total_bytes_used += col_indices[tid*m];
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
    row_lengths_in,row_arrays_in,row_range_data,
    col_lengths_in,col_arrays_in,col_range_data,
    inputGraph->id_map->data(),NULL,NULL,NULL);
}
#endif
