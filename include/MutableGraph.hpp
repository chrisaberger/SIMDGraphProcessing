#ifndef MUTABLEGRAPH_H
#define MUTABLEGRAPH_H

#include <sstream>
#include <unordered_set>
#include <assert.h>

#include "common.hpp"
#include "set/layouts/hybrid.hpp"

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
  void reorder_by_shingles();

  void reassign_ids(vector<uint32_t> const& new2old_ids);

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

  static MutableGraph* directedFromAttributeList(const string path, const string node_path);
  static MutableGraph* undirectedFromAttributeList(const string path,const string node_path);
  static MutableGraph* syntheticUndirected(const size_t num_nodes, const size_t degree);
  static MutableGraph* directedFromBinary(const string path);
  static MutableGraph* undirectedFromBinary(const string path);
  static MutableGraph* directedFromEdgeList(const string path);
  static MutableGraph* undirectedFromEdgeList(const string path);
  static MutableGraph* undirectedFromAdjList(const string path,const int num_files);

  template<typename F, typename G>
  void prune_and_reorder_out_nbrs(F node_selection, G edge_selection) {
    const size_t num_layouts = 3;

    num_nodes = out_neighborhoods->size();

    int64_t* old2new = new int64_t[num_nodes];
    common::type* layouts = new common::type[num_nodes];
    for(uint32_t i = 0; i < num_nodes; i++) {
      old2new[i] = -1;
    }

    uint32_t next_id[] = {0, 0, 0};
    vector<vector<vector<uint32_t>*>*> nodes(num_layouts);
    for(size_t l = 0; l < num_layouts; l++) {
      nodes[l] = new vector<vector<uint32_t>*>();
    }

    for(uint32_t n = 0; n < num_nodes; n++) {
      if(node_selection(n, 0)) {
        vector<uint32_t>* nbrs = out_neighborhoods->at(n);
        vector<uint32_t>* new_nbrs = new vector<uint32_t>();
        for(uint32_t i = 0; i < nbrs->size(); i++) {
          uint32_t nbr = nbrs->at(i);
          if(edge_selection(n, nbr, 0)) {
            new_nbrs->push_back(nbr);
          }
        }
        common::type layout = hybrid::get_type(new_nbrs->data(), new_nbrs->size());
        nodes[layout]->push_back(new_nbrs);
        old2new[n] = next_id[layout];
        layouts[n] = layout;
        next_id[layout]++;
        delete nbrs;
      }
    }

    size_t offsets[num_layouts];
    offsets[0] = 0;
    for(size_t l = 1; l < num_layouts; l++) {
      offsets[l] = offsets[l - 1] + nodes[l - 1]->size();
    }

    size_t edges = 0;
    out_neighborhoods->clear();
    for(size_t l = 0; l < num_layouts; l++) {
      for(uint32_t n = 0; n < nodes[l]->size(); n++) {
        vector<uint32_t>* nbrs = nodes.at(l)->at(n);
        for(size_t i = 0; i < nbrs->size(); i++) {
          uint32_t nbr = nbrs->at(i);
          nbrs->at(i) = (uint32_t)(old2new[nbr] + offsets[layouts[nbr]]);
          edges++;
        }
        std::sort(nbrs->begin(), nbrs->end());
        out_neighborhoods->push_back(nbrs);
      }
    }
  }
};

#endif
