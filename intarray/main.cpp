// class templates
#include "structures/Matrix.cpp"
#include "structures/Common.hpp"

using namespace std;

int main (int argc, char* argv[]) { 
  unsigned int *data = new unsigned int[10];
  for(size_t i=0; i < 10; i++){
    data[i] = -2+i*2;
  }
  unsigned short *result = new unsigned short[30];
  size_t index = Array16::preprocess(result,0,data,10);
  Array16::print_data(result,index);
}
