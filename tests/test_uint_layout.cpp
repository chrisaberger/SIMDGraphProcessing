// class templates
#include <stdio.h>
#include <stdlib.h>
#include "UnsignedIntegerArray.hpp"

//sparsity = length/max
void create_synthetic_array(uint32_t *data, size_t length, uint32_t max){
  if(length > 0){
    srand ( time(NULL) );
    data[0] = max;
    for(size_t data_i = 1; data_i < length; data_i++){
      data[data_i] = rand() % max;
    }
    std::sort(data,data+length);
  }
}
size_t increment_size(size_t cur_size){
  if(cur_size < 200){
    cur_size++;
  } else if(cur_size < 500000){
    cur_size+=128;
  } else{
    cur_size += 65536;
  }
  return cur_size;
}
void print_data(size_t *data, string type, ofstream &data_file,size_t max ,size_t max_sparsity, size_t sparsity_shifter){
  size_t i = 0;
  data_file << type << endl;
  while(max <= max_sparsity){
    data_file << max << " " << data[i] << endl;
    max <<= sparsity_shifter;
    i++;
  }
  data_file << endl << endl;
}
void print_data(unsigned long long *data, string type, ofstream &data_file,size_t max ,size_t max_sparsity, size_t sparsity_shifter){
  size_t i = 0;
  data_file << type << endl;
  while(max <= max_sparsity){
    data_file << max << " " << data[i] << endl;
    max <<= sparsity_shifter;
    i++;
  }
  data_file << endl << endl;
}
void print_data(size_t *data, string type, ofstream &data_file, size_t size_start, size_t size_end){
  size_t i = 0;
  data_file << type << endl;
  size_t cur_size = size_start;
  while(cur_size <= size_end){
    data_file << cur_size << " " << data[i] << endl;
    cur_size = increment_size(cur_size);
    i++;
  }
  data_file << endl << endl;
}
void print_data(unsigned long long *data, string type, ofstream &data_file, size_t size_start, size_t size_end){
  size_t i = 0;
  data_file << type << endl;
  size_t cur_size = size_start;
  while(cur_size <= size_end){
    data_file << cur_size << " " << data[i] << endl;
    cur_size = increment_size(cur_size);
    i++;
  }
  data_file << endl << endl;
}
void time_decode(unsigned long long *data, UnsignedIntegerArray *my_data, size_t i){
  unsigned long long t1_tim;
  unsigned long long t2_tim;
  struct timeval timer;
  std::string fn = "dummy.txt";

  gettimeofday(&timer, NULL);  
  t1_tim=(unsigned long long)timer.tv_sec * 1000000ULL +
    (unsigned long long)timer.tv_usec; 
  my_data->print_data(fn);
  gettimeofday(&timer, NULL);  
  t2_tim=(unsigned long long)timer.tv_sec * 1000000ULL +
    (unsigned long long)timer.tv_usec; 
  data[i] = t2_tim-t1_tim;
}

int main (int argc, char* argv[]) {
  size_t *a32bp_sizes = new size_t[1000000];
  size_t *a32bpd_sizes = new size_t[1000000];
  size_t *v_sizes = new size_t[1000000];
  size_t *vd_sizes = new size_t[1000000];
  size_t *a16_sizes = new size_t[1000000];
  size_t *a32_sizes = new size_t[1000000];
  
  unsigned long long *a32bp_time = new unsigned long long[1000000];
  unsigned long long *a32bpd_time = new unsigned long long[1000000];
  unsigned long long *v_time = new unsigned long long[1000000];
  unsigned long long *vd_time = new unsigned long long[1000000];
  unsigned long long *a16_time = new unsigned long long[1000000];
  unsigned long long *a32_time = new unsigned long long[1000000];

  uint32_t *data = new uint32_t[1000000];
  
  uint32_t max_size = 0x0000000f;   

  /*
  while(max_size < 0xF0000000){
    size_t size_start = 0;
    size_t size_end = 100;
    if(size_end > max_size){
      size_end = max_size;
    }

    size_t i = 0;
    UnsignedIntegerArray *my_data;
    size_t cur_size = size_start;
    while(cur_size <= size_end){
      create_synthetic_array(data,cur_size-size_start,max_size);

      my_data = new UnsignedIntegerArray(data,cur_size,common::A32BITPACKED);
      a32bp_sizes[i] = my_data->length;
      time_decode(a32bp_time, my_data,i);
      my_data->~UnsignedIntegerArray();

      my_data = new UnsignedIntegerArray(data,cur_size,common::A32BITPACKED_DELTA);
      a32bpd_sizes[i] = my_data->length;
      time_decode(a32bpd_time, my_data,i);
      my_data->~UnsignedIntegerArray();

      my_data = new UnsignedIntegerArray(data,cur_size,common::VARIANT);
      v_sizes[i] = my_data->length;
      time_decode(v_time, my_data,i);
      my_data->~UnsignedIntegerArray();
      
      my_data = new UnsignedIntegerArray(data,cur_size,common::VARIANT_DELTA);
      vd_sizes[i] = my_data->length;
      time_decode(vd_time, my_data,i);
      my_data->~UnsignedIntegerArray();

      my_data = new UnsignedIntegerArray(data,cur_size,common::ARRAY16);
      a16_sizes[i] = my_data->length;
      time_decode(a16_time, my_data,i);
      my_data->~UnsignedIntegerArray();

      my_data = new UnsignedIntegerArray(data,cur_size,common::ARRAY32);
      a32_sizes[i] = my_data->length;
      my_data->print_data(fn);
      time_decode(a32_time, my_data,i);
      my_data->~UnsignedIntegerArray();

      cur_size = increment_size(cur_size);
      i++;
    }

    ofstream data_file;
    std::string filename = "sizes/layout_sizes_";
    filename += std::to_string(size_end);
    filename += "_";
    filename += std::to_string(max_size);
    filename += ".dat";
    data_file.open(filename);

    print_data(a32_sizes,"\"A32\"",data_file,size_start,size_end);
    print_data(a16_sizes,"\"A16\"",data_file,size_start,size_end);
    print_data(v_sizes,"\"VARIANT\"",data_file,size_start,size_end);
    print_data(vd_sizes,"\"VARIANT DELTA\"",data_file,size_start,size_end);
    print_data(a32bp_sizes,"\"A32 BITPACKED\"",data_file,size_start,size_end);
    print_data(a32bpd_sizes,"\"A32 BITPACKED DELTA\"",data_file,size_start,size_end);

    data_file.close();

    filename = "times/layout_times_";
    filename += std::to_string(size_end);
    filename += "_";
    filename += std::to_string(max_size);
    filename += ".dat";

    cout << "Opening file: " << filename << endl;
    data_file.open(filename);

    print_data(a32_time,"\"A32\"",data_file,size_start,size_end);
    print_data(a16_time,"\"A16\"",data_file,size_start,size_end);
    print_data(v_time,"\"VARIANT\"",data_file,size_start,size_end);
    print_data(vd_time,"\"VARIANT DELTA\"",data_file,size_start,size_end);
    print_data(a32bp_time,"\"A32 BITPACKED\"",data_file,size_start,size_end);
    print_data(a32bpd_time,"\"A32 BITPACKED DELTA\"",data_file,size_start,size_end);

    data_file.close();

    max_size <<= 2;
  }
  */

  size_t max_start = 0x0000F4240;
  max_size = max_start;
  size_t cur_size = 1000000;
  size_t i = 0;
  size_t max_sparsity = 0xEFFFFFFF;
  size_t sparsity_shifter = 1;
  while(max_size < max_sparsity){
    create_synthetic_array(data,cur_size,max_size);

    UnsignedIntegerArray *my_data = new UnsignedIntegerArray(data,cur_size,common::A32BITPACKED);
    time_decode(a32bp_time, my_data, i);
    a32bp_sizes[i] = my_data->length;
    my_data->~UnsignedIntegerArray();

    my_data = new UnsignedIntegerArray(data,cur_size,common::A32BITPACKED_DELTA);
    time_decode(a32bpd_time, my_data, i);
    a32bpd_sizes[i] = my_data->length;
    my_data->~UnsignedIntegerArray();

    my_data = new UnsignedIntegerArray(data,cur_size,common::VARIANT);
    time_decode(v_time, my_data, i);
    v_sizes[i] = my_data->length;
    my_data->~UnsignedIntegerArray();
    
    my_data = new UnsignedIntegerArray(data,cur_size,common::VARIANT_DELTA);
    time_decode(vd_time, my_data, i);
    vd_sizes[i] = my_data->length;
    my_data->~UnsignedIntegerArray();

    my_data = new UnsignedIntegerArray(data,cur_size,common::ARRAY16);
    time_decode(a16_time, my_data, i);
    a16_sizes[i] = my_data->length;
    my_data->~UnsignedIntegerArray();

    my_data = new UnsignedIntegerArray(data,cur_size,common::ARRAY32);
    time_decode(a32_time, my_data, i);
    a32_sizes[i] = my_data->length;
    my_data->~UnsignedIntegerArray();

    max_size <<= sparsity_shifter;
    i++;
  }

  ofstream data_file;
  std::string filename = "sizes/million_sparsity_";
  filename += std::to_string(cur_size);
  filename += "_";
  filename += std::to_string(max_size);
  filename += ".dat";
  data_file.open(filename);

  print_data(a32_sizes,"\"A32\"",data_file,max_start,max_sparsity,sparsity_shifter);
  print_data(a16_sizes,"\"A16\"",data_file,max_start,max_sparsity,sparsity_shifter);
  print_data(v_sizes,"\"VARIANT\"",data_file,max_start,max_sparsity,sparsity_shifter);
  print_data(vd_sizes,"\"VARIANT DELTA\"",data_file,max_start,max_sparsity,sparsity_shifter);
  print_data(a32bp_sizes,"\"A32 BITPACKED\"",data_file,max_start,max_sparsity,sparsity_shifter);
  print_data(a32bpd_sizes,"\"A32 BITPACKED DELTA\"",data_file,max_start,max_sparsity,sparsity_shifter);

  data_file.close();

  filename = "times/million_times_";
  filename += std::to_string(max_start);
  filename += "_";
  filename += std::to_string(max_size);
  filename += ".dat";

  cout << "Opening file: " << filename << endl;
  data_file.open(filename);

  print_data(a32_time,"\"A32\"",data_file,max_start,max_sparsity,sparsity_shifter);
  print_data(a16_time,"\"A16\"",data_file,max_start,max_sparsity,sparsity_shifter);
  print_data(v_time,"\"VARIANT\"",data_file,max_start,max_sparsity,sparsity_shifter);
  print_data(vd_time,"\"VARIANT DELTA\"",data_file,max_start,max_sparsity,sparsity_shifter);
  print_data(a32bp_time,"\"A32 BITPACKED\"",data_file,max_start,max_sparsity,sparsity_shifter);
  print_data(a32bpd_time,"\"A32 BITPACKED DELTA\"",data_file,max_start,max_sparsity,sparsity_shifter);


  return 0;
}
