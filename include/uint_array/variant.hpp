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
  inline __m128i get_next_sse(uint8_t *data,size_t &data_i,unsigned int &prev){
    unsigned int a,b,c,d;
    unsigned int cur = variant::variant_decode(data,data_i);
    cur += prev;
    a = cur;
    prev = cur;
    //cout << "\tdata_i: " << a << endl;

    cur = variant::variant_decode(data,data_i);
    cur += prev;
    b = cur;
    prev = cur;
    //cout << "\tdata_i: " << b << endl;

    cur = variant::variant_decode(data,data_i);
    cur += prev;
    c = cur;
    prev = cur;
    //cout << "\tdata_i: " << c << endl;

    cur = variant::variant_decode(data,data_i);
    cur += prev;
    d = cur;
    prev = cur;
    //cout << "\tdata_i: " << d << endl << endl;

    return _mm_set_epi32(d,c,b,a);
  }
  inline size_t preprocess(uint8_t *result_in, unsigned int *data_in, size_t length){
    if(length > 0){
      size_t data_i = 0;
      size_t result_i = 0;

      unsigned int *data = new unsigned int[length];
      produce_deltas(data_in,length,data,0);
      result_i = variant_encode(data,data_i,length,result_in,result_i);

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
  inline T sum(std::function<T(unsigned int,unsigned int,unsigned int*)> function,unsigned int col,uint8_t *data, size_t cardinality){
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
        cur += prev;
        prev = cur;

        result += function(col,prev);
        num_decoded++;
      }
    }
    return result;
  }
  inline void decode(unsigned int *output,uint8_t *data, size_t cardinality){
    size_t output_i = 0;    

    if(cardinality != 0){
      //cout << "bits_used: " << (uint)bits_used << endl;
      size_t data_i = 0;
      size_t num_decoded = 0;

      unsigned int prev = variant::variant_decode(data,data_i);
      output[output_i++] = prev;
      num_decoded++;

      //cout << "starting variant decode at: " << data_i << endl;
      while(num_decoded < cardinality){
        //cout << "\tdata_i: " << data_i << endl;
        unsigned int cur = variant::variant_decode(data,data_i);
        cur += prev;
        prev = cur;

        output[output_i++] = prev;
        num_decoded++;
      }
    }
  }
} 