#define WRITE_VECTOR 0

#include "SparseMatrix.hpp"
#include "MutableGraph.hpp"

int main(int argc, char* argv[]) {
  if(argc != 3) {
    std::cout << "print_dataset_stats <binary graph file> <prune>" << std::endl;
  }

  auto node_selection =
    [] (uint32_t node, uint32_t attribute) -> bool {
      (void)node; (void) attribute;
      return true;
    };

  bool prune = std::string("true").compare(argv[2]);
  auto edge_selection = [&] (uint32_t node, uint32_t nbr, uint32_t attribute) -> bool {
      (void) attribute;
      return prune ? nbr < node : true;
  };

  MutableGraph *inputGraph = MutableGraph::undirectedFromBinary(argv[1]);
  auto graph = SparseMatrix<hybrid, hybrid>::from_symmetric_graph(
      inputGraph,
      node_selection,
      edge_selection,
      1);

  uint64_t total = graph->matrix_size;
  std::cout << (double) common::num_bs / total << "\t";
  std::cout << (double) common::num_pshort / total << "\t";
  std::cout << (double) common::num_uint / total << "\t";
  std::cout << (double) common::num_bp / total << "\t";
  std::cout << (double) common::num_v / total << "\t";
  std::cout << std::endl;
}

