// class templates
#include "Common.hpp"

using namespace std;

int main (int argc, char* argv[]) {  
  omp_set_num_threads(atoi(argv[2]));        

  unsigned short *test = new unsigned short[4096];
  for(size_t i = 0; i < 4096; ++i){
    test[i] = i*2;
  }

  createBitSet(test,4096);

  for(size_t i = 4096; i < 65536; ++i){
    addToBitSet(i*2,test);
  }

  //addToBitSet(1,test);
  //addToBitSet(3,test);
  long count = andCardinalityInRange(test,test,10);
  cout << "Count: " << count << endl;

  for(size_t i = 0; i < 16; ++i){
    cout << "Index: " << i << " Data: " << test[i] << endl;
  }

  return 0;
}
