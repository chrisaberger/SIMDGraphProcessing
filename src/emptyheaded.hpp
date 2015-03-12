#include "SparseMatrix.hpp"
#include "MutableGraph.hpp"
#include "pcm_helper.hpp"
#include "Parser.hpp"

template<class T, class R> 
void application(Parser input_data);

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) {
  Parser input_data = input_parser::parse(argc, argv, "undirected_triangle_counting", common::UNDIRECTED);

  common::alloc_scratch_space(512 * input_data.input_graph->max_nbrhood_size * sizeof(uint32_t), input_data.num_threads);

  if(input_data.layout.compare("uint") == 0){
    application<uinteger,uinteger>(input_data);
  }
  /* else if(input_data.layout.compare("bs") == 0){
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
  */
  return 0;
}
