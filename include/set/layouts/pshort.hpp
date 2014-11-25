/*

THIS CLASS IMPLEMENTS THE FUNCTIONS ASSOCIATED WITH A PREFIX SHORT SET LAYOUT.

*/

#include "common.hpp"
#include "../ops/intersection.hpp"

class pshort{
  public:
    static common::type get_type();
    static size_t build(uint8_t *r_in, const uint32_t *data, const size_t length);
    static size_t build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length);
    static tuple<uint8_t,size_t,size_t> get_flattened_data(const uint8_t *set_data, const size_t cardinality);
    static void foreach(const std::function <void (uint32_t)>& f,const uint8_t *data_in, const size_t cardinality, const size_t number_of_bytes);
    static tuple<size_t,size_t,common::type> intersect(uint8_t *C_in, uint8_t *A_in, uint8_t *B_in, size_t A_cardinality, size_t B_cardinality, size_t A_num_bytes, size_t B_num_bytes);
};

inline common::type pshort::get_type(){
  return common::ARRAY16;
}
//Copies data from input array of ints to our set data r_in
inline size_t pshort::build(uint8_t *r_in, const uint32_t *A, const size_t s_a){
  uint16_t *R = (uint16_t*) r_in;

  uint16_t high = 0;
  size_t partition_length = 0;
  size_t partition_size_position = 1;
  size_t counter = 0;
  for(size_t p = 0; p < s_a; p++) {
    uint16_t chigh = (A[p] & 0xFFFF0000) >> 16; // upper dword
    uint16_t clow = A[p] & 0x0FFFF;   // lower dword
    if(chigh == high && p != 0) { // add element to the current partition
      partition_length++;
      R[counter++] = clow;
    }else{ // start new partition
      R[counter++] = chigh; // partition prefix
      R[counter++] = 0;     // reserve place for partition size
      R[counter++] = clow;  // write the first element
      R[partition_size_position] = partition_length;

      partition_length = 1; // reset counters
      partition_size_position = counter - 2;
      high = chigh;
    }
  }
  R[partition_size_position] = partition_length;
  return counter*2;
}
//Nothing is different about build flattened here. The number of bytes
//can be infered from the type. This gives us back a true CSR representation.
inline size_t pshort::build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length){
  if(length > 0){
    size_t *size_ptr = (size_t*) r_in;
    size_t num_bytes = build(r_in+sizeof(size_t),data,length);
    size_ptr[0] = num_bytes;
    return num_bytes+sizeof(size_t);
  } else{
    return 0;
  }
}

inline tuple<uint8_t,size_t,size_t> pshort::get_flattened_data(const uint8_t *set_data, const size_t cardinality){
  if(cardinality > 0){
    const size_t *size_ptr = (size_t*) set_data;
    return make_tuple(common::ARRAY16,size_ptr[0],sizeof(size_t));
  } else{
    return make_tuple(common::ARRAY16,0,0);
  }
}

//Iterates over set applying a lambda.
inline void pshort::foreach(const std::function <void (uint32_t)>& f, const uint8_t *A_in, 
  const size_t cardinality, const size_t number_of_bytes){
  (void) number_of_bytes; 

  uint16_t *A = (uint16_t*) A_in;

  size_t count = 0;
  size_t i = 0;
  while(count < cardinality){
    uint32_t prefix = (A[i] << 16);
    unsigned short size = A[i+1];
    i += 2;

    size_t inner_end = i+size;
    while(i < inner_end){
      uint32_t tmp = prefix | A[i];
      f(tmp);
      ++count;
      ++i;
    }
  }
}

inline tuple<size_t,size_t,common::type> pshort::intersect(uint8_t *C_in, uint8_t *A_in, uint8_t *B_in, size_t card_a, size_t card_b, size_t s_bytes_a, size_t s_bytes_b) {
  (void) card_a; (void) card_b;
  return ops::intersect_pshort_pshort(C_in,(uint16_t*)A_in,(uint16_t*)B_in,s_bytes_a/sizeof(uint16_t),s_bytes_b/sizeof(uint16_t));
}
