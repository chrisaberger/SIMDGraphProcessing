// class templates
#include "SparseMatrix.hpp"
#include "pcm_helper.hpp"
#include "Table.hpp"
#include "Parser.hpp"

using namespace pcm_helper;

template<class T, class R>
class application{
  public:

  SparseMatrix<T,R>* graph;
  long num_cliques;
  MutableGraph *inputGraph;
  size_t num_threads;
  string layout;
  size_t query_depth;

  application(Parser input_data){
    num_cliques = 0;
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
  inline size_t apply_function(size_t node, size_t depth, Set<R> **set_buffers, Table<uint32_t>* decode_buffers, Table<uint64_t> *output){
    Set<R> A(*set_buffers[depth-1]);
    Set<R> B = graph->get_decoded_row(node,decode_buffers->data[depth]);
    Set<R> C = ops::set_intersect(Set<R>(set_buffers[depth]),A,B);    
    set_buffers[depth] = &C;

    size_t count = 0;
    if(++depth == query_depth){
      #if WRITE_TABLE == 1
      output->write_table(C,graph->id_map);
      #endif
      return C.cardinality;
    } else{
      C.foreach([this,&count,depth,set_buffers,output,query_depth,decode_buffers] (uint32_t i){
        output->tuple[depth-1] = graph->id_map[i]; 
        count += this->apply_function(i,depth,set_buffers,decode_buffers,output);
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
    ParallelTable<uint32_t>* decode_buffers = new ParallelTable<uint32_t>(num_threads,query_depth,graph->max_nbrhood_size);
    Set<R> **set_buffers = new Set<R>*[PADDING*query_depth*num_threads];

    common::par_for_range(num_threads,0,matrix_size,100,
      ///////////////////////////////////////////////////////////
      [this,query_depth,set_buffers,decode_buffers,output,graph](size_t tid){
        for(size_t j = 0; j < query_depth; j++){
          set_buffers[PADDING*tid*query_depth+j] = new Set<R>(graph->max_nbrhood_size*sizeof(uint32_t)); 
        }
        decode_buffers->allocate(tid);
        output->allocate(tid);
      },
      //////////////////////////////////////////////////////////
      [this,output,query_depth,set_buffers,decode_buffers](size_t tid, size_t i) {
        Table<uint32_t> *thread_decode_buffers = decode_buffers->table[tid];
        Table<uint64_t> *thread_output = output->table[tid];
        Set<R> **thread_set_buffers = &set_buffers[PADDING*tid*query_depth];

        Set<R> A = this->graph->get_decoded_row(i,thread_decode_buffers->data[query_depth*tid]);
        thread_output->tuple[0] = graph->id_map[i];  
        thread_set_buffers[1] = &A;

        A.foreach([this,tid,query_depth,thread_decode_buffers,thread_set_buffers,thread_output] (uint32_t j){
          thread_output->tuple[1] = graph->id_map[j];  
          thread_output->cardinality += this->apply_function(j,2,thread_set_buffers,thread_decode_buffers,thread_output);
        });
      },
      [this,output](size_t tid){
      /////////////////////////////////////////////////////////
        Table<uint64_t> *thread_output = output->table[tid];
        output->cardinality += thread_output->cardinality;
      }
    );
    num_cliques = output->cardinality;
    
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

    //graph->print_data("graph.txt");

    start_time = common::startClock();
    queryOver();
    common::stopClock("N-CLIQUE",start_time);

    cout << "Count: " << num_cliques << endl << endl;
    pcm_cleanup();
  }
};

//////////////////////////////////////////////////////////////////////////////////////////
//Main setup code
//////////////////////////////////////////////////////////////////////////////////////////
//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]){ 
  Parser input_data = input_parser::parse(argc,argv,"n_clique");

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
  return 0;
}
