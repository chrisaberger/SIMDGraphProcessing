/*

THIS CLASS IMPLEMENTS THE FUNCTIONS ASSOCIATED WITH THE kunle LAYOUT.

*/

#include "common.hpp"

#define BITS_PER_BIN 512
#define MAX_LEVELS 10

#define BITS_PER_WORD 64
#define ADDRESS_BITS_PER_WORD 6
#define BYTES_PER_WORD 8

class kunle {
  public:
    static const size_t range = 1000000;
    static const size_t num_level = (size_t) 3; //ceil(log(range)/log(BITS_PER_BIN));

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
    static void par_foreach(
      F f,
      const size_t num_threads,
      const uint8_t* A,
      const size_t cardinality,
      const size_t number_of_bytes,
      const common::type t);
};
//compute word of data
inline size_t kunle::word_index(const uint32_t bit_index){
  return bit_index >> ADDRESS_BITS_PER_WORD;
}
//check if a bit is set
inline bool kunle::is_set(const uint32_t index, const uint64_t * const in_array, const uint64_t start_index){
  return (in_array)[word_index(index)-start_index] & ((uint64_t) 1 << (index%BITS_PER_WORD));
}
//check if a bit is set
inline void kunle::set(const uint32_t index, uint64_t * const in_array, const uint64_t start_index){
  *(in_array + ((index >> ADDRESS_BITS_PER_WORD)-start_index)) |= ((uint64_t)1 << (index & 0x3F));
}
inline common::type kunle::get_type(){
  return common::KUNLE;
}
//Copies data from input array of ints to our set data r_in
inline size_t kunle::build(uint8_t *R, const uint32_t *A, const size_t s_a){
  if(s_a > 0){
    uint64_t **levels = new uint64_t*[num_level];
    size_t *current_level_bin = new size_t[num_level];
    uint64_t **current_level_pointer = new uint64_t*[num_level];

    size_t *bins_filled = new size_t[num_level];
    size_t *elems_per_bit = new size_t[num_level];

    //setup code, allocs should probably occur outside
    size_t alloc_size = BITS_PER_BIN;
    for(size_t i = 0; i < num_level; i++){
      bins_filled[i] = 0;
      elems_per_bit[i] = range/alloc_size;
      levels[i] = new uint64_t[alloc_size/sizeof(uint64_t)];
      memset(levels[i],(uint8_t)0,alloc_size);
      current_level_pointer[i] = levels[i];
      current_level_bin[i] = 0;
      alloc_size *= BITS_PER_BIN;
    }

    for(size_t i = 0; i < s_a; i++){
      const uint32_t current_value = A[i];
      size_t level_bin = 0;
      for(size_t j = 0; j < num_level; j++){
        //bit to be set inside of the level
        const size_t level_bit = current_value / alloc_size;
        
        //Where we are actually storing the bin
        uint64_t *my_bin = current_level_pointer[j];
        if(current_level_bin[j] != level_bin){
          cout << "incrementing" << endl;
          my_bin += BITS_PER_BIN/BITS_PER_WORD;
          current_level_bin[j] = level_bin;
          current_level_pointer[j] = my_bin;
        }
        //Word we need to set
        const size_t my_level_word = (level_bit % BITS_PER_BIN) / BITS_PER_WORD;
        //Bit we need to set
        const size_t bit_level_word = level_bit % BITS_PER_WORD;
        cout << "bit level word: " << bit_level_word << endl;
        my_bin[my_level_word] |= 1 << bit_level_word;

        level_bin = level_bit;
        alloc_size /= BITS_PER_WORD;
      }
    }

    for(size_t i = 0; i < num_level; i++){
      cout << "Num bytes: " << ((current_level_bin[i]+1)*BITS_PER_BIN)/8 << endl;
      ((uint64_t*)(&R[i*sizeof(uint64_t)]))[0] = ((current_level_bin[i]+1)*BITS_PER_BIN)/8;
    }
    R += num_level*sizeof(uint64_t);

    size_t bytes_used = 0;
    for(size_t i = 0; i < num_level; i++){
      const size_t bytes_to_copy = ((current_level_bin[i]+1)*BITS_PER_BIN)/sizeof(uint8_t);
      memcpy(R,levels[i],bytes_to_copy);
      R += bytes_to_copy;
      bytes_used += bytes_to_copy;
    }
    return bytes_used;
  }
  return 0;
}
//Nothing is different about build flattened here. The number of bytes
//can be infered from the type. This gives us back a true CSR representation.
inline size_t kunle::build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length){
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

inline tuple<size_t,size_t,common::type> kunle::get_flattened_data(const uint8_t *set_data, const size_t cardinality){
  if(cardinality > 0){
    const uint32_t *size_ptr = (uint32_t*) set_data;
    return make_tuple(sizeof(uint32_t),(size_t)size_ptr[0],common::KUNLE);
  } else{
    return make_tuple(0,0,common::KUNLE);
  }
}

//Iterates over set applying a lambda.
template<typename F>
inline void kunle::foreach_until(
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
inline void kunle::foreach(
    F f,
    const uint8_t * const A,
    const size_t cardinality,
    const size_t number_of_bytes,
    const common::type type) {
  (void) cardinality; (void) type;

  if(number_of_bytes > 0){
    uint64_t *level_bin[MAX_LEVELS];
    uint64_t level_bin_bit[MAX_LEVELS];
    uint32_t level_offset[MAX_LEVELS+1];

    uint64_t* A64 = (uint64_t*) A;
    uint64_t *data = A64+num_level;
    for(size_t i = 0; i < num_level; i++){
      level_bin[i] = data + A64[i];
      level_bin_bit[i] = 0;
      level_offset[i] = 0;
    }

    size_t cur_level = 0;
    while(true){
    START_LEVEL:
      //covers either a 1 or a 0 being hit last in first level...kinda shitty
      if((cur_level == 0 && level_bin_bit[0] == BITS_PER_BIN) 
        || cur_level == 0xFFFFFFFFFFFFFFFF)
        break;

      cout << cur_level << endl;
      data = level_bin[cur_level];
      if(cur_level < (num_level-1)){
        for(size_t i = level_bin_bit[cur_level]; i < BITS_PER_BIN; i++){
          const uint64_t cur_word = data[i >> ADDRESS_BITS_PER_WORD];
          if((cur_word >> (i % BITS_PER_WORD)) % 2){
            level_bin_bit[cur_level] = i+1;
            level_offset[cur_level+1] = BITS_PER_BIN*(i+level_offset[cur_level-1]);
            cur_level++;
            goto START_LEVEL;
          }
        }
      } else{
        for(size_t i = level_bin_bit[cur_level]; i < BITS_PER_BIN; i++){
          const uint64_t cur_word = data[i >> ADDRESS_BITS_PER_WORD];
          if((cur_word >> (i % BITS_PER_WORD)) % 2){
            level_bin_bit[cur_level] = i+1;
            f(level_offset[cur_level]+i);
          }
        }
      }
      cur_level--;
    }
  }
}

// Iterates over set applying a lambda in parallel.
template<typename F>
inline void kunle::par_foreach(
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
