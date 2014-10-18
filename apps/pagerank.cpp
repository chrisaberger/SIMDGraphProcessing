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

  inline void print_pr_data(string file){
    ofstream myfile;
    myfile.open(file);
    cout << graph->external_ids->size() << endl;
    for (auto iter = graph->external_ids->begin(); iter != graph->external_ids->end(); iter++){
      myfile << "Node: " << iter->first <<"(" << iter->second << ")" << " PR: " << pr_data[iter->second] << endl;
    }
  }
  inline void queryOverNew(){
    const size_t num_nodes = graph->matrix_size;
    float *scaling_array = new float[num_nodes];
    float *new_pr_data = new float[num_nodes];
    pr_data = new float[num_nodes];

    const size_t st_a = (num_nodes / 8) * 8;

    //Initialization code
    const float k = 0.85;
    const float initVal = (1.0/float(num_nodes));

    size_t pr_i=0;
    float add_val = (1.0-k)/num_nodes;

    __m256 add_reg = _mm256_set1_ps(add_val);
    __m256 k_reg = _mm256_set1_ps(k);
    float *tmp_diff = new float[8];
    __m256 initReg = _mm256_set1_ps(initVal);
    #if VECTORIZE == 1
    for(; pr_i<st_a;pr_i+=8){
      for(size_t j=0;j<8;j++){
        scaling_array[pr_i+j] = ((graph->row_lengths[pr_i+j]==0)+graph->row_lengths[pr_i+j]);
      }
      __m256 new_vals = _mm256_div_ps(initReg,_mm256_loadu_ps(&scaling_array[pr_i]));
      new_vals = _mm256_mul_ps(k_reg,new_vals);
      _mm256_storeu_ps(&pr_data[pr_i],new_vals);
    }
    #endif
    for(;pr_i<num_nodes;pr_i++){
      scaling_array[pr_i] = ((graph->row_lengths[pr_i]==0)+graph->row_lengths[pr_i]);
      pr_data[pr_i] = k*(initVal/scaling_array[pr_i]);
    }

    size_t num_iterations = 0;
    while(num_iterations < max_iterations){
      //returns the sum of the difference of new and old for all arrays.
      graph->map_columns(&Matrix::sum_over_rows_in_column,new_pr_data,pr_data);
      size_t pr_i = 0;
      float diff = 0.0;
    
      #if VECTORIZE == 1
      __m256 diff_sse = _mm256_setzero_ps();
      for(; pr_i<st_a;pr_i+=8){
        __m256 new_vals = _mm256_add_ps(_mm256_loadu_ps(&new_pr_data[pr_i]),add_reg);
        new_vals = _mm256_div_ps(new_vals,_mm256_loadu_ps(&scaling_array[pr_i]));
        new_vals = _mm256_mul_ps(k_reg,new_vals);
        _mm256_storeu_ps(&pr_data[pr_i],new_vals);

        __m256 tdiff = _mm256_mul_ps(_mm256_sub_ps(new_vals,_mm256_loadu_ps(&pr_data[pr_i])),_mm256_set1_ps(-1.0)); //take abs
        diff_sse = _mm256_add_ps(diff_sse,tdiff);
      }
      diff += common::_mm256_reduce_add_ps(diff_sse); 
      #endif  
      for(;pr_i<graph->matrix_size;pr_i++){
        float new_val = new_pr_data[pr_i]+add_val;
        float old_data = pr_data[pr_i];
        pr_data[pr_i] = k*(new_val/scaling_array[pr_i]); 
        diff += abs(pr_data[pr_i]-old_data);
      }
      num_iterations++;
    }
    //Need to compensate now.
    pr_i = 0;
    #if VECTORIZE == 1
    for(; pr_i<st_a;pr_i+=8){
      __m256 new_vals = _mm256_mul_ps(_mm256_loadu_ps(&pr_data[pr_i]),_mm256_loadu_ps(&scaling_array[pr_i]));
      new_vals = _mm256_div_ps(new_vals,k_reg);
      _mm256_storeu_ps(&pr_data[pr_i],new_vals);
    } 
    #endif
    for(;pr_i<graph->matrix_size;pr_i++){
      float new_val = (pr_data[pr_i]*scaling_array[pr_i])/k;
      pr_data[pr_i] = new_val; 
    }
    delete[] tmp_diff;
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
      float diff = graph->map_columns_pr(&Matrix::sum_over_rows_in_column_pr,new_pr_data,pr_data);
      diff = diff;
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
  application::queryOverNew();
  common::stopClock("NEW CSR PAGE RANK");
  //application::print_pr_data("pr2.txt");

  common::startClock();
  application::queryOver();
  common::stopClock("CSR PAGE RANK");
    application::graph->Matrix::~Matrix(); 

  //application::print_pr_data("pr1.txt");

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
