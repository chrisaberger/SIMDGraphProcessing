#ifndef COMMON_H
#define COMMON_H

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
#endif

#define VECTORIZE 1
#define HYBRID_LAYOUT 0
#define WRITE_VECTOR 0
#define SHORTS_PER_REG 8
#define INTS_PER_REG 4

using namespace std;

namespace common{
  static double t1;
  static double t2;
  static struct timeval tim;  

  static void startClock (){
    gettimeofday(&tim, NULL);  
    t1=tim.tv_sec+(tim.tv_usec/1000000.0); 
  }

  static void stopClock(string in){
    gettimeofday(&tim, NULL);  
    t2=tim.tv_sec+(tim.tv_usec/1000000.0); 
    std::cout << "Time["+in+"]: " << t2-t1 << " s" << std::endl;
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
    A32BITPACKED_DELTA = 5,
    VARIANT = 6,
    VARIANT_DELTA = 7
  };
  
}
#endif
