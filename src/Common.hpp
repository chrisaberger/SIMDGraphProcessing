#ifndef COMMON_H
#define COMMON_H

#include <unordered_map>
#include <ctime>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>  
#include <iostream>
#include <vector>
#include <iterator>
#include <algorithm>  // for std::find
#include <iostream>   // for std::cout
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */
#include <fstream>

#include "Graph.hpp"
#include "CSRGraph.hpp"
#include "Partition.hpp"

using namespace std;

double t1;
double t2;
struct timeval tim;  

void startClock (){
  gettimeofday(&tim, NULL);  
  t1=tim.tv_sec+(tim.tv_usec/1000000.0); 
}

void stopClock(string in){
  gettimeofday(&tim, NULL);  
  t2=tim.tv_sec+(tim.tv_usec/1000000.0); 
  std::cout << "Time["+in+"]: " << t2-t1 << " s" << std::endl;
}

void allocateStack(){
  const rlim_t kStackSize = 64L * 1024L * 1024L;   // min stack size = 64 Mb
  struct rlimit rl;
  int result;

  result = getrlimit(RLIMIT_STACK, &rl);
  if (result == 0){
    if (rl.rlim_cur < kStackSize){
      rl.rlim_cur = kStackSize;
      result = setrlimit(RLIMIT_STACK, &rl);
      if (result != 0){
        fprintf(stderr, "setrlimit returned result = %d\n", result);
      }
    }
  } 
}

struct VectorGraph {
  size_t num_nodes;
  size_t num_edges;
  unordered_map<unsigned int,unsigned int> *external_ids;
  vector< vector<unsigned int>*  > *neighborhoods;

  VectorGraph(  size_t num_nodes_in, 
      size_t num_edges_in,
      unordered_map<unsigned int,unsigned int> *external_ids_in,
      vector< vector<unsigned int>*  > *neighborhoods_in): 
    num_nodes(num_nodes_in), 
    num_edges(num_edges_in),
    external_ids(external_ids_in), 
    neighborhoods(neighborhoods_in){}
  ~VectorGraph(){
    delete external_ids;
    for(size_t i = 0; i < num_nodes; ++i) {
      vector<unsigned int> *hood = neighborhoods->at(i);
      hood->clear();
      delete hood;
    }
    neighborhoods->erase(neighborhoods->begin(),neighborhoods->end());
    delete neighborhoods;
  }
};
struct CompareIndexVector : std::binary_function<size_t, size_t, bool>
{
  CompareIndexVector(const std::vector<unsigned int> *data)
  : m_data(data)
  {}

  bool operator()(size_t Lhs, size_t Rhs)const
  {
    return m_data->at(Lhs) < m_data->at(Rhs);
  }

  const std::vector<unsigned int>* m_data;
};
//this is a functor
struct Comparator {
  bool operator()(vector<unsigned int> *i,vector<unsigned int> *j) const { 
    return (i->size() > j->size()); 
  }
};

static inline VectorGraph* ReadFile (const string path,const int num_files) {

  vector< vector<unsigned int>* >* *graph_in = new vector< vector<unsigned int>* >*[num_files];

  size_t num_edges = 0;
  size_t num_nodes = 0;
  //Place graph into vector of vectors then decide how you want to
  //store the graph.
  
  //startClock();
  #pragma omp parallel for default(none) shared(graph_in,path) reduction(+:num_edges) reduction(+:num_nodes)
  for(size_t i=0; i <= (size_t) num_files;++i){
    vector< vector<unsigned int>*  > *file_adj = new vector< vector<unsigned int>* >();

    string file_path = path;
    if(num_files!=0) file_path.append(to_string(i));

    ifstream myfile (file_path);
    string line;
    if (myfile.is_open()){
      while ( getline (myfile,line) ){
        vector<unsigned int> *cur = new vector<unsigned int>(); //guess a size
        cur->reserve(line.length());
        istringstream iss(line);
        do{
          string sub;
          iss >> sub;
          if(sub.compare("")){
            cur->push_back(atoi(sub.c_str()));
          }
        } while (iss);
        cur->resize(cur->size());   
        num_edges += cur->size()-1;
        num_nodes++;
        file_adj->push_back(cur);
      }
      //Pre-sort by degree.
      //std::sort(file_adj->begin(),file_adj->end(),Comparator());
      graph_in[i] = file_adj;
    }
  }
  //stopClock("Reading files from disk");

  //startClock();
  //Serial Merge: Could actually do a merge sort if I cared enough.
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();
  neighborhoods->reserve(num_nodes);
  for(size_t i=0; i <= (size_t) num_files;++i){
    neighborhoods->insert(neighborhoods->end(),graph_in[i]->begin(),graph_in[i]->end());
    graph_in[i]->erase(graph_in[i]->begin(),graph_in[i]->end());
  }
  delete[] graph_in;
  //Sort the neighborhoods by degree.
  std::sort(neighborhoods->begin(),neighborhoods->end(),Comparator());
  //stopClock("Merging and sorting");

  //startClock();
  //Build hash map
  unordered_map<unsigned int,unsigned int> *external_ids = new unordered_map<unsigned int,unsigned int>();
  external_ids->reserve(neighborhoods->size());
  //#pragma omp parallel for default(none) shared(neighborhoods,external_ids)
  for(size_t i = 0; i < neighborhoods->size(); ++i) {
    vector<unsigned int> *hood = neighborhoods->at(i);
    external_ids->insert(make_pair(hood->at(0),i));
    hood->erase(hood->begin());
  }
 // stopClock("Building hashmap");

  //startClock();
  #pragma omp parallel for default(none) shared(neighborhoods,external_ids) schedule(static,150)
  for(size_t i = 0; i < neighborhoods->size(); ++i) {
    vector<unsigned int> *hood = neighborhoods->at(i);
    //cout << "Node: " << i << endl;
    size_t index = 0;
    for(size_t j = 0; j < hood->size(); ++j) {
      hood->at(index) = external_ids->at(hood->at(j));
      index++;
    }
    hood->resize(index);
    sort(hood->begin(),hood->end());
  } 
  //stopClock("Reassigning ids");

  return new VectorGraph(num_nodes,num_edges,external_ids,neighborhoods);
}


static inline CompressedGraph* createCompressedGraph (VectorGraph *vg) {
  prepare_shuffling_dictionary16();
  size_t num_nodes = vg->num_nodes;

  size_t *nodes = new size_t[vg->num_nodes+1];
  unsigned short *edges = new unsigned short[vg->num_edges*5];
  unsigned int *nbrlengths = new unsigned int[vg->num_nodes];
  unsigned short *unions = new unsigned short[vg->num_nodes*1000];
  size_t *union_size = new size_t[vg->num_nodes+1]; 
  unordered_map<unsigned int,size_t> **back_index_map = new unordered_map<unsigned int,size_t>*[num_nodes];
  
  size_t *node_back_index_pointer = new size_t[vg->num_nodes+1];
  unsigned int *union_info_flat = new unsigned int[vg->num_edges*100];
  size_t *back_index_dat = new size_t[vg->num_edges*25];
  unsigned int *back_edges = new unsigned int[vg->num_edges*50];

  const unordered_map<unsigned int,unsigned int> *external_ids = vg->external_ids;

  size_t lower_prefix = 16;
  size_t upper_prefix = 32-lower_prefix;

  cout  << "Num nodes: " << vg->num_nodes << " Num edges: " << vg->num_edges << endl;
  
  size_t num_edges = 0;
  size_t index = 0;
  size_t back_index_index = 0;
  size_t back_dat_index = 0;
  size_t union_index = 0;

  cout << "upper: " << upper_prefix << " lower: " << lower_prefix << endl;

  for(size_t i = 0; i < vg->num_nodes; ++i){
    vector<unsigned int> *hood = vg->neighborhoods->at(i);
    num_edges += hood->size();
    nbrlengths[i] = hood->size();

    size_t hood_size = 0;

    vector<unsigned int> *un = new vector<unsigned int>();
    vector<unsigned int> *back_nbr = new vector<unsigned int>();
    vector<size_t> *index_v = new vector<size_t>();
    //index_v->reserve(vg->num_edges);

    //cout << "Finished allocations" << endl;

    size_t num_in_union = 0;
    for(size_t j = 0; j < hood->size(); ++j) {
      size_t nbr = hood->at(j);
      /*if(i ==3782)
        cout << "Neighbor: " << nbr << endl; */
      if(nbr < i){
        size_t k = 0;
        while(k < vg->neighborhoods->at(nbr)->size() && vg->neighborhoods->at(nbr)->at(k) < nbr){
          un->push_back(vg->neighborhoods->at(nbr)->at(k));
          back_nbr->push_back(nbr);
          index_v->push_back(num_in_union++);
          k++;
        }
        hood_size++;
      }
    }
    //cout << "Finished pushing vectors" << endl;
    nodes[i] = index;
    index = partition(hood->data(),hood_size,edges,index);

    std::sort(index_v->begin(),index_v->end(),CompareIndexVector(un));

    unsigned int *union_dat = new unsigned int[un->size()];
    unordered_map<unsigned int, size_t> *bm = new unordered_map<unsigned int,size_t>();
    bm->reserve(un->size());

    size_t u_index = 0;
    unsigned int prev = 0xffffffff;
      
    node_back_index_pointer[i] = back_index_index;
    /*if(i ==3782){
      cout << "Back index: " << back_index_index << endl;
    } */

    //cout << "Finished sorting" << endl;
    for(size_t j =0; j<index_v->size();j++){
      size_t cur_index = index_v->at(j);

      if(prev != un->at(cur_index)){
        union_dat[u_index++] = un->at(cur_index);
        union_info_flat[back_index_index] = un->at(cur_index);
        /*
        if( i == 148881){
          cout <<  "Union: " << un->at(cur_index) << " " << back_dat_index << endl;
        }*/
        //bm->insert(make_pair(un->at(cur_index),back_index_index));
        back_index_dat[back_index_index++] = back_dat_index;
        
      }
      back_edges[back_dat_index++] = back_nbr->at(cur_index);
      prev = un->at(cur_index);
    }
    //cout << "Finished loop" << endl;

    back_index_map[i] = bm;
    
    union_size[i] = union_index;
    union_index = partition(union_dat,u_index,unions,union_index);

    //cout << "Finished partitioning union" << endl;

    un->clear();
    delete un;
    back_nbr->clear();
    delete back_nbr;
    index_v->clear();
    delete index_v;
    delete[] union_dat;
  }
  node_back_index_pointer[vg->num_nodes] = back_index_index;

  cout << "Union Index: " << union_index << endl;
  back_index_dat[back_index_index] = back_dat_index;
  nodes[vg->num_nodes] = index;
  union_size[vg->num_nodes] = union_index;
  
  cout << "COMPRESSED EDGE SIZE[BYTES]: " << index*2 << endl;
  cout << "Back Edges: " << back_dat_index << " Back Index: " << back_index_index << " Union Index: " << union_index << endl;
  //cout << "num sets: " << numSets << " numSetsCompressed: " << numSetsCompressed << endl;

  return new CompressedGraph(upper_prefix,lower_prefix,num_nodes,num_edges,index,nbrlengths,nodes,edges,external_ids,unions,union_size,back_index_map,back_index_dat,back_edges,node_back_index_pointer,union_info_flat);
}

CSRGraph* createCSRGraph(VectorGraph *vg){
  size_t *nodes = new size_t[vg->num_nodes+1];
  const size_t num_nodes = vg->num_nodes;
  const size_t num_edges = vg->num_edges;
  unsigned int *edges = new unsigned int[num_edges]; 
  const unordered_map<unsigned int,unsigned int> *external_ids = vg->external_ids;

  size_t index = 0;
  for(size_t i = 0; i < vg->num_nodes; ++i) {
    nodes[i] = index;
    vector<unsigned int> *hood = vg->neighborhoods->at(i);
    for(size_t j = 0; j < hood->size(); ++j) {
      size_t nbr = hood->at(j);
      if(nbr < i){
        edges[index++] = hood->at(j); 
      }
    }
    //delete hood;
  }
  nodes[num_nodes] = index;

  cout << "CSR EDGE SIZE[BYTES]: " << index*4 << endl;
  return new CSRGraph(num_nodes,num_edges,nodes,edges,external_ids);
}
void createGraphLabFile(VectorGraph *vg){
  for(size_t i = 0; i < vg->num_nodes; ++i) {
    vector<unsigned int> *hood = vg->neighborhoods->at(i);
    for(size_t j = 0; j < hood->size(); ++j) {
      size_t nbr = hood->at(j);
      if(nbr < i){
        cout << i << "\t" << nbr << endl;
      }
    }
    //delete hood;
  }
  //cout << "CSR EDGE SIZE[BYTES]: " << index*4 << endl;
  //return new CSRGraph(num_nodes,num_edges,nodes,edges,external_ids);
}

#endif
