#include "SparseMatrix.hpp"
#include "MutableGraph.hpp"
#include "pcm_helper.hpp"
#include "Parser.hpp"

template<class T, class R>
class application{
  public:
    SparseMatrix<T,R>* graph;
    MutableGraph *input_graph;
    size_t num_threads;
    string layout;
    size_t query_depth;
    long start_node;

    application(Parser input_data) {
      input_graph = input_data.input_graph; 
      num_threads = input_data.num_threads;
      layout = input_data.layout;
      query_depth = input_data.n;
      start_node = input_data.start_node;
    }

    virtual void run() = 0;
};

template<class T, class R> 
application<T,R>* compute(Parser input_data);

#ifndef GOOGLE_TEST
//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) {
  std::string s(argv[0]);
  size_t count = 0;
  while((s.size()-1-count)>0 && s.compare(s.size()-1-count,1,"/")!=0){
    count++;
  }
  std::string app = s.substr(s.size()-count,count+1);

  Parser input_data = input_parser::parse(argc, argv, app);
  
  common::alloc_scratch_space(512 * input_data.input_graph->max_nbrhood_size * sizeof(uint32_t), input_data.num_threads);

  if(input_data.layout.compare("uint") == 0){
    application<uinteger,uinteger>* myapp = compute<uinteger,uinteger>(input_data);
    myapp->run();
  } else if(input_data.layout.compare("bs") == 0){
    application<bitset,bitset>* myapp = compute<bitset,bitset>(input_data);
    myapp->run();
  } else if(input_data.layout.compare("pshort") == 0){
    application<pshort,pshort>* myapp = compute<pshort,pshort>(input_data);
    myapp->run();
  } else if(input_data.layout.compare("hybrid") == 0){
    application<hybrid,hybrid>* myapp = compute<hybrid,hybrid>(input_data);
    myapp->run();
  } 
  #ifndef PATH
  //BFS is not supported on these layouts.
  else if(input_data.layout.compare("new_type") == 0){
    application<new_type,new_type>* myapp = compute<new_type,new_type>(input_data);
    myapp->run();
  } else if(input_data.layout.compare("bitset_new") == 0){
    application<bitset_new,bitset_new>* myapp = compute<bitset_new,bitset_new>(input_data);
    myapp->run();
  }
  #endif
  #if COMPRESSION == 1
  else if(input_data.layout.compare("v") == 0){
    application<variant,uinteger>* myapp = compute<variant,uinteger>(input_data);
    myapp->run();
  } else if(input_data.layout.compare("bp") == 0){
    application<bitpacked,uinteger>* myapp = compute<bitpacked,uinteger>(input_data);
    myapp->run();
  }
  #endif
  else{
    cout << "No valid layout entered." << endl;
    exit(0);
  }
  return 0;
}
#endif
