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
  auto graph = SparseMatrix<uinteger, uinteger>::from_symmetric_graph(
      inputGraph,
      node_selection,
      edge_selection,
      1);

  vector<double> densities;

  uint64_t total = graph->matrix_size;
  uint32_t* src_buffer = new uint32_t[total * 4];
  uint64_t total_range = 0;
  uint64_t max_range = 0;
  uint64_t total_card = 0;
  uint64_t max_card = 0;
  double total_density = 0.0;

  for(size_t i = 0; i < total; i++) {
    Set<uinteger> S = graph->get_decoded_row(i, src_buffer);
    uint64_t range = S.data[S.cardinality - 1] - S.data[0];
    double density = (double) S.cardinality / range;

    total_density += density;
    densities.push_back(density);
  }

  std::sort (densities.begin(), densities.end());

  const size_t num_buckets = 100;
  vector<size_t> num_elems(num_buckets);
  for(auto density : densities) {
    num_elems->at((size_t) (density * num_buckets))++;
  }
  double mode = (double) std::distance(num_elems.begin(), std::max_element(num_elems.begin(), num_elems.end())) / num_buckets;
  double median = densities->at(densities.size() / 2);
  double avg_density = ((double) total_density) / total;

  double variance = 0.0;
  for(auto density : densities) {
    variance += (density - avg_density) * (density - avg_density);
  }
  variance /= total;

  std::cout << "Avg. range: " << ((double) total_range) / total << std::endl;
  std::cout << "Max range: " << max_range << std::endl;
  std::cout << "Avg. card: " << ((double) total_card) / total << std::endl;
  std::cout << "Max card: " << max_card << std::endl;
  std::cout << "Avg. density: " << avg_density << std::endl;
  std::cout << "Median. density: " << median << std::endl;
  std::cout << "Density mode: " << mode << std::endl;
  std::cout << "Density variance: " << variance << std::endl;
  std::cout << "Pearson first skewness: " << (mean - mode) / sqrt(variance) << std::endl;
  std::cout << "Pearson second skewness: " << (mean - median) / sqrt(variance) << std::endl;

  std::cout << "Stats: " << std::endl;
  std::cout << (double) common::num_bs / total << "\t";
  std::cout << (double) common::num_pshort / total << "\t";
  std::cout << (double) common::num_uint / total << "\t";
  std::cout << (double) common::num_bp / total << "\t";
  std::cout << (double) common::num_v / total << "\t";
  std::cout << std::endl;
}

