#include "common.hpp"
#include "variant.hpp"

namespace variant_delta {
  inline unsigned int produce_deltas(unsigned int *data_in, size_t length, unsigned int *data, unsigned int prev){    
    size_t max = 0;

    for(size_t i = 0; i < length; i++){
      unsigned int cur = data_in[i] - prev;
      //cout  << "Writing delta index: " << i << "  " << cur << endl;
      data[i] = cur;
      prev = data_in[i];
    }
    return max;
  }

  inline size_t preprocess(uint8_t *result_in, unsigned int *data_in, size_t length){
    if(length > 0){
      size_t data_i = 0;
      size_t result_i = 0;

      unsigned int *data = new unsigned int[length];
      produce_deltas(data_in,length,data,0);
      result_i = variant::variant_encode(data_in,data_i++,1,result_in,result_i);

      //cout << "bits_used: " << (uint) result_in[0] << endl;
      //cout << "sending in: " << data[data_i] << endl;
      //cout << " num_simd_packed: " << num_simd_packed << endl;
      
      //cout << "starting encode at: " << result_i << " data elem: " << data_i << " result: " << result_i << endl;
      result_i = variant::variant_encode(data,data_i,length,result_in,result_i);

      delete[] data;

      return result_i;
    } else{
      return 0;
    }
  }
  inline void print_data(uint8_t *data, const size_t length, const size_t cardinality, ofstream &file){
    (void)length;

    if(cardinality != 0){
      //cout << "bits_used: " << (uint)bits_used << endl;

      size_t data_i = 0;
      size_t num_decoded = 0;

      unsigned int prev = variant::variant_decode(data,data_i);
      file << " Data: " << prev << endl;
      num_decoded++;

      //cout << "starting variant decode at: " << data_i << endl;
      while(num_decoded < cardinality){
        //cout << "\tdata_i: " << data_i << endl;
        unsigned int cur = variant::variant_decode(data,data_i);
        //cout << "cur[" << num_decoded << "]" << cur << endl;
        cur += prev;
        prev = cur;

        file << " Data: " << cur << endl;
        num_decoded++;
      }
    }
  }
  template<typename T> 
  inline T reduce(T (*function)(unsigned int,unsigned int),unsigned int col,uint8_t *data, size_t cardinality){
    T result = (T) 0;

    if(cardinality != 0){
      //cout << "bits_used: " << (uint)bits_used << endl;
      size_t data_i = 0;
      size_t num_decoded = 0;

      unsigned int prev = variant::variant_decode(data,data_i);
      result += function(col,prev);
      num_decoded++;

      //cout << "starting variant decode at: " << data_i << endl;
      while(num_decoded < cardinality){
        //cout << "\tdata_i: " << data_i << endl;
        unsigned int cur = variant::variant_decode(data,data_i);
        //cout << "cur[" << num_decoded << "]" << cur << endl;
        cur += prev;
        prev = cur;

        result += function(col,prev);
        num_decoded++;
      }
    }
    return result;
  }
} 