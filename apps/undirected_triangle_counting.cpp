// class templates
#include "Matrix.hpp"
#include "MutableGraph.hpp"
#include "Node.hpp"

namespace application{
  Matrix *graph;
  uint8_t *result;
  long num_triangles = 0;
  common::type graphType = common::ARRAY32;
  
  inline bool myNodeSelection(unsigned int node){
    (void)node;
    return true;
  }
  inline bool myEdgeSelection(unsigned int node, unsigned int nbr){
    return nbr < node;
  }
  inline long edgeApply(unsigned int src, unsigned int dst){
    return graph->row_intersect(result,src,dst);
  }
  inline void queryOver(){
    num_triangles = graph->reduce_row(&Matrix::reduce_column_in_row,&edgeApply);
  }
}

class Worker {
private:
  Node* node;
  Matrix *local_graph;

public:
  Worker(int node, MutableGraph* input_graph) {
    this->node = new Node(node);
    this->node->run_on();
    this->local_graph = new Matrix(
      input_graph->out_neighborhoods,
      input_graph->num_nodes, input_graph->num_edges,
      &application::myNodeSelection,
      &application::myEdgeSelection,
      application::graphType);
    this->local_graph->print_data("test.txt");
  }

  void run() {
    cout << local_graph->reduce_row(&Matrix::reduce_column_in_row, &application::edgeApply);
  }
};

//Ideally the user shouldn't have to concern themselves with what happens down here.
int main (int argc, char* argv[]) { 
  if(argc != 3){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of threads>" << endl;
    exit(0);
  }

  numa_set_localalloc();

  int num_nodes = numa_max_node() + 1;
  cout << "Number of nodes: " << num_nodes << endl;
  cout << endl << "Number of threads: " << atoi(argv[2]) << endl;

  common::startClock();
  MutableGraph inputGraph = MutableGraph::undirectedFromAdjList(argv[1],1); //filename, # of files
  application::result = new uint8_t[inputGraph.num_nodes]; //we don't actually use this for just a count
  inputGraph.print_data();
  cout << "hello";
    //for more sophisticated queries this would be used.
  common::stopClock("Reading File");

  common::startClock();

  num_nodes = 1;
  std::vector<Worker> nodes;
  for(int i = 0; i < num_nodes; i++) {
    nodes.push_back(Worker(i, &inputGraph));
  }

  common::stopClock("Building Graph");

  common::startClock();
  for(auto worker : nodes) {
    worker.run();
  }
  common::stopClock("CSR TRIANGLE COUNTING");

  cout << "Count: " << application::num_triangles << endl << endl;

  return 0;
}
