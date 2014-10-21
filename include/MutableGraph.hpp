#ifndef MUTABLEGRAPH_H
#define MUTABLEGRAPH_H

#include "common.hpp"
#include <sstream>

struct MutableGraph {
  size_t num_nodes;
  size_t num_edges;
  bool symmetric;
  unordered_map<unsigned int,unsigned int> *external_ids;
  vector< vector<unsigned int>*  > *out_neighborhoods;
  vector< vector<unsigned int>*  > *in_neighborhoods;

  void reorder_runs();


  MutableGraph(  size_t num_nodes_in, 
      size_t num_edges_in,
      bool symmetric_in,
      unordered_map<unsigned int,unsigned int> *external_ids_in,
      vector< vector<unsigned int>*  > *out_neighborhoods_in,
      vector< vector<unsigned int>*  > *in_neighborhoods_in): 
    num_nodes(num_nodes_in), 
    num_edges(num_edges_in),
    symmetric(symmetric_in),
    external_ids(external_ids_in), 
    out_neighborhoods(out_neighborhoods_in),
    in_neighborhoods(in_neighborhoods_in){}
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

  static MutableGraph* undirectedFromAdjList(const string path,const int num_files);
  static MutableGraph* undirectedFromEdgeList(const string path,const int num_files);
  static MutableGraph* directedFromEdgeList(const string path,const int num_files);

};

#endif
