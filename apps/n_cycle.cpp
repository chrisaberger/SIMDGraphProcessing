// class templates
#include "SparseMatrix.hpp"
#include "pcm_helper.hpp"
#include "Table.hpp"
#include "Parser.hpp"

using namespace pcm_helper;
/*
template<class T, class R>
class application{
  public:

  SparseMatrix<T,R>* graph;
  long num_cycles;
  MutableGraph *inputGraph;
  size_t num_threads;
  string layout;
  size_t query_depth;

  application(Parser input_data){
    num_cycles = 0;
    inputGraph = input_data.input_graph; 
    num_threads = input_data.num_threads;
    layout = input_data.layout;
    query_depth = input_data.n;
  }
  #ifdef ATTRIBUTES
  inline bool myNodeSelection(uint32_t node, uint32_t attribute){
    (void)node; (void) attribute;
    return attribute > 500;
  }
  inline bool myEdgeSelection(uint32_t src, uint32_t dst, uint32_t attribute){
    (void) attribute;
    return attribute == 2012 && src < dst;
  }
  #else
  inline bool myNodeSelection(uint32_t node, uint32_t attribute){
    (void)node; (void) attribute;
    return true;
  }
  inline bool myEdgeSelection(uint32_t node, uint32_t nbr, uint32_t attribute){
    (void) attribute;
    return nbr < node;
  }
  #endif
  inline void produceSubgraph(){
    auto node_selection = std::bind(&application::myNodeSelection, this, _1, _2);
    auto edge_selection = std::bind(&application::myEdgeSelection, this, _1, _2, _3);
    graph = SparseMatrix<T,R>::from_symmetric_graph(inputGraph,node_selection,edge_selection,num_threads);
  }
  inline size_t apply_function(Set<R> *A, size_t node, size_t depth, 
    uint32_t * dst_buffer, uint8_t * result_buffer, Table<uint64_t> *output){

    size_t count = 0;
    if(++depth == query_depth){
      Set<R> B = graph->get_decoded_row(node,dst_buffer);
      Set<R> C = ops::set_intersect(Set<R>(result_buffer),Set<R>(A),B);
      #if WRITE_TABLE == 1
      output->write_table(C,graph->id_map);
      #endif
      return C.cardinality;
    } else{
      Set<R> B = graph->get_row(node);
      B.foreach([this,A,&count,depth,output,query_depth,dst_buffer,result_buffer] (uint32_t i){
        output->tuple[depth-1] = graph->id_map[i]; 
        count += this->apply_function(A,i,depth,dst_buffer,result_buffer,output);
      });
    }
    return count;
  }
  inline void queryOver(){
    system_counter_state_t before_sstate = pcm_get_counter_state();
    server_uncore_power_state_t* before_uncstate = pcm_get_uncore_power_state();

    const size_t matrix_size = graph->matrix_size;
    const size_t estimated_table_size = ((graph->cardinality*40)/num_threads);

    ParallelTable<uint64_t>* output = new ParallelTable<uint64_t>(num_threads,query_depth,estimated_table_size);
    ParallelBuffer<uint32_t>* src_buffers = new ParallelBuffer<uint32_t>(num_threads,graph->max_nbrhood_size);
    ParallelBuffer<uint32_t>* dst_buffers = new ParallelBuffer<uint32_t>(num_threads,graph->max_nbrhood_size);
    ParallelBuffer<uint8_t>* result_buffers = new ParallelBuffer<uint8_t>(num_threads,graph->max_nbrhood_size*sizeof(uint32_t));

    common::par_for_range(num_threads,0,matrix_size,100,
      ///////////////////////////////////////////////////////////
      [this,query_depth,src_buffers,dst_buffers,output,graph,result_buffers](size_t tid){
        src_buffers->allocate(tid);
        dst_buffers->allocate(tid);
        output->allocate(tid);
        result_buffers->allocate(tid);
      },
      //////////////////////////////////////////////////////////
      [this,output,query_depth,src_buffers,dst_buffers,result_buffers](size_t tid, size_t i) {
        Table<uint64_t> *thread_output = output->table[tid];

        Set<R> A = this->graph->get_decoded_row(i,src_buffers->data[tid]);
        thread_output->tuple[0] = graph->id_map[i];  
        uint32_t * dst_buffer = dst_buffers->data[tid];
        uint8_t * result_buffer = result_buffers->data[tid];

        A.foreach([this,query_depth,&A,dst_buffer,thread_output,result_buffer] (uint32_t j){
          thread_output->tuple[1] = graph->id_map[j];  
          thread_output->cardinality += this->apply_function(&A,j,2,dst_buffer,result_buffer,thread_output);
        });
      },
      [this,output](size_t tid){
      /////////////////////////////////////////////////////////
        Table<uint64_t> *thread_output = output->table[tid];
        output->cardinality += thread_output->cardinality;
      }
    );
    num_cycles = output->cardinality;
    
    //output->print_data("table.txt");

    server_uncore_power_state_t* after_uncstate = pcm_get_uncore_power_state();
    pcm_print_uncore_power_state(before_uncstate, after_uncstate);
    system_counter_state_t after_sstate = pcm_get_counter_state();
    pcm_print_counter_stats(before_sstate, after_sstate);
  }
  inline void run(){
    double start_time = common::startClock();
    produceSubgraph();
    common::stopClock("Selections",start_time);

    if(pcm_init() < 0)
       return;

    graph->print_data("graph.txt");

    start_time = common::startClock();
    queryOver();
    common::stopClock("N-CYCLE",start_time);

    cout << "Count: " << num_cycles << endl << endl;
    pcm_cleanup();
  }
};

*/
//////////////////////////////////////////////////////////////////////////////////////////
//Main setup code
//////////////////////////////////////////////////////////////////////////////////////////
//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]){ 
  Parser input_data = input_parser::parse(argc,argv,"n_cycle");
  /*
  if(input_data.layout.compare("uint") == 0){
    application<uinteger,uinteger> myapp(input_data);
    myapp.run();
  } else if(input_data.layout.compare("bs") == 0){
    application<bitset,bitset> myapp(input_data);
    myapp.run();  
  } else if(input_data.layout.compare("pshort") == 0){
    application<pshort,pshort> myapp(input_data);
    myapp.run();  
  } else if(input_data.layout.compare("hybrid") == 0){
    application<hybrid,hybrid> myapp(input_data);
    myapp.run();  
  } 
  #if COMPRESSION == 1
  else if(input_data.layout.compare("v") == 0){
    application<variant,uinteger> myapp(input_data);
    myapp.run();  
  } else if(input_data.layout.compare("bp") == 0){
    application<bitpacked,uinteger> myapp(input_data);
    myapp.run();
  } 
  #endif
  else{
    cout << "No valid layout entered." << endl;
    exit(0);
  }
  */
  return 0;
}