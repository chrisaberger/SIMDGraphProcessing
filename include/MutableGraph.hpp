#ifndef MUTABLEGRAPH_H
#define MUTABLEGRAPH_H

#include "common.hpp"
#include <sstream>
#include <unordered_set>
#include <assert.h>

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

  void reorder_bfs();
  void reorder_random();
  void reorder_strong_run();
  void reorder_by_rev_degree();
  void reorder_by_degree();
  void reorder_by_the_game();

  void reassign_ids(vector< vector<uint32_t>* > *neighborhoods,vector< vector<uint32_t>* > *new_neighborhoods,uint32_t *new2old_ids,uint32_t *old2new_ids);
  
  void writeDirectedToLigra(const string path);
  void writeUndirectedToBinary(const string path);
  void writeDirectedToBinary(const string path);

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
  static MutableGraph* undirectedFromAttributeList(const string path,const string node_path);
  static MutableGraph* syntheticUndirected(const size_t num_nodes, const size_t degree);
  static MutableGraph* directedFromBinary(const string path);
  static MutableGraph* undirectedFromBinary(const string path);
  static MutableGraph* directedFromEdgeList(const string path);
  static MutableGraph* undirectedFromEdgeList(const string path);
  static MutableGraph* undirectedFromAdjList(const string path,const int num_files);
};

#endif
