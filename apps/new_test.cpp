// class templates
#include "SparseMatrix.hpp"
#include "MutableGraph.hpp"

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) { 

  uint32_t *a_data = new uint32_t[10];
  for(size_t i = 0; i < 10; i++){
    a_data[i] = i*4;
  }

  uint8_t *s_data = new uint8_t[100];

  Set<uinteger> me = Set<uinteger>::from_array(s_data,a_data,10);
  me.foreach([](uint32_t i){
    cout << "Data: " << i << endl;
  });
  return 0;
}
