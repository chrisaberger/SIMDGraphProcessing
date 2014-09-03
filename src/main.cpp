// class templates
#include "Common.hpp"

using namespace std;

int main (int argc, char* argv[]) { 
  prepare_shuffling_dictionary16();
  unsigned int *a = new unsigned int[100];
  unsigned int *b = new unsigned int[200];

  for(size_t i = 0; i < 100; i++){
    a[i] = i*4;
  }
  for(size_t i = 0; i < 200; i++){
    b[i] = i;
  }

  unsigned short *a_r = new unsigned short[500];
  unsigned short *b_r = new unsigned short[500];
  size_t a_i = partition(a,100,a_r,0);
  size_t b_i = partition(b,100,b_r,0);

  unsigned short *c_r = new unsigned short[500];
  size_t c_i = intersect_partitioned(c_r,a_r,b_r,a_i,b_i);
  print_partition(c_r,c_i);
  return 0;
}
