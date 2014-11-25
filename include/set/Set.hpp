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

    //Implicit Conversion Between Unlike Types
    template <class U> 
    Set<T>(Set<U> in){
      data = in.data;
      cardinality = in.cardinality;
      number_of_bytes = in.number_of_bytes;
      type = in.type;
    }

    //basic traversal
    void foreach(const std::function <void (uint32_t)>& f);

    //ops (will take any type you give)
    static Set<T> intersect(Set<T> C_in, Set<T> A_in, Set<T> B_in);

    //Constructors
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

///////////////////////////////////////////////////////////////////////////////
//CREATE A SET FROM AN ARRAY OF UNSIGNED INTEGERS
///////////////////////////////////////////////////////////////////////////////
template <class T>
inline Set<T> Set<T>::from_array(uint8_t *set_data, uint32_t *array_data, size_t data_size){
  size_t bytes_in = T::build(set_data,array_data,data_size);
  return Set<T>(set_data,data_size,bytes_in,T::get_type());
}

///////////////////////////////////////////////////////////////////////////////
//CREATE A SET FROM A FLATTENED ARRAY WITH INFO.  
//THIS IS USED FOR A CSR GRAPH IMPLEMENTATION.
///////////////////////////////////////////////////////////////////////////////
template <class T>
inline Set<T> Set<T>::from_flattened(uint8_t *set_data, size_t cardinality_in){
  auto flattened_data = T::get_flattened_data(set_data,cardinality_in);
  return Set<T>(&set_data[get<0>(flattened_data)],cardinality_in,get<1>(flattened_data),get<2>(flattened_data));
}

///////////////////////////////////////////////////////////////////////////////
//FLATTEN A SET INTO AN ARRAY.  
//THIS IS USED FOR A CSR GRAPH IMPLEMENTATION.
///////////////////////////////////////////////////////////////////////////////
template <class T>
inline size_t Set<T>::flatten_from_array(uint8_t *set_data, uint32_t *array_data, size_t data_size){
  return T::build_flattened(set_data,array_data,data_size);
}

//ops (will take any type you give)
template<class T>
inline Set<T> Set<T>::intersect(Set<T> C_in, Set<T> A_in, Set<T> B_in){
  tuple<size_t,size_t,common::type> intersection_values = T::intersect(C_in.data,
    A_in.data,B_in.data,
    A_in.cardinality,B_in.cardinality,
    A_in.number_of_bytes,B_in.number_of_bytes,
    A_in.type,B_in.type);

  return Set<T>(C_in.data,get<0>(intersection_values),get<1>(intersection_values),get<2>(intersection_values));
}

#endif