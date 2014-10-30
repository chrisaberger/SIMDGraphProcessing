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
#define COMPRESSION 1
#define HYBRID_LAYOUT 1
#define WRITE_VECTOR 0
#define SHORTS_PER_REG 8
#define INTS_PER_REG 4
#define BYTES_PER_REG 16
#define ALLOCATOR 40

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

  static void _mm256_print_ps(__m256 x) {
    float *data = new float[8];
    _mm256_storeu_ps(&data[0],x);
    for(size_t i =0 ; i < 8; i++){
      cout << "Data[" << i << "]: " << data[i] << endl;
    }
    delete[] data;
  }
  static void _mm256i_print(__m256i x) {
    int *data = new int[8];
    _mm256_storeu_si256((__m256i*)&data[0],x);
    for(size_t i =0 ; i < 8; i++){
      cout << "Data[" << i << "]: " << data[i] << endl;
    }
    delete[] data;
  }
  //Thanks stack overflow.
  static inline float _mm256_reduce_add_ps(__m256 x) {
    /* ( x3+x7, x2+x6, x1+x5, x0+x4 ) */
    const __m128 x128 = _mm_add_ps(_mm256_extractf128_ps(x, 1), _mm256_castps256_ps128(x));
    /* ( -, -, x1+x3+x5+x7, x0+x2+x4+x6 ) */
    const __m128 x64 = _mm_add_ps(x128, _mm_movehl_ps(x128, x128));
    /* ( -, -, -, x0+x1+x2+x3+x4+x5+x6+x7 ) */
    const __m128 x32 = _mm_add_ss(x64, _mm_shuffle_ps(x64, x64, 0x55));
    /* Conversion to float is a no-op on x86-64 */
    return _mm_cvtss_f32(x32);
  }

  enum type: uint8_t {
    BITSET = 0,
    ARRAY16 = 1,
    ARRAY32 = 2,
    A32BITPACKED = 3,
    VARIANT = 4,
    HYBRID_PERF = 5,
    HYBRID_COMP = 6,
    DENSE_RUNS = 7,
    EMPTY = 8
  };

  static void* allocate_local(size_t num, size_t size, int node) {
     size_t total_size = num * size;
     return numa_alloc_onnode(total_size, node);
     //return calloc(num, size);
  }

  static void free_local(void* start, size_t num, size_t size) {
     size_t total_size = num * size;
     numa_free(start, total_size);
     //free(start);
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
