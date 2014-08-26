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

static inline VectorGraph* ReadFile (const string path,const int num_files) {

  vector< vector<size_t>* >* *graph_in = new vector< vector<size_t>* >*[num_files];

  size_t num_edges = 0;
  size_t num_nodes = 0;
  //Place graph into vector of vectors then decide how you want to
  //store the graph.
  
  //startClock();
  #pragma omp parallel for default(none) shared(graph_in,path) reduction(+:num_edges) reduction(+:num_nodes)
  for(size_t i=0; i <= (size_t) num_files;++i){
    vector< vector<size_t>*  > *file_adj = new vector< vector<size_t>* >();

    string file_path = path;
    if(num_files!=0) file_path.append(to_string(i));

    ifstream myfile (file_path);
    string line;
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
  vector< vector<size_t>*  > *neighborhoods = new vector< vector<size_t>* >();
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
  unordered_map<size_t,size_t> *external_ids = new unordered_map<size_t,size_t>();
  external_ids->reserve(neighborhoods->size());
  //#pragma omp parallel for default(none) shared(neighborhoods,external_ids)
  for(size_t i = 0; i < neighborhoods->size(); ++i) {
    vector<size_t> *hood = neighborhoods->at(i);
    external_ids->insert(make_pair(hood->at(0),i));
    hood->erase(hood->begin());
  }
 // stopClock("Building hashmap");

  //startClock();
  #pragma omp parallel for default(none) shared(neighborhoods,external_ids) schedule(static,150)
  for(size_t i = 0; i < neighborhoods->size(); ++i) {
    vector<size_t> *hood = neighborhoods->at(i);
    //cout << "Node: " << i << endl;
    size_t index = 0;
    for(size_t j = 0; j < hood->size(); ++j) {
      if(external_ids->at(hood->at(j)) < i){
        hood->at(index) = external_ids->at(hood->at(j));
        index++;
      }
    }
    hood->resize(index);
    sort(hood->begin(),hood->end());
  } 
  //stopClock("Reassigning ids");

  return new VectorGraph(num_nodes,num_edges,external_ids,neighborhoods);
}


static inline CompressedGraph* createCompressedGraph (VectorGraph *vg) {
  prepare_shuffling_dictionary16();
  size_t sq = vg->num_nodes * vg->num_nodes;
  cout << "SQ: " << sq << endl;
  unsigned short *unions = new unsigned short[vg->num_nodes*500];//(unsigned short *) malloc(751346540*sizeof(unsigned short));	

  size_t *nodes = new size_t[vg->num_nodes+1];
  unsigned int *nbrlengths = new unsigned int[vg->num_nodes];
  unsigned short *edges = new unsigned short[vg->num_edges*5];

  size_t *union_size = new size_t[vg->num_nodes+1]; 
  size_t num_nodes = vg->num_nodes;
  const unordered_map<size_t,size_t> *external_ids = vg->external_ids;

  size_t bitsused = ceil(log2(num_nodes));

  size_t lower_prefix = 16;
  size_t upper_prefix = 32-lower_prefix;

  cout << "bitsused: " << bitsused << endl;
  cout << "upper: " << upper_prefix << " lower: " << lower_prefix << endl;

  //cout  << "Num nodes: " << vg->num_nodes << " Num edges: " << vg->num_edges << endl;
  size_t num_edges = 0;
  size_t index = 0;
  size_t union_index = 0;
  for(size_t i = 0; i < vg->num_nodes; ++i){
    vector<size_t> *hood = vg->neighborhoods->at(i);
    num_edges += hood->size();
    nbrlengths[i] = hood->size();
    unsigned int *tmp_hood = new unsigned int[hood->size()];
    size_t hood_size = 0;
    vector<size_t> *un = new vector<size_t>();
    //un->reserve(vg->num_nodes);
    bool clearable = false;
    for(size_t j = 0; j < hood->size(); ++j) {
      size_t nbr = hood->at(j);

      if(nbr < i){
    	std::copy(vg->neighborhoods->at(nbr)->begin(), vg->neighborhoods->at(nbr)->end(), std::back_inserter(*un));   
	/*	 
        if(j == 0){
          un = vg->neighborhoods->at(j);
        }
        else{
          vector<size_t> *un_tmp = new vector<size_t>(); 
          set_union(vg->neighborhoods->at(nbr)->begin(), vg->neighborhoods->at(nbr)->end(), un->begin(), un->end(), back_inserter(*un_tmp));
          if(clearable){
            un->clear();
            delete un;
          }
          un = un_tmp;
          clearable = true;
        }
	*/
        tmp_hood[j] = hood->at(j);
        hood_size++;
      }
    }
    nodes[i] = index;
    index = partition(tmp_hood,hood_size,edges,index,upper_prefix,lower_prefix);

    std::sort(un->begin(),un->end());
    un->erase(unique(un->begin(),un->end()),un->end());
    
    //FIXME    
    unsigned int *tmp_un = new unsigned int[un->size()];//(unsigned int *) malloc((un->size())*sizeof(unsigned int));	 
    for(size_t j =0; j<un->size(); j++){
      tmp_un[j] = (unsigned int) un->at(j);
    } 
    union_size[i] = union_index;
    //cout << union_index << " " << un->size() << " " << sq << endl;
    union_index = partition(tmp_un,un->size(),unions,union_index,upper_prefix,lower_prefix);
    //cout << "done" << endl;
    if(clearable){
      un->clear();
      delete un;
    }
    delete[] tmp_un;
    delete[] tmp_hood;
  }
 
  cout << "Union Index: " << union_index << endl;
  nodes[vg->num_nodes] = index;
  union_size[vg->num_nodes] = union_index;
  
  cout << "COMPRESSED EDGE SIZE[BYTES]: " << index*2 << endl;

  //cout << "num sets: " << numSets << " numSetsCompressed: " << numSetsCompressed << endl;

  return new CompressedGraph(upper_prefix,lower_prefix,num_nodes,num_edges,index,nbrlengths,nodes,edges,external_ids,unions,union_size);
}

CSRGraph* createCSRGraph(VectorGraph *vg){
  size_t *nodes = new size_t[vg->num_nodes+1];
  const size_t num_nodes = vg->num_nodes;
  const size_t num_edges = vg->num_edges;
  unsigned int *edges = new unsigned int[num_edges]; 
  const unordered_map<size_t,size_t> *external_ids = vg->external_ids;

  size_t index = 0;
  for(size_t i = 0; i < vg->num_nodes; ++i) {
    nodes[i] = index;
    vector<size_t> *hood = vg->neighborhoods->at(i);
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
    vector<size_t> *hood = vg->neighborhoods->at(i);
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
