/*

THIS CLASS IMPLEMENTS THE FUNCTIONS ASSOCIATED WITH THE BITSET LAYOUT.

*/

#include "common.hpp"

#define BITS_PER_WORD 64
#define ADDRESS_BITS_PER_WORD 6
#define BYTES_PER_WORD 8

class bitset{
  public:
    static size_t word_index(const uint32_t bit_index);
    static bool is_set(const uint32_t index, const uint64_t *in_array, const uint32_t start_index);
    static void set(const uint32_t index, uint64_t *in_array, const uint32_t start_index);

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
      const uint8_t* A,
      const size_t cardinality,
      const size_t number_of_bytes,
      const common::type t);
};
//compute word of data
inline size_t bitset::word_index(const uint32_t bit_index){
  return bit_index >> ADDRESS_BITS_PER_WORD;
}
//check if a bit is set
inline bool bitset::is_set(const uint32_t index, const uint64_t * const in_array, const uint32_t start_index){
  return (in_array)[word_index(index)-start_index] & ((uint64_t) 1 << (index%BITS_PER_WORD));
}
//check if a bit is set
inline void bitset::set(const uint32_t index, uint64_t * const in_array, const uint32_t start_index){
  *(in_array + ((index >> ADDRESS_BITS_PER_WORD)-start_index)) |= ((uint64_t)1 << (index & 0x3F));
}
inline common::type bitset::get_type(){
  return common::BITSET;
}
//Copies data from input array of ints to our set data r_in
inline size_t bitset::build(uint8_t *R, const uint32_t *A, const size_t s_a){
  uint32_t* offset = (uint32_t*) R; 
  uint64_t* R64 = (uint64_t*)(R+sizeof(uint32_t));
  size_t word = 0;
  size_t i = 0;
  size_t cleared_word_index = 0;

  offset[0] = s_a > 0 ? word_index(A[0]):0;
  size_t off_bytes = 0;
  while(i < s_a){
    off_bytes = sizeof(uint32_t);
    uint32_t cur = A[i];
    //std::cout << cur << std::endl;
    word = word_index(cur);
    while(cleared_word_index < word){
      R64[cleared_word_index++] = 0;
      cleared_word_index++;
    }
    cleared_word_index = word+1;

    uint64_t set_value = (uint64_t) 1 << (cur % BITS_PER_WORD);
    bool same_word = true;
    ++i;
    while(i<s_a && same_word){
      if(word_index(A[i])==word){
        cur = A[i];
        set_value |= ((uint64_t) 1 << (cur%BITS_PER_WORD));
        ++i;
      } else same_word = false;
    }
    R64[word-offset[0]] = set_value;
    word++;
  }
  return ((word-offset[0]) * BYTES_PER_WORD) + off_bytes;
}
//Nothing is different about build flattened here. The number of bytes
//can be infered from the type. This gives us back a true CSR representation.
inline size_t bitset::build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length){
  if(length > 0){
    common::num_bs++;
    uint32_t *size_ptr = (uint32_t*) r_in;
    size_t num_bytes = build(r_in+sizeof(uint32_t),data,length);
    size_ptr[0] = (uint32_t)num_bytes;
    return num_bytes+sizeof(uint32_t);
  } else{
    return 0;
  }
}

inline tuple<size_t,size_t,common::type> bitset::get_flattened_data(const uint8_t *set_data, const size_t cardinality){
  if(cardinality > 0){
    const uint32_t *size_ptr = (uint32_t*) set_data;
    return make_tuple(sizeof(uint32_t),(size_t)size_ptr[0],common::BITSET);
  } else{
    return make_tuple(0,0,common::BITSET);
  }
}

//Iterates over set applying a lambda.
inline void bitset::foreach_until(const std::function <bool (uint32_t)>& f,
  const uint8_t *A,
  const size_t cardinality,
  const size_t number_of_bytes,
  const common::type type){
  (void) cardinality; (void) type;

  if(number_of_bytes > 0){
    const size_t num_data_words = ((number_of_bytes-sizeof(uint32_t))/sizeof(uint64_t));
    const uint32_t *offset_pointer = (uint32_t*) A;
    const uint32_t offset = offset_pointer[0];
    const uint64_t* A64 = (uint64_t*)(A+sizeof(uint32_t));
    for(size_t i = 0; i < num_data_words; i++){
      const uint64_t cur_word = A64[i];
      for(size_t j = 0; j < BITS_PER_WORD; j++){
        if((cur_word >> j) % 2){
          if(f(BITS_PER_WORD*(i+offset) + j))
            break;
        }
      }
    }
  }
}

//Iterates over set applying a lambda.
inline void bitset::foreach(const std::function <void (uint32_t)>& f,
  const uint8_t * const A,
  const size_t cardinality,
  const size_t number_of_bytes,
  const common::type type){
  (void) cardinality; (void) type;

  if(number_of_bytes > 0){
    const size_t num_data_words = ((number_of_bytes-sizeof(uint32_t))/sizeof(uint64_t));
    const uint32_t *offset_pointer = (uint32_t*) A;
    const uint32_t offset = offset_pointer[0];
    const uint64_t* A64 = (uint64_t*)(A+sizeof(uint32_t));

    for(size_t i = 0; i < num_data_words; i++){
      const uint64_t cur_word = A64[i];
      if(cur_word != 0) {
        for(size_t j = 0; j < BITS_PER_WORD; j++){
          if((cur_word >> j) % 2) {
            f(BITS_PER_WORD *(i+offset) + j);
          }
        }
      }
    }
  }
}

// Iterates over set applying a lambda in parallel.
inline void bitset::par_foreach(
      const size_t num_threads,
      const std::function <void (size_t, uint32_t)>& f,
      const uint8_t* A,
      const size_t cardinality,
      const size_t number_of_bytes,
      const common::type t) {
   (void) number_of_bytes; (void) t; (void) cardinality;

  if(number_of_bytes > 0){
    const size_t num_data_words = ((number_of_bytes-sizeof(uint32_t))/sizeof(uint64_t));
    const uint32_t *offset_pointer = (uint32_t*) A;
    const uint32_t offset = offset_pointer[0];
    const uint64_t* A64 = (uint64_t*)(A+sizeof(uint32_t));
    common::par_for_range(num_threads, 0, num_data_words, 512,
           [&f, &A64, cardinality,offset](size_t tid, size_t i) {
              const uint64_t cur_word = A64[i];
              if(cur_word != 0) {
                for(size_t j = 0; j < BITS_PER_WORD; j++){
                  const uint32_t curr_nb = BITS_PER_WORD*(i+offset) + j;
                  if((cur_word >> j) % 2) {
                    f(tid, curr_nb);
                  }
                }
              }
           });
  }
}
