/*

THIS CLASS IMPLEMENTS THE FUNCTIONS ASSOCIATED WITH A PREFIX SHORT SET LAYOUT.

*/

#include "common.hpp"

class pshort{
  public:
    static common::type get_type();
    static size_t build(uint8_t *r_in, const uint32_t *data, const size_t length);
    static size_t build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length);
    static tuple<size_t,size_t,common::type> get_flattened_data(const uint8_t *set_data, const size_t cardinality);

    static void foreach_until(const std::function <bool (uint32_t)>& f,
      const uint8_t *data_in,
      const size_t cardinality,
      const size_t number_of_bytes,
      const common::type type);
    static void foreach(const std::function <void (uint32_t)>& f,
      const uint8_t *data_in,
      const size_t cardinality,
      const size_t number_of_bytes,
      const common::type type);
    static void par_foreach(
      const size_t num_threads,
      const std::function <void (size_t, uint32_t)>& f,
      const uint8_t *data_in,
      const size_t cardinality,
      const size_t number_of_bytes,
      const common::type t);
};

inline common::type pshort::get_type(){
  return common::PSHORT;
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
  return counter*sizeof(uint16_t);
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

inline tuple<size_t,size_t,common::type> pshort::get_flattened_data(const uint8_t *set_data, const size_t cardinality){
  if(cardinality > 0){
    const size_t *size_ptr = (size_t*) set_data;
    return make_tuple(sizeof(size_t),size_ptr[0],common::PSHORT);
  } else{
    return make_tuple(0,0,common::PSHORT);
  }
}

//Iterates over set applying a lambda.
inline void pshort::foreach_until(const std::function <bool (uint32_t)>& f, 
  const uint8_t *A_in, 
  const size_t cardinality, 
  const size_t number_of_bytes, 
  const common::type type){
  (void) number_of_bytes; (void) type;

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
      if(f(tmp))
        goto DONE;
      ++count;
      ++i;
    }
  }
  DONE: ;
}

//Iterates over set applying a lambda.
inline void pshort::foreach(
    const std::function <void (uint32_t)>& f,
    const uint8_t *A_in,
    const size_t cardinality,
    const size_t number_of_bytes,
    const common::type type) {
  (void) number_of_bytes; (void) type;

  uint16_t *A = (uint16_t*) A_in;
  size_t count = 0;
  size_t i = 0;
  while(count < cardinality) {
    uint32_t prefix = (A[i] << 16);
    unsigned short size = A[i+1];
    i += 2;

    size_t inner_end = i+size;
    for(; i < inner_end; i++) {
      uint32_t tmp = prefix | A[i];
      f(tmp);
      ++count;
    }
  }
}

class PShortDecoder {
  private:
    uint16_t* curr_prefix_start;
    size_t curr_prefix_index_start;
    uint16_t curr_prefix_len;

  public:
    PShortDecoder() {
      this->curr_prefix_start = NULL;
      this->curr_prefix_len = 0;
      this->curr_prefix_index_start = 0;
    }

    PShortDecoder(uint16_t* data) {
      this->curr_prefix_start = data;
      this->curr_prefix_len = data[1];
      this->curr_prefix_index_start = 0;
    }

    uint32_t at(size_t i) {
      while(this->curr_prefix_index_start + this->curr_prefix_len <= i) {
        this->curr_prefix_start += this->curr_prefix_len + 2;
        this->curr_prefix_index_start += this->curr_prefix_len;
        this->curr_prefix_len = this->curr_prefix_start[1];
      }

      uint32_t prefix = (uint32_t)(this->curr_prefix_start[0]) << 16;
      return prefix | this->curr_prefix_start[i - this->curr_prefix_index_start + 2];
    }
};

// Iterates over set applying a lambda in parallel.
inline void pshort::par_foreach(
    const size_t num_threads,
    const std::function <void (size_t, uint32_t)>& f,
    const uint8_t *data_in,
    const size_t cardinality,
    const size_t number_of_bytes,
    const common::type t) {
  (void) number_of_bytes; (void) t;

  uint16_t* data = (uint16_t*) data_in;

  PShortDecoder* decoders = new PShortDecoder[num_threads];
  for(size_t i = 0; i < num_threads; i++) {
    decoders[i] = PShortDecoder(data);
  }

  common::par_for_range(num_threads, 0, cardinality, 64,
    [&f, &decoders](size_t tid, size_t i) {
      f(tid, decoders[tid].at(i));
    }
  );

  delete[] decoders;
}
