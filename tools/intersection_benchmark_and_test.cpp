#include "SparseMatrix.hpp"
#include "common.hpp"

//#define DEBUG
//#define FROM_FILE

#define BUF_SIZE 1024L * 1024L * 1024L
#define DUMMY_SIZE 1024L * 1024L * 64L

uint32_t* dummy1;
uint32_t* dummy2;

enum intersection_type: uint8_t {
  STANDARD = 0,
  GALLOP = 1,
  IBM = 2,
  V1 = 3,
  V3 = 4
};

template<class T, class R> Set<R> decode_array(size_t n, uint8_t* set_data, uint32_t *buffer) {
  Set<T> set = Set<T>::from_flattened(set_data, n);
  if(set.type == common::VARIANT || set.type == common::BITPACKED){
    return set.decode(buffer);
  }

  return Set<R>(set);
}
template<class T, class R, class U, class P, class F> void intersect(size_t len_a, size_t len_b, 
  uint32_t* a, uint32_t* b, string filename, intersection_type id) {

  uint32_t* buffer1 = new uint32_t[len_a];
  uint32_t* buffer2 = new uint32_t[len_b];

  uint8_t* set_a_buffer = new uint8_t[BUF_SIZE];
  uint8_t* set_b_buffer = new uint8_t[BUF_SIZE];
  uint8_t* set_c_buffer = new uint8_t[BUF_SIZE];

  Set<F> set_c(set_c_buffer);

  std::cout << "Start encoding" << std::endl;
  
  auto start_time_encoding = common::startClock();
  size_t as = Set<T>::flatten_from_array(set_a_buffer, a, len_a);
  size_t bs = Set<U>::flatten_from_array(set_b_buffer, b, len_b);

  common::stopClock("encoding", start_time_encoding);

  // erase caches
  for(size_t i = 0; i < DUMMY_SIZE; i++) {
    dummy1[i] = dummy1[i] + dummy2[i] + 1;
  }

  std::cout << "Size: " << (as+bs) << std::endl;
  auto start_time_intersect = common::startClock();

  Set<R> set_a_dec = decode_array<T, R>(len_a, set_a_buffer, buffer1);
  Set<P> set_b_dec = decode_array<U, P>(len_b, set_b_buffer, buffer2);

  switch(id){
    case V1:
      ops::set_intersect_v1((Set<uinteger>*)&set_c,(Set<uinteger>*)&set_a_dec,(Set<uinteger>*)&set_b_dec);
      break;
    case V3:
      ops::set_intersect_v3((Set<uinteger>*)&set_c,(Set<uinteger>*)&set_a_dec,(Set<uinteger>*)&set_b_dec);
      break;
    case GALLOP:
      ops::set_intersect_galloping((Set<uinteger>*)&set_c,(Set<uinteger>*)&set_a_dec,(Set<uinteger>*)&set_b_dec);
      break;
    case IBM:
      ops::set_intersect_ibm((Set<uinteger>*)&set_c,(Set<uinteger>*)&set_a_dec,(Set<uinteger>*)&set_b_dec);
      break;
    default:
      ops::set_intersect(&set_c, &set_a_dec,&set_b_dec);
      break;
  }

  common::stopClock("intersect", start_time_intersect);
  std::cout << "End intersect. |C| = " << set_c.cardinality << std::endl;

#ifdef DEBUG
  ofstream myfile;
  myfile.open(filename.append("_output"));
  size_t index = 0;
  set_c.foreach([&myfile,&index](uint32_t i){
    myfile << "Index: " << index++ << " Data: " << i << endl;
  });
#else
  (void) filename;
#endif
}

uint64_t c_str_to_uint64_t(char* str) {
  uint64_t result;
  std::istringstream ss(str);
  if(!(ss >> result)) {
    std::cout << "failed to convert string to uint64_t" << std::endl;
  }
  return result;
}

int main(int argc, char* argv[]) {
  ops::prepare_shuffling_dictionary16();

  dummy1 = new uint32_t[DUMMY_SIZE];
  dummy2 = new uint32_t[DUMMY_SIZE];

#ifdef FROM_FILE
  uint32_t* a;
  uint32_t* b;
  
  uint32_t len_a;
  uint32_t len_b;

  ifstream myfileA("input_a");
  myfileA >> len_a;
  a = new uint32_t[len_a];
  size_t i = 0;
  //cout << "len_a: " << len_a << endl;
  while(myfileA >> a[i++]){
    //cout << "a[" << i-1 << "]: " << a[i-1] << endl;
  }
  myfileA.close();

  ifstream myfileB("input_b");
  myfileB >> len_b;
  b = new uint32_t[len_b];
  i = 0;
  //cout << "len_b: " << len_b << endl;
  while(myfileB >> b[i++]){
    //cout << "b[" << i-1 << "]: " << b[i-1] << endl;
  }
  myfileB.close();

#else
  if(argc != 4) {
    std::cout << "Expected 3 arguments: Size Range, density a, density b" << std::endl;
    return -1;
  }
  srand(time(NULL));

  const uint64_t n = c_str_to_uint64_t(argv[1]);
  double densityA = std::stod(argv[2]);
  double densityB = std::stod(argv[3]);
  const uint64_t max_offset = 100;//n*0.1;

  uint32_t a_v = rand() % max_offset;
  uint32_t b_v = rand() % max_offset;

  const uint32_t a_e = n+a_v;
  const uint32_t b_e = n+b_v;

  const uint32_t len_a = (densityA*n);
  const uint32_t len_b = (densityB*n);

  uint32_t* a = new uint32_t[n];
  uint32_t* b = new uint32_t[n];

#ifdef DEBUG
  ofstream myfileA;
  myfileA.open("input_a");
  myfileA << len_a << endl;
  ofstream myfileB;
  myfileB.open("input_b");
  myfileB << len_b << endl;
#endif

  for(uint64_t i = 0; i < len_a; i++) {
#ifdef DEBUG
    myfileA << a_v << endl;
#endif
    a[i] = a_v;
    a_v += (rand() % ((a_e-a_v)/(len_a-i)))+1;
  }
  for(uint64_t i = 0; i < len_b; i++) {
#ifdef DEBUG
    myfileB << b_v << endl;
#endif
    b[i] = b_v;
    b_v += (rand() % (b_e-b_v)/(len_b-i))+1;
  }
#endif

  double denA = (double)len_a / (a[len_a-1]-a[0]);
  double denB = (double)len_b / (b[len_b-1]-b[0]);

  uint32_t *in1 = a;
  uint32_t *in2 = b;
  const size_t len1 = len_a;
  const size_t len2 = len_b;

  cout << "Density A: " << min(denA,denB) << endl;
  cout << "Density B: " << max(denA,denB) << endl;

  cout << endl << "UINTEGER_UINTEGER_IBM" << endl;
  intersect<uinteger,uinteger,uinteger,uinteger,uinteger>(len1, len2, in1, in2, "uinteger_uinteger_ibm",IBM);

  cout << endl << "UINTEGER_UINTEGER_STANDARD" << endl;
  intersect<uinteger,uinteger,uinteger,uinteger,uinteger>(len1, len2, in1, in2, "uinteger_uinteger_standard",STANDARD);

  cout << endl << "UINTEGER_UINTEGER_V3" << endl;
  intersect<uinteger,uinteger,uinteger,uinteger,uinteger>(len1, len2, in1, in2, "uinteger_uinteger_v3",V3);

  cout << endl << "UINTEGER_UINTEGER_V1" << endl;
  intersect<uinteger,uinteger,uinteger,uinteger,uinteger>(len1, len2, in1, in2, "uinteger_uinteger_v1",V1);

  cout << endl << "UINTEGER_UINTEGER_GALLOP" << endl;
  intersect<uinteger,uinteger,uinteger,uinteger,uinteger>(len1, len2, in1, in2, "uinteger_uinteger_gallop",GALLOP);

  cout << endl << "PSHORT_PSHORT" << endl;
  intersect<pshort,pshort,pshort,pshort,pshort>(len1, len2, in1, in2, "pshort_pshort",STANDARD);

  cout << endl << "BITSET_BITSET" << endl;
  intersect<bitset,bitset,bitset,bitset,bitset>(len1, len2, in1, in2, "bitset_bitset",STANDARD);
  
  cout << endl << "UINTEGER_PSHORT" << endl;
  intersect<uinteger,uinteger,pshort,pshort,uinteger>(len1, len2, in1, in2, "uinteger_pshort",STANDARD);

  cout << endl << "UINTEGER_BITSET" << endl;
  intersect<uinteger,uinteger,bitset,bitset,uinteger>(len1, len2, in1, in2, "uinteger_bitset",STANDARD);

  cout << endl << "PSHORT_BITSET" << endl;
  intersect<pshort,pshort,bitset,bitset,pshort>(len1, len2, in1, in2, "pshort_bitset",STANDARD);

  cout << endl << "HYBRID" << endl;
  intersect<hybrid,hybrid,hybrid,hybrid,hybrid>(len1, len2, in1, in2, "hybrid",STANDARD);


  return 0;
}
