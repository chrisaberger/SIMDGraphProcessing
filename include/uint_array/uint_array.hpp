#include "hybrid.hpp"

namespace uint_array{
  inline common::type get_perf_hybrid_array_type(unsigned int *r_data, size_t row_size, size_t matrix_size){
    double sparsity = (double) row_size/matrix_size;
    if( sparsity > (double) 1/32 ){
      return common::BITSET;
    } else if(row_size != 0 && 
      (row_size/((r_data[row_size-1] >> 16) - (r_data[0] >> 16) + 1)) > 12){
      return common::ARRAY16;
    } else if(row_size < 10){
      return common::ARRAY32;
    }else{
      return common::A32BITPACKED;
    }
  }

  inline common::type get_compressed_hybrid_array_type(unsigned int *r_data, size_t row_size, size_t matrix_size){
    (void) r_data;
    double sparsity = (double) row_size/matrix_size;
    if( sparsity > (double) 2/32 ){
      return common::BITSET;
    } else if(row_size != 0 && 
      (row_size/((r_data[row_size-1] >> 16) - (r_data[0] >> 16) + 1)) > 8 && row_size > 100){
      return common::A32BITPACKED;
    }  else{
      return common::VARIANT;
    }
  }

  inline common::type get_array_type(common::type t_stat,unsigned int *r_data, size_t len, size_t size){
    #if HYBRID_LAYOUT == 1
    if(t_stat == common::DENSE_RUNS){
      if(len < 8){
        return common::ARRAY32;
      } else{
        return common::DENSE_RUNS;
      }
    } else if(t_stat == common::HYBRID_COMP){
      return get_compressed_hybrid_array_type(r_data,len,size);
    } else if(t_stat == common::HYBRID_PERF){
      return get_perf_hybrid_array_type(r_data,len,size);
    } else{
      return t_stat;
    }
    #else
    (void)r_data;
    (void)len;
    (void)size;
    return t_stat;
    #endif
  }

  inline size_t preprocess(uint8_t *data, size_t index, unsigned int *data_in, size_t length_in, size_t mat_size, common::type t){
    #if HYBRID_LAYOUT == 1
    data[index++] = t;
    #endif

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
      case common::VARIANT:
        index += variant::preprocess((data+index),data_in,length_in);
        break;
      case common::DENSE_RUNS:
        index += hybrid::preprocess((data+index),data_in,length_in,mat_size);
      default:
        break;
    }
    return index;
  }
  template<typename T> 
  inline T sum(std::function<T(unsigned int,unsigned int,unsigned int*)> function,unsigned int col,uint8_t *data,size_t length, size_t card,common::type t, unsigned int *outputA){
    #if HYBRID_LAYOUT == 1
    t = (common::type) data[0];
    data++;
    length--;
    #endif

    T result;
    switch(t){
      case common::ARRAY32:
        result = array32::sum(function,col,(unsigned int*)data,length/4,outputA);
        break;
      case common::ARRAY16:
        result = array16::sum(function,col,(unsigned short*)data,length/2,outputA);
        break;
      case common::BITSET:
        result = bitset::sum(function,col,(unsigned short*)data,length/2,outputA);
        break;
      case common::A32BITPACKED:
        outputA = new unsigned int[card];
        a32bitpacked::decode(outputA,data,card);
        result = array32::sum(function,col,outputA,card,outputA);
        delete[] outputA;
        break;
      case common::VARIANT:
        outputA = new unsigned int[card];
        variant::decode(outputA,data,card);
        result = array32::sum(function,col,outputA,card,outputA);
        delete[] outputA;
        break;
      default:
        return (T) 0;
        break;
    }
    return result;
  } 
  inline size_t intersect_homogeneous(uint8_t *R, uint8_t *A, uint8_t *B, size_t s_a, size_t s_b, 
    unsigned int card_a, unsigned int card_b, common::type t,unsigned int *outputA){
    size_t count = 0;
    unsigned int *outputB;

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
      case common::A32BITPACKED:
        outputB = new unsigned int[card_b];
        a32bitpacked::decode(outputB,B,card_b);
        count = array32::intersect((unsigned int*)R,outputA,outputB,card_a,card_b);
        delete[] outputB;
        break;
      case common::VARIANT:
        outputB = new unsigned int[card_b];
        variant::decode(outputB,B,card_b);
        count = array32::intersect((unsigned int*)R,outputA,outputB,card_a,card_b);
        delete[] outputB;
        break;
      default:
        break;
    }
    return count;
  }

  inline size_t intersect_heterogenous(uint8_t *R, uint8_t *A, uint8_t *B,
    size_t s_a, size_t s_b, unsigned int card_a, unsigned int card_b,
    common::type t1, common::type t2, unsigned int *outputA){
    size_t count = 0;
    if(t1 == common::ARRAY32){
      if(t2 == common::ARRAY16){
        //cout << "a32 a16" << endl;
        count = hybrid::intersect_a32_a16((unsigned int*)R,(unsigned int*)A,(unsigned short*)B,s_a/4,s_b/2);
      } else if(t2 == common::BITSET){
        //cout << "a32 bs" << endl;
        count = hybrid::intersect_a32_bs((unsigned int*)R,(unsigned int*)A,(unsigned short*)B,s_a/4,s_b/2);
      } 
      #if COMPRESSION == 1
      else if(t2 == common::VARIANT){
        unsigned int *outputB = new unsigned int[card_b];
        variant::decode(outputB,B,card_b);
        count = array32::intersect((unsigned int*)R,(unsigned int*)A,outputB,s_a/4,card_b);
        delete[] outputB;
      } else if(t2 == common::A32BITPACKED){
        unsigned int *outputB = new unsigned int[card_b];
        a32bitpacked::decode(outputB,B,card_b);
        count = array32::intersect((unsigned int*)R,(unsigned int*)A,outputB,s_a/4,card_b);
        delete[] outputB;
      }
      #endif
    } else if(t1 == common::ARRAY16){
      if(t2 == common::ARRAY32){
        //cout << "a32 a16" << endl;
        count = hybrid::intersect_a32_a16((unsigned int*)R,(unsigned int*)B,(unsigned short*)A,s_b/4,s_a/2);
      } else if(t2 == common::BITSET){
        //cout << "a16 bs" << endl;
        count = hybrid::intersect_a16_bs((unsigned int*)R,(unsigned short*)A,(unsigned short*)B,s_a/2,s_b/2);
      } 
      #if COMPRESSION == 1
      else if(t2 == common::VARIANT){
        unsigned int *outputB = new unsigned int[card_b];
        variant::decode(outputB,B,card_b);
        count = hybrid::intersect_a32_a16((unsigned int*)R,outputB,(unsigned short*)A,card_b,s_a/2);
        delete[] outputB;
      } else if(t2 == common::A32BITPACKED){
        unsigned int *outputB = new unsigned int[card_b];
        a32bitpacked::decode(outputB,B,card_b);
        count = hybrid::intersect_a32_a16((unsigned int*)R,outputB,(unsigned short*)A,card_b,s_a/2);
        delete[] outputB;
      }
      #endif
    } else if(t1 == common::BITSET){
      if(t2 == common::ARRAY32){
        //cout << "bs a32" << endl;
        count = hybrid::intersect_a32_bs((unsigned int*)R,(unsigned int*)B,(unsigned short*)A,s_b/4,s_a/2);
      } else if(t2 == common::ARRAY16){
        //cout << "bs a16" << endl;
        count = hybrid::intersect_a16_bs((unsigned int*)R,(unsigned short*)B,(unsigned short*)A,s_b/2,s_a/2);
      } 
      #if COMPRESSION == 1
      else if(t2 == common::VARIANT){
        unsigned int *outputB = new unsigned int[card_b];
        variant::decode(outputB,B,card_b);
        count = hybrid::intersect_a32_bs((unsigned int*)R,outputB,(unsigned short*)A,card_b,s_a/2);
        delete[] outputB;
      } else if(t2 == common::A32BITPACKED){
        unsigned int *outputB = new unsigned int[card_b];
        a32bitpacked::decode(outputB,B,card_b);
        count = hybrid::intersect_a32_bs((unsigned int*)R,outputB,(unsigned short*)A,card_b,s_a/2);
        delete[] outputB;
      }
      #endif
    } 
    #if COMPRESSION == 1
    else if(t1 == common::A32BITPACKED){
      if(t2 == common::ARRAY32){
        count = array32::intersect((unsigned int*)R,outputA,(unsigned int*)B,card_a,s_b/4);
      } else if(t2 == common::ARRAY16){
        count = hybrid::intersect_a32_a16((unsigned int*)R,outputA,(unsigned short*)B,card_a,s_b/2);
      } else if(t2 == common::VARIANT){
        unsigned int *outputB = new unsigned int[card_b];
        variant::decode(outputB,B,card_b);
        count = array32::intersect((unsigned int*)R,outputA,outputB,card_a,card_b);
        delete[] outputB;
      } else if(t2 == common::BITSET){
        count = hybrid::intersect_a32_bs((unsigned int*)R,outputA,(unsigned short*)B,card_a,s_b/2);
      } 
    } else if(t1 == common::VARIANT){
      if(t2 == common::ARRAY32){
        count = array32::intersect((unsigned int*)R,outputA,(unsigned int*)B,card_a,s_b/4);
      } else if(t2 == common::ARRAY16){
        count = hybrid::intersect_a32_a16((unsigned int*)R,outputA,(unsigned short*)B,card_a,s_b/2);
      } else if(t2 == common::A32BITPACKED){
        unsigned int *outputB = new unsigned int[card_b];
        a32bitpacked::decode(outputB,B,card_b);
        count = array32::intersect((unsigned int*)R,outputA,outputB,card_a,card_b);
        delete[] outputB;
      } else if(t2 == common::BITSET){
        count = hybrid::intersect_a32_bs((unsigned int*)R,outputA,(unsigned short*)B,card_a,s_b/2);
      } 
    }
    #else
    (void) card_a; (void) card_b; (void) outputA;
    #endif

    return count;
  }
  inline size_t intersect(uint8_t *R, uint8_t *A, uint8_t *B, size_t s_a, size_t s_b, 
    unsigned int card_a, unsigned int card_b, common::type t, unsigned int *outputA){

    #if HYBRID_LAYOUT == 1
    (void) t;
    const common::type t1 = (common::type) A[0];
    const common::type t2 = (common::type) B[0];
    s_a--; s_b--;
    if(t1 == t2){
      return uint_array::intersect_homogeneous(R,++A,++B,s_a,s_b,card_a,card_b,t1,outputA);
    } else{
      return uint_array::intersect_heterogenous(R,++A,++B,s_a,s_b,card_a,card_b,t1,t2,outputA);
    }
    #else 
    return uint_array::intersect_homogeneous(R,A,B,s_a,s_b,card_a,card_b,t,outputA);
    #endif
  }
  /*
  inline size_t union(uint8_t *R, uint8_t *A, uint8_t *B, size_t s_a, size_t s_b, unsigned int card_a, unsigned int card_b, common::type t){
    #if HYBRID_LAYOUT == 1
    (void) t;
    const common::type t1 = (common::type) A[0];
    const common::type t2 = (common::type) B[0];
    s_a--; s_b--;
    if(t1 == t2){
      return array32::union((unsigned int*)R,(unsigned int*)A,(unsigned int*)B,s_a/4,s_b/4);
    } else{
      return 0;
    }
    #else 
    return 0;
    #endif
  }
  */

  inline void decode(unsigned int *result, uint8_t *data, size_t length, size_t cardinality, common::type t){
    #if HYBRID_LAYOUT == 1
    t = (common::type) data[0];
    data++;
    length--;
    #endif

    switch(t){
      case common::ARRAY32:
        std::copy((unsigned int*)data,(unsigned int*)data+(length/4),result);
        break;
      case common::ARRAY16:
        array16::decode(result,(unsigned short*)data,length/2);
        break;
      case common::BITSET:
        bitset::decode(result,(unsigned short*)data,length/2);
        break;
      case common::A32BITPACKED:
        a32bitpacked::decode(result,data,cardinality);
        break;
      case common::VARIANT:
        variant::decode(result,data,cardinality);
        break;
      default:
        break;
    }
  }
  inline void print_data(uint8_t *data, size_t length, size_t cardinality, common::type t, std::ofstream &file){
    #if HYBRID_LAYOUT == 1
    t = (common::type) data[0];
    data++;
    length--;
    #endif

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
      case common::VARIANT:
        variant::print_data(data,length,cardinality,file);
        break;
      case common::DENSE_RUNS:
        hybrid::print_data(data,length,cardinality,file);
        break;
      default:
        break;
    }
  }
}