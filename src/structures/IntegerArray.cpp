#include "IntegerArray.hpp"
/*
  Most of the implementation is here.  This is designed with matrices in mind.
  I use a namespace so I avoid storing or creating multiple objects when 
  using matrices.
*/

class Matrix;

namespace integerarray{
  inline size_t preprocess(unsigned short *data, size_t index, unsigned int *data_in, size_t length_in, common::type t){
    switch(t){
      case common::ARRAY32:
        index = array32::preprocess(data,index,data_in,length_in);
        break;
      case common::ARRAY16:
        index = array16::preprocess(data,index,data_in,length_in);
        break;
      case common::BITSET:
        index = bitset::preprocess(data,index,data_in,length_in);
        break;
      default:
        break;
    }
    return index;
  }

  template<typename T> 
  inline T foreach(T (*function)(unsigned int,unsigned int),unsigned int col,unsigned short *data,size_t length,common::type t){
    switch(t){
      case common::ARRAY32:
        return array32::foreach(function,col,data,length);
        break;
      case common::ARRAY16:
        return array16::foreach(function,col,data,length);
        break;
      case common::BITSET:
        return bitset::foreach(function,col,data,length);
        break;
      default:
        return (T) 0;
        break;
    }
  }
  inline size_t intersect(unsigned short *R, unsigned short *A, unsigned short *B, size_t s_a, size_t s_b, common::type t){
    size_t count = 0;
    switch(t){
      case common::ARRAY32:
        count = array32::intersect((unsigned int*)R,(unsigned int*)A,(unsigned int*)B,s_a/2,s_b/2);
        break;
      case common::ARRAY16:
        count = array16::intersect(R,A,B,s_a,s_b);
        break;
      case common::BITSET:
        count = bitset::intersect(R,A,B,s_a,s_b);
        break;
      default:
        break;
    }
    return count;
  }
  inline size_t intersect(unsigned short *R, unsigned short *A, unsigned short *B, size_t s_a, size_t s_b, common::type t1, common::type t2){
    size_t count = 0;
    if(t1 == common::ARRAY32){
      if(t2 == common::ARRAY16){
        //cout << "a32 a16" << endl;
        count = hybrid::intersect_a32_a16(R,A,B,s_a,s_b);
      } else if(t2 == common::BITSET){
        //cout << "a32 bs" << endl;
        count = hybrid::intersect_a32_bs(R,A,B,s_a,s_b);
      } 
    } else if(t1 == common::ARRAY16){
      if(t2 == common::ARRAY32){
        //cout << "a32 a16" << endl;
        count = hybrid::intersect_a32_a16(R,B,A,s_b,s_a);
      } else if(t2 == common::BITSET){
        //cout << "a16 bs" << endl;
        count = hybrid::intersect_a16_bs(R,A,B,s_a,s_b);
      } 
    } else if(t1 == common::BITSET){
      if(t2 == common::ARRAY32){
        //cout << "bs a32" << endl;
        count = hybrid::intersect_a32_bs(R,B,A,s_b,s_a);
      } else if(t2 == common::ARRAY16){
        //cout << "bs a16" << endl;
        count = hybrid::intersect_a16_bs(R,B,A,s_b,s_a);
      }
    }
    return count;
  }
  inline void print_data(unsigned short *data, size_t length, common::type t){
    switch(t){
      case common::ARRAY32:
        array32::print_data(data,length);
        break;
      case common::ARRAY16:
        array16::print_data(data,length);
        break;
      case common::BITSET:
        bitset::print_data(data,length);
        break;
      default:
        break;
    }
  }
}