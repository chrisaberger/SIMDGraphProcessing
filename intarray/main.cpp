// class templates
#include "structures/Matrix.cpp"
#include "structures/Common.hpp"

using namespace std;

int main (int argc, char* argv[]) { 
  unsigned int *data = new unsigned int[65536];
  for(size_t i=0; i < 65536; i++){
    data[i] = i;
  }
  unsigned short *result = new unsigned short[65536];
  unsigned short *output = new unsigned short[65536];
  size_t index = bitset::preprocess(result,0,data,65536);
  cout << "index: " << index << endl;
  bitset::print_data(result,index);
  cout << "Count: " << bitset::intersect(output,result,result,index,index) << endl;
}
