// class templates
#define WRITE_VECTOR 1

#include "SparseMatrix.hpp"
#include "MutableGraph.hpp"
#include "pcm_helper.hpp"
#include "Parser.hpp"

using namespace pcm_helper;

template<class T, class R>
class application{
  public:
    SparseMatrix<T,R>* graph;
    long num_triangles;
    MutableGraph *inputGraph;
    size_t num_threads;
    string layout;

    application(Parser input_data){
      num_triangles = 0;
      inputGraph = input_data.input_graph; 
      num_threads = input_data.num_threads;
      layout = input_data.layout;
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
      (void) attribute;
      return nbr < node;
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

      const size_t matrix_size = graph->matrix_size;
      common::alloc_scratch_space(512*matrix_size*matrix_size*sizeof(uint32_t),num_threads);


      uint32_t *src_buffer = new uint32_t[matrix_size];
      uint32_t *dst_buffer = new uint32_t[matrix_size];
      uint8_t *result_buffer = new uint8_t[matrix_size*8];

      uint32_t i = 1147;
      uint32_t j = 1081;

      cout << "src: " << i << " dst: " << j << endl;

      Set<R> A = this->graph->get_decoded_row(i,src_buffer);

      size_t index = 0;
      cout << "A type: " << (uint32_t)A.type << endl;
      A.foreach([&index](uint32_t data){
        cout << "Index: " << index++ << " AData: " << data << endl;
      });

      Set<R> B = this->graph->get_decoded_row(j,dst_buffer);

      cout << "B type: " << (uint32_t)B.type << endl;
      i = 0;
      B.foreach([&index](uint32_t data){
        cout <<"Index: " << index++ <<  " BData: " << data << endl;
      });

      Set<R> C(result_buffer);

     size_t tmp_count = ops::set_intersect(&C,&A,&B)->cardinality;
     cout << "Card: " << tmp_count << endl;

      C.foreach([](uint32_t data){
        cout << "CData: " << data << endl;
      });

      server_uncore_power_state_t* after_uncstate = pcm_get_uncore_power_state();
      pcm_print_uncore_power_state(before_uncstate, after_uncstate);
      system_counter_state_t after_sstate = pcm_get_counter_state();
      pcm_print_counter_stats(before_sstate, after_sstate);
  }

  inline void run(){
    double start_time = common::startClock();
    produceSubgraph();
    common::stopClock("Selections",start_time);

    graph->print_data("graph.txt");

    if(pcm_init() < 0)
       return;

    start_time = common::startClock();
    queryOver();
    common::stopClock("UNDIRECTED TRIANGLE COUNTING",start_time);

    cout << "Count: " << num_triangles << endl << endl;

    common::dump_stats();
    
    pcm_cleanup();
  }
};

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
  } else if(input_data.layout.compare("new_type") == 0){
    application<new_type,new_type> myapp(input_data);
    myapp.run();  
  } else if(input_data.layout.compare("bitset_new") == 0){
    application<bitset_new,bitset_new> myapp(input_data);
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
