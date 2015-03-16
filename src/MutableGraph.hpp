/*

A vector of vector structure that handles file loading
and the ordering of the node ID's. 

*/
#ifndef MUTABLEGRAPH_H
#define MUTABLEGRAPH_H

#include <sstream>
#include <unordered_set>
#include <assert.h>

#include "common.hpp"
#include "set/layouts/hybrid.hpp"

/*
Functors to perform sorts for node orderings. 
*/
struct OrderNeighborhoodByDegree{
  vector< vector<uint32_t>*  > *g;
  OrderNeighborhoodByDegree(vector< vector<uint32_t>*  > *g_in){
    g = g_in;
  }
  bool operator()(uint32_t i, uint32_t j) const {
    size_t i_size = g->at(i)->size();
    size_t j_size = g->at(j)->size();
    if(i_size == j_size)
      return i < j;
    return i_size > j_size;
  }
};
struct OrderNeighborhoodByRevDegree{
  vector< vector<uint32_t>*  > *g;
  OrderNeighborhoodByRevDegree(vector< vector<uint32_t>*  > *g_in){
    g = g_in;
  }
  bool operator()(uint32_t i, uint32_t j) const { 
    return (g->at(i)->size() < g->at(j)->size()); 
  }
};
struct OrderByID{
  OrderByID(){}
  bool operator()(pair<uint32_t,uint32_t> i, pair<uint32_t,uint32_t> j) const {
    return i.first < j.first;
  }
};

struct MutableGraph {
  size_t num_nodes;
  size_t num_edges;
  size_t max_nbrhood_size;
  bool symmetric;
  vector<uint64_t> *id_map;
  vector<uint32_t> *node_attr;
  vector< vector<uint32_t>*  > *out_neighborhoods;
  vector< vector<uint32_t>*  > *in_neighborhoods;
  vector< vector<uint32_t>*  > *out_edge_attributes;
  vector< vector<uint32_t>*  > *in_edge_attributes;

  MutableGraph(  size_t num_nodes_in, 
      size_t num_edges_in,
      size_t max_nbrhood_size_in,
      bool symmetric_in,
      vector<uint64_t> *id_map_in,
      vector<uint32_t> *node_attr_in,
      vector< vector<uint32_t>*  > *out_neighborhoods_in,
      vector< vector<uint32_t>*  > *in_neighborhoods_in,
      vector< vector<uint32_t>*  > *out_edge_attributes_in,
      vector< vector<uint32_t>*  > *in_edge_attributes_in): 
    num_nodes(num_nodes_in), 
    num_edges(num_edges_in),
    max_nbrhood_size(max_nbrhood_size_in),
    symmetric(symmetric_in),
    id_map(id_map_in), 
    node_attr(node_attr_in), 
    out_neighborhoods(out_neighborhoods_in),
    in_neighborhoods(in_neighborhoods_in),
    out_edge_attributes(out_edge_attributes_in),
    in_edge_attributes(in_edge_attributes_in){}
  ~MutableGraph(){
    for(size_t i = 0; i < out_neighborhoods->size(); ++i) {
      delete out_neighborhoods->at(i);
    }
    delete out_neighborhoods;

    if(!symmetric){
      for(size_t i = 0; i < in_neighborhoods->size(); ++i) {
        delete in_neighborhoods->at(i);
      }
      delete in_neighborhoods;
    }
  }

  /*
  Given a mapping of IDs remap the IDs into a new vector structure.
  Then set the out and in neighborhoods in the graph.
  This function only works for undirected graphs currently.
  */
  void reassign_ids(vector<uint32_t> const& new2old_ids) {
    vector<uint32_t> old2new_ids(num_nodes);
    for(size_t i = 0; i < num_nodes; i++) {
      old2new_ids.at(new2old_ids.at(i)) = i;
    }

    vector<uint64_t> *new_id_map = new vector<uint64_t>();
    vector<vector<uint32_t>*>* new_neighborhoods =
      new vector<vector<uint32_t>*>(num_nodes);

    for(size_t i = 0; i < out_neighborhoods->size(); ++i) {
      vector<uint32_t> *hood = out_neighborhoods->at(new2old_ids.at(i));
      new_id_map->push_back(id_map->at(new2old_ids.at(i)));
      for(size_t j = 0; j < hood->size(); ++j) {
        hood->at(j) = old2new_ids.at(hood->at(j));
      }
      sort(hood->begin(),hood->end());
      new_neighborhoods->at(i) = hood;
    }

    id_map->swap(*new_id_map);
    out_neighborhoods = new_neighborhoods;
    in_neighborhoods = new_neighborhoods;
  }

  //BFS ordering.
  void reorder_bfs(){
    vector<uint32_t> tmp_new2old_ids = common::range(num_nodes);
    std::random_shuffle(tmp_new2old_ids.begin(), tmp_new2old_ids.end());
    //std::sort(tmp_new2old_ids.begin(), tmp_new2old_ids.end(), OrderNeighborhoodByDegree(out_neighborhoods));

    vector<uint32_t> new2old_ids;
    new2old_ids.reserve(num_nodes);

    std::unordered_set<uint32_t> visited;

    vector<uint32_t> cur_level;
    size_t bfs_i = 0;
    uint32_t mapped_bfs_i = tmp_new2old_ids.at(bfs_i);
    cur_level.push_back(mapped_bfs_i);
    new2old_ids.push_back(mapped_bfs_i);
    visited.insert(mapped_bfs_i);
    bfs_i++;

    vector<uint32_t> next_level;
    while(new2old_ids.size() != out_neighborhoods->size()){
      for(size_t i = 0; i < cur_level.size(); i++) {
        vector<uint32_t>* hood = out_neighborhoods->at(cur_level.at(i));
        for(size_t j = 0; j < hood->size(); ++j) {
          if(visited.find(hood->at(j)) == visited.end()){
            next_level.push_back(hood->at(j));
            new2old_ids.push_back(hood->at(j));
            visited.insert(hood->at(j));
          }
        }
      }
      if(next_level.size() == 0 && new2old_ids.size() < out_neighborhoods->size()) {
        while(visited.find(tmp_new2old_ids.at(bfs_i)) != visited.end()) {
          bfs_i++;
        }
        uint32_t mapped_bfs_i = tmp_new2old_ids.at(bfs_i);
        next_level.push_back(mapped_bfs_i);
        new2old_ids.push_back(mapped_bfs_i);
        visited.insert(mapped_bfs_i);
        bfs_i++;
      }

      cur_level.swap(next_level);
      next_level.clear();
    }

    reassign_ids(new2old_ids);
  }

  /*
  Strong run ordering takes the largest neighborhood and assigns IDs 0-size(nbrhood).
  Next we take the second largest neighborhood and assign the next consecutive 
  IDs.  Proceed until whole graph has been labeled.
  */
  void reorder_strong_run(){
    vector<uint32_t> tmp_new2old_ids = common::range(num_nodes);
    std::sort(tmp_new2old_ids.begin(), tmp_new2old_ids.end(), OrderNeighborhoodByDegree(out_neighborhoods));

    vector<uint32_t> new2old_ids;
    new2old_ids.reserve(num_nodes);

    std::unordered_set<uint32_t> visited;
    for(uint32_t v : tmp_new2old_ids) {
      vector<uint32_t> *hood = out_neighborhoods->at(v);
      for(size_t j = 0; j < hood->size(); ++j) {
        if(visited.find(hood->at(j)) == visited.end()){
          new2old_ids.push_back(hood->at(j));
          visited.insert(hood->at(j));
        }
      }
    }

    reassign_ids(new2old_ids);
  }

  /*
  A random ordering of node IDs.
  */
  void reorder_random() {
    vector<uint32_t> new2old_ids = common::range(num_nodes);
    std::random_shuffle(new2old_ids.begin(), new2old_ids.end());
    reassign_ids(new2old_ids);
  }

  /*
  Advanced ordering designed for compression.

  http://www.eecs.harvard.edu/~michaelm/postscripts/kdd2009.pdf
  */
  void reorder_by_shingles() {
    // Initialize ordering
    vector<uint32_t> ordering = common::range(num_nodes);
    vector<uint32_t> new2old_ids = common::range(num_nodes);

    // Find shingles for different orderings
    const size_t num_orderings = 2;
    vector<vector<uint32_t>> shingles(num_orderings, vector<uint32_t>(num_nodes));
    for(size_t i = 0; i < num_orderings; i++) {
      // New ordering
      std::random_shuffle(ordering.begin(), ordering.end());

      // Find shingles for each neighbor set
      for(size_t j = 0; j < num_nodes; j++) {
        vector<uint32_t>* neighbors = out_neighborhoods->at(j);

        uint32_t shingle = 0;
        if(neighbors->size() > 0) {
          shingle = neighbors->at(0);
          uint32_t shingle_ord = ordering.at(shingle);
          for(size_t k = 1; k < neighbors->size(); k++) {
            uint32_t curr_elem = neighbors->at(k);
            uint32_t curr_ord = ordering.at(curr_elem);
            if(curr_ord < shingle_ord) {
              shingle = curr_elem;
              shingle_ord = curr_ord;
            }
          }
        }
        shingles.at(i).at(j) = shingle;
      }
    }

    auto cmp_nodes = [&shingles](uint32_t a, uint32_t b) -> bool {
      for(size_t i = 0; i < num_orderings; i++) {
        uint32_t a_val = shingles.at(i).at(a);
        uint32_t b_val = shingles.at(i).at(b);

        if(a_val < b_val) {
          return true;
        } else if(a_val > b_val) {
          return false;
        }
      }

      return false;
    };

    // Sort using shingles
    std::sort(new2old_ids.begin(), new2old_ids.end(), cmp_nodes);
    reassign_ids(new2old_ids);
  }

  //Order by degree
  void reorder_by_degree(){
    vector<uint32_t> new2old_ids = common::range(num_nodes);
    std::sort(new2old_ids.begin(), new2old_ids.end(), OrderNeighborhoodByDegree(out_neighborhoods));
    reassign_ids(new2old_ids);
  }

  //Reverse degree.
  void reorder_by_rev_degree(){
    vector<uint32_t> new2old_ids = common::range(num_nodes);
    std::sort(new2old_ids.begin(), new2old_ids.end(), OrderNeighborhoodByRevDegree(out_neighborhoods));
    reassign_ids(new2old_ids);
  }

  //Our hybrid ordering scheme
  void reorder_by_the_game() {
    reorder_bfs();
    reorder_by_degree();
  }

  /*
  File format

  Edges must be duplicated.  If you have edge (0,1) you must
  also have (1,0) listed.

  src0 dst1
  dst1 src0
  ...

  */
  void writeUndirectedToBinary(const string path) {
    std::cout << "Writing graph to binary file..." << std::endl;

    ofstream outfile;
    outfile.open(path, ios::binary | ios::out);

    size_t osize = out_neighborhoods->size();
    outfile.write((char *)&osize, sizeof(osize));
    for(size_t i = 0; i < out_neighborhoods->size(); ++i){
      vector<uint32_t> *row = out_neighborhoods->at(i);
      size_t rsize = row->size();
      outfile.write((char*)&id_map->at(i),sizeof(uint64_t));
      outfile.write((char *)&rsize, sizeof(rsize));
      outfile.write((char *)row->data(),sizeof(uint32_t)*rsize);
    }
    outfile.close();
  }

  static MutableGraph* undirectedFromBinary(const string path) {
    ifstream infile;
    infile.open(path, ios::binary | ios::in);

    vector<uint64_t> *id_map = new vector<uint64_t>();
    vector<uint32_t> *id_attributes = NULL;
    vector< vector<uint32_t>*  > *neighborhoods = new vector< vector<uint32_t>* >();
    vector< vector<uint32_t>*  > *edge_attributes = NULL;

    size_t num_edges = 0;
    size_t num_nodes = 0;

    size_t max_nbrhood_size = 0;

    infile.read((char *)&num_nodes, sizeof(num_nodes));
    for(size_t i = 0; i < num_nodes; ++i){
      uint64_t external_id;
      infile.read((char *)&external_id, sizeof(uint64_t));
      id_map->push_back(external_id);

      size_t row_size = 0;
      infile.read((char *)&row_size, sizeof(row_size));
      num_edges += row_size;

      if(row_size > max_nbrhood_size)
        max_nbrhood_size = row_size;

      vector<uint32_t> *row = new vector<uint32_t>(row_size);
      infile.read((char*)(row->data()), sizeof(uint32_t) * row->size());
      neighborhoods->push_back(row);
    }
    infile.close();

    return new MutableGraph(
        neighborhoods->size(),
        num_edges,
        max_nbrhood_size,
        true,
        id_map,
        id_attributes,
        neighborhoods,
        neighborhoods,
        edge_attributes,
        edge_attributes);
  }

  static MutableGraph* undirectedFromAttributeList(const string path, const string node_path) {
    ////////////////////////////////////////////////////////////////////////////////////
    //Place graph into vector of vectors then decide how you want to
    //store the graph.
    unordered_map<uint64_t,uint32_t> *extern_ids = new unordered_map<uint64_t,uint32_t>();
    vector<uint64_t> *id_map = new vector<uint64_t>();
    vector< vector<uint32_t>*  > *neighborhoods = new vector< vector<uint32_t>* >();
    vector< vector<uint32_t>*  > *edge_attributes = new vector< vector<uint32_t>* >();

    cout << path << endl;
    FILE *pFile = fopen(path.c_str(),"r");
    if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

    // obtain file size:
    fseek(pFile,0,SEEK_END);
    size_t lSize = ftell(pFile);
    rewind(pFile);

    // allocate memory to contain the whole file:
    char *buffer = (char*) malloc (sizeof(char)*lSize + 1);

    neighborhoods->reserve(lSize/4);
    extern_ids->reserve(lSize/4);
    if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

    // copy the file into the buffer:
    size_t result = fread (buffer,1,lSize,pFile);
    if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
    buffer[result] = '\0';

    std::set<pair<uint32_t,uint32_t>> *edge_set = new std::set<pair<uint32_t,uint32_t>>(); 
    char *test = strtok(buffer," |\t\nA");
    while(test != NULL){
      uint64_t src;
      sscanf(test,"%lu",&src);
      test = strtok(NULL," |\t\nA");
      
      uint64_t dst;
      sscanf(test,"%lu",&dst);
      test = strtok(NULL," |\t\nA");

      uint32_t year;
      sscanf(test,"%u",&year);
      test = strtok(NULL," -|\t\nA");

      if(edge_set->find(make_pair(src,dst)) == edge_set->end()){
        edge_set->insert(make_pair(src,dst));
        edge_set->insert(make_pair(dst,src));

        vector<uint32_t> *src_row;
        vector<uint32_t> *src_attr;
        if(extern_ids->find(src) == extern_ids->end()){
          extern_ids->insert(make_pair(src,extern_ids->size()));
          id_map->push_back(src);
          src_row = new vector<uint32_t>();
          neighborhoods->push_back(src_row);
          src_attr = new vector<uint32_t>();
          edge_attributes->push_back(src_attr);
        } else{
          src_attr = edge_attributes->at(extern_ids->at(src));
          src_row = neighborhoods->at(extern_ids->at(src));
        }

        vector<uint32_t> *dst_row;
        vector<uint32_t> *dst_attr;
        if(extern_ids->find(dst) == extern_ids->end()){
          extern_ids->insert(make_pair(dst,extern_ids->size()));
          id_map->push_back(dst);
          dst_row = new vector<uint32_t>();
          neighborhoods->push_back(dst_row);
          dst_attr = new vector<uint32_t>();
          edge_attributes->push_back(dst_attr);
        } else{
          dst_attr = edge_attributes->at(extern_ids->at(dst));
          dst_row = neighborhoods->at(extern_ids->at(dst));
        }

        src_attr->push_back(year);
        dst_attr->push_back(year);
        src_row->push_back(extern_ids->at(dst));
        dst_row->push_back(extern_ids->at(src));
      }
    }
    // terminate
    fclose(pFile);
    free(buffer);

    cout << node_path << endl;

    //////////////////////////////////////////////////////////////////////////////
    vector<uint32_t> *id_attributes = new vector<uint32_t>();
    id_attributes->resize(neighborhoods->size()); 

    FILE *pFile2 = fopen(node_path.c_str(),"r");
    if (pFile2==NULL) {fputs ("File error",stderr); exit (1);}

    // obtain file size:
    fseek(pFile2,0,SEEK_END);
    lSize = ftell(pFile2);
    rewind(pFile2);

    // allocate memory to contain the whole file:
    buffer = (char*) malloc (sizeof(char)*lSize + 1);
    if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

    // copy the file into the buffer:
    result = fread (buffer,1,lSize,pFile2);
    if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
    buffer[result] = '\0';

    test = strtok(buffer," |\t\nA");
    while(test != NULL){
      uint64_t id;
      sscanf(test,"%lu",&id);
      test = strtok(NULL," |\t\nA");

      uint32_t attr;
      sscanf(test,"%u",&attr);
      test = strtok(NULL," |\t\nA");

      if(extern_ids->find(id) != extern_ids->end()){
        id_attributes->at(extern_ids->at(id)) = attr;
      }
    }
    // terminate
    fclose(pFile2);
    free(buffer);
    //////////////////////////////////////////////////////////////////////////////

    size_t max_nbrhood_size = 0;
    size_t num_edges = 0;
    for(size_t i = 0; i < neighborhoods->size(); i++){
      vector<uint32_t> *row = neighborhoods->at(i);
      vector<uint32_t> *row_attr = edge_attributes->at(i);

      vector<pair<uint32_t,uint32_t>> *pair_list = new vector<pair<uint32_t,uint32_t>>();
      for(size_t j = 0; j < row->size(); j++){
        pair_list->push_back(make_pair(row->at(j),row_attr->at(j)));
      }
      std::sort(pair_list->begin(),pair_list->end(),OrderByID());
      for(size_t j = 0; j < row->size(); j++){
        row->at(j) = pair_list->at(j).first;
        row_attr->at(j) = pair_list->at(j).second;
      }
      delete pair_list;

      if(row->size() > max_nbrhood_size)
        max_nbrhood_size = row->size();

      //row->erase(unique(row->begin(),row->begin()+row->size()),row->end());
      num_edges += row->size();
    }

    return new MutableGraph(id_map->size(),num_edges,max_nbrhood_size,true,id_map,id_attributes,neighborhoods,neighborhoods,edge_attributes,edge_attributes); 
  } 
  static MutableGraph* undirectedFromEdgeList(const string path) {
    ////////////////////////////////////////////////////////////////////////////////////
    //Place graph into vector of vectors then decide how you want to
    //store the graph.
    unordered_map<uint64_t,uint32_t> *extern_ids = new unordered_map<uint64_t,uint32_t>();
    vector<uint64_t> *id_map = new vector<uint64_t>();
    vector<uint32_t> *id_attributes = NULL;
    vector< vector<uint32_t>*  > *neighborhoods = new vector< vector<uint32_t>* >();
    vector< vector<uint32_t>*  > *edge_attributes = NULL;

    cout << path << endl;
    FILE *pFile = fopen(path.c_str(),"r");
    if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

    // obtain file size:
    fseek(pFile,0,SEEK_END);
    size_t lSize = ftell(pFile);
    rewind(pFile);

    // allocate memory to contain the whole file:
    char *buffer = (char*)malloc(sizeof(char) * lSize + 1);
    neighborhoods->reserve(lSize/4);
    extern_ids->reserve(lSize/4);
    if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

    // copy the file into the buffer:
    size_t result = fread (buffer,1,lSize,pFile);
    if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
    buffer[result] = '\0';

    char *test = strtok(buffer," \t\nA");
    while(test != NULL){
      uint64_t src;
      sscanf(test,"%lu",&src);
      test = strtok(NULL," \t\nA");
      
      uint64_t dst;
      sscanf(test,"%lu",&dst);
      test = strtok(NULL," \t\nA");

      vector<uint32_t> *src_row;
      if(extern_ids->find(src) == extern_ids->end()){
        extern_ids->insert(make_pair(src,extern_ids->size()));
        id_map->push_back(src);
        src_row = new vector<uint32_t>();
        neighborhoods->push_back(src_row);
      } else{
        src_row = neighborhoods->at(extern_ids->at(src));
      }

      vector<uint32_t> *dst_row;
      if(extern_ids->find(dst) == extern_ids->end()){
        extern_ids->insert(make_pair(dst,extern_ids->size()));
        id_map->push_back(dst);
        dst_row = new vector<uint32_t>();
        neighborhoods->push_back(dst_row);
      } else{
        dst_row = neighborhoods->at(extern_ids->at(dst));
      }

      src_row->push_back(extern_ids->at(dst));
      dst_row->push_back(extern_ids->at(src));
    }
    // terminate
    fclose(pFile);
    free(buffer);

    size_t max_nbrhood_size = 0;
    size_t num_edges = 0;
    for(size_t i = 0; i < neighborhoods->size(); i++){
      vector<uint32_t> *row = neighborhoods->at(i);
      std::sort(row->begin(),row->end());

      if(row->size() > max_nbrhood_size)
        max_nbrhood_size = row->size();

      row->erase(unique(row->begin(),row->begin()+row->size()),row->end());
      num_edges += row->size();
    }

    delete extern_ids;
    return new MutableGraph(neighborhoods->size(),num_edges,max_nbrhood_size,true,id_map,id_attributes,neighborhoods,neighborhoods,edge_attributes,edge_attributes); 
  }

  void writeDirectedToLigra(const string path) {
    ofstream myfile;
    myfile.open(path);

    myfile << "AdjacencyGraph" << endl;
    myfile << num_nodes << endl;

    size_t numedges = 0;
    for(size_t i = 0; i < num_nodes; ++i){
      /////////////////////////////////////////////////////////////////////
      vector<uint32_t> *row = out_neighborhoods->at(i);
      numedges += row->size();
    }
    myfile << numedges << endl;


    size_t index = 0;
    for(size_t i = 0; i < num_nodes; ++i){
      myfile << index << endl;

      /////////////////////////////////////////////////////////////////////
      vector<uint32_t> *row = out_neighborhoods->at(i);
      size_t rsize = row->size();
      index += rsize;
    }
    for(size_t i = 0; i < num_nodes; ++i){
      vector<uint32_t> *row = out_neighborhoods->at(i);
      size_t rsize = row->size();
      for(size_t j = 0; j < rsize; j++){
        myfile << row->at(j) << endl;
      }
    }
    myfile.close();
  }

  void writeDirectedToBinary(const string path) {
    ofstream outfile;
    outfile.open(path, ios::binary | ios::out);

    outfile.write((char *)&num_nodes, sizeof(num_nodes));
    for(size_t i = 0; i < num_nodes; ++i){
      outfile.write((char*)&id_map->at(i),sizeof(uint64_t));

      /////////////////////////////////////////////////////////////////////
      vector<uint32_t> *row = out_neighborhoods->at(i);
      size_t rsize = row->size();
      outfile.write((char *)&rsize, sizeof(rsize)); 
      outfile.write((char *)row->data(),sizeof(uint32_t)*rsize);

      /////////////////////////////////////////////////////////////////////
      vector<uint32_t> *col = in_neighborhoods->at(i);
      size_t csize = col->size();
      outfile.write((char *)&csize, sizeof(csize)); 
      outfile.write((char *)col->data(),sizeof(uint32_t)*csize);
    }
    outfile.close();
  }
  static MutableGraph* directedFromBinary(const string path) {
    ifstream infile; 
    infile.open(path, ios::binary | ios::in); 

    vector<uint64_t> *id_map = new vector<uint64_t>();
    vector< vector<uint32_t>*  > *out_neighborhoods = new vector< vector<uint32_t>* >();
    vector< vector<uint32_t>*  > *in_neighborhoods = new vector< vector<uint32_t>* >();
    vector< vector<uint32_t>*  > *edge_attributes = NULL;
    vector<uint32_t> *id_attributes = NULL;

    size_t num_edges = 0;
    size_t num_nodes = 0;
    infile.read((char *)&num_nodes, sizeof(num_nodes)); 

    size_t max_nbrhood_size = 0;

    cout << "num nodes: " << num_nodes << endl;
    for(size_t i = 0; i < num_nodes; ++i){
      uint64_t external_id;
      infile.read((char *)&external_id, sizeof(uint64_t)); 
      id_map->push_back(external_id);

      //////////////////////////////////////////////////////////////////// 
      size_t row_size = 0;
      infile.read((char *)&row_size, sizeof(row_size)); 
      num_edges += row_size;

      if(row_size > max_nbrhood_size)
        max_nbrhood_size = row_size;

      vector<uint32_t> *row = new vector<uint32_t>();
      row->reserve(row_size);
      uint32_t *r_tmp_data = new uint32_t[row_size];
      infile.read((char *)&r_tmp_data[0], sizeof(uint32_t)*row_size); 
      row->assign(&r_tmp_data[0],&r_tmp_data[row_size]);
      out_neighborhoods->push_back(row);

      ////////////////////////////////////////////////////////////////////
      size_t col_size = 0;
      infile.read((char *)&col_size, sizeof(col_size)); 
      num_edges += col_size;

      if(col_size > max_nbrhood_size)
        max_nbrhood_size = col_size;

      vector<uint32_t> *col = new vector<uint32_t>();
      col->reserve(col_size);
      uint32_t *c_tmp_data = new uint32_t[col_size];
      infile.read((char *)&c_tmp_data[0], sizeof(uint32_t)*col_size); 
      col->assign(&c_tmp_data[0],&c_tmp_data[col_size]);
      in_neighborhoods->push_back(col);
    }
    infile.close();

    std::cout << "Number of edges in file: " << num_edges << std::endl;

    return new MutableGraph(out_neighborhoods->size(),num_edges,max_nbrhood_size,false,id_map,id_attributes,out_neighborhoods,in_neighborhoods,edge_attributes,edge_attributes); 
  } 
  /*
  File format

  src0 dst0
  src1 dst1
  ...

  */
  static MutableGraph* directedFromAttributeList(const string path, const string node_path) {  
    //Place graph into vector of vectors then decide how you want to
    //store the graph.
    size_t num_edges = 0;

    vector<uint64_t> *id_map = new vector<uint64_t>();
    unordered_map<uint64_t,uint32_t> *extern_ids = new unordered_map<uint64_t,uint32_t>();
    vector< vector<uint32_t>*  > *in_neighborhoods = new vector< vector<uint32_t>* >();
    vector< vector<uint32_t>*  > *out_neighborhoods = new vector< vector<uint32_t>* >();
    vector< vector<uint32_t>*  > *out_edge_attributes = new vector< vector<uint32_t>* >();
    vector< vector<uint32_t>*  > *in_edge_attributes = new vector< vector<uint32_t>* >();

    FILE *pFile = fopen(path.c_str(),"r");
    if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

    // obtain file size:
    fseek(pFile,0,SEEK_END);
    size_t lSize = ftell(pFile);
    rewind(pFile);

    // allocate memory to contain the whole file:
    char *buffer = (char*) malloc (sizeof(char)*lSize + 1);
    if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

    // copy the file into the buffer:
    size_t result = fread (buffer,1,lSize,pFile);
    if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
    buffer[result] = '\0';

    char *test = strtok(buffer," \t\nA");
    while(test != NULL){
      uint64_t src;
      sscanf(test,"%lu",&src);
      test = strtok(NULL," \t\nA");
      
      uint64_t dst;
      sscanf(test,"%lu",&dst);
      test = strtok(NULL," \t\nA");

      uint32_t year;
      sscanf(test,"%u",&year);
      test = strtok(NULL," -|\t\nA");

      num_edges++;

      vector<uint32_t> *src_row;
      vector<uint32_t> *src_attr;
      if(extern_ids->find(src) == extern_ids->end()){
        extern_ids->insert(make_pair(src,extern_ids->size()));
        id_map->push_back(src);
        src_row = new vector<uint32_t>();
        vector<uint32_t> *new_row = new vector<uint32_t>();
        in_neighborhoods->push_back(new_row);
        out_neighborhoods->push_back(src_row);
        src_attr = new vector<uint32_t>();
        out_edge_attributes->push_back(src_attr);
        in_edge_attributes->push_back(new vector<uint32_t>());
      } else{
        src_attr = out_edge_attributes->at(extern_ids->at(src));
        src_row = out_neighborhoods->at(extern_ids->at(src));
      }

      vector<uint32_t> *dst_row;
      vector<uint32_t> *dst_attr;
      if(extern_ids->find(dst) == extern_ids->end()){
        extern_ids->insert(make_pair(dst,extern_ids->size()));
        id_map->push_back(dst);
        dst_row = new vector<uint32_t>();
        out_neighborhoods->push_back(new vector<uint32_t>());
        in_neighborhoods->push_back(dst_row);
        dst_attr = new vector<uint32_t>();
        in_edge_attributes->push_back(dst_attr);
        out_edge_attributes->push_back(new vector<uint32_t>());
      } else{
        dst_row = in_neighborhoods->at(extern_ids->at(dst));
        dst_attr = in_edge_attributes->at(extern_ids->at(dst));
      }
      src_row->push_back(extern_ids->at(dst));
      src_attr->push_back(year);
      dst_row->push_back(extern_ids->at(src));
      dst_attr->push_back(year);
    }
    // terminate
    fclose(pFile);
    free(buffer);

    //////////////////////////////////////////////////////////////////////////////
    vector<uint32_t> *id_attributes = new vector<uint32_t>();
    id_attributes->resize(in_neighborhoods->size()); 

    FILE *pFile2 = fopen(node_path.c_str(),"r");
    if (pFile2==NULL) {fputs ("File error",stderr); exit (1);}

    // obtain file size:
    fseek(pFile2,0,SEEK_END);
    lSize = ftell(pFile2);
    rewind(pFile2);

    // allocate memory to contain the whole file:
    buffer = (char*) malloc (sizeof(char)*lSize + 1);
    if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

    // copy the file into the buffer:
    result = fread (buffer,1,lSize,pFile2);
    if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
    buffer[result] = '\0';

    test = strtok(buffer," |\t\nA");
    while(test != NULL){
      uint64_t id;
      sscanf(test,"%lu",&id);
      test = strtok(NULL," |\t\nA");

      uint32_t attr;
      sscanf(test,"%u",&attr);
      test = strtok(NULL," |\t\nA");

      if(extern_ids->find(id) != extern_ids->end()){
        id_attributes->at(extern_ids->at(id)) = attr;
      }
    }
    // terminate
    fclose(pFile2);
    free(buffer);
    //////////////////////////////////////////////////////////////////////////////
    size_t max_nbrhood_size = 0;
    for(size_t i = 0; i < in_neighborhoods->size(); i++){
      vector<uint32_t> *row = out_neighborhoods->at(i);
      vector<uint32_t> *row_attr = out_edge_attributes->at(i);

      if(row->size() > 0){
        vector<pair<uint32_t,uint32_t>> *pair_list = new vector<pair<uint32_t,uint32_t>>();
        for(size_t j = 0; j < row->size(); j++){
          pair_list->push_back(make_pair(row->at(j),row_attr->at(j)));
        }
        std::sort(pair_list->begin(),pair_list->end(),OrderByID());
        for(size_t j = 0; j < row->size(); j++){
          row->at(j) = pair_list->at(j).first;
          row_attr->at(j) = pair_list->at(j).second;
        }
        delete pair_list;

        if(row->size() > max_nbrhood_size)
          max_nbrhood_size = row->size();
      }

      row = in_neighborhoods->at(i);
      row_attr = in_edge_attributes->at(i);
      if(row->size() > 0){
        vector<pair<uint32_t,uint32_t>> *pair_list = new vector<pair<uint32_t,uint32_t>>();
        for(size_t j = 0; j < row->size(); j++){
          pair_list->push_back(make_pair(row->at(j),row_attr->at(j)));
        }
        std::sort(pair_list->begin(),pair_list->end(),OrderByID());
        for(size_t j = 0; j < row->size(); j++){
          row->at(j) = pair_list->at(j).first;
          row_attr->at(j) = pair_list->at(j).second;
        }
        delete pair_list;

        if(row->size() > max_nbrhood_size)
          max_nbrhood_size = row->size();
      }
      //row->erase(unique(row->begin(),row->begin()+row->size()),row->end());
    }

    delete extern_ids;
    return new MutableGraph(in_neighborhoods->size(),num_edges,max_nbrhood_size,false,id_map,id_attributes,out_neighborhoods,in_neighborhoods,out_edge_attributes,in_edge_attributes); 
  }
  static MutableGraph* directedFromEdgeList(const string path) {  
    //Place graph into vector of vectors then decide how you want to
    //store the graph.
    size_t num_edges = 0;

    vector<uint64_t> *id_map = new vector<uint64_t>();
    unordered_map<uint64_t,uint32_t> *extern_ids = new unordered_map<uint64_t,uint32_t>();
    vector< vector<uint32_t>*  > *in_neighborhoods = new vector< vector<uint32_t>* >();
    vector< vector<uint32_t>*  > *out_neighborhoods = new vector< vector<uint32_t>* >();
    vector<uint32_t> *id_attributes = NULL;
    vector< vector<uint32_t>*  > *edge_attributes = NULL;

    FILE *pFile = fopen(path.c_str(),"r");
    if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

    // obtain file size:
    fseek(pFile,0,SEEK_END);
    size_t lSize = ftell(pFile);
    rewind(pFile);

    // allocate memory to contain the whole file:
    char *buffer = (char*) malloc (sizeof(char)*lSize + 1);
    if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

    // copy the file into the buffer:
    size_t result = fread (buffer,1,lSize,pFile);
    if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
    buffer[result] = '\0';

    char *test = strtok(buffer," \t\nA");
    while(test != NULL){
      uint64_t src;
      sscanf(test,"%lu",&src);
      test = strtok(NULL," \t\nA");
      
      uint64_t dst;
      sscanf(test,"%lu",&dst);
      test = strtok(NULL," \t\nA");

      num_edges++;

      vector<uint32_t> *src_row;
      if(extern_ids->find(src) == extern_ids->end()){
        extern_ids->insert(make_pair(src,extern_ids->size()));
        id_map->push_back(src);
        src_row = new vector<uint32_t>();
        vector<uint32_t> *new_row = new vector<uint32_t>();
        in_neighborhoods->push_back(new_row);
        out_neighborhoods->push_back(src_row);
      } else{
        src_row = out_neighborhoods->at(extern_ids->at(src));
      }

      vector<uint32_t> *dst_row;
      if(extern_ids->find(dst) == extern_ids->end()){
        extern_ids->insert(make_pair(dst,extern_ids->size()));
        id_map->push_back(dst);
        dst_row = new vector<uint32_t>();
        vector<uint32_t> *new_row = new vector<uint32_t>();
        out_neighborhoods->push_back(new_row);
        in_neighborhoods->push_back(dst_row);
      } else{
        dst_row = in_neighborhoods->at(extern_ids->at(dst));
      }
      src_row->push_back(extern_ids->at(dst));
      dst_row->push_back(extern_ids->at(src));
    }
    // terminate
    fclose(pFile);
    free(buffer);

    size_t max_nbrhood_size = 0;
    for(size_t i = 0; i < in_neighborhoods->size(); i++){
      vector<uint32_t> *row = in_neighborhoods->at(i);
      std::sort(row->begin(),row->end());

      if(row->size() > max_nbrhood_size)
        max_nbrhood_size = row->size();

      row->erase(unique(row->begin(),row->begin()+row->size()),row->end());
    }
    for(size_t i = 0; i < out_neighborhoods->size(); i++){
      vector<uint32_t> *row = out_neighborhoods->at(i);
      std::sort(row->begin(),row->end());

      if(row->size() > max_nbrhood_size)
        max_nbrhood_size = row->size();

      row->erase(unique(row->begin(),row->begin()+row->size()),row->end());
    }

    delete extern_ids;
    return new MutableGraph(in_neighborhoods->size(),num_edges,max_nbrhood_size,false,id_map,id_attributes,out_neighborhoods,in_neighborhoods,edge_attributes,edge_attributes); 
  }


};

#endif
