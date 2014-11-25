#ifndef SparseMatrix_H
#define SparseMatrix_H

#include "MutableGraph.hpp"
#include "set/Set.hpp"


template<class V>
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

    static SparseMatrix* from_symmetric(MutableGraph *inputGraph,
      const std::function<bool(uint32_t,uint32_t)> node_selection,
      const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection);

    static SparseMatrix* from_asymmetric(MutableGraph *inputGraph,
      const std::function<bool(uint32_t)> node_selection,
      const std::function<bool(uint32_t,uint32_t)> edge_selection, 
      const common::type t_in);


    size_t union_sparse_neighbors(uint32_t node, uint32_t *union_data, uint8_t *visited);
    size_t union_dense_neighbors(uint32_t offset, uint8_t &visited, uint32_t *union_data, uint8_t *parents);
    size_t get_union_distinct_neighbors();

    size_t row_intersect(uint8_t *R, uint32_t i, uint32_t j, uint32_t *decoded_a);
    size_t buffer_intersect(uint8_t *R, uint32_t j, uint8_t *A, uint32_t card_a);

    template<typename T> 
    T map_columns(std::function<T(uint32_t, std::function<T(uint32_t)>)> rowfunction, std::function<T(uint32_t)> f, T *new_data);
    template<typename T> 
    T map_columns_pr(std::function<T(uint32_t, std::function<T(uint32_t)>)> rowfunction, std::function<T(uint32_t)> f, T *new_data, T *old_data);
    template<typename T> 
    T sum_over_rows(std::function<T(uint32_t, uint32_t*, std::function<T(uint32_t,uint32_t,uint32_t*)>)> rowfunction, std::function<T(uint32_t,uint32_t,uint32_t*)> f);
    
    void foreach_column_in_row(uint32_t col, const std::function <void (uint32_t,uint32_t)>& ef);
    template<typename T> 
    T sum_over_rows_in_column(uint32_t row,std::function<T(uint32_t)> f);
        
    void print_data(string filename);

};

template<class V>
inline size_t SparseMatrix<V>::row_intersect(uint8_t *R, uint32_t i, uint32_t j, uint32_t *decoded_a){
  (void) decoded_a;
  //change the set in A to point to decoded_a, after sum is finished.
  size_t card_a = row_lengths[i];
  size_t card_b = row_lengths[j];

  Set<V> A = Set<V>::from_flattened(row_arrays[i],card_a);
  Set<V> B = Set<V>::from_flattened(row_arrays[j],card_b);
  Set<V> C(R,0,0,(V::get_type()));

  return Set<V>::intersect(C,A,B).cardinality;  
}

template<class V>
void SparseMatrix<V>::foreach_column_in_row(uint32_t row, const std::function <void (uint32_t,uint32_t)>& ef){
  size_t card = row_lengths[row];
  Set<V> row_set = Set<V>::from_flattened(row_arrays[row],card);
  row_set.foreach( [row,ef] (uint32_t column){
    ef(row,column);
  });
}

template<class V>
void SparseMatrix<V>::print_data(string filename){
  ofstream myfile;
  myfile.open(filename);

  //Printing out neighbors
  cout << "Writing matrix row_data to file: " << filename << endl;
  for(size_t i = 0; i < matrix_size; i++){
    size_t card = row_lengths[i];
    myfile << "External ID: " << id_map[i] << " ROW: " << i << " LEN: " << row_lengths[i] << endl;
    //if(card > 0){
      Set<V> row = Set<V>::from_flattened(row_arrays[i],card);
      row.foreach( [&myfile] (uint32_t data){
        myfile << " DATA: " << data << endl;
      });
    //}
  }
  myfile << endl;
  //Printing in neighbors
  if(!symmetric){
    for(size_t i = 0; i < matrix_size; i++){
      myfile << "External ID: " << id_map[i] << " COLUMN: " << i << " LEN: " << column_lengths[i] << endl;
      size_t card = column_lengths[i];
      if(card > 0){
        //uint_array::print_data(column_arrays[i],card,t,myfile,id_map);
      }
    }
  }

  myfile.close();
}
/////////////////////////////////////////////////////////////////////////////////////////////
//Constructors
template<class V>
SparseMatrix<V>* SparseMatrix<V>::from_symmetric(MutableGraph* inputGraph,
  const std::function<bool(uint32_t,uint32_t)> node_selection,
  const std::function<bool(uint32_t,uint32_t,uint32_t)> edge_selection){
  
  const vector< vector<uint32_t>*  > *g = inputGraph->out_neighborhoods;
  const size_t matrix_size_in = inputGraph->num_nodes;
  const size_t cardinality_in = inputGraph->num_edges;
  const size_t max_nbrhood_size_in = inputGraph->max_nbrhood_size;
  const vector<uint32_t> *node_attr = inputGraph->node_attr;
  const vector<uint64_t> *imap = inputGraph->id_map;
  const vector<vector<uint32_t>*> *edge_attr = inputGraph->out_edge_attributes;

  ops::prepare_shuffling_dictionary16();

  cout << "Number of nodes: " << matrix_size_in << endl;
  cout << "Number of edges: " << cardinality_in << endl;

  uint8_t **row_arrays_in = new uint8_t*[matrix_size_in];
  uint32_t *row_lengths_in = new uint32_t[matrix_size_in];

  int *old2newids = new int[matrix_size_in];
  size_t new_num_nodes = 0;


  bool attributes_set = node_attr->size() > 0 && edge_attr->size() > 0;

  cout << "attributes set: " << attributes_set << endl;

  common::startClock();
  if(attributes_set){
    for(size_t i = 0; i < matrix_size_in; ++i){
      if(node_selection(i,node_attr->at(i))){
        old2newids[i] = new_num_nodes;
        new_num_nodes++;
      } else{
        old2newids[i] = -1;
      }
    }
  } else{
    #pragma omp parallel for default(shared) schedule(static) 
    for(size_t i = 0; i < matrix_size_in; ++i){
      old2newids[i] = i;
    }
    new_num_nodes = matrix_size_in;
  }

  uint32_t *node_attributes_in;
  vector<vector<uint32_t>*> *edge_attributes_in = new vector<vector<uint32_t>*>();
  if(attributes_set){
    node_attributes_in= new uint32_t[new_num_nodes];
    edge_attributes_in->reserve(cardinality_in);
  }

  common::stopClock("Node Selections");

  uint64_t *new_imap = new uint64_t[new_num_nodes];  
  size_t new_cardinality = 0;
  size_t total_bytes_used = 0;

  size_t alloc_size = cardinality_in*sizeof(int);//sizeof(size_t)*(cardinality_in/omp_get_num_threads());
  if(alloc_size < new_num_nodes){
    alloc_size = new_num_nodes;
  }

  common::startClock();

  if(attributes_set){
    #pragma omp parallel default(shared) reduction(+:total_bytes_used) reduction(+:new_cardinality)
    {
      uint8_t *row_data_in = new uint8_t[alloc_size];
      uint32_t *selected_row = new uint32_t[new_num_nodes];
      size_t index = 0;
      #pragma omp for schedule(static)
      for(size_t i = 0; i < matrix_size_in; ++i){
        if(old2newids[i] != -1){
          new_imap[old2newids[i]] = imap->at(i);
          vector<uint32_t> *row = g->at(i);
          size_t new_size = 0;

          vector<uint32_t> *new_edge_attribute = new vector<uint32_t>();
          vector<uint32_t> *row_attr = edge_attr->at(i);
          node_attributes_in[old2newids[i]] = node_attr->at(i);
        
          for(size_t j = 0; j < row->size(); ++j) {
            if(node_selection(row->at(j),node_attr->at(row->at(j))) && edge_selection(i,row->at(j),row_attr->at(j))){
              selected_row[new_size++] = old2newids[row->at(j)];
              new_edge_attribute->push_back(row_attr->at(j));
            } 
          }
          edge_attributes_in->push_back(new_edge_attribute);
          
          row_lengths_in[old2newids[i]] = new_size;
          row_arrays_in[old2newids[i]] = &row_data_in[index];
          index += Set<V>::flatten_from_array(row_data_in+index,selected_row,new_size);
        
          new_cardinality += new_size;
        }
      }
      delete[] selected_row;
      total_bytes_used += index;
      row_data_in = (uint8_t*) realloc((void *) row_data_in, index*sizeof(uint8_t));
    }
  } else{

    #pragma omp parallel default(shared) reduction(+:total_bytes_used) reduction(+:new_cardinality)
    {
      uint8_t *row_data_in = new uint8_t[alloc_size];
      uint32_t *selected_row = new uint32_t[new_num_nodes];
      size_t index = 0;
      #pragma omp for schedule(static)
      for(size_t i = 0; i < matrix_size_in; ++i){
        if(old2newids[i] != -1){
          new_imap[old2newids[i]] = imap->at(i);
          vector<uint32_t> *row = g->at(i);
          size_t new_size = 0;

          for(size_t j = 0; j < row->size(); ++j) {
            if(node_selection(row->at(j),0) && edge_selection(i,row->at(j),0)){
              selected_row[new_size++] = old2newids[row->at(j)];
            } 
          }          

          row_lengths_in[old2newids[i]] = new_size;
          row_arrays_in[old2newids[i]] = &row_data_in[index];
          //if(new_size > 0){
          index += Set<V>::flatten_from_array(row_data_in+index,selected_row,new_size);
          //}
          new_cardinality += new_size;
        }
      }
    delete[] selected_row;
    total_bytes_used += index;
    row_data_in = (uint8_t*) realloc((void *) row_data_in, index*sizeof(uint8_t));
    }
  }
  delete[] old2newids;

  common::stopClock("Edge Selections");

  cout << "Number of edges: " << new_cardinality << endl;
  cout << "ROW DATA SIZE (Bytes): " << total_bytes_used << endl;

  return new SparseMatrix(new_num_nodes,new_cardinality,total_bytes_used,0,max_nbrhood_size_in,true,row_lengths_in,row_arrays_in,row_lengths_in,row_arrays_in,new_imap,node_attributes_in,edge_attributes_in,edge_attributes_in);
}

template<class V>
SparseMatrix<V>* SparseMatrix<V>::from_asymmetric(MutableGraph *inputGraph,
  const std::function<bool(uint32_t)> node_selection,
  const std::function<bool(uint32_t,uint32_t)> edge_selection, 
  const common::type t_in){
  
  const vector< vector<uint32_t>*  > *out_nbrs = inputGraph->out_neighborhoods;
  const vector< vector<uint32_t>*  > *in_nbrs = inputGraph->in_neighborhoods;
  const size_t matrix_size_in = inputGraph->num_nodes;
  const size_t cardinality_in = inputGraph->num_edges;
  const size_t max_nbrhood_size_in = inputGraph->max_nbrhood_size;
  //const vector<uint32_t> *node_attr = inputGraph->node_attr;
  //const vector<vector<uint32_t>*> *edge_attr = inputGraph->out_edge_attributes;

  uint64_t *imap = inputGraph->id_map->data();

  ops::prepare_shuffling_dictionary16();

  uint8_t **row_arrays_in = new uint8_t*[matrix_size_in];
  uint32_t *row_lengths_in = new uint32_t[matrix_size_in];
  uint8_t **col_arrays_in = new uint8_t*[matrix_size_in];
  uint32_t *col_lengths_in = new uint32_t[matrix_size_in];
  uint32_t *node_attributes_in = new uint32_t[matrix_size_in];
  vector<vector<uint32_t>*> *edge_attributes_in = new vector<vector<uint32_t>*>();
  edge_attributes_in->reserve(cardinality_in);
  cout << "Number of edges: " << cardinality_in << endl;

  size_t new_cardinality = 0;
  size_t row_total_bytes_used = 0;
  size_t col_total_bytes_used = 0;
    
  size_t alloc_size = (sizeof(uint32_t)+2)*(cardinality_in/omp_get_num_threads());
  if(alloc_size <= matrix_size_in*6){
    alloc_size = matrix_size_in*10;
  }
  #pragma omp parallel default(shared) reduction(+:row_total_bytes_used) reduction(+:new_cardinality)
  {
    uint8_t *row_data_in = new uint8_t[alloc_size];
    uint8_t *col_data_in = new uint8_t[alloc_size];

    uint32_t *selected = new uint32_t[matrix_size_in];
    size_t row_index = 0;
    size_t col_index = 0;

    #pragma omp for schedule(static,100)
    for(size_t i = 0; i < matrix_size_in; ++i){
      if(node_selection(i)){
        vector<uint32_t> *row = out_nbrs->at(i);
        size_t new_row_size = 0;
        for(size_t j = 0; j < row->size(); ++j) {
          if(node_selection(row->at(j)) && edge_selection(i,row->at(j))){
            new_cardinality++;
            selected[new_row_size++] = row->at(j);
          } 
        }
      
        row_lengths_in[i] = new_row_size;
        row_arrays_in[i] = &row_data_in[row_index];

        if(new_row_size > 0){
          index += Set<V>::flatten_from_array(row_data_in+row_index,selected,new_row_size);
        }
        new_cardinality += new_row_size;
        ////////////////////////////////////////////////////////////////////////////////////////////
        vector<uint32_t> *col = in_nbrs->at(i);
        size_t new_col_size = 0;
        for(size_t j = 0; j < col->size(); ++j) {
          if(node_selection(col->at(j)) && edge_selection(i,col->at(j))){
            new_cardinality++;
            selected[new_col_size++] = col->at(j);
          } 
        }
      
        col_lengths_in[i] = new_col_size;
        col_arrays_in[i] = &col_data_in[col_index];

        if(new_col_size > 0){
          index += Set<V>::flatten_from_array(col_data_in+col_index,selected,new_col_size);
        }
        new_cardinality += new_col_size;
      } else{
        row_lengths_in[i] = 0;
        col_lengths_in[i] = 0;
      }
    }
    delete[] selected;

    row_total_bytes_used += row_index;
    col_total_bytes_used += col_index;
    col_data_in = (uint8_t*) realloc((void *) col_data_in, col_index*sizeof(uint8_t));
    row_data_in = (uint8_t*) realloc((void *) row_data_in, row_index*sizeof(uint8_t));
  }

  cout << "ROW DATA SIZE (Bytes): " << row_total_bytes_used << endl;
  cout << "COLUMN DATA SIZE (Bytes): " << col_total_bytes_used << endl;

  return new SparseMatrix(matrix_size_in,new_cardinality,row_total_bytes_used,col_total_bytes_used,max_nbrhood_size_in,false,row_lengths_in,row_arrays_in,col_lengths_in,col_arrays_in,imap,node_attributes_in,edge_attributes_in,edge_attributes_in);
}

// FIXME: This code only works for undirected graphs
template<class V>
SparseMatrix<V>* SparseMatrix<V>::clone_on_node(int node) {
   numa_run_on_node(node);
   numa_set_preferred(node);

   size_t matrix_size = this->matrix_size;
   uint64_t lengths_size = matrix_size * sizeof(uint32_t);
   uint32_t* cloned_row_lengths =
      (uint32_t*)numa_alloc_onnode(lengths_size, node);
   std::copy(this->row_lengths, this->row_lengths + matrix_size + 1,
         cloned_row_lengths);

   std::cout << this->cardinality * sizeof(uint32_t) << std::endl;
   std::cout << this->row_total_bytes_used << std::endl;
   uint8_t** cloned_row_arrays =
      (uint8_t**) numa_alloc_onnode(matrix_size * sizeof(uint8_t*), node);
   uint8_t* neighborhood =
      (uint8_t*) numa_alloc_onnode(this->row_total_bytes_used + matrix_size, node);
   for(uint64_t i = 0; i < matrix_size; i++) {

      uint32_t row_length = cloned_row_lengths[i];
      uint8_t* data = this->row_arrays[i];
      //FIXME:
      (void) row_length;
      size_t num_bytes = 0;//uint_array::size_of_array(data, row_length, this->t);
      std::copy(data, data + num_bytes, neighborhood);
      cloned_row_arrays[i] = (uint8_t*) neighborhood;
      neighborhood += num_bytes;
   }

   uint32_t* cloned_column_lengths = NULL;
   uint8_t** cloned_column_arrays = NULL;
   /*
   if(this->symmetric) {
      cloned_column_lengths = (uint32_t*) numa_alloc_onnode(lengths_size, node);
      std::copy(column_lengths, column_lengths + matrix_size, cloned_column_lengths);

      cloned_column_arrays =
         (uint8_t**) numa_alloc_onnode(col_total_bytes_used, node);
      for(uint64_t i = 0; i < matrix_size; i++) {
         uint32_t col_length = cloned_column_lengths[i];
         uint8_t* neighborhood =
            (uint8_t*) numa_alloc_onnode(col_length * sizeof(uint8_t), node);
         std::copy(this->column_arrays[i], this->column_arrays[i] + col_length, neighborhood);
         cloned_column_arrays[i] = neighborhood;
      }
   }
   */

   std::cout << "Target node: " << node << std::endl;
   std::cout << "Node of row_lengths: " << common::find_memory_node_for_addr(cloned_row_lengths) << std::endl;
   std::cout << "Node of row_arrays: " << common::find_memory_node_for_addr(cloned_row_arrays) << std::endl;
   std::cout << "Node of neighborhood: " << common::find_memory_node_for_addr(neighborhood) << std::endl;

   return new SparseMatrix(
         matrix_size,
         this->cardinality,
         this->row_total_bytes_used,
         this->col_total_bytes_used,
         this->max_nbrhood_size,
         this->symmetric,
         cloned_row_lengths,
         cloned_row_arrays,
         cloned_column_lengths,
         cloned_column_arrays,
         this->id_map,
         this->node_attributes,
         this->out_edge_attributes,
         this->in_edge_attributes);
}



#endif
