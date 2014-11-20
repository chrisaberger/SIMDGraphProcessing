// class templates
#include "AOA_Matrix.hpp"
#include "MutableGraph.hpp"
#include "UnsignedIntegerArray.hpp"

namespace application{
  uint8_t *result;
  long num_triangles = 0;
  
  inline bool myNodeSelection(uint32_t node){
    (void)node;
    return true;
  }
  inline bool myEdgeSelection(uint32_t node, uint32_t nbr){
    (void) node; (void) nbr;
    return true;
  }
}

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) { 
  if(argc != 5){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of threads> <layout type=bs,a16,a32,hybrid,v,bp>" << endl;
    exit(0);
  }

  cout << endl << "Number of threads: " << atoi(argv[2]) << endl;
  size_t num_threads = atoi(argv[2]);
  omp_set_num_threads(atoi(argv[2]));        

  std::string input_layout = argv[3];

  common::type layout;
  if(input_layout.compare("a32") == 0){
    layout = common::ARRAY32;
  } else if(input_layout.compare("bs") == 0){
    layout = common::BITSET;
  } else if(input_layout.compare("a16") == 0){
    layout = common::ARRAY16;
  } else if(input_layout.compare("hybrid") == 0){
    layout = common::HYBRID_PERF;
  } else if(input_layout.compare("v") == 0){
    layout = common::VARIANT;
  } else if(input_layout.compare("bp") == 0){
    layout = common::A32BITPACKED;
  } else{
    cout << "No valid layout entered." << endl;
    exit(0);
  }

  auto node_selection = std::bind(&application::myNodeSelection, _1);
  auto edge_selection = std::bind(&application::myEdgeSelection, _1, _2);

  common::startClock();
  MutableGraph *inputGraph = MutableGraph::directedFromBinary(argv[1]); //filename, # of files
  common::stopClock("Reading File");
  
  cout << endl;

  common::startClock();
  AOA_Matrix *graph = AOA_Matrix::from_asymmetric(inputGraph,node_selection,edge_selection,layout);
  common::stopClock("selections");
  
  //graph->print_data(argv[4]);

  //initialize the fronteir.
  size_t fronteir_length = 1;
  uint8_t *fronteir = new uint8_t[graph->matrix_size*sizeof(uint32_t)];
  uint32_t *fronteir_32 = (uint32_t*)fronteir;
  for(size_t i = 0; i < fronteir_length; i++){
    fronteir_32[i] = i;
  }

  //allocate a visited bitset
  size_t bitset_size = (graph->matrix_size >> 3) + 1;
  cout << "matrix size: " << graph->matrix_size << " bs size: " << bitset_size << endl;
  uint8_t *visited = new uint8_t[bitset_size];
  memset(visited,(uint8_t)0,bitset_size);


  //per thread
  //then perform some sort of binnin


  thread* threads = new thread[num_threads];
  std::atomic<long> reducer;
  reducer = 0;
  std::atomic<size_t> next_work;

  std::atomic<size_t> next_fronteir_length;
  uint32_t *t_local_fronteir_size = new uint32_t[num_threads];
  uint32_t **t_local_fronteirs = new uint32_t*[num_threads];
  for(size_t i = 0; i < num_threads; i++){
    t_local_fronteirs[i] = new uint32_t[graph->matrix_size*sizeof(int)];
  }
  uint32_t *fronteir_index_array = new uint32_t[num_threads];

  bool done = false;
  size_t path_length = 0;
  bool bitset_f = false;
  
  common::startClock();
  while(!done){
    next_work = 0;
    next_fronteir_length = 0;
  
    const size_t block_size = (fronteir_length/(num_threads*4))+1; //matrix_size / num_threads;

    common::startClock();
    if(bitset_f){
      cout << "BITSET" << endl;
      for(size_t k = 0; k < num_threads; k++){
        threads[k] = thread([k, block_size, &t_local_fronteir_size, &t_local_fronteirs, &bitset_f, &next_fronteir_length, &fronteir, &fronteir_32, &fronteir_length, &next_work, &graph, &visited](void) -> void {
          size_t t_local_next_fronteir = 0;
          uint32_t *mybuffer = t_local_fronteirs[k];
          while(true) {
            size_t work_start = next_work.fetch_add(block_size, std::memory_order_relaxed);
            if(work_start > fronteir_length)
              break;

            size_t work_end = min(work_start + block_size, fronteir_length);
            for(size_t j = work_start; j < work_end; j++) {
              t_local_next_fronteir += graph->union_dense_neighbors(j,visited[j],&mybuffer[t_local_next_fronteir],fronteir);
            }
          }
          t_local_fronteir_size[k] = t_local_next_fronteir;
          next_fronteir_length += t_local_next_fronteir;
       });
      } 
    } else{
      cout << "SPARSE" << endl;
      for(size_t k = 0; k < num_threads; k++){
        threads[k] = thread([k, block_size, &t_local_fronteir_size, &t_local_fronteirs, &bitset_f, &next_fronteir_length, &fronteir, &fronteir_32, &fronteir_length, &next_work, &graph, &visited](void) -> void {
          size_t t_local_next_fronteir = 0;
          uint32_t *mybuffer = t_local_fronteirs[k];
          while(true) {
            size_t work_start = next_work.fetch_add(block_size, std::memory_order_relaxed);
            if(work_start > fronteir_length)
              break;

            size_t work_end = min(work_start + block_size, fronteir_length);
            for(size_t j = work_start; j < work_end; j++) {
              t_local_next_fronteir += graph->union_sparse_neighbors(fronteir_32[j],&mybuffer[t_local_next_fronteir],visited);
            }
          }
          t_local_fronteir_size[k] = t_local_next_fronteir;
          next_fronteir_length += t_local_next_fronteir;
       });
      } 
    }

    //cleanup
    size_t fronteir_index = 0;
    for(size_t k = 0; k < num_threads; k++) {
      threads[k].join();
      fronteir_index_array[k] = fronteir_index;
      fronteir_index += t_local_fronteir_size[k];
    } 
    common::stopClock("searching neighbors");


    path_length++;
    done = next_fronteir_length == 0;
    fronteir_length = next_fronteir_length;

    cout << "Frontier length: " << fronteir_length << endl;


    if(!done){
      common::startClock();
      ////////////////////////////////////
      //depending on density frontier will either be a bitset or  array of ints
      if(next_fronteir_length*32 > graph->matrix_size){
        cout << "BITSET COPY" << endl;
        bitset_f = true;
        memset(fronteir,(uint8_t)0,bitset_size);
        for(size_t k = 0; k < num_threads; k++){
          threads[k] = thread([k, &t_local_fronteir_size, &t_local_fronteirs, &fronteir, &fronteir_index_array](void) -> void {
            uint32_t *A = t_local_fronteirs[k];
            size_t s_a = t_local_fronteir_size[k];
            size_t i = 0;
            while(i<s_a){
              uint32_t cur = A[i];
              size_t word = bitset::word_index(cur);
              uint8_t set_value = 1 << (cur % 8);
              bool same_word = true;
              ++i;
              while(i<s_a && same_word){
                if(bitset::word_index(A[i])==word){
                  cur = A[i];
                  set_value |= (1 << (cur%8));
                  ++i;
                } else same_word = false;
              }
              __sync_fetch_and_or(&fronteir[word],set_value);
            }
          });
        }
        fronteir_length = bitset_size;
      } else{
        cout << "SPARSE COPY" << endl;
        bitset_f = false;
        for(size_t k = 0; k < num_threads; k++){
          threads[k] = thread([k, &t_local_fronteir_size, &fronteir_32, &t_local_fronteirs, &fronteir, &fronteir_index_array](void) -> void {
            uint32_t *copy_dat = t_local_fronteirs[k];
            for(size_t i = 0 ; i < t_local_fronteir_size[k]; i++){
              fronteir_32[i+fronteir_index_array[k]] = copy_dat[i];
            }
          });
        }
        fronteir_length = next_fronteir_length;
      }

      
      for(size_t k = 0; k < num_threads; k++) {
        threads[k].join();
      }

      /*
      if(!bitset_f){
        cout << "fronteir size: " << fronteir_length << endl;
        for(size_t i = 0; i < fronteir_length; i++){
          cout << fronteir_32[i] << endl;
        }
      } else{
        for(size_t i = 0; i < fronteir_length; i++){
          uint8_t cur_word = fronteir[i];
          for(size_t j = 0; j < 8; j++){
            if((cur_word >> j) % 2)
              cout << " Data: " << 8*i + j << endl;
          }
        }
      }
      */
      common::stopClock("Setting up next frontier");
    } //end if !done
  }

  common::stopClock("BFS");
  cout << "Path Length: " << path_length << endl;
  cout << "Final fronteir size: " << fronteir_length << endl;

  return 0;
}
