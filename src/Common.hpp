#ifndef COMMON_H
#define COMMON_H

#include <ctime>
#include <sys/time.h>
#include <sys/resource.h>

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

#endif
