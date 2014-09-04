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
        index = Array32::preprocess(data,index,data_in,length_in);
        break;
      case common::ARRAY16:
        index = Array16::preprocess(data,index,data_in,length_in);
        break;
      case common::BITSET:
        break;
      default:
        break;
    }
    return index;
  }

  template<typename T> 
  inline T foreach(T (*function)(unsigned int,unsigned int,Matrix*),unsigned int col,Matrix *m, unsigned short *data,size_t length,common::type t){
    switch(t){
      case common::ARRAY32:
        return Array32::foreach(function,col,m,data,length);
        break;
      case common::ARRAY16:
        return Array16::foreach(function,col,m,data,length);
        break;
      case common::BITSET:
        return (T) 0;
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
        count = Array32::intersect((unsigned int*)R,(unsigned int*)A,(unsigned int*)B,s_a/2,s_b/2);
        break;
      case common::ARRAY16:
        count = Array16::intersect(R,A,B,s_a,s_b);
        break;
      case common::BITSET:
        break;
      default:
        break;
    }
    return count;
  }
  template<typename T> 
  inline T reduce(common::type t,unsigned short *data, size_t length,T (*function)(T,T), T zero){
    T result;

    switch(t){
      case common::ARRAY32:
        result = Array32::reduce(data,length,function,zero);
        break;
      case common::ARRAY16:
        break;
      case common::BITSET:
        break;
      default:
        break;
    }

    return result;
  }
  inline void print_data(common::type t, unsigned short *data, size_t length){
    switch(t){
      case common::ARRAY32:
        Array32::print_data(data,length);
        break;
      case common::ARRAY16:
        Array16::print_data(data,length);
        break;
      case common::BITSET:
        break;
      default:
        break;
    }
  }
}

/*
  For the actual object.
*/
IntegerArray::IntegerArray(unsigned int *data_in, size_t length_in, common::type t_in){
  t = t_in;
  length = length_in;
  data = new unsigned short[length_in*3];
  physical_size = integerarray::preprocess(data,0,data_in,length_in,t_in);
}
template<typename T> 
T IntegerArray::reduce(T (*function)(T,T), T zero){
  return integerarray::reduce(t,data,length,function,zero);
}
void IntegerArray::print_data(){
  return integerarray::print_data(t,data,length);
}

