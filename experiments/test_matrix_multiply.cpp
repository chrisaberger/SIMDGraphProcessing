// class templates
#include <stdio.h>
#include <stdlib.h>
#include "UnsignedIntegerArray.hpp"

int main (int argc, char* argv[]) {
  size_t num_rows = 200000;
  size_t sparsity = 3;
  size_t max_iterations = 1;

  size_t num_set_in_row = num_rows/sparsity;
  size_t total_data_size = num_set_in_row*num_rows;

  float *vector_data = new float[num_rows*sparsity];  //just over allocate and don't worry
  float *next_vector_data = new float[num_rows*sparsity];  
  for(size_t i = 0; i < (num_rows*sparsity); i++){
    vector_data[i] = 0.25;
  }

  common::startClock();
  //Worst case matrix
  uint32_t *row_indicies = new uint32_t[total_data_size];
  float *matrix_data = new float[total_data_size];
  for(size_t i = 0; i < num_rows; i++){
    for(size_t j = 0; j < num_set_in_row; j++){
      row_indicies[i*num_set_in_row+j] = sparsity*j+i; 
      matrix_data[i*num_set_in_row+j] = 0.25;
    }
  }
  common::stopClock("Building matrix");


  common::startClock();
  //Worse case multiply.
  size_t num_iterations = 0;
  while(num_iterations < max_iterations){
    for(size_t i = 0; i < num_rows; i++){
      float sum = 0.0;
      for(size_t j = 0; j < num_set_in_row; j++){
        sum += vector_data[row_indicies[i*num_set_in_row+j]]*matrix_data[i*num_set_in_row+j]; 
      }
      next_vector_data[i] = sum;
    }

    float *tmp = vector_data;
    vector_data = next_vector_data;
    next_vector_data = tmp;
    num_iterations++;
  }
  common::stopClock("Worst case");

/////////////////////////////////////////////////////////////////////////////////

  //Best case matrix
  common::startClock();
  //Best case multiply.
  num_iterations = 0;
  while(num_iterations < max_iterations){
    for(size_t i = 0; i < num_rows; i++){
      __m128 sse_sum = _mm_setzero_ps(); 
      size_t st_row = (num_set_in_row / 4) * 4;
      for(size_t j = 0; j < st_row; j+=4){
        __m128 mul = _mm_mul_ps(_mm_loadu_ps(&matrix_data[i*num_set_in_row+j]),_mm_loadu_ps(&vector_data[j/4]));
        sse_sum = _mm_add_ps(sse_sum,mul);
      }

      float sum = 0.0;
      for(size_t j = st_row; j < num_set_in_row; j++){
        sum += vector_data[j]; 
      }
      next_vector_data[i] = sum;
    }

    float *tmp = vector_data;
    vector_data = next_vector_data;
    next_vector_data = tmp;
    num_iterations++;
  }
  common::stopClock("Best case");

  return 0;
}
