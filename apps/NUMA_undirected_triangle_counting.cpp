#include <atomic>

#include "Matrix.hpp"
#include "MutableGraph.hpp"
#include "Node.hpp"

namespace application{
  //Matrix *graph;
  long num_triangles = 0;
  common::type graphType = common::ARRAY32;

  inline bool myNodeSelection(unsigned int node){
    (void)node;
    return true;
  }
  inline bool myEdgeSelection(unsigned int node, unsigned int nbr){
    return nbr < node;
  }
}

/*long edgeApply(void* graph, unsigned int src, unsigned int dst) {
  return graph->row_intersect(result, src, dst);
}*/

class Worker {
private:
  Node* node;
  Matrix* graph;
  int num_nodes;
  std::atomic<long> triangles;
  uint8_t *result;

public:
  static void* operator new(std::size_t sz) {
    return ::operator new(sz);
  }

  static void* operator new(std::size_t sz, int32_t node) {
    numa_set_preferred(node);
    return numa_alloc_onnode(sz, node);
  }

  static void operator delete(void* ptr) {
  }

  Worker(int node, int num_nodes, MutableGraph* input_graph, common::type layout) {
    numa_run_on_node(node);
    numa_set_preferred(node);

    this->node = new Node(node);

    /*
    this->graph = new (node) Matrix(
      node,
      input_graph->out_neighborhoods,
      input_graph->num_nodes, input_graph->num_edges,
      &application::myNodeSelection,
      &application::myEdgeSelection,
      application::graphType);
      */

    new Matrix(input_graph->out_neighborhoods,
       input_graph->num_nodes,input_graph->num_edges,
       &application::myNodeSelection,&application::myEdgeSelection,input_graph->external_ids,layout);
    this->num_nodes = num_nodes;
    this->result = new uint8_t[input_graph->num_nodes];
  }

  Worker(int node, int num_nodes, Matrix* input_graph) {
    this->node = new Node(node);
    this->node->run_on();
    this->graph = input_graph;
    this->num_nodes = num_nodes;
    this->triangles = 0;
    this->result = new uint8_t[input_graph->matrix_size];
  }

  int edgeApply(unsigned int src, unsigned int dst, unsigned int *src_nbrhood){
    long count = graph->row_intersect(result,src,dst,src_nbrhood);
    return count;
  }

  void run(int num_threads) {
    using namespace std::placeholders;

    this->triangles = 0;
    this->node->run_on();

    for(int i = 0; i < num_threads; i++) {
       auto edge_fun = std::bind(&Worker::edgeApply, this, _1, _2, _3);
       auto col_fun = std::bind(&Matrix::sum_over_columns_in_row<int>, this->graph, _1, _2);
       //auto thread_fun = std::bind(&Matrix::part_reduce_row_omp<int>, this->local_graph, std::cref(col_fun), std::cref(edge_fun), this->node->get_id(), this->num_nodes);
       auto thread_fun = std::bind(&Matrix::sum_over_rows_part<int>, this->graph, std::cref(col_fun), std::cref(edge_fun), i + this->node->get_id() * num_threads, this->num_nodes * num_threads);
       std::thread* worker_thread = new std::thread(thread_fun);
       this->node->add_thread(worker_thread);
    }

    cout << "Launched " << num_threads << " threads on node " << this->node->get_id() << endl;
  }

  long join() {
     this->node->join_threads();
     return this->triangles;
  }
};

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) {
  if(argc != 3){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of threads per node>" << endl;
    exit(0);
  }

  ofstream outfile;
  outfile.open("times.txt");

  int num_threads = atoi(argv[2]);

  int num_nodes = 1;
  if(numa_available() < 0) {
     cout << "Warning: NUMA API not supported" << endl;
  }
  else {
     num_nodes = numa_max_node() + 1;
     cout << "NUMA API supported" << endl;
     cout << "Number of nodes: " << num_nodes << endl;
  }

  numa_run_on_node(2);

  common::type layout = common::ARRAY32;

  // Allocate memory where the thread is running
  //numa_set_localalloc();

  // Read file
  common::startClock();
  MutableGraph *inputGraph = MutableGraph::undirectedFromBinary(argv[1]);
  common::stopClock("Reading File");

 common::startClock();
  inputGraph->reorder_by_degree();
  common::stopClock("Reordering");

  common::startClock();

  Matrix* mat = new Matrix(inputGraph->out_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &application::myNodeSelection,&application::myEdgeSelection,inputGraph->external_ids,layout);

  cout << "Number of nodes: " << mat->matrix_size << endl;

  std::vector<Worker*> nodes;
  for(int32_t i = 0; i < num_nodes; i++) {
    nodes.push_back(new (i) Worker(i, num_nodes, inputGraph, layout));
    //nodes.push_back(new Worker(i, num_nodes, mat));
  }

  common::stopClock("Building Graph");

  for(int i = 1; i <= 24; i++) {
     common::startClock();
     for(auto worker : nodes) {
       worker->run(i);
     }

     long sum = 0;
     for(auto worker : nodes) {
       sum += worker->join();
     }
     cout << "Triangles: " << sum << endl;
     cout << i << endl;

     double time = common::stopClock("CSR TRIANGLE COUNTING");
     outfile << i << "\t" << time << endl;
  }

  outfile.close();
  return 0;
}
