// class templates
#include "AOA_Matrix.hpp"
#include "MutableGraph.hpp"

#define PRINT 1

namespace application{
  AOA_Matrix *graph;
  float *pr_data;
  size_t max_iterations;
  
  inline bool myNodeSelection(uint32_t node){
    (void)node;
    return true;
  }
  inline bool myEdgeSelection(uint32_t node, uint32_t nbr){
    (void) node; (void) nbr;
    return true;
  }

  inline void print_pr_data(string file){
    ofstream myfile;
    myfile.open(file);
    for (auto iter = graph->external_ids->begin(); iter != graph->external_ids->end(); iter++){
      myfile << "Node: " << iter->first <<"(" << iter->second << ")" << " PR: " << pr_data[iter->second] << endl;
    }
  }
  
  inline float nbrApply_old(uint32_t nbr){
    return pr_data[nbr]/graph->row_lengths[nbr];
  }
  inline float nbrApply(uint32_t nbr){
    return pr_data[nbr];
  }
  inline void queryOverNew(){
    const size_t num_nodes = graph->matrix_size;
    float *scaling_array = new float[num_nodes];
    float *new_pr_data = new float[num_nodes];
    pr_data = new float[num_nodes];

    auto nbr_function = std::bind(&nbrApply, _1);
    auto col_function = std::bind(&AOA_Matrix::sum_over_rows_in_column<float>,graph,_1,_2);

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
    #pragma omp parallel for default(shared) schedule(static)
    for(pr_i = 0; pr_i<st_a;pr_i+=8){
      for(size_t j=0;j<8;j++){
        scaling_array[pr_i+j] = ((graph->row_lengths[pr_i+j]==0)+graph->row_lengths[pr_i+j]);
      }
      __m256 new_vals = _mm256_div_ps(initReg,_mm256_loadu_ps(&scaling_array[pr_i]));
      new_vals = _mm256_mul_ps(k_reg,new_vals);
      _mm256_storeu_ps(&pr_data[pr_i],new_vals);
    }
    #endif
    #pragma omp parallel for default(shared) schedule(static)
    for(pr_i = st_a ;pr_i<num_nodes;pr_i++){
      scaling_array[pr_i] = ((graph->row_lengths[pr_i]==0)+graph->row_lengths[pr_i]);
      pr_data[pr_i] = k*(initVal/scaling_array[pr_i]);
    }

    size_t num_iterations = 0;
    while(num_iterations < max_iterations){
      //returns the sum of the difference of new and old for all arrays.
      graph->map_columns<float>(col_function,nbr_function,new_pr_data);
      size_t pr_i = 0;
      float diff = 0.0;
    
      #if VECTORIZE == 1
      __m256 diff_sse = _mm256_setzero_ps();
      #pragma omp parallel for default(shared) schedule(static)
      for(pr_i = 0; pr_i<st_a;pr_i+=8){
        __m256 new_vals = _mm256_add_ps(_mm256_loadu_ps(&new_pr_data[pr_i]),add_reg);
        new_vals = _mm256_div_ps(new_vals,_mm256_loadu_ps(&scaling_array[pr_i]));
        new_vals = _mm256_mul_ps(k_reg,new_vals);
        _mm256_storeu_ps(&pr_data[pr_i],new_vals);

        __m256 tdiff = _mm256_mul_ps(_mm256_sub_ps(new_vals,_mm256_loadu_ps(&pr_data[pr_i])),_mm256_set1_ps(-1.0)); //take abs
        diff_sse = _mm256_add_ps(diff_sse,tdiff);
      }
      diff += common::_mm256_reduce_add_ps(diff_sse); 
      #endif  
      
      #pragma omp parallel for default(shared) schedule(static)
      for(pr_i = st_a;pr_i<graph->matrix_size;pr_i++){
        float new_val = new_pr_data[pr_i]+add_val;
        float old_data = pr_data[pr_i];
        pr_data[pr_i] = k*(new_val/scaling_array[pr_i]); 
        diff += abs(pr_data[pr_i]-old_data);
      }
      num_iterations++;
    }
    //Need to compensate now.
    #if VECTORIZE == 1
    #pragma omp parallel for default(shared) schedule(static)
    for(pr_i = 0; pr_i<st_a;pr_i+=8){
      __m256 new_vals = _mm256_mul_ps(_mm256_loadu_ps(&pr_data[pr_i]),_mm256_loadu_ps(&scaling_array[pr_i]));
      new_vals = _mm256_div_ps(new_vals,k_reg);
      _mm256_storeu_ps(&pr_data[pr_i],new_vals);
    } 
    #endif
    #pragma omp parallel for default(shared) schedule(static)
    for(pr_i = st_a;pr_i<graph->matrix_size;pr_i++){
      float new_val = (pr_data[pr_i]*scaling_array[pr_i])/k;
      pr_data[pr_i] = new_val; 
    }
    delete[] tmp_diff;
  }

  inline void queryOver(){
    float *new_pr_data = new float[graph->matrix_size];
    pr_data = new float[graph->matrix_size];
    
    const float init_val = 1.0f/graph->matrix_size;
    #pragma omp parallel for default(none) shared(graph,pr_data) schedule(dynamic)
    for(size_t i = 0; i < graph->matrix_size; i++){
      pr_data[i] = init_val;
    }    

    auto nbr_function = std::bind(&nbrApply_old, _1);
    auto col_function = std::bind(&AOA_Matrix::sum_over_rows_in_column<float>,graph,_1,_2);

    size_t num_iterations = 0;
    while(num_iterations < max_iterations){
      float diff = graph->map_columns_pr<float>(col_function,nbr_function,new_pr_data,pr_data);
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
  if(argc != 5){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of threads> <layout type=bs,a16,a32,hybrid,v,bp> <# iterations>" << endl;
    exit(0);
  }

  cout << endl << "Number of threads: " << atoi(argv[2]) << endl;
  omp_set_num_threads(atoi(argv[2]));        

  std::string input_layout = argv[3];
  application::max_iterations = atoi(argv[4]);

  common::type layout;
  if(input_layout.compare("a32") == 0){
    layout = common::ARRAY32;
  } else if(input_layout.compare("bs") == 0){
    layout = common::BITSET;
  } else if(input_layout.compare("a16") == 0){
    layout = common::ARRAY16;
  } else if(input_layout.compare("hybrid") == 0){
    layout = common::HYBRID_PERF;
  } else if(input_layout.compare("v") == 0){
    layout = common::VARIANT;
  } else if(input_layout.compare("bp") == 0){
    layout = common::A32BITPACKED;
  } else{
    cout << "No valid layout entered." << endl;
    exit(0);
  }

  auto node_selection = std::bind(&application::myNodeSelection, _1);
  auto edge_selection = std::bind(&application::myEdgeSelection, _1, _2);

  common::startClock();
  MutableGraph *inputGraph = MutableGraph::directedFromEdgeList(argv[1]); //filename, # of files
  common::stopClock("Reading File");
  
  common::startClock();
  //inputGraph->reorder_by_degree();
  common::stopClock("Reordering");
 
  cout << endl;

  common::startClock();
  application::graph = AOA_Matrix::from_asymmetric(inputGraph,node_selection,edge_selection,layout);
  common::stopClock("selections");
  
  inputGraph->MutableGraph::~MutableGraph(); 

  common::startClock();
  application::queryOver();
  common::stopClock("APP");
  application::print_pr_data("pr.txt");
  
  common::startClock();
  application::queryOverNew();
  common::stopClock("APP FISION");
  application::print_pr_data("new_pr.txt");
  
  return 0;
}
