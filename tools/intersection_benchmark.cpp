#include "SparseMatrix.hpp"
#include "common.hpp"

#define BUF_SIZE 1024L * 1024L * 32L

template<class T> void intersect(uint64_t n, uint32_t* a, uint32_t* b) {

  uint8_t* set_a_buffer = new uint8_t[BUF_SIZE];
  uint8_t* set_b_buffer = new uint8_t[BUF_SIZE];
  uint8_t* set_c_buffer = new uint8_t[BUF_SIZE];
  Set<T> set_a = Set<T>::from_array(set_a_buffer, a, n);
  Set<T> set_b = Set<T>::from_array(set_b_buffer, b, n);
  Set<T> set_c(set_c_buffer);

  std::cout << "Start intersect" << std::endl;
  auto start_time = common::startClock();
  ops::set_intersect(&set_c, &set_a, &set_b);
  common::stopClock("intersect", start_time);
  std::cout << "End intersect. |C| = " << set_c.cardinality << std::endl;
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
  if(argc != 4) {
    std::cout << "Expected 3 arguments" << std::endl;
    return -1;
  }

  srand(time(NULL));

  uint64_t n = c_str_to_uint64_t(argv[1]);
  uint64_t run_len = c_str_to_uint64_t(argv[2]);
  uint64_t gap_len = c_str_to_uint64_t(argv[3]);
  std::cout << "Elems: " << n << ", Run len: " << run_len << ", Gap len: " << gap_len << std::endl;

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

    if(i % run_len == 0) {
      a_v += gap_len + rand() % max_gap_offset - half_max_gap_offset;
      b_v += gap_len + rand() % max_gap_offset - half_max_gap_offset;
    }
    else {
      a_v++;
      b_v++;
    }
  }

  intersect<uinteger>(n, a, b);
  intersect<pshort>(n, a, b);
  intersect<bitset>(n, a, b);
  return 0;
}
