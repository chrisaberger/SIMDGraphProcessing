#ifndef _UINT_ARRAY_H_
#define _UINT_ARRAY_H_


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

    void foreach(const std::function <void (uint32_t)>& f);
    static Set<T> from_array(uint8_t *set_data, uint32_t *array_data, size_t data_size);
    static Set<T> from_flattened(uint8_t *set_data, size_t cardinality_in);
    static size_t flatten_from_array(uint8_t *set_data, uint32_t *array_data, size_t data_size);
};

template <>
inline void Set<hybrid>::foreach(const std::function <void (uint32_t)>& f){ 
  //T::foreach(f,data,cardinality,number_of_bytes,type);
} 

template <class T>
inline void Set<T>::foreach(const std::function <void (uint32_t)>& f){ 
  T::foreach(f,data,cardinality,number_of_bytes,type);
} 

template <class T>
Set<T> Set<T>::from_array(uint8_t *set_data, uint32_t *array_data, size_t data_size){
  size_t bytes_in = T::build(set_data,array_data,data_size);
  return Set<T>(set_data,data_size,bytes_in,T::get_type());
}

template <class T>
Set<T> Set<T>::from_flattened(uint8_t *set_data, size_t cardinality_in){
  auto flattened_data = T::get_flattened_data(set_data,cardinality_in);
  return Set<T>(set_data+std::get<2>(flattened_data),cardinality_in,std::get<1>(flattened_data),(common::type)std::get<0>(flattened_data));
}

template <class T>
size_t Set<T>::flatten_from_array(uint8_t *set_data, uint32_t *array_data, size_t data_size){
  return T::build_flattened(set_data,array_data,data_size);
}


#include "ops/sse_masks.hpp"
#include "ops/intersection.hpp"

#endif