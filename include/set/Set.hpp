#ifndef _SET_H_
#define _SET_H_

/*
TOP LEVEL CLASS FOR OUR SORTED SETS.  PROVIDE A SET OF GENERAL PRIMITIVE 
TYPES THAT WE PROVIDE (PSHORT,UINT,VARIANT,BITPACKED,BITSET).  WE ALSO
PROVIDE A HYBRID SET IMPLEMENTATION THAT DYNAMICALLY CHOOSES THE TYPE
FOR THE DEVELOPER. IMPLEMENTATION FOR PRIMITIVE TYPE OPS CAN BE 
FOUND IN STATIC CLASSES IN THE LAYOUT FOLDER.
*/

#include "layouts/hybrid.hpp"
#include "layouts/uint32.hpp"

template <class T>
class Set{ 
  public: 
    uint8_t *data;
    size_t cardinality;
    size_t number_of_bytes;
    common::type type;

    Set(uint8_t *data_in, 
      size_t cardinality_in, 
      size_t number_of_bytes_in,
      common::type type_in):
      data(data_in),
      cardinality(cardinality_in),
      number_of_bytes(number_of_bytes_in),
      type(type_in){}

    template <class U>
    Set<T>(const Set<U>&);

    void foreach(const std::function <void (uint32_t)>& f);

    static Set<T> from_array(uint8_t *set_data, uint32_t *array_data, size_t data_size);
    static Set<T> from_flattened(uint8_t *set_data, size_t cardinality_in);
    static size_t flatten_from_array(uint8_t *set_data, uint32_t *array_data, size_t data_size);
};

///////////////////////////////////////////////////////////////////////////////
//FOR EACH ELEMNT IN SET APPLY A LAMBDA
///////////////////////////////////////////////////////////////////////////////
template <class T>
inline void Set<T>::foreach(const std::function <void (uint32_t)>& f){ 
  T::foreach(f,data,cardinality,number_of_bytes,type);
}
template <>
inline void Set<hybrid>::foreach(const std::function <void (uint32_t)>& f){ 
  switch(type){
    case common::ARRAY32 :
      uint32::foreach(f,data,cardinality,number_of_bytes,type);
    break;
    default:
    break;
  }
}  

///////////////////////////////////////////////////////////////////////////////
//CREATE A SET FROM AN ARRAY OF UNSIGNED INTEGERS
///////////////////////////////////////////////////////////////////////////////
template <class T>
inline Set<T> Set<T>::from_array(uint8_t *set_data, uint32_t *array_data, size_t data_size){
  size_t bytes_in = T::build(set_data,array_data,data_size);
  return Set<T>(set_data,data_size,bytes_in,T::get_type());
}
template <>
inline Set<hybrid> Set<hybrid>::from_array(uint8_t *set_data, uint32_t *array_data, size_t data_size){
  common::type t = hybrid::get_type(array_data,data_size);
  switch(t){
    case common::ARRAY32 :
      return Set<uint32>::from_array(set_data,array_data,data_size);
    break;
    default:
      return Set<hybrid>(set_data,0,0,common::ARRAY32);
  }
}

///////////////////////////////////////////////////////////////////////////////
//CREATEE A SET FROM A FLATTENED ARRAY WITH INFO.  
//THIS IS USED FOR A CSR GRAPH IMPLEMENTATION.
///////////////////////////////////////////////////////////////////////////////
template <class T>
inline Set<T> Set<T>::from_flattened(uint8_t *set_data, size_t cardinality_in){
  auto flattened_data = T::get_flattened_data(set_data,cardinality_in);
  return Set<T>(set_data+std::get<2>(flattened_data),cardinality_in,std::get<1>(flattened_data),(common::type)std::get<0>(flattened_data));
}
template <>
inline Set<hybrid> Set<hybrid>::from_flattened(uint8_t *set_data, size_t cardinality_in){
  common::type t = (common::type)set_data[0];
  set_data[0] = (uint8_t) t;
  switch(t){
    case common::ARRAY32 :
      return Set<uint32>::from_flattened(&set_data[1],cardinality_in);
    break;
    default:
      return Set<hybrid>(set_data,0,0,common::ARRAY32);
  }
}

///////////////////////////////////////////////////////////////////////////////
//FLATTEN A SET INTO AN ARRAY.  
//THIS IS USED FOR A CSR GRAPH IMPLEMENTATION.
///////////////////////////////////////////////////////////////////////////////
template <class T>
inline size_t Set<T>::flatten_from_array(uint8_t *set_data, uint32_t *array_data, size_t data_size){
  return T::build_flattened(set_data,array_data,data_size);
}
template <>
inline size_t Set<hybrid>::flatten_from_array(uint8_t *set_data, uint32_t *array_data, size_t data_size){
  common::type t = hybrid::get_type(array_data,data_size);
  switch(t){
    case common::ARRAY32 :
      set_data[0] = (uint8_t) common::ARRAY32;
      return uint32::build_flattened(&set_data[1],array_data,data_size);
    break;
    default:
      return 0;
  }
  return 0;//T::build_flattened(set_data,array_data,data_size);
}

//IMPLEMENTS THE SET PRIMITIVE OPERATIONS.
#include "ops/sse_masks.hpp"
#include "ops/intersection.hpp"

#endif