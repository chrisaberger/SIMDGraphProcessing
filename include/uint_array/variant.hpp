#include "common.hpp"

namespace variant {
  inline size_t variant_encode(unsigned int *data, size_t data_i, size_t length, uint8_t *result, size_t result_i){
    //cout << "Starting variant encode at: " << result_i << " " << data_i << " " << length << endl;
    while(data_i < length){
      unsigned int cur = data[data_i++];
      //cout << "data_i: " << data_i-1 << " cur: " << cur << endl;
      unsigned int bytes_needed = ceil((log2(cur)+1)/7); //1 bit is reserved for continue
      if(bytes_needed == 0) bytes_needed = 1;
      //cout << "Bytes needed: " << bytes_needed << " " << (unsigned int)log2(cur) << " " << cur << " " << data_i-1 << endl;
      size_t bytes_set = 0;
      while(bytes_set < bytes_needed){
        uint8_t continue_bit = 0x00;
        if(bytes_set+1 < bytes_needed){
          continue_bit = 0x01;
        }
        result[result_i++] = ((uint8_t)(cur << 1)) | continue_bit;
        cur = cur >> 7;
        //cout << "result[" << result_i-1 << "]: "  << (uint)result[result_i-1] << endl;
        bytes_set++; 
      }
      //cout << endl;
    }
    return result_i;
  }
  inline unsigned int variant_decode(uint8_t *data, size_t &data_i){
    uint8_t byte_in = data[data_i++];
    unsigned int cur = (byte_in >> 1);
    //cout << "byte: " << (uint)byte_in << " cur: " << cur << endl;

    size_t i = 1;
    while(byte_in % 2){
      byte_in = data[data_i++]; 
      //cout << "byte[" << data_i-1 <<"]: " << (uint)byte_in << endl;

      cur |= (unsigned int)((byte_in >> 1) << 7*i++);
    }
    //cout << endl;
    return cur;
  }

  inline size_t preprocess(uint8_t *result_in, unsigned int *data_in, size_t length){
    if(length > 0){
      size_t data_i = 0;
      size_t result_i = 0;

      //cout << "bits_used: " << (uint) result_in[0] << endl;
      //cout << "sending in: " << data[data_i] << endl;
      //cout << " num_simd_packed: " << num_simd_packed << endl;


      //cout << "starting encode at: " << result_i << " data elem: " << data_i << " result: " << result_i << endl;
      result_i = variant_encode(data_in,data_i,length,result_in,result_i);

      return result_i;
    } else{
      return 0;
    }
  }
  inline void print_data(uint8_t *data, const size_t length, const size_t cardinality, std::ofstream &file){
    (void)length;

    if(cardinality != 0){
      size_t data_i = 0;
      //cout << "bits_used: " << (uint)bits_used << endl;

      size_t num_decoded = 0;
      //cout << "Length: " << length << endl;
      //cout << "starting variant decode at: " << data_i << endl;
      while(num_decoded < cardinality){
        unsigned int cur = variant_decode(data,data_i);
        //cout << "data_i: " << data_i << endl;

        file << " Data: " << cur << endl;
        num_decoded++;
      }
    }
  }
} 