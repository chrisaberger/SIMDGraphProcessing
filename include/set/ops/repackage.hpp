#ifndef _REPACKAGE_H_
#define _REPACKAGE_H_

namespace ops{
  template <class T>
  inline Set<uinteger> repackage_as_uinteger(Set<T> cur, uint8_t *new_data){
    uint32_t *R = (uint32_t*) new_data;
    size_t i = 0; 
    cur.foreach( [&i,&R] (uint32_t old_data){
      R[i++] = old_data; 
    });
    return Set<uinteger>(new_data,i,i*sizeof(uint32_t),cur.density,common::UINTEGER);
  }
  template <class T>
  inline Set<pshort> repackage_as_pshort(Set<T> cur, uint8_t *new_data){
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
    return Set<pshort>(new_data,count,counter*sizeof(uint16_t),cur.density,common::PSHORT);
  }
  template <class T>
  inline Set<bitset> repackage_as_bitset(Set<T> cur, uint8_t *new_data){
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
      word++;
    });
    return Set<bitset>(new_data,count,word,cur.density,common::BITSET);
  }

  template<class T>
  inline Set<hybrid> repackage(Set<T> cur, uint8_t *new_data){
    common::type type = hybrid::compute_type(cur.density);
    if(type == cur.type || cur.density == 0.0){
      return cur;
    } else if(type == common::UINTEGER){
      return Set<hybrid>(repackage_as_uinteger(cur,new_data));
    } else if(type == common::PSHORT){
      return Set<hybrid>(repackage_as_pshort(cur,new_data));
    } else if(type == common::BITSET){
      return Set<hybrid>(repackage_as_bitset(cur,new_data));
    }
    return cur;    
  }
}

#endif
