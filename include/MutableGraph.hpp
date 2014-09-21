#ifndef MUTABLEGRAPH_H
#define MUTABLEGRAPH_H

#include "common.hpp"
#include <sstream>

struct MutableGraph {
  size_t num_nodes;
  size_t num_edges;
  unordered_map<unsigned int,unsigned int> *external_ids;
  vector< vector<unsigned int>*  > *neighborhoods;

  MutableGraph(  size_t num_nodes_in, 
      size_t num_edges_in,
      unordered_map<unsigned int,unsigned int> *external_ids_in,
      vector< vector<unsigned int>*  > *neighborhoods_in): 
    num_nodes(num_nodes_in), 
    num_edges(num_edges_in),
    external_ids(external_ids_in), 
    neighborhoods(neighborhoods_in){}
  ~MutableGraph(){
    delete external_ids;
    for(size_t i = 0; i < num_nodes; ++i) {
      vector<unsigned int> *hood = neighborhoods->at(i);
      hood->clear();
      delete hood;
    }
    neighborhoods->erase(neighborhoods->begin(),neighborhoods->end());
    delete neighborhoods;
  }
  MutableGraph(const string path,const int num_files);
};

#endif
