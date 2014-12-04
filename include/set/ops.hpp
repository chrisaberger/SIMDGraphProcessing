#ifndef _OPS_H_
#define _OPS_H_

#include "ops/intersection.hpp"
#include "ops/union.hpp"

namespace ops{
  /////////////////////////////////////////////////////////////////////////////
  //Repackage Interface
  /////////////////////////////////////////////////////////////////////////////
  /*
  inline Set<uinteger> repackage(Set<uinteger> cur, const uint8_t *new_data, const common::type type){
    (void) new_data; (void) type;
    return cur;    
  }
  inline Set<pshort> repackage(Set<pshort> cur, const uint8_t *new_data, const common::type type){
    (void) new_data; (void) type;
    return cur;    
  }
  inline Set<bitset> repackage(Set<bitset> cur, const uint8_t *new_data, const common::type type){
    (void) new_data; (void) type;
    return cur;    
  }
  inline Set<variant> repackage(Set<variant> cur, const uint8_t *new_data, const common::type type){
    (void) new_data; (void) type;
    return cur;    
  }
  inline Set<bitpacked> repackage(Set<bitpacked> cur, const uint8_t *new_data, const common::type type){
    (void) new_data; (void) type;
    return cur;    
  }
  inline Set<hybrid> repackage(Set<hybrid> cur, uint8_t *new_data){
    common::type type = hybrid::compute_type(0.1);
    if(type == cur.type){
      return cur;
    } else if(type == common::UINTEGER){
      uint32_t *R = (uint32_t*) new_data;
      size_t i = 0; 
      cur.foreach( [&i,&R] (uint32_t old_data){
        R[i++] = old_data; 
      });
      return Set<hybrid>(new_data,i,i*sizeof(uint32_t),type);
    } else if(type == common::PSHORT){
      uint16_t *R = (uint16_t*) new_data;

      size_t counter = 0;
      uint16_t high = 0;
      size_t partition_length = 0;
      size_t partition_size_position = 1;
      size_t count = 0;
      cur.foreach( [&count,&partition_length,&partition_size_position,&R,&high,&counter] (uint32_t old_data){
        uint16_t chigh = (old_data & 0xFFFF0000) >> 16; // upper dword
        uint16_t clow = old_data & 0x0FFFF;   // lower dword
        if(chigh == high && count != 0) { // add element to the current partition
          partition_length++;
          R[counter++] = clow;
        }else{ // start new partition
          R[counter++] = chigh; // partition prefix
          R[counter++] = 0;     // reserve place for partition size
          R[counter++] = clow;  // write the first element
          R[partition_size_position] = partition_length;

          partition_length = 1; // reset counters
          partition_size_position = counter - 2;
          high = chigh;
        }
        count++;
      });
      R[partition_size_position] = partition_length;
      return Set<hybrid>(new_data,count,counter*sizeof(uint16_t),type);
    } else if(type == common::BITSET){
      size_t count = 0;
      size_t word = 0;
      size_t cleared_word_index = 0;
      cur.foreach( [&count,&word,&cleared_word_index,&new_data] (uint32_t old_data){
        word = bitset_ops::word_index(old_data);
        //clear bits up to word
        while(cleared_word_index < word){
          new_data[cleared_word_index++] = 0;
        }
        cleared_word_index = word+1;

        uint8_t set_value = 1 << (old_data % BITS_PER_WORD);
        new_data[word] |= set_value; 
        count++;
      });
      return Set<hybrid>(new_data,count,word,type);
    }
    return cur;    
  }
  */
  /////////////////////////////////////////////////////////////////////////////
  //Intersection Interface
  /////////////////////////////////////////////////////////////////////////////
  //UINTEGER & UINTEGER
  /*
  //PSHORT & PSHORT 
  inline Set<pshort> set_intersect(Set<pshort> C_in, Set<pshort> A_in, Set<pshort> B_in){
    auto intersect_data = ops::intersect_pshort_pshort((uint16_t*)C_in.data,(uint16_t*)A_in.data,(uint16_t*)B_in.data,A_in.number_of_bytes/sizeof(uint16_t),B_in.number_of_bytes/sizeof(uint16_t));
    return Set<pshort>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  //UINTEGER & PSHORT
  inline Set<uinteger> set_intersect(Set<uinteger> C_in, Set<uinteger> A_in, Set<pshort> B_in){
    auto intersect_data = ops::intersect_uint_pshort((uint32_t*)C_in.data,(uint32_t*)A_in.data,(uint16_t*)B_in.data,A_in.cardinality,B_in.number_of_bytes/sizeof(uint16_t));
    return Set<pshort>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  //PSHORT & UNINTEGER
  inline Set<uinteger> set_intersect(Set<uinteger> C_in, Set<pshort> A_in, Set<uinteger> B_in){
    auto intersect_data = ops::intersect_uint_pshort((uint32_t*)C_in.data,(uint32_t*)B_in.data,(uint16_t*)A_in.data,B_in.cardinality,A_in.number_of_bytes/sizeof(uint16_t));
    return Set<pshort>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  //BITSET & BITSET
  inline Set<bitset> set_intersect(Set<bitset> C_in, Set<bitset> A_in, Set<bitset> B_in){
    auto intersect_data = ops::intersect_bs_bs(C_in.data,A_in.data,B_in.data,A_in.number_of_bytes,B_in.number_of_bytes);
    return Set<bitset>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  //PSHORT & BITSET
  inline Set<pshort> set_intersect(Set<pshort> C_in, Set<pshort> A_in, Set<bitset> B_in){
    auto intersect_data = ops::intersect_pshort_bs((uint16_t*)C_in.data,(uint16_t*)A_in.data,B_in.data,A_in.number_of_bytes/sizeof(uint16_t),B_in.number_of_bytes);
    return Set<pshort>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  //BITSET & PSHORT
  inline Set<pshort> set_intersect(Set<bitset> C_in, Set<bitset> A_in, Set<pshort> B_in){
    auto intersect_data = ops::intersect_pshort_bs((uint16_t*)C_in.data,(uint16_t*)B_in.data,A_in.data,B_in.number_of_bytes/sizeof(uint16_t),A_in.number_of_bytes);
    return Set<pshort>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  //UINTEGER & BITSET
  inline Set<uinteger> set_intersect(Set<bitset> C_in, Set<uinteger> A_in, Set<bitset> B_in){
    auto intersect_data = ops::intersect_uint_bs((uint32_t*)C_in.data,(uint32_t*)A_in.data,B_in.data,A_in.cardinality,B_in.number_of_bytes);
    return Set<uinteger>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  //BITSET & UINTEGER
  inline Set<uinteger> set_intersect(Set<bitset> C_in, Set<bitset> A_in, Set<uinteger> B_in){
    auto intersect_data = ops::intersect_uint_bs((uint32_t*)C_in.data,(uint32_t*)B_in.data,A_in.data,B_in.cardinality,A_in.number_of_bytes);
    return Set<uinteger>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }

  //HYBRID & HYBRID
  inline Set<hybrid> set_intersect(Set<hybrid> C_in, Set<hybrid> A_in, Set<hybrid> B_in){
    if(A_in.type == common::UINTEGER){
      if(B_in.type == common::UINTEGER){
        auto intersect_data = ops::intersect_uint_uint((uint32_t*)C_in.data,(uint32_t*)A_in.data,(uint32_t*)B_in.data,A_in.cardinality,B_in.cardinality);
        return Set<hybrid>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
      } else if(B_in.type == common::PSHORT){
        auto intersect_data = ops::intersect_uint_pshort((uint32_t*)C_in.data,(uint32_t*)A_in.data,(uint16_t*)B_in.data,A_in.cardinality,B_in.number_of_bytes/sizeof(uint16_t));
        return Set<hybrid>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
      } else if(B_in.type == common::BITSET){
        auto intersect_data = ops::intersect_uint_bs((uint32_t*)C_in.data,(uint32_t*)A_in.data,B_in.data,A_in.cardinality,B_in.number_of_bytes);
        return Set<hybrid>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
      }
    }
    else if(A_in.type == common::PSHORT){
      if(B_in.type == common::PSHORT){
        auto intersect_data = ops::intersect_pshort_pshort((uint16_t*)C_in.data,(uint16_t*)A_in.data,(uint16_t*)B_in.data,A_in.number_of_bytes/sizeof(uint16_t),B_in.number_of_bytes/sizeof(uint16_t));
        return Set<hybrid>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
      } else if(B_in.type == common::UINTEGER){
        auto intersect_data = ops::intersect_uint_pshort((uint32_t*)C_in.data,(uint32_t*)B_in.data,(uint16_t*)A_in.data,B_in.cardinality,A_in.number_of_bytes/sizeof(uint16_t));
        return Set<hybrid>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
      } else if(B_in.type == common::BITSET){
        auto intersect_data = ops::intersect_pshort_bs((uint16_t*)C_in.data,(uint16_t*)A_in.data,B_in.data,A_in.number_of_bytes/sizeof(uint16_t),B_in.number_of_bytes);
        return Set<hybrid>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
      }
    }
    else if(A_in.type == common::BITSET){
      if(B_in.type == common::BITSET){
        auto intersect_data = ops::intersect_bs_bs(C_in.data,A_in.data,B_in.data,A_in.number_of_bytes,B_in.number_of_bytes);
        return Set<hybrid>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
      } else if(B_in.type == common::UINTEGER){
        auto intersect_data = ops::intersect_uint_bs((uint32_t*)C_in.data,(uint32_t*)B_in.data,A_in.data,B_in.cardinality,A_in.number_of_bytes);
        return Set<hybrid>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
      } else if(B_in.type == common::PSHORT){
        auto intersect_data = ops::intersect_pshort_bs((uint16_t*)C_in.data,(uint16_t*)B_in.data,A_in.data,B_in.number_of_bytes/sizeof(uint16_t),A_in.number_of_bytes);
        return Set<hybrid>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
      }
    }
    cout << "ERROR" << endl;
    return Set<hybrid>(C_in.data,0,0,common::HYBRID_PERF);
  }

  /////////////////////////////////////////////////////////////////////////////
  //Union Interface
  /////////////////////////////////////////////////////////////////////////////
  //UINTEGER & UINTEGER
  inline Set<uinteger> set_union(Set<uinteger> C_in, Set<uinteger> A_in, Set<uinteger> B_in){
    auto intersect_data = ops::union_uint_uint((uint32_t*)C_in.data,(uint32_t*)A_in.data,(uint32_t*)B_in.data,A_in.cardinality,B_in.cardinality);
    return Set<uinteger>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  //BITSET & BITSET
  inline Set<bitset> set_union(Set<bitset> C_in, Set<bitset> A_in, Set<bitset> B_in){
    auto intersect_data = ops::union_bs_bs(C_in.data,A_in.data,B_in.data,A_in.number_of_bytes,B_in.number_of_bytes);
    return Set<bitset>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  */
}
#endif
