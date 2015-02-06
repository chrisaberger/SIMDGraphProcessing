#define WRITE_VECTOR 0

#include "SparseMatrix.hpp"
#include "common.hpp"

#define DEBUG
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

vector<uint32_t> gen_set(uint64_t len, double density) {
  set<uint32_t> result;

  const uint64_t max_offset = (uint64_t)(max(1.0, len * 0.1));
  const uint32_t min_v = rand() % max_offset;
  const uint32_t max_v = min_v + len / density;

  uint32_t v = min_v;
  while(result.size() < len - 1) {
    while(result.find(v) != result.end()) {
      v = rand() % (max_v - min_v - 1) + min_v + 1;
    }
    result.insert(v);
  }

  result.insert(max_v);

  return vector<uint32_t>(result.begin(), result.end());
}

void vector_to_file(string file, vector<uint32_t>& v) {
  ofstream out_file;
  out_file.open(file);

  for(auto n : v)
    out_file << n << endl;

  out_file.close();
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
  if(argc != 5) {
    std::cout << "Expected 4 arguments: len a, len b, density a, density b" << std::endl;
    return -1;
  }
  srand(time(NULL));

  const uint64_t len_a = atoi(argv[1]);
  const uint64_t len_b = atoi(argv[2]);
  double density_a = std::stod(argv[3]);
  double density_b = std::stod(argv[4]);

  vector<uint32_t> vec_a = gen_set(len_a, density_a);
  vector<uint32_t> vec_b = gen_set(len_b, density_b);
  uint32_t* a = vec_a.data();
  uint32_t* b = vec_b.data();

#ifdef DEBUG
  vector_to_file("input_a", vec_a);
  vector_to_file("input_b", vec_b);
#endif

#endif

  uint32_t *in1 = (len_a <= len_b) ? a : b;
  uint32_t *in2 = (len_a <= len_b) ? b : a;
  const size_t len1 = min(len_a, len_b);
  const size_t len2 = max(len_a, len_b);

  double den1 = (double)len1 / (in1[len1-1]-a[0]);
  double den2 = (double)len2 / (in2[len2-1]-b[0]);

  cout << "Len A: " << len1 << endl;
  cout << "Len B: " << len2 << endl;
  cout << "Density A: " << den1 << endl;
  cout << "Density B: " << den2 << endl;

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
  intersect<pshort,pshort,uinteger,uinteger,uinteger>(len1, len2, in1, in2, "uinteger_pshort",STANDARD);

  cout << endl << "UINTEGER_BITSET" << endl;
  intersect<uinteger,uinteger,bitset,bitset,uinteger>(len1, len2, in1, in2, "uinteger_bitset",STANDARD);

  cout << endl << "PSHORT_BITSET" << endl;
  intersect<pshort,pshort,bitset,bitset,pshort>(len1, len2, in1, in2, "pshort_bitset",STANDARD);

  /*
  cout << endl << "HYBRID" << endl;
  intersect<hybrid,hybrid,hybrid,hybrid,hybrid>(len1, len2, in1, in2, "hybrid",STANDARD);
  */


  return 0;
}
