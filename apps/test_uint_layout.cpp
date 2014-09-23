// class templates
#include "UnsignedIntegerArray.hpp"

//sparsity = length/max
void create_synthetic_array(unsigned int *data, size_t length, unsigned int max){
  if(length > 0){
    srand ( time(NULL) );
    data[0] = max;
    for(size_t data_i = 1; data_i < length; data_i++){
      data[data_i] = rand() % max;
    }
    std::sort(data,data+length);
  }
}

int main (int argc, char* argv[]) {
  size_t size_start = 0;
  size_t size_end = 1000000;

  size_t len = size_end-size_start + 1;

  size_t *a32bp_sizes = new size_t[len];
  size_t *a32bpd_sizes = new size_t[len];
  size_t *v_sizes = new size_t[len];
  size_t *vd_sizes = new size_t[len];
  size_t *a16_sizes = new size_t[len];
  size_t *a32_sizes = new size_t[len];


  unsigned int *data = new unsigned int[len];
  unsigned int max_size = 0x000F00FFF; 
  create_synthetic_array(data,len,max_size);

  ofstream myfile;
  myfile.open("reference.txt");
  for(size_t i = 0; i < len; i++){
    myfile << " Data: " << data[i] << endl;
  }
  
  //do fixed sizes
  //do multiple lines plotting bytes used versus max int

  size_t i = 0;
  UnsignedIntegerArray *my_data;
  size_t cur_size = size_start;
  while(cur_size <= size_end){
    my_data = new UnsignedIntegerArray(data,cur_size,common::A32BITPACKED);
    a32bp_sizes[i] = my_data->length;
    my_data->~UnsignedIntegerArray();

    my_data = new UnsignedIntegerArray(data,cur_size,common::A32BITPACKED_DELTA);
    a32bpd_sizes[i] = my_data->length;
    my_data->~UnsignedIntegerArray();

    my_data = new UnsignedIntegerArray(data,cur_size,common::VARIANT);
    v_sizes[i] = my_data->length;
    my_data->~UnsignedIntegerArray();
    
    my_data = new UnsignedIntegerArray(data,cur_size,common::VARIANT_DELTA);
    vd_sizes[i] = my_data->length;
    my_data->~UnsignedIntegerArray();

    my_data = new UnsignedIntegerArray(data,cur_size,common::ARRAY16);
    a16_sizes[i] = my_data->length;
    my_data->~UnsignedIntegerArray();

    my_data = new UnsignedIntegerArray(data,cur_size,common::ARRAY32);
    a32_sizes[i] = my_data->length;
    my_data->~UnsignedIntegerArray();
    
    if(cur_size < 200){
      cur_size++;
    } else if(cur_size < 500000){
      cur_size+=128;
    } else{
      cur_size += 65536;
    }
    i++;
  }

  ofstream data_file;
  data_file.open("layout_sizes.dat");

  i = 0;
  data_file << "\"A32 BITPACKED\"" << endl;
  cur_size = size_start;
  while(cur_size <= size_end){
    data_file << cur_size << " " << a32bp_sizes[i] << endl;
    if(cur_size < 200){
      cur_size++;
    } else if(cur_size < 500000){
      cur_size+=128;
    } else{
      cur_size += 65536;
    }
    i++; 
  }
  data_file << endl << endl;

  i = 0;
  data_file << "\"A32 BITPACKED DELTA\"" << endl;
  cur_size = size_start;
  while(cur_size <= size_end){
    data_file << cur_size << " " << a32bpd_sizes[i] << endl;
    if(cur_size < 200){
      cur_size++;
    } else if(cur_size < 500000){
      cur_size+=128;
    } else{
      cur_size += 65536;
    }
    i++;
  }
  data_file << endl << endl;

  i = 0;
  data_file << "\"VARIANT\"" << endl;
  cur_size = size_start;
  while(cur_size <= size_end){
    data_file << cur_size << " " << v_sizes[i] << endl;
    if(cur_size < 200){
      cur_size++;
    } else if(cur_size < 500000){
      cur_size+=128;
    } else{
      cur_size += 65536;
    }
    i++;
  }
  data_file << endl << endl;

  i = 0;
  data_file << "\"VARIANT_DELTA\"" << endl;
  cur_size = size_start;
  while(cur_size <= size_end){
    data_file << cur_size << " " << vd_sizes[i] << endl;
    if(cur_size < 200){
      cur_size++;
    } else if(cur_size < 500000){
      cur_size+=128;
    } else{
      cur_size += 65536;
    }
    i++;
  }
  data_file << endl << endl;

  i = 0;
  data_file << "\"A16\"" << endl;
  cur_size = size_start;
  while(cur_size <= size_end){
    data_file << cur_size << " " << a16_sizes[i] << endl;
    if(cur_size < 200){
      cur_size++;
    } else if(cur_size < 500000){
      cur_size+=128;
    } else{
      cur_size += 65536;
    }
    i++;
  }
  data_file << endl << endl;

  i = 0;
  data_file << "\"A32\"" << endl;
  cur_size = size_start;
  while(cur_size <= size_end){
    data_file << cur_size << " " << a32_sizes[i] << endl;
    if(cur_size < 200){
      cur_size++;
    } else if(cur_size < 500000){
      cur_size+=128;
    } else{
      cur_size += 65536;
    }
    i++;
  }
  data_file << endl << endl;



  return 0;
}
