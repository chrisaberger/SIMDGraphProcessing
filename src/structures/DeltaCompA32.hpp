#include "Common.hpp"
#include "Array32.hpp"
#include <x86intrin.h>
#include <math.h>

using namespace std;

namespace deltacompa32 {
  inline size_t encode_array(unsigned int *data, unsigned int length, unsigned int *result){
    unsigned int bits_used = log2(data[length-1]);

    return 0;
  }
} 