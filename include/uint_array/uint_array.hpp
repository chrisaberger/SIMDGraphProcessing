#include "hybrid.hpp"

namespace uint_array{
  inline common::type get_perf_hybrid_array_type(uint32_t *r_data, size_t row_size, size_t matrix_size){
    double sparsity = (double) row_size/matrix_size;
    if( sparsity > (double) 1/32 ){
      return common::BITSET;
    } else if(row_size != 0 && 
      (row_size/((r_data[row_size-1] >> 16) - (r_data[0] >> 16) + 1)) > 12){
      return common::ARRAY16;
    } else {
      return common::ARRAY32;
    }
  }

  inline common::type get_compressed_hybrid_array_type(uint32_t *r_data, size_t row_size, size_t matrix_size){
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

  inline common::type get_array_type(common::type t_stat,uint32_t *r_data, size_t len, size_t size){
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

  inline size_t preprocess(uint8_t *data, size_t index, uint32_t *data_in, size_t length_in, common::type t){
    #if HYBRID_LAYOUT == 1
    data[index++] = t;    
    #endif

    size_t *size_ptr;
    size_t start_index = index+sizeof(size_t);
    switch(t){
      case common::ARRAY32:
        index += array32::preprocess((uint32_t*)(data+index),data_in,length_in);
        break;
      case common::ARRAY16:
        size_ptr = (size_t*)&data[index];
        index += sizeof(size_t);
        index += array16::preprocess((unsigned short*)(data+index),data_in,length_in);
        size_ptr[0] = (index-start_index);
        break;
      case common::BITSET:
        size_ptr = (size_t*)&data[index];
        index += sizeof(size_t);
        index += bitset::preprocess((unsigned short*)(data+index),data_in,length_in);
        size_ptr[0] = (index-start_index);
        break;
      case common::A32BITPACKED:
        size_ptr = (size_t*)&data[index];
        index += sizeof(size_t);
        index += a32bitpacked::preprocess((data+index),data_in,length_in);
        size_ptr[0] = (index-start_index);
        break;
      case common::VARIANT:
        size_ptr = (size_t*)&data[index];
        index += sizeof(size_t);
        index += variant::preprocess((data+index),data_in,length_in);
        size_ptr[0] = (index-start_index);
        break;
        /*
      case common::DENSE_RUNS:
        index += hybrid::preprocess((data+index),data_in,length_in,mat_size);
      */
      default:
        break;
    }
    return index;
  }
  template<typename T> 
  inline T sum(std::function<T(uint32_t,uint32_t)> function,uint32_t col,uint8_t *data, size_t card,common::type t, uint32_t *outputA){
    #if HYBRID_LAYOUT == 1
    t = (common::type) data[0];
    data++;
    #endif

    T result;
    size_t *size_ptr;
    switch(t){
      case common::ARRAY32:
        result = array32::sum(function,col,(uint32_t*)data,card);
        break;
      case common::ARRAY16:
        size_ptr = (size_t*)&data[0];
        data += sizeof(size_t);
        result = array16::sum(function,col,(unsigned short*)data,size_ptr[0]/sizeof(short));
        break;
      case common::BITSET:
        size_ptr = (size_t*)&data[0];
        data += sizeof(size_t);
        result = bitset::sum(function,col,(unsigned short*)data,size_ptr[0]/sizeof(short));
        break;
      case common::A32BITPACKED:
        data += sizeof(size_t);
        a32bitpacked::decode(outputA,data,card);
        result = array32::sum(function,col,outputA,card);
        break;
      case common::VARIANT:
        data += sizeof(size_t);
        variant::decode(outputA,data,card);
        result = array32::sum(function,col,outputA,card);
        break;
      default:
        return (T) 0;
        break;
    }
    return result;
  } 
  template<typename T> 
  inline T sum(std::function<T(uint32_t)> function,uint8_t *data,size_t card,common::type t){
    #if HYBRID_LAYOUT == 1
    t = (common::type) data[0];
    data++;
    #endif

    card = card;

    T result;
    switch(t){
      case common::ARRAY32:
        result = array32::sum(function,(uint32_t*)data,card);
        break;
        /*
      case common::ARRAY16:
        result = array16::sum(function,col,(unsigned short*)data,length/2,outputA);
        break;
      case common::BITSET:
        result = bitset::sum(function,col,(unsigned short*)data,length/2,outputA);
        break;
      case common::A32BITPACKED:
        outputA = new uint32_t[card];
        a32bitpacked::decode(outputA,data,card);
        result = array32::sum(function,col,outputA,card,outputA);
        delete[] outputA;
        break;
      case common::VARIANT:
        outputA = new uint32_t[card];
        variant::decode(outputA,data,card);
        result = array32::sum(function,col,outputA,card,outputA);
        delete[] outputA;
        break;
        */
      default:
        return (T) 0;
        break;
    }
    return result;
  } 
  inline size_t intersect_homogeneous(uint8_t *R, uint8_t *A, uint8_t *B,
    uint32_t card_a, uint32_t card_b, common::type t,uint32_t *outputA){
    size_t count = 0;
    uint32_t *outputB;
 
    size_t *size_ptr_a; size_t *size_ptr_b;
    switch(t){
      case common::ARRAY32:
        count = array32::intersect(R,(uint32_t*)A,(uint32_t*)B,card_a,card_b);
        break;
      case common::ARRAY16:
        size_ptr_a = (size_t*)&A[0];
        A += sizeof(size_t);
        size_ptr_b = (size_t*)&B[0];
        B += sizeof(size_t);
        count = array16::intersect(R,(unsigned short*)A,(unsigned short*)B,size_ptr_a[0]/sizeof(short),size_ptr_b[0]/sizeof(short));
        break;
      case common::BITSET:
        size_ptr_a = (size_t*)&A[0];
        A += sizeof(size_t);
        size_ptr_b = (size_t*)&B[0];
        B += sizeof(size_t);
        count = bitset::intersect(R,(unsigned short*)A,(unsigned short*)B,size_ptr_a[0]/sizeof(short),size_ptr_b[0]/sizeof(short));
        break;
      case common::A32BITPACKED:
        B += sizeof(size_t);
        outputB = new uint32_t[card_b];
        a32bitpacked::decode(outputB,B,card_b);
        count = array32::intersect(R,outputA,outputB,card_a,card_b);
        delete[] outputB;
        break;
      case common::VARIANT:
        B += sizeof(size_t);
        outputB = new uint32_t[card_b];
        variant::decode(outputB,B,card_b);
        count = array32::intersect(R,outputA,outputB,card_a,card_b);
        delete[] outputB;
        break;
      default:
        break;
    }
    return count;
  }

  inline size_t intersect_heterogenous(uint8_t *R, uint8_t *A, uint8_t *B, uint32_t card_a, uint32_t card_b,
    common::type t1, common::type t2, uint32_t *outputA){

    size_t *size_ptr_a; size_t *size_ptr_b;
    size_t count = 0;
    if(t1 == common::ARRAY32){
      if(t2 == common::ARRAY16){
        size_ptr_b = (size_t*)&B[0];
        B += sizeof(size_t);
        count = hybrid::intersect_a32_a16(R,(uint32_t*)A,(unsigned short*)B,card_a,size_ptr_b[0]/sizeof(short));
      } else if(t2 == common::BITSET){
        size_ptr_b = (size_t*)&B[0];
        B += sizeof(size_t);
        count = hybrid::intersect_a32_bs(R,(uint32_t*)A,(unsigned short*)B,card_a,size_ptr_b[0]/sizeof(short));
      } 
      #if COMPRESSION == 1
      else if(t2 == common::VARIANT){
        B += sizeof(size_t);
        uint32_t *outputB = new uint32_t[card_b];
        variant::decode(outputB,B,card_b);
        count = array32::intersect(R,(uint32_t*)A,outputB,card_a,card_b);
        delete[] outputB;
      } else if(t2 == common::A32BITPACKED){
        B += sizeof(size_t);
        uint32_t *outputB = new uint32_t[card_b];
        a32bitpacked::decode(outputB,B,card_b);
        count = array32::intersect(R,(uint32_t*)A,outputB,card_a,card_b);
        delete[] outputB;
      }
      #endif
    } else if(t1 == common::ARRAY16){
      if(t2 == common::ARRAY32){
        size_ptr_a = (size_t*)&A[0];
        A += sizeof(size_t);
        count = hybrid::intersect_a32_a16(R,(uint32_t*)B,(unsigned short*)A,card_b,size_ptr_a[0]/sizeof(short));
      } else if(t2 == common::BITSET){
        size_ptr_a = (size_t*)&A[0];
        A += sizeof(size_t);
        size_ptr_b = (size_t*)&B[0];
        B += sizeof(size_t);
        count = hybrid::intersect_a16_bs(R,(unsigned short*)A,(unsigned short*)B,size_ptr_a[0]/sizeof(short),size_ptr_b[0]/sizeof(short));
      } 
      #if COMPRESSION == 1
      else if(t2 == common::VARIANT){
        B += sizeof(size_t);
        size_ptr_a = (size_t*)&A[0];
        A += sizeof(size_t);
        uint32_t *outputB = new uint32_t[card_b];
        variant::decode(outputB,B,card_b);
        count = hybrid::intersect_a32_a16(R,outputB,(unsigned short*)A,card_b,size_ptr_a[0]/sizeof(short));
        delete[] outputB;
      } else if(t2 == common::A32BITPACKED){
        B += sizeof(size_t);
        size_ptr_a = (size_t*)&A[0];
        A += sizeof(size_t);
        uint32_t *outputB = new uint32_t[card_b];
        a32bitpacked::decode(outputB,B,card_b);
        count = hybrid::intersect_a32_a16(R,outputB,(unsigned short*)A,card_b,size_ptr_a[0]/sizeof(short));
        delete[] outputB;
      }
      #endif
    } else if(t1 == common::BITSET){
      if(t2 == common::ARRAY32){
        size_ptr_a = (size_t*)&A[0];
        A += sizeof(size_t);
        count = hybrid::intersect_a32_bs(R,(uint32_t*)B,(unsigned short*)A,card_b,size_ptr_a[0]/sizeof(short));
      } else if(t2 == common::ARRAY16){
        size_ptr_a = (size_t*)&A[0];
        A += sizeof(size_t);
        size_ptr_b = (size_t*)&B[0];
        B += sizeof(size_t);
        count = hybrid::intersect_a16_bs(R,(unsigned short*)B,(unsigned short*)A,size_ptr_b[0]/sizeof(short),size_ptr_a[0]/sizeof(short));
      } 
      #if COMPRESSION == 1
      else if(t2 == common::VARIANT){
        B += sizeof(size_t);
        size_ptr_a = (size_t*)&A[0];
        A += sizeof(size_t);
        uint32_t *outputB = new uint32_t[card_b];
        variant::decode(outputB,B,card_b);
        count = hybrid::intersect_a32_bs(R,outputB,(unsigned short*)A,card_b,size_ptr_a[0]/sizeof(short));
        delete[] outputB;
      } else if(t2 == common::A32BITPACKED){
        B += sizeof(size_t);
        size_ptr_a = (size_t*)&A[0];
        A += sizeof(size_t);
        uint32_t *outputB = new uint32_t[card_b];
        a32bitpacked::decode(outputB,B,card_b);
        count = hybrid::intersect_a32_bs(R,outputB,(unsigned short*)A,card_b,size_ptr_a[0]/sizeof(short));
        delete[] outputB;
      }
      #endif
    } 
    #if COMPRESSION == 1
    else if(t1 == common::A32BITPACKED){
      if(t2 == common::ARRAY32){
        A += sizeof(size_t);
        count = array32::intersect(R,outputA,(uint32_t*)B,card_a,card_b);
      } else if(t2 == common::ARRAY16){
        A += sizeof(size_t);
        size_ptr_b = (size_t*)&B[0];
        B += sizeof(size_t);
        count = hybrid::intersect_a32_a16(R,outputA,(unsigned short*)B,card_a,size_ptr_b[0]/sizeof(short));
      } else if(t2 == common::VARIANT){
        A += sizeof(size_t);
        B += sizeof(size_t);
        uint32_t *outputB = new uint32_t[card_b];
        variant::decode(outputB,B,card_b);
        count = array32::intersect(R,outputA,outputB,card_a,card_b);
        delete[] outputB;
      } else if(t2 == common::BITSET){
        A += sizeof(size_t);
        size_ptr_b = (size_t*)&B[0];
        B += sizeof(size_t);
        count = hybrid::intersect_a32_bs(R,outputA,(unsigned short*)B,card_a,size_ptr_b[0]/sizeof(short));
      } 
    } else if(t1 == common::VARIANT){
      if(t2 == common::ARRAY32){
        A += sizeof(size_t);
        count = array32::intersect(R,outputA,(uint32_t*)B,card_a,card_b);
      } else if(t2 == common::ARRAY16){
        A += sizeof(size_t);
        size_ptr_b = (size_t*)&B[0];
        B += sizeof(size_t);
        count = hybrid::intersect_a32_a16(R,outputA,(unsigned short*)B,card_a,size_ptr_b[0]/sizeof(short));
      } else if(t2 == common::A32BITPACKED){
        A += sizeof(size_t);
        B += sizeof(size_t);
        uint32_t *outputB = new uint32_t[card_b];
        a32bitpacked::decode(outputB,B,card_b);
        count = array32::intersect(R,outputA,outputB,card_a,card_b);
        delete[] outputB;
      } else if(t2 == common::BITSET){
        A += sizeof(size_t);
        size_ptr_b = (size_t*)&B[0];
        B += sizeof(size_t);
        count = hybrid::intersect_a32_bs(R,outputA,(unsigned short*)B,card_a,size_ptr_b[0]/sizeof(short));
      } 
    }
    #else
    (void) outputA;
    #endif


    return count;
  }
  inline size_t intersect(uint8_t *R, uint8_t *A, uint8_t *B,
    uint32_t card_a, uint32_t card_b, common::type t, uint32_t *outputA){

    #if HYBRID_LAYOUT == 1
    (void) t;
    const common::type t1 = (common::type) A[0];
    const common::type t2 = (common::type) B[0];
    if(t1 == t2){
      return uint_array::intersect_homogeneous(R,++A,++B,card_a,card_b,t1,outputA);
    } else{
      return uint_array::intersect_heterogenous(R,++A,++B,card_a,card_b,t1,t2,outputA);
    }
    #else 
    return uint_array::intersect_homogeneous(R,A,B,card_a,card_b,t,outputA);
    #endif
  }
  inline size_t union_homogeneous(uint8_t *R, uint8_t *A, uint8_t *B, uint32_t card_a, uint32_t card_b,common::type t){
    size_t count = 0;
    //uint32_t *outputB;
 
    size_t *size_ptr_a; size_t *size_ptr_b;
    switch(t){
      case common::ARRAY32:
        count = array32::set_union(R,(uint32_t*)A,(uint32_t*)B,card_a,card_b);
        break;
      case common::ARRAY16:
        size_ptr_a = (size_t*)&A[0];
        A += sizeof(size_t);
        size_ptr_b = (size_t*)&B[0];
        B += sizeof(size_t);
        count = array16::set_union(R,(unsigned short*)A,(unsigned short*)B,size_ptr_a[0]/sizeof(short),size_ptr_b[0]/sizeof(short));
        break;
        /*
      case common::BITSET:
        size_ptr_a = (size_t*)&A[0];
        A += sizeof(size_t);
        size_ptr_b = (size_t*)&B[0];
        B += sizeof(size_t);
        count = bitset::intersect(R,(unsigned short*)A,(unsigned short*)B,size_ptr_a[0]/sizeof(short),size_ptr_b[0]/sizeof(short));
        break;
      case common::A32BITPACKED:
        outputB = new uint32_t[card_b];
        a32bitpacked::decode(outputB,B,card_b);
        count = array32::intersect(R,outputA,outputB,card_a,card_b);
        delete[] outputB;
        break;
      case common::VARIANT:
        outputB = new uint32_t[card_b];
        variant::decode(outputB,B,card_b);
        count = array32::intersect(R,outputA,outputB,card_a,card_b);
        delete[] outputB;
        break;
        */
      default:
        break;
    }
    return count;
  }

  inline size_t set_union(uint8_t *R, uint8_t *A, uint8_t *B, uint32_t card_a, uint32_t card_b, common::type t){
    #if HYBRID_LAYOUT == 1
    (void) t;
    const common::type t1 = (common::type) A[0];
    const common::type t2 = (common::type) B[0];

    if(t1 == t2){
      return union_homogeneous(R,++A,++B,card_a,card_b,t1);
    } else{
      return 0;
    }
    #else 
    return union_homogeneous(R,++A,++B,card_a,card_b,t);
    #endif  
  }

  inline void decode(uint32_t *result, uint8_t *data, long card, common::type t){
    #if HYBRID_LAYOUT == 1
    t = (common::type) data[0];
    data++;
    #endif

    switch(t){
      case common::ARRAY32:
        std::copy((uint32_t*)data,(uint32_t*)data+card,result);
        break;
      case common::ARRAY16:
        data += sizeof(size_t);
        array16::decode(result,(unsigned short*)data,card);
        break;
      case common::BITSET:
        data += sizeof(size_t);
        bitset::decode(result,(unsigned short*)data,card);
        break;
      case common::A32BITPACKED:
        data += sizeof(size_t);
        a32bitpacked::decode(result,data,card);
        break;
      case common::VARIANT:
        data += sizeof(size_t);
        variant::decode(result,data,card);
        break;
      default:
        break;
    }
  }
  inline void print_data(uint8_t *data, size_t cardinality, common::type t, std::ofstream &file){
    #if HYBRID_LAYOUT == 1
    t = (common::type) data[0];
    data++;
    #endif
    
    size_t *size_ptr;
    switch(t){
      case common::ARRAY32:
        array32::print_data((uint32_t*)data,cardinality,file);
        break;
      case common::ARRAY16:
        size_ptr = (size_t*)&data[0];
        data += sizeof(size_t);
        array16::print_data((unsigned short*)data,size_ptr[0]/sizeof(short),file);
        break;
      case common::BITSET:
        size_ptr = (size_t*)&data[0];
        data += sizeof(size_t);
        bitset::print_data((unsigned short*)data,size_ptr[0]/sizeof(short),file);
        break;
      case common::A32BITPACKED:
        data += sizeof(size_t);
        a32bitpacked::print_data(data,cardinality,file);
        break;
      case common::VARIANT:
        data += sizeof(size_t);
        variant::print_data(data,cardinality,file);
        break;
        /*
      case common::DENSE_RUNS:
        hybrid::print_data(data,length,cardinality,file);
        break;
        */
      default:
        break;
    }
  }
}