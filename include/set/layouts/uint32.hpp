/*

THIS CLASS IMPLEMENTS THE FUNCTIONS ASSOCIATED WITH AN UNCOMPRESSED SET LAYOUT.
QUITE SIMPLE THE LAYOUT JUST CONTAINS UNSIGNED INTEGERS IN THE SET.

*/

#include "common.hpp"
#include "../ops/intersection.hpp"

class uint32{
  public:
    static common::type get_type();
    static size_t build(uint8_t *r_in, const uint32_t *data, const size_t length);
    static size_t build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length);
    static tuple<uint8_t,size_t,size_t> get_flattened_data(const uint8_t *set_data, const size_t cardinality);
    static void foreach(const std::function <void (uint32_t)>& f,const uint8_t *data_in, const size_t cardinality, const size_t number_of_bytes);
    static tuple<size_t,size_t,common::type> intersect(uint8_t *C_in, uint8_t *A_in, uint8_t *B_in, size_t A_cardinality, size_t B_cardinality, size_t A_num_bytes, size_t B_num_bytes);
};

inline common::type uint32::get_type(){
  return common::ARRAY32;
}
//Copies data from input array of ints to our set data r_in
inline size_t uint32::build(uint8_t *r_in, const uint32_t *data, const size_t length){
  uint32_t *r = (uint32_t*) r_in;
  std::copy(data,data+length,r);
  return length*sizeof(uint32_t);
}
//Nothing is different about build flattened here. The number of bytes
//can be infered from the type. This gives us back a true CSR representation.
inline size_t uint32::build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length){
  return build(r_in,data,length);
}

inline tuple<uint8_t,size_t,size_t> uint32::get_flattened_data(const uint8_t *set_data, const size_t cardinality){
  (void) set_data;
  return make_tuple(common::ARRAY32,cardinality*sizeof(uint32_t),0);
}

//Iterates over set applying a lambda.
inline void uint32::foreach(const std::function <void (uint32_t)>& f, const uint8_t *data_in, 
  const size_t cardinality, const size_t number_of_bytes){

 (void) number_of_bytes;
 uint32_t *data = (uint32_t*) data_in;
 for(size_t i=0; i<cardinality;i++){
  f(data[i]);
 }
}

inline tuple<size_t,size_t,common::type> uint32::intersect(uint8_t *C_in, uint8_t *A_in, uint8_t *B_in, size_t card_a, size_t card_b, size_t s_bytes_a, size_t s_bytes_b) {
  (void) s_bytes_b; (void) s_bytes_a;
  return ops::intersect_u32_u32(C_in,(uint32_t*)A_in,(uint32_t*)B_in,card_a,card_b);
}