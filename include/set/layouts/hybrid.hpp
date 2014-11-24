/*

THIS CLASS IMPLEMENTS THE FUNCTIONS ASSOCIATED WITH AN UNCOMPRESSED SET LAYOUT.
QUITE SIMPLE THE LAYOUT JUST CONTAINS UNSIGNED INTEGERS IN THE SET.

*/


//Implement implicit type conversions from one templated type to another when 
//an op is called in the hybrid class.

#include "common.hpp"
class hybrid{
  public:
};
/*
inline common::type hybridC::get_type(){
  return common::HYBRID_PERF;
}
//Copies data from input array of ints to our set data r_in
inline size_t hybridC::build(uint8_t *r_in, const uint32_t *data, const size_t length){
  if(length > 0){
    uint32_t max_value = data[length-1];
    double sparsity = (double) length/max_value;
    if( sparsity > (double) 1/32 ){
      return uint32::build(r_in,data,length);
    } else if((length/((max_value >> 16) - (data[0] >> 16) + 1)) > 12){
      return uint32::build(r_in,data,length);
    } else {
      return uint32::build(r_in,data,length);
    }  
  }
  return 0;
}
//Nothing is different about build flattened here. The number of bytes
//can be infered from the type. This gives us back a true CSR representation.
inline size_t hybridC::build_flattened(uint8_t *r_in, const uint32_t *data, const size_t length){
  if(length > 0){
    uint32_t max_value = data[length-1];
    double sparsity = (double) length/max_value;
    if( sparsity > (double) 1/32 ){
      r_in[0] = common::ARRAY32;
      size_t *size_ptr = (size_t*) &r_in[1];
      size_t byte_length = uint32::build(r_in+1+sizeof(size_t),data,length);
      size_ptr[0] = byte_length;
    } else if((length/((max_value >> 16) - (data[0] >> 16) + 1)) > 12){
      r_in[0] = common::ARRAY32;
      size_t *size_ptr = (size_t*) &r_in[1];
      size_t byte_length = uint32::build(r_in+1+sizeof(size_t),data,length);
      size_ptr[0] = byte_length;
    } else {
      r_in[0] = common::ARRAY32;
      size_t *size_ptr = (size_t*) &r_in[1];
      size_t byte_length = uint32::build(r_in+1+sizeof(size_t),data,length);
      size_ptr[0] = byte_length;
    }  
  }
  return 0;
}
inline tuple<uint8_t,size_t,size_t> hybridC::get_flattened_data(const uint8_t *set_data, const size_t cardinality){
  common::type type = (common::type) set_data[0];
  if(type == common::ARRAY32){
    return uint32::get_flattened_data(&set_data[1],cardinality);
  }else{
    return uint32::get_flattened_data(&set_data[1],cardinality);
  }
}

//Iterates over set applying a lambda.
inline void hybridC::foreach(const std::function <void (uint32_t)>& f, const uint8_t *data_in, 
  const size_t cardinality, const size_t number_of_bytes, const common::type type){

 (void) number_of_bytes; (void) type;
 uint32_t *data = (uint32_t*) data_in;
 for(size_t i=0; i<cardinality;i++){
  f(data[i]);
 }
}
*/