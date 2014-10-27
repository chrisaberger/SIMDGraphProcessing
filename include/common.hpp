#ifndef COMMON_H
#define COMMON_H

//#include "numa_alloc.hpp"
#include <x86intrin.h>
#include <omp.h>
#include <unordered_map>
#include <ctime>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>  
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>  // for std::find
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */
#include <math.h>
#include <unistd.h>

#define VECTORIZE 1
#define HYBRID_LAYOUT 0
#define WRITE_VECTOR 0
#define SHORTS_PER_REG 8
#define INTS_PER_REG 4
#define BYTES_PER_REG 16

// As seen in the dimmwitted project...

// If we are not compiling on OSX, we use the NUMA library
#ifndef __MACH__
#include <numa.h>
#include <numaif.h>
#endif

// If we are compiling on OSX, we define stubs for functions in the NUMA library
#ifdef __MACH__
#include <math.h>
#include <stdlib.h>
#define numa_alloc_onnode(X,Y) malloc(X)
#define numa_max_node() 0
#define numa_run_on_node(X) 0
#define numa_set_localalloc() 0
#define numa_available() -1
#endif

using namespace std;

namespace common{
  static double t1;
  static double t2;
  static struct timeval tim;  

  static void startClock (){
    gettimeofday(&tim, NULL);  
    t1=tim.tv_sec+(tim.tv_usec/1000000.0); 
  }

  static double stopClock(string in){
    gettimeofday(&tim, NULL);  
    t2=tim.tv_sec+(tim.tv_usec/1000000.0); 
    std::cout << "Time["+in+"]: " << t2-t1 << " s" << std::endl;
    return t2 - t1;
  }

  static void allocateStack(){
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

  enum type: uint8_t {
    BITSET = 0,
    ARRAY16 = 1,
    ARRAY32 = 2,
    HYBRID = 3,
    A32BITPACKED = 4,
    VARIANT = 5
  };

  static void* allocate_local(size_t num, size_t size, int node) {
     size_t total_size = num * size;

#ifdef NUMA_ALLOC
     return numa_alloc_onnode(total_size, node);
#else
     return calloc(num, size);
#endif
  }

  static void free_local(void* start, size_t num, size_t size) {
     size_t total_size = num * size;

#ifdef NUMA_ALLOC
     numa_free(start, total_size);
#else
     free(start);
#endif
  }

  static int find_memory_node_for_addr(void* ptr) {
    // See: http://stackoverflow.com/questions/7986903/can-i-get-the-numa-node-from-a-pointer-address-in-c-on-linux
    int numa_node = -1;
    if(get_mempolicy(&numa_node, NULL, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR) < 0)
       cout << "WARNING: get_mempolicy failed" << endl;
    return numa_node;
  }
}
#endif
