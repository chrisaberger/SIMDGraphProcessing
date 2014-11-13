// class templates
#include "AOA_Matrix.hpp"
#include "MutableGraph.hpp"
#include "Table.hpp"

namespace application{
  AOA_Matrix *graph;
  long num_triangles = 0;
  Table *output;
  size_t num_threads;

  inline bool myNodeSelection(uint32_t node){
    (void)node;
    return true;
  }
  inline bool myEdgeSelection(uint32_t node, uint32_t nbr){
    return nbr < node;
  }

  struct thread_data{
    size_t depth;
    size_t query_depth;
    size_t thread_id;

    uint8_t *buffer;
    uint32_t *decoded_src;
    thread_data(size_t buffer_lengths, size_t depth_in, const size_t query_depth_in, const size_t thread_id_in){
      buffer = new uint8_t[buffer_lengths*sizeof(int)];
      query_depth = query_depth_in;
      depth = depth_in;
      thread_id = thread_id_in;
      decoded_src = new uint32_t[buffer_lengths];
    }
    ~thread_data() { 
      /*
      for(size_t i = 0; i < query_depth-2; i++){
        delete[] buffers[i];
      }*/
      delete[] buffer;
      delete[] decoded_src;
    }

    inline long edgeApply(uint32_t src, uint32_t dst){
      const size_t index = (depth-1) + query_depth*thread_id;
      long count = 0;
      if(depth == 3){
        output->tuple[query_depth*thread_id] = src;
        output->tuple[query_depth*thread_id+1] = dst;
      } else{
        output->tuple[index-1] = dst;
      }
      //cout << "tid: " << thread_id <<" count: " << count << " src: " << src << " dst: " << dst << " depth: " << depth << " query_depth: " << query_depth << endl;
      if(depth == query_depth){
        count = graph->row_intersect(buffer,src,dst,decoded_src);
        size_t cur_size = output->table_size[thread_id];
        uint32_t *output_table = (output->table_pointers[index])+cur_size;
        uint_array::decode(output_table,buffer,count);
        for(long i = 0; i < count; i++){
          for(size_t j = 0; j < output->num_tuples-1; j++){ //the last row is taken care of in decode
            uint32_t *tmp_row = output->table_pointers[query_depth*thread_id+j];
            tmp_row[cur_size+i] = output->tuple[query_depth*thread_id+j];
          }
        }
        output->table_size[thread_id] += count;   
      } else {
        auto edge_function = std::bind(&thread_data::edgeApply,this,_1,_2);
        depth++;
        count = graph->sum_over_columns_in_row<long>(dst,decoded_src,edge_function);
        depth--;
      }
      return count;
    }
  };
  thread_data **t_data_pointers;

  inline void allocBuffers(const size_t query_depth){
    const size_t cardinality = graph->cardinality;
    
    output = new Table(query_depth,num_threads,cardinality);

    t_data_pointers = new thread_data*[num_threads];
    for(size_t k= 0; k < num_threads; k++){
      t_data_pointers[k] = new thread_data(graph->max_nbrhood_size,3,query_depth,k);
    }
  }
  inline void queryOver(){
    auto row_function = std::bind(&AOA_Matrix::sum_over_columns_in_row<long>, graph, _1, _2, _3);

    const size_t matrix_size = graph->matrix_size;
    
    thread* threads = new thread[num_threads];
    std::atomic<long> reducer;
    reducer = 0;
    const size_t block_size = 50; //matrix_size / num_threads;
    std::atomic<size_t> next_work;
    next_work = 0;

    if(num_threads > 1){
      for(size_t k = 0; k < num_threads; k++){
        auto edge_function = std::bind(&thread_data::edgeApply,t_data_pointers[k],_1,_2);
        threads[k] = thread([k, &matrix_size, &next_work, &reducer, &t_data_pointers, edge_function, &row_function](void) -> void {
          long t_local_reducer = 0;
          while(true) {
            size_t work_start = next_work.fetch_add(block_size, std::memory_order_relaxed);
            if(work_start > matrix_size)
              break;

            size_t work_end = min(work_start + block_size, matrix_size);
            for(size_t j = work_start; j < work_end; j++) {
              t_local_reducer += (row_function)(j,t_data_pointers[k]->decoded_src,edge_function);
            }
          }
          reducer += t_local_reducer;
       });
      }
      //cleanup
      for(size_t k = 0; k < num_threads; k++) {
        threads[k].join();
        //t_data_pointers[k]->thread_data::~thread_data();
      }   
    } else{
      auto edge_function = std::bind(&thread_data::edgeApply,t_data_pointers[0],_1,_2);
      long t_local_reducer = 0;
      for(size_t i = 0; i  < matrix_size;  i++){
        t_local_reducer += (row_function)(i,t_data_pointers[0]->decoded_src,edge_function);
      }
      reducer = t_local_reducer;
    }
    num_triangles = reducer;
  }
}

int main (int argc, char* argv[]) { 
  if(argc != 5){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of threads> <layout type=bs,a16,a32,hybrid,v,bp> <depth>" << endl;
    exit(0);
  }

  cout << endl << "Number of threads: " << atoi(argv[2]) << endl;
  omp_set_num_threads(atoi(argv[2]));        
  application::num_threads = atoi(argv[2]);

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
  }
  #if COMPRESSION == 1
  else if(input_layout.compare("v") == 0){
    layout = common::VARIANT;
  } else if(input_layout.compare("bp") == 0){
    layout = common::A32BITPACKED;
  } 
  #endif
  else{
    cout << "No valid layout entered." << endl;
    exit(0);
  }

  auto node_selection = std::bind(&application::myNodeSelection, _1);
  auto edge_selection = std::bind(&application::myEdgeSelection, _1, _2);

  common::startClock();
  MutableGraph *inputGraph = MutableGraph::undirectedFromEdgeList(argv[1]); //filename, # of files
  common::stopClock("Reading File");
  
  common::startClock();
  //inputGraph->reorder_bfs();
  inputGraph->reorder_by_degree();
  common::stopClock("Reordering");
  
  cout << endl;

  common::startClock();
  application::graph = AOA_Matrix::from_symmetric(inputGraph->out_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,inputGraph->max_nbrhood_size,
    node_selection,edge_selection,inputGraph->external_ids,layout);
  common::stopClock("selections");
  
  inputGraph->MutableGraph::~MutableGraph(); 
  
  common::startClock();
  application::allocBuffers(atoi(argv[4]));
  common::stopClock("buffer allocation");  

  common::startClock();
  application::queryOver();
  common::stopClock(input_layout);  

  //application::graph->AOA_Matrix::~AOA_Matrix();
  cout << "Count: " << application::num_triangles << endl << endl;

  application::output->print_data("table.txt");
  return 0;
}
