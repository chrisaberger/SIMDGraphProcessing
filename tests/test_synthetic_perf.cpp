// class templates
#include "Matrix.hpp"
#include "MutableGraph.hpp"

namespace application{
  Matrix *graph;
  uint8_t *result;
  long num_triangles = 0;
  common::type graphType = common::A32BITPACKED;
  
  inline bool myNodeSelection(unsigned int node){
    (void)node;
    return true;
  }
  inline bool myEdgeSelection(unsigned int node, unsigned int nbr){
    return nbr < node;
  }
  inline long edgeApply(unsigned int src, unsigned int dst, unsigned int *src_nbrhood){
    //cout << "src: " << src << " dst: " << dst << endl;
    long count = graph->row_intersect(result,src,dst,src_nbrhood);
    //cout << count << endl;
    return count;
  }
  inline void queryOver(){
    using namespace std::placeholders;
    auto edge_fun = std::bind(&edgeApply, _1, _2, _3);
    auto row_fun = std::bind(&Matrix::sum_over_columns_in_row<long>, graph, _1, _2);

    num_triangles = graph->sum_over_rows<long>(row_fun,edge_fun);
  }
}

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) { 
  if(argc != 3){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of threads>" << endl;
    exit(0);
  }

  cout << endl << "Number of threads: " << atoi(argv[2]) << endl;
  omp_set_num_threads(atoi(argv[2]));        

  common::startClock();
  MutableGraph *inputGraph = MutableGraph::undirectedFromEdgeList(argv[1]); //filename, # of files
  application::result = new uint8_t[inputGraph->num_nodes]; //we don't actually use this for just a count
  inputGraph->reorder_by_degree();
  common::stopClock("Reading File");

  vector<string> layout_names;
  layout_names.push_back("BITSET");
  layout_names.push_back("ARRAY16");
  layout_names.push_back("ARRAY32");
  layout_names.push_back("A32BITPACKED");
  layout_names.push_back("VARIANT");
  layout_names.push_back("HYBRID");

  for(uint8_t i =0; i < 6; i++){
    common::type layout = (common::type) i;
    common::startClock();
    application::graph = new Matrix(inputGraph->out_neighborhoods,
      inputGraph->num_nodes,inputGraph->num_edges,
      &application::myNodeSelection,&application::myEdgeSelection,inputGraph->external_ids,layout);
    common::stopClock("Freezing Graph");
    
    common::startClock();
    application::queryOver();
    common::stopClock(layout_names.at(i));
    application::graph->Matrix::~Matrix(); 
    cout << "Count: " << application::num_triangles << endl << endl;
  }
  inputGraph->MutableGraph::~MutableGraph(); 
  return 0;
}
