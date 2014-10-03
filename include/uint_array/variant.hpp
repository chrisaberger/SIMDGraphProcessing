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
        file << "Data_i: " << data_i << endl;
        unsigned int cur = variant::variant_decode(data,data_i);
        //cout << "cur[" << num_decoded << "]" << cur << endl;
        cur += prev;
        prev = cur;

        file << " Data: " << cur << endl;
        num_decoded++;
      }
    }
  }
  template<typename T, typename U>
  inline T reduce(U env,T (*function)(U,unsigned int,unsigned int),unsigned int col,uint8_t *data, size_t cardinality){
    T result = (T) 0;

    if(cardinality != 0){
      //cout << "bits_used: " << (uint)bits_used << endl;
      size_t data_i = 0;
      size_t num_decoded = 0;

      unsigned int prev = variant::variant_decode(data,data_i);
      result += function(env,col,prev);
      num_decoded++;

      //cout << "starting variant decode at: " << data_i << endl;
      while(num_decoded < cardinality){
        //cout << "\tdata_i: " << data_i << endl;
        unsigned int cur = variant::variant_decode(data,data_i);
        cur += prev;
        prev = cur;

        result += function(env,col,prev);
        num_decoded++;
      }
    }
    return result;
  }
  inline size_t intersect(unsigned int *C, uint8_t *A, uint8_t *B, size_t s_a, size_t s_b) {
    size_t count = 0;
    size_t i_a = 0, i_b = 0;

    size_t num_checked_a = 0;
    size_t num_checked_b = 0;
    unsigned int prev_a = 0;
    unsigned int prev_b = 0;

    #if VECTORIZE == 1
    size_t st_a = (s_a / 4) * 4;
    size_t st_b = (s_b / 4) * 4;
    if(num_checked_a < st_a  && num_checked_b < st_b){
      size_t old_ia = i_a;
      size_t old_preva = prev_a;
      size_t old_prevb = prev_b;
      size_t old_ib = i_b;
      __m128i v_a = get_next_sse(A,i_a,prev_a);
      __m128i v_b = get_next_sse(B,i_b,prev_b);
      unsigned int a_max = _mm_extract_epi32(v_a, 3);
      unsigned int b_max = _mm_extract_epi32(v_b, 3);
      while(num_checked_a < st_a && num_checked_b < st_b) {


        //[ compute mask of common elements
       // cout << "looping: " << num_checked_a << " " << st_a << " " << num_checked_b << " " << st_b << endl;
        unsigned int cyclic_shift = _MM_SHUFFLE(0,3,2,1);
        __m128i cmp_mask1 = _mm_cmpeq_epi32(v_a, v_b);    // pairwise comparison
        v_b = _mm_shuffle_epi32(v_b, cyclic_shift);       // shuffling
        __m128i cmp_mask2 = _mm_cmpeq_epi32(v_a, v_b);    // again...
        v_b = _mm_shuffle_epi32(v_b, cyclic_shift);
        __m128i cmp_mask3 = _mm_cmpeq_epi32(v_a, v_b);    // and again...
        v_b = _mm_shuffle_epi32(v_b, cyclic_shift);
        __m128i cmp_mask4 = _mm_cmpeq_epi32(v_a, v_b);    // and again.
        __m128i cmp_mask = _mm_or_si128(
                _mm_or_si128(cmp_mask1, cmp_mask2),
                _mm_or_si128(cmp_mask3, cmp_mask4)
        ); // OR-ing of comparison masks
        // convert the 128-bit mask to the 4-bit mask
        unsigned int mask = _mm_movemask_ps((__m128)cmp_mask);
        //]

        //[ copy out common elements
        #if WRITE_VECTOR == 1
        __m128i p = _mm_shuffle_epi8(v_a, shuffle_mask32[mask]);
        _mm_storeu_si128((__m128i*)&C[count], p);
        #endif

        count += _mm_popcnt_u32(mask); // a number of elements is a weight of the mask
        //]

        //cout << "maxes: " << a_max << " " << b_max << " i: " << i_a << " " << i_b<< endl;
        bool canAdvance = (((num_checked_a+INTS_PER_REG) < st_a) && ((num_checked_b+INTS_PER_REG) < st_b)) || 
          (((num_checked_a+INTS_PER_REG) < st_a) && (a_max < b_max)) ||
         (((num_checked_b+INTS_PER_REG) < st_b) && (a_max > b_max));
         //cout << "can canAdvance: " << canAdvance << endl;
         unsigned int a_max_new = a_max;
        if(a_max <= b_max){
          if(canAdvance){
            //cout << "advance a" << endl;
            old_ia = i_a;
            old_preva = prev_a;
            v_a = get_next_sse(A,i_a,prev_a);
            a_max_new = _mm_extract_epi32(v_a, 3);
          } else if(a_max != b_max){
            //cout << "backing b up" << endl;
            i_b = old_ib;
            prev_b = old_prevb;
          }
          num_checked_a += INTS_PER_REG;
        }
        if(a_max >= b_max){
          if(canAdvance){
            //cout <<"advance b" << endl;
            old_ib = i_b;
            old_prevb = prev_b;
            v_b = get_next_sse(B,i_b,prev_b);
            b_max = _mm_extract_epi32(v_b, 3);
          } else if(a_max != b_max){
            //cout << "a should stay put" << endl;
            i_a = old_ia;
            prev_a = old_preva;
          }
          num_checked_b += INTS_PER_REG;
        }
        a_max = a_max_new;
        //cout << "count: " << count << endl;
      }
      #endif
    }
    //cout << i_a << " " << i_b << " count: " <<  count << endl;
    // intersect the tail using scalar intersection
    unsigned int b_cur = 0;
    if(num_checked_b < s_b){
      b_cur = prev_b + variant::variant_decode(B,i_b);
      prev_b = b_cur;
    }
    //cout << "BBBB: " << b_cur << endl;
    //cout << "num_checked_a: " << num_checked_a << " num_checked_b: " << num_checked_b << endl;
    bool notFinished = num_checked_a < s_a && num_checked_b < s_b;
    while(notFinished){
      //cout << "i_a: " << i_a << "i_b: " << i_b << endl;
      unsigned int a_cur = prev_a + variant::variant_decode(A,i_a);
      prev_a = a_cur;

      while(notFinished && b_cur < a_cur){
        num_checked_b++;
        //cout << "acur: " << a_cur << " bcur: " << b_cur << " i_b: " << i_b << endl;;
        //cout << "num_checked_b: " << num_checked_b << " s_b: " << s_b << endl;

        notFinished = num_checked_b < s_b;
        if(notFinished){
          b_cur = prev_b + variant::variant_decode(B,i_b);
          prev_b = b_cur;
        }
      }
      //cout << "COUNTacur: " << a_cur << " bcur: " << b_cur << endl;
      if(notFinished && a_cur == b_cur){
        //cout << "HIT: " << count  << endl << endl;
        #if WRITE_VECTOR == 1
        C[count] = a_cur;
        #endif
        ++count;
      }
      num_checked_a++;
      notFinished = notFinished && num_checked_a < s_a;
    }

    #if WRITE_VECTOR == 0
    C = C;
    #endif
    
    return count;
  }
} 
