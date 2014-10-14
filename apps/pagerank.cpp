// class templates
#include "Matrix.hpp"
#include "MutableGraph.hpp"

namespace application{
  Matrix *graph;
  common::type graphType = common::A32BITPACKED;
  
  inline bool myNodeSelection(unsigned int node){
    (void)node;
    return true;
  }
  inline bool myEdgeSelection(unsigned int node, unsigned int nbr){
    (void) node;
    (void) nbr;
    return true;
  }

  inline void queryOver(){
    float *new_pr_data = new float[graph->matrix_size];
    float *old_pr_data = new float[graph->matrix_size];
    memset (old_pr_data,(1.0/graph->matrix_size),graph->matrix_size);

    size_t num_iterations = 0;
    while(num_iterations < 10){
      float diff = graph->map_columns(&Matrix::sum_over_rows_in_column,new_pr_data,old_pr_data);
      float *tmp = old_pr_data;
      old_pr_data = new_pr_data;
      new_pr_data = tmp;
      num_iterations++;
    }
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
  cout << "file: " << argv[1] << endl;
  MutableGraph inputGraph = MutableGraph::undirectedFromEdgeList(argv[1],1); //filename, # of files
  //for more sophisticated queries this would be used.
  common::stopClock("Reading File");
  /*
  cout << endl;
  application::graph = new Matrix(inputGraph.out_neighborhoods,inputGraph.in_neighborhoods,
    inputGraph.num_nodes,inputGraph.num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,common::ARRAY32);
  common::startClock();
  application::queryOver();
  common::stopClock("CSR PAGE RANK");
  application::graph->Matrix::~Matrix(); 
  */
  /*
  cout << endl;
  application::graph = new Matrix(inputGraph.out_neighborhoods,inputGraph.in_neighborhoods,
    inputGraph.num_nodes,inputGraph.num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,common::ARRAY16);
  common::startClock();
  application::queryOver();
  common::stopClock("ARRAY16 PAGE RANK");
  application::graph->Matrix::~Matrix(); 

    cout << endl;
  application::graph = new Matrix(inputGraph.out_neighborhoods,inputGraph.in_neighborhoods,
    inputGraph.num_nodes,inputGraph.num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,common::HYBRID);
  common::startClock();
  application::queryOver();
  common::stopClock("HYBRID PAGE RANK");
  application::graph->Matrix::~Matrix(); 

    cout << endl;
  application::graph = new Matrix(inputGraph.out_neighborhoods,inputGraph.in_neighborhoods,
    inputGraph.num_nodes,inputGraph.num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,common::VARIANT);
  common::startClock();
  application::queryOver();
  common::stopClock("VARIANT PAGE RANK");
  application::graph->Matrix::~Matrix(); 

  cout << endl;
  application::graph = new Matrix(inputGraph.out_neighborhoods,inputGraph.in_neighborhoods,
    inputGraph.num_nodes,inputGraph.num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,common::A32BITPACKED);
  common::startClock();
  application::queryOver();
  common::stopClock("A32BITPACKED PAGE RANK");
  application::graph->Matrix::~Matrix(); 
  */
  return 0;
}
