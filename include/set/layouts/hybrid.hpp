#ifndef _HYBRID_H_
#define _HYBRID_H_
/*

THIS CLASS IMPLEMENTS THE FUNCTIONS ASSOCIATED WITH AN HYBRID SET LAYOUT.
HERE WE DYNAMICALLY DETECT THE UNDERLYING LAYOUT OF THE SET AND
RUN THE CORRESPONDING SET MEHTODS.

*/

#include "uinteger.hpp"
#include "pshort.hpp"
#include "bitset.hpp"
#include "bitpacked.hpp" //this file includes variant.hpp

class hybrid{
  public:
    static common::type get_type();
    static common::type get_type(const uint32_t *data, const size_t length);
    static common::type compute_type(const double density);
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
      const uint8_t *data_in,
      const size_t cardinality,
      const size_t number_of_bytes,
      const common::type t);
};
inline common::type hybrid::get_type(){
  return common::HYBRID;
}

inline common::type hybrid::compute_type(const double density){
  if(density > 1.0 / 32.0) {
    return common::BITSET;
  } else if(density > 1.0 / 256.0){
    return common::PSHORT;
  } else {
    return common::UINTEGER;
  }
}

inline double compressibility(const uint32_t* data, const size_t length) {
  double inv_compressibility = 0.0;
  for(size_t i = 1; i < length; i++) {
    inv_compressibility += log(data[i] - data[i - 1]);
  }
  return length / inv_compressibility;
}

#if PERFORMANCE == 1
inline common::type hybrid::get_type(const uint32_t *data, const size_t length){
  if(length > 1) {
    uint32_t range = data[length - 1] - data[0];
    if(range > 0){
      double density = (double) length / range;
     // double c = compressibility(data, length);
      if(density > ((double)1/256)) {
        return common::BITSET;
      }
      else if(length/((range/65536)+1) > 8) {
        return common::PSHORT;
      } 
      else {
        return common::UINTEGER;
      }
    }
    return common::UINTEGER;
  } else{
    return common::UINTEGER;
  }
  //return common::UINTEGER;
}
#endif
#if COMPRESSION == 1
inline common::type hybrid::get_type(const uint32_t *data, const size_t length){
  if(length > 0){
    uint32_t max_value = data[length-1];
    double sparsity = (double) length/max_value;
    if( sparsity > (double) 1/32 ){
      return common::PSHORT;
    } else if(length > 100){
      return common::BITPACKED;
    } else {
      return common::VARIANT;
    } 
  } else{
    return common::PSHORT;
  }
}
#endif

//Copies data from input array of ints to our set data r_in
inline size_t hybrid::build(uint8_t *r_in, const uint32_t *data, const size_t length){
  common::type t = hybrid::get_type(data,length);
  switch(t){
    case common::UINTEGER :
      return uinteger::build(r_in,data,length);
    break;
    case common::PSHORT :
      return pshort::build(r_in,data,length);
    break;
    case common::BITSET :
      return bitset::build(r_in,data,length);
    break;
    case common::VARIANT :
      return variant::build(r_in,data,length);
    break;
    case common::BITPACKED :
      return bitpacked::build(r_in,data,length);
    break;
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
    case common::UINTEGER :
      return 1+uinteger::build_flattened(&r_in[1],data,length);
    break;
    case common::PSHORT :
      return 1+pshort::build_flattened(&r_in[1],data,length);
    break;
    case common::BITSET :
      return 1+bitset::build_flattened(&r_in[1],data,length);
    break;
    case common::VARIANT :
      return 1+variant::build_flattened(&r_in[1],data,length);
    break;
    case common::BITPACKED :
      return 1+bitpacked::build_flattened(&r_in[1],data,length);
    break;
    default:
      return 0;
  }
}

inline tuple<size_t,size_t,common::type> hybrid::get_flattened_data(const uint8_t *set_data, const size_t cardinality){
  common::type t = (common::type) set_data[0];
  tuple<size_t,size_t,common::type> mytup;
  switch(t){
    case common::UINTEGER :
      mytup = uinteger::get_flattened_data(&set_data[1],cardinality);
      get<0>(mytup)++;
      break;
    case common::PSHORT :
      mytup = pshort::get_flattened_data(&set_data[1],cardinality);
      get<0>(mytup)++;
      break;
    case common::BITSET :
      mytup = bitset::get_flattened_data(&set_data[1],cardinality);
      get<0>(mytup)++;
      break;
    case common::VARIANT :
      mytup = variant::get_flattened_data(&set_data[1],cardinality);
      get<0>(mytup)++;
      break;
    case common::BITPACKED :
      mytup = bitpacked::get_flattened_data(&set_data[1],cardinality);
      get<0>(mytup)++;
      break;
    default:
      mytup = make_tuple(0,0,common::UINTEGER);
      break;
  }
  return mytup;
}

//Iterates over set applying a lambda.
template<typename F>
inline void hybrid::foreach_until(
    F f,
    const uint8_t *data_in,
    const size_t cardinality,
    const size_t number_of_bytes,
    const common::type t) {
  switch(t){
    case common::UINTEGER :
      uinteger::foreach(f,data_in,cardinality,number_of_bytes,common::UINTEGER);
    break;
    case common::PSHORT :
      pshort::foreach(f,data_in,cardinality,number_of_bytes,common::PSHORT);
    break;
    case common::BITSET :
      bitset::foreach(f,data_in,cardinality,number_of_bytes,common::BITSET);
    break;
    case common::VARIANT :
      variant::foreach(f,data_in,cardinality,number_of_bytes,common::BITSET);
    break;
    case common::BITPACKED :
      bitpacked::foreach(f,data_in,cardinality,number_of_bytes,common::BITSET);
    break;
    default:
    break;
  }
}

//Iterates over set applying a lambda.
template<typename F>
inline void hybrid::foreach(
    F f,
    const uint8_t *data_in,
    const size_t cardinality,
    const size_t number_of_bytes,
    const common::type t) {
  switch(t){
    case common::UINTEGER :
      uinteger::foreach(f,data_in,cardinality,number_of_bytes,common::UINTEGER);
      break;
    case common::PSHORT :
      pshort::foreach(f,data_in,cardinality,number_of_bytes,common::PSHORT);
      break;
    case common::BITSET :
      bitset::foreach(f,data_in,cardinality,number_of_bytes,common::BITSET);
      break;
    case common::VARIANT :
      variant::foreach(f,data_in,cardinality,number_of_bytes,common::BITSET);
      break;
    case common::BITPACKED :
      bitpacked::foreach(f,data_in,cardinality,number_of_bytes,common::BITSET);
      break;
    default:
      break;
  }
}

//Iterates over set applying a lambda.
template<typename F>
inline size_t hybrid::par_foreach(
      F f,
      const size_t num_threads,
      const uint8_t *data_in,
      const size_t cardinality,
      const size_t number_of_bytes,
      const common::type t) {
  switch(t){
    case common::UINTEGER :
      return uinteger::par_foreach(num_threads,f,data_in,cardinality,number_of_bytes,common::UINTEGER);
      break;
    case common::PSHORT :
      return pshort::par_foreach(num_threads,f,data_in,cardinality,number_of_bytes,common::PSHORT);
      break;
    case common::BITSET :
      return bitset::par_foreach(num_threads,f,data_in,cardinality,number_of_bytes,common::BITSET);
      break;
    case common::VARIANT :
      std::cout << "Parallel foreach for VARIANT is not implemented" << std::endl;
      exit(EXIT_FAILURE);
      // variant::par_foreach(num_threads,f,data_in,cardinality,number_of_bytes,common::BITSET);
      break;
    case common::BITPACKED :
      std::cout << "Parallel foreach for BITPACKED is not implemented" << std::endl;
      exit(EXIT_FAILURE);
      // bitpacked::par_foreach(num_threads,f,data_in,cardinality,number_of_bytes,common::BITSET);
      break;
    default:
      break;
  }
}
#endif
