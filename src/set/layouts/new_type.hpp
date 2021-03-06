/*

THIS CLASS IMPLEMENTS THE FUNCTIONS ASSOCIATED WITH THE BITSET LAYOUT.

*/

#include "bitset_new.hpp"
#include "uinteger.hpp"

class new_type{
  public:
    static size_t word_index(const uint32_t bit_index);
    static bool is_set(const uint32_t index, const uint64_t *in_array, const uint64_t start_index);
    static void set(const uint32_t index, uint64_t *in_array, const uint64_t start_index);

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

    template<typename F>
    static size_t par_foreach(
      F f,
      const size_t num_threads,
      const uint8_t* A,
      const size_t cardinality,
      const size_t number_of_bytes,
      const common::type t);
};
inline common::type new_type::get_type(){
  return common::NEW_TYPE;
}
//Copies data from input array of ints to our set data r_in
inline size_t new_type::build(uint8_t *R, const uint32_t *A, const size_t s_a){
  if(s_a > 0){
    size_t i = 0;
    uint32_t *uint_array = new uint32_t[s_a];
    size_t uint_i = 0;
    uint32_t *bitset_array = new uint32_t[s_a];
    size_t bs_i = 0;

    while(i < s_a){
      const size_t block_id = A[i] / BLOCK_SIZE;
      const size_t block_start_index = i;
      while(i < s_a && in_range(A[i],block_id)) {
        i++;
      }

      double density = ((i-block_start_index) < 2) ? 0.0:(double)(i-block_start_index)/BLOCK_SIZE;

      if(density > BITSET_THRESHOLD){
        for(size_t j = block_start_index; j < i; j++){
          bitset_array[bs_i++] = A[j];
        }
      } else{
        for(size_t j = block_start_index; j < i; j++){
          uint_array[uint_i++] = A[j];
        }
      }
    }
    size_t total_bytes_used = 0;
    const size_t num_uint_bytes = uinteger::build(R+sizeof(size_t),uint_array,uint_i);
    ((size_t*)R)[0] = num_uint_bytes;
    total_bytes_used += (sizeof(size_t)+num_uint_bytes);
    R += total_bytes_used;

    total_bytes_used += bitset_new::build(R,bitset_array,bs_i);
    std::cout << "Num uints: " << num_uint_bytes / sizeof(uint) << std::endl;
    return total_bytes_used;
  }
  return 0;
}
//Nothing is different about build flattened here. The number of bytes
//can be infered from the type. This gives us back a true CSR representation.
inline size_t new_type::build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length){
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

inline tuple<size_t,size_t,common::type> new_type::get_flattened_data(const uint8_t *set_data, const size_t cardinality){
  if(cardinality > 0){
    const uint32_t *size_ptr = (uint32_t*) set_data;
    return make_tuple(sizeof(uint32_t),(size_t)size_ptr[0],common::BITSET);
  } else{
    return make_tuple(0,0,common::BITSET);
  }
}

//Iterates over set applying a lambda.
template<typename F>
inline void new_type::foreach_until(
    F f,
    const uint8_t *A,
    const size_t cardinality,
    const size_t number_of_bytes,
    const common::type type) {
  (void) cardinality; (void) type;

  if(number_of_bytes > 0){
    const size_t num_data_words = ((number_of_bytes-sizeof(uint64_t))/sizeof(uint64_t));
    const uint64_t offset = ((uint64_t*)A)[0];
    const uint64_t* A64 = (uint64_t*)(A+sizeof(uint64_t));
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
template<typename F>
inline void new_type::foreach(
    F f,
    const uint8_t * const A,
    const size_t cardinality,
    const size_t number_of_bytes,
    const common::type type) {
  (void) cardinality; (void) type;

  if(number_of_bytes > 0){
    const size_t num_uint_bytes = ((size_t*)A)[0];
    const uint8_t * const uinteger_data = A+sizeof(size_t);
    const uint8_t * const new_bs_data = A+sizeof(size_t)+num_uint_bytes;
    const size_t num_bs_bytes = number_of_bytes-(sizeof(size_t)+num_uint_bytes);
    const size_t uint_card = num_uint_bytes/sizeof(uint32_t);

    uinteger::foreach(f,uinteger_data,uint_card,num_uint_bytes,common::UINTEGER);
    bitset_new::foreach(f,new_bs_data,cardinality-uint_card,num_bs_bytes,common::BITSET_NEW);
  }
}

// Iterates over set applying a lambda in parallel.
template<typename F>
inline size_t new_type::par_foreach(
      F f,
      const size_t num_threads,
      const uint8_t* A,
      const size_t cardinality,
      const size_t number_of_bytes,
      const common::type t) {
   (void) number_of_bytes; (void) t; (void) cardinality;

  if(number_of_bytes > 0){
    const size_t num_data_words = ((number_of_bytes-sizeof(uint64_t))/sizeof(uint64_t));
    const uint64_t offset = ((uint64_t*)A)[0];
    const uint64_t* A64 = (uint64_t*)(A+sizeof(uint64_t));
    return common::par_for_range(num_threads, 0, num_data_words, 512,
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

  return 1;
}
