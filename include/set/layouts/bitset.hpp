/*

THIS CLASS IMPLEMENTS THE FUNCTIONS ASSOCIATED WITH A PREFIX SHORT SET LAYOUT.

*/

#include "common.hpp"

#define BITS_PER_WORD 8
#define ADDRESS_BITS_PER_WORD 3

class bitset{
  public:
    static size_t word_index(const uint32_t bit_index);
    static int get_bit(const uint32_t value, const uint32_t position);
    static bool is_set(const uint32_t index, const uint8_t *in_array);
    static void set(const uint32_t index, uint8_t *in_array);

    static common::type get_type();
    static size_t build(uint8_t *r_in, const uint32_t *data, const size_t length);
    static size_t build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length);
    static tuple<size_t,size_t,common::type> get_flattened_data(const uint8_t *set_data, const size_t cardinality);
   
    static void notforeach(const std::function <void (uint32_t)>& f, const uint8_t *A, const size_t number_of_bytes, const size_t mat_size);
    static void foreach(const std::function <void (uint32_t)>& f,
      const uint8_t *data_in, 
      const size_t cardinality, 
      const size_t number_of_bytes,
      const common::type type);
};
//compute word of data
inline size_t bitset::word_index(const uint32_t bit_index){
  return bit_index >> ADDRESS_BITS_PER_WORD;
}
//compute index bit
inline int bitset::get_bit(const uint32_t value, const uint32_t position) {
  return ( ( value & (1 << position) ) >> position);
}
//check if a bit is set
inline bool bitset::is_set(uint32_t index, const uint8_t *in_array){
  return (in_array[word_index(index)] & (1 << (index%BITS_PER_WORD)));
}
//check if a bit is set
inline void bitset::set(uint32_t index, uint8_t *in_array){
  in_array[word_index(index)] |= (1 << (index%BITS_PER_WORD));
}
inline common::type bitset::get_type(){
  return common::BITSET;
}
//Copies data from input array of ints to our set data r_in
inline size_t bitset::build(uint8_t *R, const uint32_t *A, const size_t s_a){
  size_t word = 0;
  size_t i = 0;
  size_t cleared_word_index = 0;
  while(i<s_a){
    uint32_t cur = A[i];
    word = word_index(cur);
    while(cleared_word_index < word){
      R[cleared_word_index++] = 0;
    }
    cleared_word_index = word+1;

    uint8_t set_value = 1 << (cur % BITS_PER_WORD);
    bool same_word = true;
    ++i;
    while(i<s_a && same_word){
      if(word_index(A[i])==word){
        cur = A[i];
        set_value |= (1 << (cur%BITS_PER_WORD));
        ++i;
      } else same_word = false;
    }
    R[word] = set_value; 
    word++;
  }
  return word;
}
//Nothing is different about build flattened here. The number of bytes
//can be infered from the type. This gives us back a true CSR representation.
inline size_t bitset::build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length){
  if(length > 0){
    size_t *size_ptr = (size_t*) r_in;
    size_t num_bytes = build(r_in+sizeof(size_t),data,length);
    size_ptr[0] = num_bytes;
    return num_bytes+sizeof(size_t);
  } else{
    return 0;
  }
}

inline tuple<size_t,size_t,common::type> bitset::get_flattened_data(const uint8_t *set_data, const size_t cardinality){
  if(cardinality > 0){
    const size_t *size_ptr = (size_t*) set_data;
    return make_tuple(sizeof(size_t),size_ptr[0],common::BITSET);
  } else{
    return make_tuple(0,0,common::BITSET);
  }
}

//Iterates over set applying a lambda.
inline void bitset::foreach(const std::function <void (uint32_t)>& f, 
  const uint8_t *A, 
  const size_t cardinality, 
  const size_t number_of_bytes, 
  const common::type type){
  (void) cardinality; (void) type;

  for(size_t i = 0; i < number_of_bytes; i++){
    uint8_t cur_word = A[i];
    for(size_t j = 0; j < BITS_PER_WORD; j++){
      if((cur_word >> j) % 2){
        f(BITS_PER_WORD*i + j);
      }
    }
  }
}
//Iterates over set applying a lambda.
inline void bitset::notforeach(const std::function <void (uint32_t)>& f, 
  const uint8_t *A, 
  const size_t number_of_bytes,
  const size_t mat_size){

  for(size_t i = 0; i < number_of_bytes; i++){
    uint8_t cur_word = A[i];
    size_t stop = (i == (number_of_bytes-1)) ? (mat_size & 0x00FF) : BITS_PER_WORD;
    for(size_t j = 0; j < stop; j++){
      if( ((cur_word >> j) & 0x01) == 0){
        f(BITS_PER_WORD*i + j);
      }
    }
  }
}