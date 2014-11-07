// class templates
#include "AOA_Matrix.hpp"
#include <cstdarg>
#include "MutableGraph.hpp"
#include "Table.hpp"

namespace application{
  AOA_Matrix *graph;
  long num_triangles = 0;
  Table *output;
  size_t num_threads;
  
  inline bool myNodeSelection(unsigned int node){
    (void)node;
    return true;
  }
  inline bool myEdgeSelection(unsigned int node, unsigned int nbr){
    return nbr < node;
  }
  inline long edgeApply(size_t depth, size_t query_depth, size_t thread_id, long t_count, uint8_t *buffer1, uint8_t *buffer2, Table *output, unsigned int *src_nbrhood, unsigned int src, unsigned int dst){
    (void) buffer2;
    long count = graph->row_intersect(buffer1,src,dst,src_nbrhood);

    size_t index = (depth-1) + query_depth*thread_id;
    //cout << index << " " << thread_id << endl;
    if(depth == 3){
      output->tuple[query_depth*thread_id] = src;
      output->tuple[query_depth*thread_id+1] = dst;
    } else{
      output->tuple[index] = dst;
    }

    if(depth == query_depth){
      unsigned int *output_table = output->table_pointers[index];
      uint_array::decode(output_table,buffer1,count);
      output->table_size[thread_id] += count; 
      if(depth == 3){
        for(long i = 0; i < count; i++){
          unsigned int *src_row = output->table_pointers[query_depth*thread_id];
          unsigned int *dst_row = output->table_pointers[query_depth*thread_id+1];
          src_row[t_count+i] = output->tuple[query_depth*thread_id];
          dst_row[t_count+i] = output->tuple[query_depth*thread_id+1];
        }
      }  
    }

    return count;
  }
  inline void queryOver(){
    const size_t query_depth = 3;
    auto row_function = std::bind(&AOA_Matrix::sum_over_columns_in_row<long>, graph, _1, _2, _3);

    const size_t matrix_size = graph->matrix_size;
    const size_t cardinality = graph->cardinality;

    output = new Table(3,num_threads,cardinality);

    long reducer = 0;
    
    #pragma omp parallel default(none) shared(row_function,output,num_threads) reduction(+:reducer) 
    {
      uint8_t *t_local_buffer_1 = new uint8_t[(cardinality*4)/(num_threads)];
      uint8_t *t_local_buffer_2 = new uint8_t[(cardinality*4)/(num_threads)];

      unsigned int *t_local_decode = new unsigned int[matrix_size];
      long t_local_reducer = 0;
      
      #pragma omp for schedule(dynamic)
      for(size_t i = 0; i < matrix_size; i++){
        auto edge_function = std::bind(&edgeApply,3,query_depth,omp_get_thread_num(),t_local_reducer,t_local_buffer_1,t_local_buffer_2,output,t_local_decode,_1,_2);
        t_local_reducer += (row_function)(i,t_local_decode,edge_function);
      }
      reducer += t_local_reducer;

      delete[] t_local_buffer_1;
      delete[] t_local_buffer_2;
      delete[] t_local_decode;
    }

    output->print_data("table.txt");

    num_triangles = reducer;
  }
}

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) { 
  if(argc != 4){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of threads> <layout type=bs,a16,a32,hybrid,v,bp>" << endl;
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
  MutableGraph *inputGraph = MutableGraph::undirectedFromEdgeList(argv[1]); //filename, # of files
  common::stopClock("Reading File");
  
  common::startClock();
  inputGraph->reorder_by_degree();
  common::stopClock("Reordering");
  
  cout << endl;

  common::startClock();
  application::graph = AOA_Matrix::from_symmetric(inputGraph->out_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    node_selection,edge_selection,inputGraph->external_ids,layout);
  common::stopClock("selections");
  
  inputGraph->MutableGraph::~MutableGraph(); 

  //application::graph->print_data("out.txt");
  
  common::startClock();
  application::queryOver();
  common::stopClock(input_layout);
  //application::graph->AOA_Matrix::~AOA_Matrix();
  cout << "Count: " << application::num_triangles << endl << endl;
  return 0;
}
