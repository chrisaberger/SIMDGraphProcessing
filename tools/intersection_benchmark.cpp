#include "SparseMatrix.hpp"
#include "common.hpp"

#define BUF_SIZE 1024L * 1024L * 256L

template<class T, class R> Set<R> decode_array(size_t n, uint8_t* set_data, uint32_t *buffer) {
  Set<T> set = Set<T>::from_flattened(set_data, n);
  if(set.type == common::VARIANT || set.type == common::BITPACKED){
    return set.decode(buffer);
  }

  return Set<R>(set);
}

template<class T, class R> void intersect(uint64_t n, uint32_t* a, uint32_t* b, string filename) {
  uint32_t* buffer1 = new uint32_t[n];
  uint32_t* buffer2 = new uint32_t[n];

  uint8_t* set_a_buffer = new uint8_t[BUF_SIZE];
  uint8_t* set_b_buffer = new uint8_t[BUF_SIZE];
  uint8_t* set_c_buffer = new uint8_t[BUF_SIZE];

  Set<R> set_c(set_c_buffer);

  std::cout << "Start encoding" << std::endl;
  auto start_time_encoding = common::startClock();
  size_t set_a_size = Set<T>::flatten_from_array(set_a_buffer, a, n);
  //size_t set_b_size =
   Set<T>::flatten_from_array(set_b_buffer, b, n);
  common::stopClock("encoding", start_time_encoding);
  std::cout << "Size: " << set_a_size << std::endl;

  std::cout << "Start intersect" << std::endl;
  auto start_time_intersect = common::startClock();

  Set<R> set_a_dec = decode_array<T, R>(n, set_a_buffer, buffer1);
  Set<R> set_b_dec = decode_array<T, R>(n, set_b_buffer, buffer2);

  ops::set_intersect(&set_c, &set_a_dec, &set_b_dec);
  common::stopClock("intersect", start_time_intersect);
  std::cout << "End intersect. |C| = " << set_c.cardinality << std::endl;

  ofstream myfile;
  cout << filename << endl;
  myfile.open(filename);
  size_t index = 0;
  set_c.foreach([&myfile,&index](uint32_t i){
    myfile << "Index: " << index++ << " Data: " << i << endl;
  });

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
  /*
  if(argc != 4) {
    std::cout << "Expected 3 arguments" << std::endl;
    return -1;
  }*/
  ops::prepare_shuffling_dictionary16();

  srand(time(NULL));

  uint64_t n = c_str_to_uint64_t(argv[1]);
  double density = 0.00128; //std::stod(argv[2]);
  uint64_t gap_len = 4;// c_str_to_uint64_t(argv[3]);
  uint64_t run_len = 1;//(uint64_t)(density * gap_len / (1.0 - density));
  //std::cout << "Elems: " << n << ", Run len: " << run_len << ", Gap len: " << gap_len << std::endl;

  if(run_len == 0) {
    std::cout << "Run len has to be at least 1" << std::endl;
    return -1;
  }

  const uint64_t max_offset = 6;
  const uint64_t max_gap_offset = min(gap_len - 1, max_offset);
  const uint64_t half_max_gap_offset = max_gap_offset / 2;

  uint32_t* a = new uint32_t[n];
  uint32_t* b = new uint32_t[n];

  uint32_t a_v = rand() % max_offset;
  uint32_t b_v = rand() % max_offset;

  for(uint64_t i = 0; i < n; i++) {
    a[i] = a_v;
    b[i] = b_v;

    //cout << "Index i: " << a_v  << " " << b_v << endl;

    if(i % run_len == 0) {
      a_v += gap_len + rand() % max_gap_offset - half_max_gap_offset;
      b_v += gap_len + rand() % max_gap_offset - half_max_gap_offset;
    }
    else {
      a_v++;
      b_v++;
    }
  }

  cout << endl << "UINTEGER" << endl;
  intersect<uinteger, uinteger>(n, a, b,"uinteger");

  cout << endl << "PSHORT" << endl;
  intersect<pshort, pshort>(n, a, b,"pshort");

  cout << endl << "BITSET" << endl;
  intersect<bitset, bitset>(n, a, b,"bitset");

  cout << endl << "VARIANT" << endl;
  intersect<variant, uinteger>(n, a, b,"variant");

  cout << endl << "BITPACKED" << endl;
  intersect<bitpacked, uinteger>(n, a, b,"bitpacked");
  return 0;
}
