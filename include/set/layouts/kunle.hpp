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
    uint64_t *levels[MAX_LEVELS];
    int current_level_bin[MAX_LEVELS];
    size_t bins_used[MAX_LEVELS];
    uint64_t *current_level_pointer[MAX_LEVELS];

    uint32_t *skip_list[MAX_LEVELS];
    uint32_t *skip_list_pointer[MAX_LEVELS];

    //setup code, allocs should probably occur outside
    size_t alloc_size = BITS_PER_BIN;
    for(size_t i = 0; i < num_level; i++){
      bins_used[i] = 0;

      levels[i] = new uint64_t[alloc_size/sizeof(uint64_t)];
      skip_list[i] = new uint32_t[alloc_size];
      memset(levels[i],(uint8_t)0,alloc_size);
      memset(skip_list[i],(uint8_t)0,alloc_size*sizeof(uint32_t));
      current_level_pointer[i] = levels[i];
      skip_list_pointer[i] = skip_list[i];
      current_level_bin[i] = -1;
      alloc_size *= BITS_PER_BIN;
    }

    for(size_t i = 0; i < s_a; i++){
      const uint32_t current_value = A[i];
      size_t level_bin = 0;
      size_t dividor = alloc_size/(BITS_PER_BIN*BITS_PER_BIN);
      for(size_t j = 0; j < num_level; j++){
        //bit to be set inside of the level
        const size_t level_bit = current_value / dividor;

        //Where we are actually storing the bin
        uint64_t *my_bin = current_level_pointer[j];
        uint32_t *my_skip_list = skip_list_pointer[j];
        if(current_level_bin[j] != (int)level_bin){
          bins_used[j]++;

          my_bin += (current_level_bin[j]==-1) ? 0:BITS_PER_BIN/BITS_PER_WORD;
          my_skip_list += (current_level_bin[j]==-1) ? 0:(num_level-j);
          skip_list_pointer[j] = my_skip_list;

          (skip_list_pointer[j])[0]++; 
          for(int k = j-1; k >= 0; k--){
            (skip_list_pointer[k])[j-k]++; 
          }

          current_level_bin[j] = level_bin;
          current_level_pointer[j] = my_bin;
        }
        //Word we need to set
        const size_t my_level_word = (level_bit % BITS_PER_BIN) / BITS_PER_WORD;
        //Bit we need to set
        const size_t bit_value = level_bit % BITS_PER_WORD;
        my_bin[my_level_word] |= ((uint64_t)1 << bit_value);

        level_bin = level_bit;
        dividor /= BITS_PER_BIN;
      }
    }

    //encode the skip list structure
    size_t bytes_used = 0;
    for(size_t i = 0; i < num_level; i++){
      const size_t skip_list_bytes = (bins_used[i]*num_level*sizeof(uint32_t));
      cout << "Skip list bytes: " << skip_list_bytes << endl;
      ((uint64_t*)R)[0] = skip_list_bytes; //simd pack each structure
      R += sizeof(uint64_t);
      bytes_used += sizeof(uint64_t);
    }
    cout << "BYTES USED: " << bytes_used << endl;

    for(size_t i = 0; i < num_level; i++){
      cout << "Bins used: " << bins_used[i] << endl;
      cout << "Skip list copy: " << num_level-i << endl;
      uint32_t *my_skip_list = skip_list[i];
      for(size_t b = 0; b < bins_used[i]; b++){
        size_t j = 0;
        for(; j < i; j++){
          //cout << "\t" << (skip_list[i])[j-i] << endl;
          ((uint32_t*)R)[b*num_level+j] = 0;
        }
        cout << "\t";
        for(; j < num_level; j++){
          cout << (my_skip_list)[j-i] << " ";
          ((uint32_t*)R)[b*num_level+j] = my_skip_list[j-i];
        }
        cout << endl;
        my_skip_list += (num_level-j);
        cout << ((uint32_t*)R)[b*num_level] << " " << ((uint32_t*)R)[b*num_level+1] << " " << ((uint32_t*)R)[b*num_level+2] << endl;
        R += sizeof(uint32_t)*num_level;
        bytes_used += sizeof(uint32_t)*num_level;
      }
    }
    cout << "BYTES USED: " << bytes_used << endl; 

    //Encode the data
    for(size_t i = 0; i < num_level; i++){
      cout << "BYTE DATA: " << (bins_used[i]*BITS_PER_BIN)/8 << endl;
      ((uint64_t*)(&R[i*sizeof(uint64_t)]))[0] = (bins_used[i]*BITS_PER_BIN)/8;
    }
    R += num_level*sizeof(uint64_t);
    bytes_used += num_level*sizeof(uint64_t);
    for(size_t i = 0; i < num_level; i++){
      const size_t bytes_to_copy = (bins_used[i]*BITS_PER_BIN)/8;
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
    uint32_t squares[MAX_LEVELS+1];

    uint64_t* A64 = (uint64_t*) A;
    size_t data_offset = 0;
    //jump over the skip list
    for(size_t i = 0; i < num_level; i++){
      cout << "HERE: " << A64[i] << endl;
      data_offset += (A64[i]);
    }
    uint64_t *data = (uint64_t*)&A[(num_level+num_level)*sizeof(uint64_t) + data_offset];
    A64 = (uint64_t*)&A[data_offset + num_level*sizeof(uint64_t)];
    data_offset =0;

    for(size_t i = 0; i < num_level; i++){
      squares[num_level-1-i] = (i==0) ? 1:(BITS_PER_BIN*squares[num_level-i]); 
      level_bin[i] = data + data_offset;
      data_offset += (A64[i]/sizeof(uint64_t));
      cout << "BYTES USED: " << (A64[i]/sizeof(uint64_t)) << endl;
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

      data = level_bin[cur_level];
      if(cur_level < (num_level-1)){
        for(size_t i = level_bin_bit[cur_level]; i < BITS_PER_BIN; i++){
          const uint64_t cur_word = data[i >> ADDRESS_BITS_PER_WORD];
          if((cur_word >> (i % BITS_PER_WORD)) % 2){
            level_bin_bit[cur_level] = i+1;
            level_offset[cur_level+1] = (squares[cur_level]*i)+level_offset[cur_level];
            cur_level++;
            goto START_LEVEL;
          }
        }
      } else{
        for(size_t i = level_bin_bit[cur_level]; i < BITS_PER_BIN; i++){
          const uint64_t cur_word = data[i >> ADDRESS_BITS_PER_WORD];
          if((cur_word >> (i % BITS_PER_WORD)) % 2){
            f(level_offset[cur_level]+i);
          }
        }
      }
      level_bin_bit[cur_level] = 0;
      level_bin[cur_level] += BITS_PER_BIN/BITS_PER_WORD;
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
