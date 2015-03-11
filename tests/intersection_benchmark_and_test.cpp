#define WRITE_VECTOR 1

#include "SparseMatrix.hpp"
#include "common.hpp"

#define DEBUG
#define FROM_FILE

#define BUF_SIZE 1024L * 1024L * 1024L
#define DUMMY_SIZE 1024L * 1024L * 64L

uint32_t* dummy1;
uint32_t* dummy2;

enum intersection_type: uint8_t {
  STANDARD = 0,
  GALLOP = 1,
  IBM = 2,
  V1 = 3,
  V3 = 4,
  DEFAULT = 5
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
    case STANDARD:
      ops::set_intersect_standard((Set<uinteger>*)&set_c,(Set<uinteger>*)&set_a_dec,(Set<uinteger>*)&set_b_dec);
      break;
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
      ops::set_intersect(&set_c, &set_a_dec, &set_b_dec);
      break;
  }

  common::stopClock("intersect", start_time_intersect);
  std::cout << "End intersect. |C| = " << set_c.cardinality << std::endl;

/*
#ifdef DEBUG
  ofstream myfile1;
  myfile1.open(filename.append("_a"));
  size_t index1 = 0;
  set_a_dec.foreach([&myfile1,&index1](uint32_t i){
    myfile1 << "Index: " << index1++ << " Data: " << i << endl;
  });
  ofstream myfile2;
  myfile2.open(filename.append("_b"));
  size_t index2 = 0;
  set_b_dec.foreach([&myfile2,&index2](uint32_t i){
    myfile2 << "Index: " << index2++ << " Data: " << i << endl;
  });
#endif
*/
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

vector<uint32_t> gen_set(uint64_t len, uint64_t range, size_t run_len) {
  set<uint32_t> result;

  const uint64_t max_offset = (uint64_t)(max(1.0, len * 0.1));
  const uint32_t min_v = rand() % max_offset;
  const uint32_t max_v = min_v + range;

  for(size_t i = 0; i < run_len; i++) {
    result.insert(min_v + i);
    result.insert(max_v - i);
  }

  uint32_t v;
  while(result.size() < len) {
    bool all_free = false;
    while(!all_free) {
      // Pick a random value in the range
      v = rand() % (max_v - min_v - 1) + min_v + 1;

      // Check if we can place a run here
      all_free = true;
      for(size_t i = 0; i < run_len; i++) {
        if(result.find(v) != result.end()) {
          all_free = false;
          break;
        }
      }
    }


    for(size_t i = 0; i < run_len; i++) {
      result.insert(v + i);
    }
  }

  std::cout << "LEN: " << result.size() << std::endl;

  return vector<uint32_t>(result.begin(), result.end());
}

void vector_to_file(string file, vector<uint32_t>& v) {
  ofstream out_file;
  out_file.open(file);

  out_file << v.size() << endl;

  for(auto n : v)
    out_file << n << endl;

  out_file.close();
}

int main(int argc, char* argv[]) {
  ops::prepare_shuffling_dictionary16();
  common::alloc_scratch_space(BUF_SIZE,1);

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
  if(argc != 7) {
    std::cout << "Expected 6 arguments: <Size Range A> <Size Range B> <Len A> <Len B> <Skew A> <Skew B>" << std::endl;
    return -1;
  }
  srand(time(NULL));

  const uint64_t range_a = c_str_to_uint64_t(argv[1]);
  const uint64_t range_b = c_str_to_uint64_t(argv[2]);

  const uint64_t len_a = c_str_to_uint64_t(argv[3]);
  const uint64_t len_b = c_str_to_uint64_t(argv[4]);

  const double skew_a = stod(argv[5]);
  const double skew_b = stod(argv[6]);

  std::cout << range_a << " " << range_b << " " << len_a << " " << len_b << " " << skew_a << " " << skew_b << endl;

  assert(range_a > len_a);
  assert(range_b > len_b);

  size_t run_len_a = (len_a * skew_a)+1;
  size_t run_len_b = (len_b * skew_b)+1;

  vector<uint32_t> vec_a = gen_set(len_a, range_a, run_len_a);
  vector<uint32_t> vec_b = gen_set(len_b, range_b, run_len_b);
  uint32_t* a = vec_a.data();
  uint32_t* b = vec_b.data();

#ifdef DEBUG
  vector_to_file("input_a", vec_a);
  vector_to_file("input_b", vec_b);
#endif
  cout << "Run len A: " << run_len_a << endl;
  cout << "Run len B: " << run_len_b << endl;

#endif

  double denA = (double)len_a/(a[len_a-1]-a[0]);
  double denB = (double)len_b/(b[len_b-1]-b[0]);

  uint32_t *in1 = a;
  uint32_t *in2 = b;
  const size_t len1 = len_a;
  const size_t len2 = len_b;

  cout << "A start: " << a[0] << " A end: " << a[len_a-1] << endl;
  cout << "B start: " << b[0] << " B end: " << b[len_b-1] << endl;
  cout << "Density A: " << denA << endl;
  cout << "Density B: " << denB << endl;
  /*
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
  intersect<pshort,pshort,pshort,pshort,pshort>(len1, len2, in1, in2, "pshort_pshort",DEFAULT);
  */

  cout << endl << "BITSET_BITSET" << endl;
  intersect<bitset,bitset,bitset,bitset,bitset>(len1, len2, in1, in2, "bitset_bitset",DEFAULT);

  cout << endl << "UINTEGER_PSHORT" << endl;
  intersect<uinteger,uinteger,pshort,pshort,uinteger>(len1, len2, in1, in2, "uinteger_pshort",DEFAULT);

  /*
  cout << endl << "UINTEGER_BITSET" << endl;
  intersect<uinteger,uinteger,bitset,bitset,uinteger>(len1, len2, in1, in2, "uinteger_bitset",DEFAULT);

  cout << endl << "PSHORT_BITSET" << endl;
  intersect<pshort,pshort,bitset,bitset,pshort>(len1, len2, in1, in2, "pshort_bitset",DEFAULT);

  int block_size[] = {256, 512, 1024, 4096};
  int address_bits[] = {8, 9, 10, 12};
  for(size_t i = 0; i < 4; i++){
    ADDRESS_BITS_PER_BLOCK = address_bits[i];
    BLOCK_SIZE = block_size[i];

    cout << endl << "BSNEW_BSNEW" << endl;
    intersect<bitset_new,bitset_new,bitset_new,bitset_new,bitset_new>(len1, len2, in1, in2, "bsnew_bsnew",DEFAULT);

    cout << endl << "UINT_BSNEW: " << BLOCK_SIZE << endl;
    intersect<uinteger,uinteger,bitset_new,bitset_new,uinteger>(len1, len2, in1, in2, "uint_bsnew",DEFAULT);

    cout << endl << "NT_NT: " << BLOCK_SIZE << endl;
    intersect<new_type,new_type,new_type,new_type,new_type>(len1, len2, in1, in2, "nt_nt",DEFAULT);
  }

  cout << endl << "HYBRID" << endl;
  intersect<hybrid,hybrid,hybrid,hybrid,hybrid>(len1, len2, in1, in2, "hybrid",DEFAULT);
  */
  return 0;
}
