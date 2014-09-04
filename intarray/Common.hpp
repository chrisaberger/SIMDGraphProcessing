#ifndef COMMON_H
#define COMMON_H

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

using namespace std;

namespace common{
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

  enum type {
    BITSET,
    ARRAY16,
    ARRAY32,
    HYBRID
  };
};
#endif
