// class templates
#include "structures/Matrix.cpp"
#include "structures/Common.hpp"

using namespace std;

int main (int argc, char* argv[]) { 
	size_t mysize = 18;
  unsigned int *data = new unsigned int[mysize];
  for(size_t i=0; i < mysize; i++){
  	cout << "i: " << i << " Data: " << i*65536 << endl;
    data[i] = i*65536;
  }
  unsigned int *result = new unsigned int[mysize*100];
  size_t length = deltacompa32::encode_array(data,mysize,result);

  cout << endl << "Length: " << length << endl;

  deltacompa32::decode_array(result,length,mysize);
}
