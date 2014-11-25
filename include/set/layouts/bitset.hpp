/*

THIS CLASS IMPLEMENTS THE FUNCTIONS ASSOCIATED WITH A PREFIX SHORT SET LAYOUT.

*/

#include "common.hpp"
#include "../ops/intersection.hpp"

#define BITS_PER_WORD 8
#define ADDRESS_BITS_PER_WORD 3

class bitset{
  public:
    static size_t word_index(const uint32_t bit_index);
    static int get_bit(const uint32_t value, const uint32_t position);
    static bool is_set(const uint32_t index, const uint8_t *in_array);

    static common::type get_type();
    static size_t build(uint8_t *r_in, const uint32_t *data, const size_t length);
    static size_t build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length);
    static tuple<size_t,size_t,common::type> get_flattened_data(const uint8_t *set_data, const size_t cardinality);
   
    static void foreach(const std::function <void (uint32_t)>& f,
      const uint8_t *data_in, 
      const size_t cardinality, 
      const size_t number_of_bytes,
      const common::type type);
    
    static tuple<size_t,size_t,common::type> intersect(uint8_t *C_in, 
      const uint8_t *A_in, const uint8_t *B_in, 
      const size_t A_cardinality, const size_t B_cardinality, 
      const size_t A_num_bytes, const size_t B_num_bytes, 
      const common::type a_t, const common::type b_t);
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
inline common::type bitset::get_type(){
  return common::BITSET;
}
//Copies data from input array of ints to our set data r_in
inline size_t bitset::build(uint8_t *R, const uint32_t *A, const size_t s_a){
  uint32_t max = A[s_a-1];
  size_t num_words = word_index(max);
  if(s_a != 0){
    num_words++;
  }

  size_t i = 0;
  while(i<s_a){
    uint32_t cur = A[i];
    size_t word = word_index(cur);
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
  }
  return num_words;
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

inline tuple<size_t,size_t,common::type> bitset::intersect(uint8_t *C_in, 
  const uint8_t *A_in, const uint8_t *B_in, 
  const size_t card_a, const size_t card_b, 
  const size_t s_bytes_a, const size_t s_bytes_b, 
  const common::type a_t, const common::type b_t) {
  (void) card_a; (void) card_b; (void) a_t; (void) b_t;
  
  return ops::intersect_bs_bs(C_in,A_in,B_in,s_bytes_a,s_bytes_b);
}
