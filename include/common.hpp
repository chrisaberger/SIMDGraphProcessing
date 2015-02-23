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
#include <thread>
#include <atomic>
#include <tuple>
#include <cstdarg>
#include <set>

static size_t ADDRESS_BITS_PER_BLOCK = 8;
static size_t BLOCK_SIZE = 256;
static double BITSET_THRESHOLD = 1.0 / 32.0;

// Experts only! Proceed wih caution!
//#define ENABLE_PCM
//#define ENABLE_PRINT_THREAD_TIMES
//#define ENABLE_ATOMIC_UNION

//TODO: Replace with new command line arguments.

#define ALLOCATOR 2
#define REALLOC_THRESHOLD 0.7

//Needed for parallelization, prevents false sharing of cache lines
#define PADDING 300
#define MAX_THREADS 512 

//#define ATTRIBUTES
#define WRITE_TABLE 0

#define COMPRESSION 0
#define PERFORMANCE 1
#define VECTORIZE 1

// Enables/disables pruning
//#define PRUNING
//#define NEW_BITSET

// Enables/disables hybrid that always chooses U-Int
//#define HYBRID_UINT

#define STATS

//CONSTANTS THAT SHOULD NOT CHANGE
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
using namespace std::placeholders;

namespace common{
  static size_t bitset_length = 0;
  static size_t pshort_requirement = 16;
  static double bitset_req = (1.0/256.0);

  static size_t tid = 0;
  static uint8_t **scratch_space = new uint8_t*[MAX_THREADS];
  static uint8_t **scratch_space1 = new uint8_t*[MAX_THREADS];

  static void alloc_scratch_space(size_t alloc_size, size_t num_threads){
    for(size_t i = 0; i < num_threads; i++){
      cout << "ALLLOC: " << alloc_size << endl;
      scratch_space[i] = new uint8_t[alloc_size];
      scratch_space1[i] = new uint8_t[alloc_size];

    }
  }

  static double startClock (){
    return omp_get_wtime();
  }

  static double stopClock(double t_in){
    double t2=omp_get_wtime();
    return t2 - t_in;
  }
  static double stopClock(string in,double t_in){
    double t2=omp_get_wtime();
    std::cout << "Time["+in+"]: " << t2-t_in << " s" << std::endl;
    return t2 - t_in;
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
  static void _mm128i_print_shorts(__m128i x) {
    for(size_t i =0 ; i < 8; i++){
      cout << "Data[" << i << "]: " << _mm_extract_epi16(x,i) << endl;
    }
  }
  static void _mm128i_print(__m128i x) {
    for(size_t i =0 ; i < 4; i++){
      cout << "Data[" << i << "]: " << _mm_extract_epi32(x,i) << endl;
    }
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

  static size_t num_bs = 0;
  static size_t num_pshort = 0;
  static size_t num_uint = 0;
  static size_t num_bp = 0;
  static size_t num_v = 0;
  static double bits_per_edge = 0;
  static double bits_per_edge_nometa = 0;

  static size_t num_uint_uint = 0;
  static size_t num_pshort_pshort = 0;
  static size_t num_bs_bs = 0;
  static size_t num_uint_pshort = 0;
  static size_t num_uint_bs = 0;
  static size_t num_pshort_bs = 0;

  enum type: uint8_t {
    BITSET = 0,
    PSHORT = 1,
    UINTEGER = 2,
    BITPACKED = 3,
    VARIANT = 4,
    HYBRID = 5,
    KUNLE = 6,
    BITSET_NEW = 7,
    NEW_TYPE = 8
  };

  static void dump_stats(){
    cout << endl;
    cout << "Num Bitset: " << num_bs << endl;
    cout << "Num PShort: " << num_pshort
     << endl;
    cout << "Num Uint: " << num_uint << endl;
    cout << "Num BP: " << num_bp << endl;
    cout << "Num V: " << num_v << endl;
    cout << "Bits per edge (meta): " << bits_per_edge << endl;
    cout << "Bits per edge (no meta): " << bits_per_edge_nometa << endl;

    cout << "Num UINT/UINT: " << num_uint_uint << endl;
    cout << "Num UINT/PSHORT: " << num_uint_pshort << endl;
    cout << "Num UINT/BS: " << num_uint_bs << endl;
    cout << "Num PSHORT/PSHORT: " << num_pshort_pshort << endl;
    cout << "Num PSHORT/BS: " << num_pshort_bs << endl;
    cout << "Num BS/BS: " << num_bs_bs << endl;
  }

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

  static thread* threads = NULL;
  static void init_threads(const size_t num_threads) {
    threads = new thread[num_threads];
  }

  // Iterates over a range of numbers in parallel
  template<typename F>
  static size_t par_for_range(const size_t num_threads, const size_t from, const size_t to, const size_t block_size, F body) {
     const size_t range_len = to - from;
     const size_t real_num_threads = min(range_len / block_size + 1, num_threads);
     //std::cout << "Range length: " << range_len << " Threads: " << real_num_threads << std::endl;

     if(real_num_threads == 1) {
        for(size_t i = from; i < to; i++) {
           body(0, i);
        }
     }
     else {
        double t_begin = omp_get_wtime();
        double* thread_times = NULL;

#ifdef ENABLE_PRINT_THREAD_TIMES
        thread_times = new double[real_num_threads];
#endif
        thread* threads = new thread[real_num_threads];
        const size_t range_len = to - from;
        std::atomic<size_t> next_work;
        next_work = 0;

        for(size_t k = 0; k < real_num_threads; k++) {
           threads[k] = thread([&block_size](double t_begin, double* thread_times, int k, std::atomic<size_t>* next_work, size_t offset, size_t range_len, std::function<void(size_t, size_t)> body) -> void {
              size_t local_block_size = block_size;

              while(true) {
                 size_t work_start = next_work->fetch_add(local_block_size, std::memory_order_relaxed);
                 if(work_start > range_len)
                    break;

                 size_t work_end = min(work_start + local_block_size, range_len);
                 local_block_size = block_size;//100 + (work_start / range_len) * block_size;
                 for(size_t j = work_start; j < work_end; j++) {
                     body(k, offset + j);
                 }
              }


#ifdef ENABLE_PRINT_THREAD_TIMES
              double t_end = omp_get_wtime();
              thread_times[k] = t_end - t_begin;
#else
              (void) t_begin;
              (void) thread_times;
#endif
           }, t_begin, thread_times, k, &next_work, from, range_len, body);
        }

        for(size_t k = 0; k < real_num_threads; k++) {
           threads[k].join();
        }

#ifdef ENABLE_PRINT_THREAD_TIMES
        for(size_t k = 0; k < real_num_threads; k++){
            std::cout << "Execution time of thread " << k << ": " << thread_times[k] << std::endl;
        }
        delete[] thread_times;
#endif
     }

     return real_num_threads;
  }
  static size_t par_for_range(const size_t num_threads, const size_t from, const size_t to, const size_t block_size,
    std::function<void(size_t)> setup,
    std::function<void(size_t, size_t)> body,
    std::function<void(size_t)> tear_down) {

    #ifdef ENABLE_PRINT_THREAD_TIMES
    double setup1 = common::startClock();
    #endif

    for(size_t i = 0; i < num_threads; i++){
      setup(i);
    }

    #ifdef ENABLE_PRINT_THREAD_TIMES
    common::stopClock("PARALLEL SETUP",setup1);
    #endif

    size_t real_num_threads = par_for_range(num_threads,from,to,block_size,body);

    #ifdef ENABLE_PRINT_THREAD_TIMES
    double td = common::startClock();
    #endif

    for(size_t i = 0; i < num_threads; i++){
      tear_down(i);
    }

    #ifdef ENABLE_PRINT_THREAD_TIMES
    common::stopClock("PARALLEL TEAR DOWN",td);
    #endif

    return real_num_threads;
  }

  static vector<uint32_t> range(uint32_t max) {
    vector<uint32_t> result;
    result.reserve(max);
    for(uint32_t i = 0; i < max; i++)
      result.push_back(i);
    return result;
  }
}
#endif
