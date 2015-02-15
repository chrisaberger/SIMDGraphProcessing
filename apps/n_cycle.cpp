#define WRITE_VECTOR 1

#include "SparseMatrix.hpp"
#include "pcm_helper.hpp"
#include "Table.hpp"
#include "Parser.hpp"

using namespace pcm_helper;

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
    return true;//attribute > 500;
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
    (void) attribute; (void) node; (void) nbr;
    return true;
  }
#endif

  inline void produceSubgraph(){
    auto node_selection = std::bind(&application::myNodeSelection, this, _1, _2);
    auto edge_selection = std::bind(&application::myEdgeSelection, this, _1, _2, _3);

    graph = SparseMatrix<T,R>::from_symmetric_graph(inputGraph,node_selection,edge_selection,num_threads);
  }

  inline void queryOver(){
    system_counter_state_t before_sstate = pcm_get_counter_state();
    server_uncore_power_state_t* before_uncstate = pcm_get_uncore_power_state();

    ParallelBuffer<uint32_t> *x_nbrs_buffers = new ParallelBuffer<uint32_t>(num_threads,graph->matrix_size);
    ParallelBuffer<uint32_t> *y_nbrs_buffers = new ParallelBuffer<uint32_t>(num_threads,graph->matrix_size);
    ParallelBuffer<uint32_t> *z_nbrs_buffers = new ParallelBuffer<uint32_t>(num_threads,graph->matrix_size);
    ParallelBuffer<uint8_t> *ws_buffers = new ParallelBuffer<uint8_t>(num_threads,graph->matrix_size*sizeof(uint32_t));

    const size_t matrix_size = graph->matrix_size;
    size_t *t_count = new size_t[num_threads * PADDING];
    common::par_for_range(num_threads, 0, matrix_size, 10,
      [&] (size_t tid){
        x_nbrs_buffers->allocate(tid);
        y_nbrs_buffers->allocate(tid);
        z_nbrs_buffers->allocate(tid);
        ws_buffers->allocate(tid);
        t_count[tid*PADDING] = 0;
      },
      ////////////////////////////////////////////////////
      [&] (size_t tid, size_t x) {
         long t_num_cycles = 0;
         uint32_t *x_nbrs_buffer = x_nbrs_buffers->data[tid];
         uint32_t *y_nbrs_buffer = y_nbrs_buffers->data[tid];
         uint32_t *z_nbrs_buffer = z_nbrs_buffers->data[tid];

         Set<R> x_nbrs = this->graph->get_decoded_row(x, x_nbrs_buffer);

         x_nbrs.foreach([&] (uint32_t y) {
           x_nbrs.foreach([&] (uint32_t z) {
             if(y > z) {
               Set<R> y_nbrs = this->graph->get_decoded_row(y, y_nbrs_buffer);
               Set<R> z_nbrs = this->graph->get_decoded_row(z, z_nbrs_buffer);

               Set<R> ws(ws_buffers->data[tid]);
               ops::set_intersect(&ws, &y_nbrs, &z_nbrs);
               t_num_cycles += ws.cardinality;

               ws.foreach([&] (uint32_t w) {
                 if(x < w) {
                   t_num_cycles++;
                 }
               });
             }
           });
         });

         t_count[tid*PADDING] += t_num_cycles;
      },
      ////////////////////////////////////////////////////////////
      [this,t_count] (size_t tid){
        num_cycles += t_count[tid*PADDING];
      }
    );

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

    start_time = common::startClock();
    queryOver();
    common::stopClock("4-cycles",start_time);

    cout << "Count: " << num_cycles / 2 << endl << endl;

    common::dump_stats();

    pcm_cleanup();
  }
};

//////////////////////////////////////////////////////////////////////////////////////////
//Main setup code
//////////////////////////////////////////////////////////////////////////////////////////
//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) { 
  Parser input_data = input_parser::parse(argc,argv,"undirected_triangle_counting");

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
