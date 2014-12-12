#ifndef ParallelBuffer_H
#define ParallelBuffer_H

template<class T>
class ParallelBuffer{
  public:
    size_t num_threads;
    size_t size;
    T **data;

    void allocate(size_t tid){
      data[tid] = new T[size];
    }
    void unallocate(size_t tid){
      delete[] data[tid];
    }

    ParallelBuffer(size_t num_threads_in, size_t size_in){
      num_threads = num_threads_in;
      size = size_in;
      data = new T*[num_threads];
    }
    ~ParallelBuffer(){}
};

#endif