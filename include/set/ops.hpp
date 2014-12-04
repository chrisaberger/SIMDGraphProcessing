#ifndef _OPS_H_
#define _OPS_H_

#include "ops/intersection.hpp"
#include "ops/union.hpp"
#include "ops/repackage.hpp"

namespace ops{
  /////////////////////////////////////////////////////////////////////////////
  //Repackage Interface
  /////////////////////////////////////////////////////////////////////////////
  /*

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
  //BITSET & BITSET
  inline Set<bitset> set_union(Set<bitset> C_in, Set<bitset> A_in, Set<bitset> B_in){
    auto intersect_data = ops::union_bs_bs(C_in.data,A_in.data,B_in.data,A_in.number_of_bytes,B_in.number_of_bytes);
    return Set<bitset>(C_in.data,get<0>(intersect_data),get<1>(intersect_data),get<2>(intersect_data));
  }
  */
}
#endif
