/*

THIS CLASS IMPLEMENTS THE FUNCTIONS ASSOCIATED WITH A PREFIX SHORT SET LAYOUT.

*/

#include "common.hpp"

class variant{
  public:
    static common::type get_type();
    static size_t build(uint8_t *r_in, const uint32_t *data, const size_t length);
    static size_t build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length);
    static tuple<size_t,size_t,common::type> get_flattened_data(const uint8_t *set_data, const size_t cardinality);

    template<typename F>
    static void foreach(
        F f,
        const uint8_t *data_in,
        const size_t cardinality,
        const size_t number_of_bytes,
        const common::type t);

    template<typename F>
    static void foreach_until(
        F f,
        const uint8_t *data_in,
        const size_t cardinality,
        const size_t number_of_bytes,
        const common::type t);

    static uint32_t produce_deltas(const uint32_t *data_in, const size_t length, uint32_t *data, uint32_t prev);
    static size_t encode(const uint32_t *data, size_t data_i, const size_t length, uint8_t *result, size_t result_i);
    static uint32_t decode(const uint8_t *data, size_t &data_i);
};

inline common::type variant::get_type(){
  return common::VARIANT;
}

inline uint32_t variant::produce_deltas(const uint32_t *data_in, const size_t length, uint32_t *data, uint32_t prev){    
  size_t max = 0;

  for(size_t i = 0; i < length; i++){
    uint32_t cur = data_in[i] - prev;
    //cout  << "Writing delta index: " << i << "  " << cur << endl;
    data[i] = cur;
    prev = data_in[i];
  }
  return max;
}
inline size_t variant::encode(const uint32_t *data, size_t data_i, const size_t length, uint8_t *result, size_t result_i){
  //cout << "Starting variant encode at: " << result_i << " " << data_i << " " << length << endl;
  while(data_i < length){
    uint32_t cur = data[data_i++];
    //cout << "data_i: " << data_i-1 << " cur: " << cur << endl;
    uint32_t bytes_needed = ceil((log2(cur)+1)/7); //1 bit is reserved for continue
    if(bytes_needed == 0) bytes_needed = 1;
    //cout << "Bytes needed: " << bytes_needed << " " << (uint32_t)log2(cur) << " " << cur << " " << data_i-1 << endl;
    size_t bytes_set = 0;
    while(bytes_set < bytes_needed){
      uint8_t continue_bit = 0x00;
      if(bytes_set+1 < bytes_needed){
        continue_bit = 0x01;
      }
      result[result_i++] = ((uint8_t)(cur << 1)) | continue_bit;
      cur = cur >> 7;
      //cout << "result[" << result_i-1 << "]: "  << (uint)result[result_i-1] << endl;
      bytes_set++; 
    }
    //cout << endl;
  }
  return result_i;
}
inline uint32_t variant::decode(const uint8_t *data, size_t &data_i){
  uint8_t byte_in = data[data_i++];
  uint32_t cur = (byte_in >> 1);
  //cout << "byte: " << (uint)byte_in << " cur: " << cur << endl;

  size_t i = 1;
  while(byte_in % 2){
    byte_in = data[data_i++]; 
    //cout << "byte[" << data_i-1 <<"]: " << (uint)byte_in << endl;

    cur |= (uint32_t)((byte_in >> 1) << 7*i++);
  }
  //cout << endl;
  return cur;
}
//Copies data from input array of ints to our set data r_in
inline size_t variant::build(uint8_t *r_in, const uint32_t *A, const size_t s_a){
  if(s_a > 0){
    size_t data_i = 0;
    size_t result_i = 0;

    uint32_t *data = new uint32_t[s_a];
    variant::produce_deltas(A,s_a,data,0);
    result_i = variant::encode(data,data_i,s_a,r_in,result_i);

    delete[] data;

    return result_i;
  } else{
    return 0;
  }
}
//Nothing is different about build flattened here. The number of bytes
//can be infered from the type. This gives us back a true CSR representation.
inline size_t variant::build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length){
  if(length > 0){
    common::num_v++;
    uint32_t *size_ptr = (uint32_t*) r_in;
    size_t num_bytes = build(r_in+sizeof(uint32_t),data,length);
    size_ptr[0] = (uint32_t)num_bytes;
    return num_bytes+sizeof(uint32_t);
  } else{
    return 0;
  }
}

inline tuple<size_t,size_t,common::type> variant::get_flattened_data(const uint8_t *set_data, const size_t cardinality){
  if(cardinality > 0){
    const uint32_t *size_ptr = (uint32_t*) set_data;
    return make_tuple(sizeof(uint32_t),(size_t)size_ptr[0],common::VARIANT);
  } else{
    return make_tuple(0,0,common::VARIANT);
  }
}

//Iterates over set applying a lambda.
template<typename F>
inline void variant::foreach_until(
    F f,
    const uint8_t *A_in,
    const size_t cardinality,
    const size_t number_of_bytes,
    const common::type type) {
  (void) number_of_bytes; (void) type;

  if(cardinality != 0){
    size_t data_i = 0;
    size_t num_decoded = 0;

    uint32_t prev = variant::decode(A_in,data_i);
    if(f(prev))
      return;
    num_decoded++;

    while(num_decoded < cardinality){
      uint32_t cur = variant::decode(A_in,data_i);
      cur += prev;
      prev = cur;

      if(f(prev))
        f(prev);
      num_decoded++;
    }
  }
}

//Iterates over set applying a lambda.
template<typename F>
inline void variant::foreach(
    F f,
    const uint8_t *A_in,
    const size_t cardinality,
    const size_t number_of_bytes,
    const common::type type) {
  (void) number_of_bytes; (void) type;

  if(cardinality != 0){
    size_t data_i = 0;
    size_t num_decoded = 0;

    uint32_t prev = variant::decode(A_in,data_i);
    f(prev);
    num_decoded++;

    while(num_decoded < cardinality){
      uint32_t cur = variant::decode(A_in,data_i);
      cur += prev;
      prev = cur;

      f(prev);
      num_decoded++;
    }
  }
}
