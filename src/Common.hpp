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
#include "BitSet.hpp"

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
  unordered_map<size_t,size_t> *external_ids;
  vector< vector<size_t>*  > *neighborhoods;

  VectorGraph(  size_t num_nodes_in, 
      size_t num_edges_in,
      unordered_map<size_t,size_t> *external_ids_in,
      vector< vector<size_t>*  > *neighborhoods_in): 
    num_nodes(num_nodes_in), 
    num_edges(num_edges_in),
    external_ids(external_ids_in), 
    neighborhoods(neighborhoods_in){}
  ~VectorGraph(){
    delete external_ids;
    for(size_t i = 0; i < num_nodes; ++i) {
      vector<size_t> *hood = neighborhoods->at(i);
      hood->clear();
      delete hood;
    }
    neighborhoods->erase(neighborhoods->begin(),neighborhoods->end());
    delete neighborhoods;
  }
};
//this is a functor
struct Comparator {
  bool operator()(vector<size_t> *i,vector<size_t> *j) const { 
    return (i->size() > j->size()); 
  }
};

static inline VectorGraph* ReadFile (string path,int num) {
  vector< vector<size_t>*  > *neighborhoods = new vector< vector<size_t>* >();

  neighborhoods->reserve(100000000);

  size_t num_edges = 0;
  string holder = path;
  //Place graph into vector of vectors then decide how you want to
  //store the graph.
  for(size_t i=0; i <= (size_t) num;++i){
    path = holder;
    if(num!=0) path.append(to_string(i));
    string line;
    ifstream myfile (path);
    if (myfile.is_open()){
      while ( getline (myfile,line) ){
        vector<size_t> *cur = new vector<size_t>(); //guess a size
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
        neighborhoods->push_back(cur);
      }
    }
  }
  neighborhoods->resize(neighborhoods->size());

  //Sort the neighborhoods by degree.
  std::sort(neighborhoods->begin(),neighborhoods->end(),Comparator());

  //Build hash map
  unordered_map<size_t,size_t> *external_ids = new unordered_map<size_t,size_t>();
  external_ids->reserve(neighborhoods->size());
  for(size_t i = 0; i < neighborhoods->size(); ++i) {
    vector<size_t> *hood = neighborhoods->at(i);
    external_ids->insert(make_pair(hood->at(0),i));
    hood->erase(hood->begin());
  }
  size_t num_nodes = neighborhoods->size();

  cout << "Finished reading file :)" << endl;
  for(size_t i = 0; i < neighborhoods->size(); ++i) {
    vector<size_t> *hood = neighborhoods->at(i);
    //cout << "Node: " << i << endl;
    for(size_t j = 0; j < hood->size(); ++j) {
      hood->at(j) = external_ids->find(hood->at(j))->second;
    }
    sort(hood->begin(),hood->end());
  } 

  return new VectorGraph(num_nodes,num_edges,external_ids,neighborhoods);

}

static inline CompressedGraph* createCompressedGraph (VectorGraph *vg) {
  size_t *nodes = new size_t[vg->num_nodes];
  unsigned int *nbrlengths = new unsigned int[vg->num_nodes];
  unsigned short *edges = new unsigned short[vg->num_edges*5];
  size_t num_nodes = vg->num_nodes;
  const unordered_map<size_t,size_t> *external_ids = vg->external_ids;

  //cout  << "Num nodes: " << vg->num_nodes << " Num edges: " << vg->num_edges << endl;
  size_t num_edges = 0;
  size_t index = 0;
  for(size_t i = 0; i < vg->num_nodes; ++i){
    vector<size_t> *hood = vg->neighborhoods->at(i);
    num_edges += hood->size();
    nbrlengths[i] = hood->size();
    int *tmp_hood = new int[hood->size()];
    for(size_t j = 0; j < hood->size(); ++j) {
      tmp_hood[j] = hood->at(j);
    }
    nodes[i] = index;
    index = partition(tmp_hood,hood->size(),edges,index);
    delete[] tmp_hood;
  }

  cout << "num sets: " << numSets << " numSetsCompressed: " << numSetsCompressed << endl;

  return new CompressedGraph(num_nodes,num_edges,index,nbrlengths,nodes,edges,external_ids);
}

CSRGraph* createCSRGraph(VectorGraph *vg){
  size_t *nodes = new size_t[vg->num_nodes];
  const size_t num_nodes = vg->num_nodes;
  const size_t num_edges = vg->num_edges;
  size_t *edges = new size_t[num_edges]; 
  const unordered_map<size_t,size_t> *external_ids = vg->external_ids;
 

  size_t index = 0;
  for(size_t i = 0; i < vg->num_nodes; ++i) {
    nodes[i] = index;
    vector<size_t> *hood = vg->neighborhoods->at(i);
    for(size_t j = 0; j < hood->size(); ++j) {
      edges[index++] = hood->at(j); 
    }
    //delete hood;
  }
  return new CSRGraph(num_nodes,num_edges,nodes,edges,external_ids);
}

#endif
