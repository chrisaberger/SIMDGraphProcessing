#include "hybrid.hpp"

namespace uint_array{
  inline size_t preprocess(uint8_t *data, size_t index, unsigned int *data_in, size_t length_in, common::type t){
    switch(t){
      case common::ARRAY32:
        index += array32::preprocess((unsigned int*)(data+index),data_in,length_in);
        break;
      case common::ARRAY16:
        index += array16::preprocess((unsigned short*)(data+index),data_in,length_in);
        break;
      case common::BITSET:
        index += bitset::preprocess((unsigned short*)(data+index),data_in,length_in);
        break;
      case common::A32BITPACKED:
        index += a32bitpacked::preprocess((data+index),data_in,length_in);
        break;
      case common::A32BITPACKED_DELTA:
        index += a32bitpacked_delta::preprocess((data+index),data_in,length_in);
        break;
      case common::VARIANT:
        index += variant::preprocess((data+index),data_in,length_in);
        break;
      case common::VARIANT_DELTA:
        index += variant_delta::preprocess((data+index),data_in,length_in);
        break;
      default:
        break;
    }
    return index;
  }

  template<typename T> 
  inline T foreach(T (*function)(unsigned int,unsigned int),unsigned int col,uint8_t *data,size_t length,common::type t){
    switch(t){
      case common::ARRAY32:
        return array32::foreach(function,col,(unsigned int*)data,length/4);
        break;
      case common::ARRAY16:
        return array16::foreach(function,col,(unsigned short*)data,length/2);
        break;
      case common::BITSET:
        return bitset::foreach(function,col,(unsigned short*)data,length/2);
        break;
      default:
        return (T) 0;
        break;
    }
  }
  inline size_t intersect(uint8_t *R, uint8_t *A, uint8_t *B, size_t s_a, size_t s_b, common::type t){
    size_t count = 0;
    switch(t){
      case common::ARRAY32:
        count = array32::intersect((unsigned int*)R,(unsigned int*)A,(unsigned int*)B,s_a/4,s_b/4);
        break;
      case common::ARRAY16:
        count = array16::intersect((unsigned short*)R,(unsigned short*)A,(unsigned short*)B,s_a/2,s_b/2);
        break;
      case common::BITSET:
        count = bitset::intersect((unsigned short*)R,(unsigned short*)A,(unsigned short*)B,s_a/2,s_b/2);
        break;
      default:
        break;
    }
    return count;
  }

  inline size_t intersect(uint8_t *R, uint8_t *A, uint8_t *B, size_t s_a, size_t s_b, common::type t1, common::type t2){
    size_t count = 0;
    if(t1 == common::ARRAY32){
      if(t2 == common::ARRAY16){
        //cout << "a32 a16" << endl;
        count = hybrid::intersect_a32_a16((unsigned int*)R,(unsigned int*)A,(unsigned short*)B,s_a/4,s_b/2);
      } else if(t2 == common::BITSET){
        //cout << "a32 bs" << endl;
        count = hybrid::intersect_a32_bs((unsigned int*)R,(unsigned int*)A,(unsigned short*)B,s_a/4,s_b/2);
      } 
    } else if(t1 == common::ARRAY16){
      if(t2 == common::ARRAY32){
        //cout << "a32 a16" << endl;
        count = hybrid::intersect_a32_a16((unsigned int*)R,(unsigned int*)B,(unsigned short*)A,s_b/4,s_a/2);
      } else if(t2 == common::BITSET){
        //cout << "a16 bs" << endl;
        count = hybrid::intersect_a16_bs((unsigned int*)R,(unsigned short*)A,(unsigned short*)B,s_a/2,s_b/2);
      } 
    } else if(t1 == common::BITSET){
      if(t2 == common::ARRAY32){
        //cout << "bs a32" << endl;
        count = hybrid::intersect_a32_bs((unsigned int*)R,(unsigned int*)B,(unsigned short*)A,s_b/4,s_a/2);
      } else if(t2 == common::ARRAY16){
        //cout << "bs a16" << endl;
        count = hybrid::intersect_a16_bs((unsigned int*)R,(unsigned short*)B,(unsigned short*)A,s_b/2,s_a/2);
      }
    }
    return count;
  }
  inline void print_data(uint8_t *data, size_t length, size_t cardinality, common::type t, std::ofstream &file){
    switch(t){
      case common::ARRAY32:
        array32::print_data((unsigned int*)data,length/4,file);
        break;
      case common::ARRAY16:
        array16::print_data((unsigned short*)data,length/2,file);
        break;
      case common::BITSET:
        bitset::print_data((unsigned short*)data,length/2,file);
        break;
      case common::A32BITPACKED:
        a32bitpacked::print_data(data,length,cardinality,file);
        break;
      case common::A32BITPACKED_DELTA:
        a32bitpacked_delta::print_data(data,length,cardinality,file);
        break;
      case common::VARIANT:
        variant::print_data(data,length,cardinality,file);
        break;
      case common::VARIANT_DELTA:
        variant_delta::print_data(data,length,cardinality,file);
        break;
      default:
        break;
    }
  }
}