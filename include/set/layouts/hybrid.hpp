#ifndef _HYBRID_H_
#define _HYBRID_H_
/*

THIS CLASS IMPLEMENTS THE FUNCTIONS ASSOCIATED WITH AN UNCOMPRESSED SET LAYOUT.
QUITE SIMPLE THE LAYOUT JUST CONTAINS UNSIGNED INTEGERS IN THE SET.

*/


//Implement implicit type conversions from one templated type to another when 
//an op is called in the hybrid class.

#include "common.hpp"
class hybrid{
  public:
    static common::type get_type();
    static common::type get_type(const uint32_t *data, const size_t length);
};

inline common::type hybrid::get_type(){
  return common::HYBRID_PERF;
}

inline common::type hybrid::get_type(const uint32_t *data, const size_t length){
  if(length > 0){
    uint32_t max_value = data[length-1];
    double sparsity = (double) length/max_value;
    if( sparsity > (double) 1/32 ){
      return common::ARRAY32;
    } else if((length/((max_value >> 16) - (data[0] >> 16) + 1)) > 12){
      return common::ARRAY32;
    } else {
      return common::ARRAY32;
    }  
  } else{
    return common::ARRAY32;
  }
}

#endif