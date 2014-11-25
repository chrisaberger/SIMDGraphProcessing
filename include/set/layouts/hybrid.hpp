#ifndef _HYBRID_H_
#define _HYBRID_H_
/*

THIS CLASS IMPLEMENTS THE FUNCTIONS ASSOCIATED WITH AN UNCOMPRESSED SET LAYOUT.
QUITE SIMPLE THE LAYOUT JUST CONTAINS UNSIGNED INTEGERS IN THE SET.

*/


//Implement implicit type conversions from one templated type to another when 
//an op is called in the hybrid class.

#include "uint32.hpp"
#include "pshort.hpp"

class hybrid{
  public:
    static common::type get_type();
    static common::type get_type(const uint32_t *data, const size_t length);
    static size_t build(uint8_t *r_in, const uint32_t *data, const size_t length);
    static size_t build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length);
    static tuple<size_t,size_t,common::type> get_flattened_data(const uint8_t *set_data, const size_t cardinality);
    static void foreach(const std::function <void (uint32_t)>& f,const uint8_t *data_in, const size_t cardinality, const size_t number_of_bytes, const common::type t);
    static tuple<size_t,size_t,common::type> intersect(uint8_t *C_in, const uint8_t *A_in, const uint8_t *B_in, const size_t A_cardinality, const size_t B_cardinality, const size_t A_num_bytes, const size_t B_num_bytes, const common::type a_t, const common::type b_t);
};

inline common::type hybrid::get_type(){
  return common::HYBRID_PERF;
}

inline common::type hybrid::get_type(const uint32_t *data, const size_t length){
  if(length > 0){
    uint32_t max_value = data[length-1];
    double sparsity = (double) length/max_value;
    if( sparsity > (double) 1/32 ){
      return common::ARRAY16;
    } else if((length/((max_value >> 16) - (data[0] >> 16) + 1)) > 12){
      return common::ARRAY16;
    } else {
      return common::ARRAY32;
    } 
  } else{
    return common::ARRAY32;
  }
}

//Copies data from input array of ints to our set data r_in
inline size_t hybrid::build(uint8_t *r_in, const uint32_t *data, const size_t length){
  common::type t = hybrid::get_type(data,length);
  switch(t){
    case common::ARRAY32 :
      return uint32::build(r_in,data,length);
    break;
    case common::ARRAY16 :
      return pshort::build(r_in,data,length);
    default:
      return 0;
  }
}
//Nothing is different about build flattened here. The number of bytes
//can be infered from the type. This gives us back a true CSR representation.
inline size_t hybrid::build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length){
  common::type t = hybrid::get_type(data,length);
  r_in[0] = (uint8_t) t;
  switch(t){
    case common::ARRAY32 :
      return 1+uint32::build_flattened(&r_in[1],data,length);
    break;
    case common::ARRAY16 :
      return 1+pshort::build_flattened(&r_in[1],data,length);
    break;
    default:
      return 0;
  }
}

inline tuple<size_t,size_t,common::type> hybrid::get_flattened_data(const uint8_t *set_data, const size_t cardinality){
  common::type t = (common::type) set_data[0];
  tuple<size_t,size_t,common::type> mytup;
  switch(t){
    case common::ARRAY32 :
      mytup = uint32::get_flattened_data(&set_data[1],cardinality);
      get<0>(mytup)++;
      return mytup;
    break;
    case common::ARRAY16 :
      mytup = pshort::get_flattened_data(&set_data[1],cardinality);
      get<0>(mytup)++;
      return mytup;
    break;
    default:
      return make_tuple(0,0,common::ARRAY32);
  }
}

//Iterates over set applying a lambda.
inline void hybrid::foreach(const std::function <void (uint32_t)>& f, const uint8_t *data_in, 
  const size_t cardinality, const size_t number_of_bytes, const common::type t){

  switch(t){
    case common::ARRAY32 :
      uint32::foreach(f,data_in,cardinality,number_of_bytes,common::ARRAY32);
    break;
    case common::ARRAY16 :
      pshort::foreach(f,data_in,cardinality,number_of_bytes,common::ARRAY16);
    break;
    default:
    break;
  }
}

inline tuple<size_t,size_t,common::type> hybrid::intersect(uint8_t *C_in, 
  const uint8_t *A_in, const uint8_t *B_in, 
  const size_t card_a, const size_t card_b, 
  const size_t s_bytes_a, const size_t s_bytes_b, 
  const common::type a_t, const common::type b_t) {
  
  if(a_t == common::ARRAY32){
    if(b_t == common::ARRAY32){
      return ops::intersect_u32_u32((uint32_t*)C_in,(uint32_t*)A_in,(uint32_t*)B_in,card_a,card_b);
    } else if(b_t == common::ARRAY16){
      return ops::intersect_uint_pshort((uint32_t*)C_in,(uint32_t*)A_in,(uint16_t*)B_in,card_a,s_bytes_b/sizeof(uint16_t));
    }
  }
  else if(a_t == common::ARRAY16){
    if(b_t == common::ARRAY16){
      return ops::intersect_pshort_pshort((uint16_t*)C_in,(uint16_t*)A_in,(uint16_t*)B_in,s_bytes_a/sizeof(uint16_t),s_bytes_b/sizeof(uint16_t));
    } else if(b_t == common::ARRAY32){
      return ops::intersect_uint_pshort((uint32_t*)C_in,(uint32_t*)B_in,(uint16_t*)A_in,card_b,s_bytes_a/sizeof(uint16_t));
    }
  } 

  return make_tuple(0,0,common::ARRAY32);//ops::intersect_u32_u32((uint32_t*)C_in,(uint32_t*)A_in,(uint32_t*)B_in,card_a,card_b);
}

#endif