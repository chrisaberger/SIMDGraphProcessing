// class templates
#include "Matrix.hpp"
#include "MutableGraph.hpp"

#define PRINT 1

namespace application{
  Matrix *graph;
  float *pr_data;
  size_t max_iterations;
  
  inline bool myNodeSelection(unsigned int node){
    (void)node;
    return true;
  }
  inline bool myEdgeSelection(unsigned int node, unsigned int nbr){
    (void) node; (void) nbr;
    return true;
  }

  inline void print_pr_data(){
    ofstream myfile;
    myfile.open("pr.txt");
    for(size_t i = 0; i < graph->matrix_size; i++){
      myfile << "Node: " << graph->external_ids->at(i) << " PR: " << pr_data[i] << endl;
    }
  }

  inline void queryOver(){
    float *scaling_array = new float[graph->matrix_size];
    float *new_pr_data = new float[graph->matrix_size];
    pr_data = new float[graph->matrix_size];
    float init_val = 1.0f/graph->matrix_size;
    for(size_t i = 0; i < graph->matrix_size; i++){
      pr_data[i] = init_val;
      scaling_array[i] = graph->row_lengths[i]*0.85+0.0000001; //to avoid non-zero #'s
    }    
    size_t num_iterations = 0;
    while(num_iterations < max_iterations){
      size_t st_a = (graph->matrix_size / 8) * 8;
      size_t pr_i = 0;
      while(pr_i < st_a){
        _mm256_storeu_ps(&pr_data[pr_i],_mm256_div_ps(_mm256_loadu_ps(&pr_data[pr_i]),_mm256_loadu_ps(&scaling_array[pr_i])));
        pr_i += 8;
      }
      while(pr_i < graph->matrix_size){
        pr_data[pr_i] = pr_data[pr_i]/scaling_array[pr_i];
        pr_i++;
      }

      //returns the sum of the difference of new and old for all arrays.
      float diff = graph->map_columns(&Matrix::sum_over_rows_in_column,new_pr_data,pr_data);
      float *tmp = pr_data;
      pr_data = new_pr_data;
      new_pr_data = tmp;
      num_iterations++;
    }
  }
}

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) { 
  if(argc != 4){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of threads> <# iterations>" << endl;
    exit(0);
  }

  cout << endl << "Number of threads: " << atoi(argv[2]) << endl;
  omp_set_num_threads(atoi(argv[2]));        
  application::max_iterations = atoi(argv[3]);

  common::startClock();
  cout << "file: " << argv[1] << endl;
  MutableGraph *inputGraph = MutableGraph::directedFromEdgeList(argv[1],1); //filename, # of files
  //for more sophisticated queries this would be used.
  common::stopClock("Reading File");
  

  cout << endl;
  application::graph = new Matrix(inputGraph->out_neighborhoods,inputGraph->in_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,
    inputGraph->external_ids,common::ARRAY32);
  //application::graph->print_data("matrix.txt");
  
  common::startClock();
  application::queryOver();
  common::stopClock("CSR PAGE RANK");
  application::graph->Matrix::~Matrix(); 

  //application::print_pr_data();
  
  /*
  cout << endl;
  application::graph = new Matrix(inputGraph->out_neighborhoods,inputGraph->in_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,common::ARRAY16);
  common::startClock();
  application::queryOver();
  common::stopClock("ARRAY16 PAGE RANK");
  application::graph->Matrix::~Matrix(); 

    cout << endl;
  application::graph = new Matrix(inputGraph->out_neighborhoods,inputGraph->in_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,common::HYBRID);
  common::startClock();
  application::queryOver();
  common::stopClock("HYBRID PAGE RANK");
  application::graph->Matrix::~Matrix(); 

    cout << endl;
  application::graph = new Matrix(inputGraph->out_neighborhoods,inputGraph->in_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,common::VARIANT);
  common::startClock();
  application::queryOver();
  common::stopClock("VARIANT PAGE RANK");
  application::graph->Matrix::~Matrix(); 

  cout << endl;
  application::graph = new Matrix(inputGraph->out_neighborhoods,inputGraph->in_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,common::A32BITPACKED);
  common::startClock();
  application::queryOver();
  common::stopClock("A32BITPACKED PAGE RANK");
  application::graph->Matrix::~Matrix(); 
  */
  return 0;
}
