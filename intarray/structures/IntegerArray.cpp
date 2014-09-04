#include "IntegerArray.hpp"
/*
  Most of the implementation is here.  This is designed with matrices in mind.
  I use a namespace so I avoid storing or creating multiple objects when 
  using matrices.
*/

class Matrix;

namespace integerarray{
  inline size_t preprocess(short *data, size_t index, int *data_in, size_t length_in, common::type t){
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

  inline void foreach(void (*function)(int,int,Matrix*),int col,Matrix *m, short *data,size_t length,common::type t){
    switch(t){
      case common::ARRAY32:
        Array32::foreach(function,col,m,data,length);
        break;
      case common::ARRAY16:
        Array16::foreach(function,col,m,data,length);
        break;
      case common::BITSET:
        break;
      default:
        break;
    }
  }
  inline size_t intersect(short *R, short *A, short *B, size_t s_a, size_t s_b, common::type t){
    size_t count = 0;
    switch(t){
      case common::ARRAY32:
        count = Array32::intersect((int*)R,(int*)A,(int*)B,s_a/2,s_b/2);
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
  inline T reduce(common::type t,short *data, size_t length,T (*function)(T,T), T zero){
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
  inline void print_data(common::type t, short *data, size_t length){
    switch(t){
      case common::ARRAY32:
        Array32::print_data(data,length);
        break;
      case common::ARRAY16:
        //Array16::
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
IntegerArray::IntegerArray(int *data_in, size_t length_in, common::type t_in){
  t = t_in;
  length = length_in;
  data = new short[length_in*3];
  physical_size = integerarray::preprocess(data,0,data_in,length_in,t_in);
}
template<typename T> 
T IntegerArray::reduce(T (*function)(T,T), T zero){
  return integerarray::reduce(t,data,length,function,zero);
}
void IntegerArray::print_data(){
  return integerarray::print_data(t,data,length);
}

