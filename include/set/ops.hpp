#ifndef _OPS_H_
#define _OPS_H_

#include "Set.hpp"
#include "ops/intersection.hpp"

namespace ops{
  inline Set<uinteger> intersect(Set<uinteger> C_in, Set<uinteger> A_in, Set<uinteger> B_in){
    auto intersect_data = ops::intersect_uint_uint((uint32_t*)C_in.data,(uint32_t*)A_in.data,(uint32_t*)B_in.data,A_in.cardinality,B_in.cardinality);
    return Set<uinteger>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  inline Set<pshort> intersect(Set<pshort> C_in, Set<pshort> A_in, Set<pshort> B_in){
    auto intersect_data = ops::intersect_pshort_pshort((uint16_t*)C_in.data,(uint16_t*)A_in.data,(uint16_t*)B_in.data,A_in.number_of_bytes/sizeof(uint16_t),B_in.number_of_bytes/sizeof(uint16_t));
    return Set<pshort>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  inline Set<uinteger> intersect(Set<uinteger> C_in, Set<uinteger> A_in, Set<pshort> B_in){
    auto intersect_data = ops::intersect_uint_pshort((uint32_t*)C_in.data,(uint32_t*)A_in.data,(uint16_t*)B_in.data,A_in.cardinality,B_in.number_of_bytes/sizeof(uint16_t));
    return Set<pshort>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  inline Set<uinteger> intersect(Set<uinteger> C_in, Set<pshort> A_in, Set<uinteger> B_in){
    auto intersect_data = ops::intersect_uint_pshort((uint32_t*)C_in.data,(uint32_t*)B_in.data,(uint16_t*)A_in.data,B_in.cardinality,A_in.number_of_bytes/sizeof(uint16_t));
    return Set<pshort>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  inline Set<bitset> intersect(Set<bitset> C_in, Set<bitset> A_in, Set<bitset> B_in){
    auto intersect_data = ops::intersect_bs_bs(C_in.data,A_in.data,B_in.data,A_in.number_of_bytes,B_in.number_of_bytes);
    return Set<bitset>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  inline Set<pshort> intersect(Set<pshort> C_in, Set<pshort> A_in, Set<bitset> B_in){
    auto intersect_data = ops::intersect_pshort_bs((uint16_t*)C_in.data,(uint16_t*)A_in.data,B_in.data,A_in.number_of_bytes/sizeof(uint16_t),B_in.number_of_bytes);
    return Set<pshort>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  inline Set<pshort> intersect(Set<bitset> C_in, Set<bitset> A_in, Set<pshort> B_in){
    auto intersect_data = ops::intersect_pshort_bs((uint16_t*)C_in.data,(uint16_t*)B_in.data,A_in.data,B_in.number_of_bytes/sizeof(uint16_t),A_in.number_of_bytes);
    return Set<pshort>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  inline Set<uinteger> intersect(Set<bitset> C_in, Set<uinteger> A_in, Set<bitset> B_in){
    auto intersect_data = ops::intersect_uint_bs((uint32_t*)C_in.data,(uint32_t*)A_in.data,B_in.data,A_in.cardinality,B_in.number_of_bytes);
    return Set<uinteger>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  inline Set<uinteger> intersect(Set<bitset> C_in, Set<bitset> A_in, Set<uinteger> B_in){
    auto intersect_data = ops::intersect_uint_bs((uint32_t*)C_in.data,(uint32_t*)B_in.data,A_in.data,B_in.cardinality,A_in.number_of_bytes);
    return Set<uinteger>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }

  inline Set<hybrid> intersect(Set<hybrid> C_in, Set<hybrid> A_in, Set<hybrid> B_in){
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
}
#endif
