
#include "UnsignedIntegerArray.hpp"

UnsignedIntegerArray::UnsignedIntegerArray(unsigned int *data_in, size_t length_in, common::type t_in){
  t = t_in;
  uint8_t *tmp_data = new uint8_t[length_in*10];
  switch(t){
    case common::ARRAY32:
      length = array32::preprocess((unsigned int*)(tmp_data),data_in,length_in);
      break;
    case common::ARRAY16:
      length = array16::preprocess((unsigned short*)(tmp_data),data_in,length_in);
      break;
    case common::BITSET:
      length = bitset::preprocess((unsigned short*)(tmp_data),data_in,length_in);
      break;
    case common::A32BITPACKED:
      length = a32bitpacked::preprocess((tmp_data),data_in,length_in);
      cout << "dd: " << (uint)tmp_data[0] << endl;
      break;
    default:
      break;
  }
  cardinality = length_in;

  data = new uint8_t[length];
  std::copy(tmp_data,tmp_data+length,data);
  delete[] tmp_data;
}

void UnsignedIntegerArray::print_data(){
  switch(t){
    case common::ARRAY32:
      array32::print_data((unsigned int*)data,length/4);
      break;
    case common::ARRAY16:
      array16::print_data((unsigned short*)data,length/2);
      break;
    case common::BITSET:
      bitset::print_data((unsigned short*)data,length/2);
      break;
    case common::A32BITPACKED:
      a32bitpacked::print_data(data,length,cardinality);
      break;
    default:
      break;
  }
}