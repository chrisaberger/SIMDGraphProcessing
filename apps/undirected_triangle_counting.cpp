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
  Matrix* local_graph;
  int num_threads;
  int num_nodes;
  std::atomic<long> triangles;
  uint8_t *result;

public:
  Worker(int node, int num_nodes, int num_threads, MutableGraph* input_graph) {
    this->node = new Node(node);
    this->node->run_on();
    this->local_graph = new Matrix(
      input_graph->out_neighborhoods,
      input_graph->num_nodes, input_graph->num_edges,
      &application::myNodeSelection,
      &application::myEdgeSelection,
      application::graphType);
    this->num_nodes = num_nodes;
    this->num_threads = num_threads;
    this->triangles = 0;
    this->result = new uint8_t[input_graph->num_nodes];
  }

  Worker(int node, int num_nodes, int num_threads, Matrix* input_graph) {
    this->node = new Node(node);
    this->node->run_on();
    this->local_graph = input_graph;
    this->num_nodes = num_nodes;
    this->num_threads = num_threads;
    this->triangles = 0;
    this->result = new uint8_t[input_graph->matrix_size];
  }

  int edgeApply(unsigned int src, unsigned int dst) {
    this->triangles += this->local_graph->row_intersect(this->result, src, dst);
    return 0;
  }

  void run() {
    using namespace std::placeholders;

    this->node->run_on();
    for(int i = 0; i < this->num_threads; i++) {
       auto edge_fun = std::bind(&Worker::edgeApply, this, _1, _2);
       auto col_fun = std::bind(&Matrix::reduce_column_in_row<int>, this->local_graph, _1, _2);
       auto thread_fun = std::bind(&Matrix::part_reduce_row_omp<int>, this->local_graph, std::cref(col_fun), std::cref(edge_fun), this->node->get_id(), this->num_nodes);
       //auto thread_fun = std::bind(&Matrix::part_reduce_row<int>, this->local_graph, std::cref(col_fun), std::cref(edge_fun), i + this->node->get_id() * this->num_threads, this->num_nodes * this->num_threads);
       std::thread* worker_thread = new std::thread(thread_fun);
       this->node->add_thread(worker_thread);
    }

    cout << "Launched " << this->num_threads << " threads on node " << this->node->get_id() << endl;
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

  // Allocate memory where the thread is running
  numa_set_localalloc();
  numa_run_on_node(0);

  int num_nodes = numa_max_node() + 1;
  int num_threads = atoi(argv[2]);
  cout << "Number of nodes: " << num_nodes << endl;
  cout << "Number of threads per node: " << num_threads << endl;

  common::startClock();
  MutableGraph inputGraph = MutableGraph::undirectedFromAdjList(argv[1],1); //filename, # of files

  common::stopClock("Reading File");

  common::startClock();

  Matrix* mat = new Matrix(
      inputGraph.out_neighborhoods,
      inputGraph.num_nodes, inputGraph.num_edges,
      &application::myNodeSelection,
      &application::myEdgeSelection,
      application::graphType);

  cout << "Number of nodes: " << mat->matrix_size << endl;

  std::vector<Worker*> nodes;
  for(int i = 0; i < num_nodes; i++) {
    nodes.push_back(new Worker(i, num_nodes, num_threads, &inputGraph));
    //nodes.push_back(new Worker(i, num_nodes, num_threads, mat));
  }

  common::stopClock("Building Graph");

  common::startClock();
  for(auto worker : nodes) {
    worker->run();
  }

  long sum = 0;
  for(auto worker : nodes) {
    sum += worker->join();
  }
  cout << "Triangles: " << sum << endl;

  common::stopClock("CSR TRIANGLE COUNTING");

  return 0;
}
