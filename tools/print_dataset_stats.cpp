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

  bool prune = (std::string("true").compare(argv[2]) == 0);
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
  uint32_t* src_buffer = new uint32_t[total * 4];
  uint64_t total_range = 0;
  uint64_t max_range = 0;
  uint64_t total_card = 0;
  uint64_t max_card = 0;
  for(size_t i = 0; i < total; i++) {
    Set<hybrid> S = graph->get_decoded_row(i, src_buffer);

    int64_t min_elem = -1;
    int64_t max_elem = -1;
    S.foreach([&] (uint32_t x) {
      if(min_elem == -1) {
        min_elem = x;
      }
      max_elem = x;
    });

    uint64_t range = max_elem - min_elem;
    max_range = max(range, max_range);
    total_range += range;

    total_card += S.cardinality;
    max_card = max(S.cardinality, max_card);
  }

  std::cout << "Avg. range: " << ((double) total_range) / total << std::endl;
  std::cout << "Max range: " << max_range << std::endl;
  std::cout << "Avg. card: " << ((double) total_card) / total << std::endl;
  std::cout << "Max card: " << max_card << std::endl;

  std::cout << "Stats: " << std::endl;
  std::cout << (double) common::num_bs / total << "\t";
  std::cout << (double) common::num_pshort / total << "\t";
  std::cout << (double) common::num_uint / total << "\t";
  std::cout << (double) common::num_bp / total << "\t";
  std::cout << (double) common::num_v / total << "\t";
  std::cout << std::endl;
}

