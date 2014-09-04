#ifndef VECTORGRAPH_H
#define VECTORGRAPH_H

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
#include <sstream>
#include <omp.h>

using namespace std;

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
  CompareIndexVector(const unsigned int *data)
  : m_data(data)
  {}

  bool operator()(size_t Lhs, size_t Rhs)const
  {
    return m_data[Lhs] < m_data[Rhs];
  }

  const unsigned int* m_data;
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

#endif
