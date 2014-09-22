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
#include <vector>
#include <iterator>
#include <algorithm>  // for std::find
#include <iostream>   // for std::cout
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */
#include <fstream>
#include <math.h>

#define DELTA 1
#define HYBRID_LAYOUT 1
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

  enum type: unsigned char {
    BITSET = 0,
    ARRAY16 = 1,
    ARRAY32 = 2,
    HYBRID = 3,
    A32BITPACKED = 4
  };
  
}
#endif
