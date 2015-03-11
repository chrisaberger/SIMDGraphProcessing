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
  vector<size_t> cardinalities;

  uint64_t total = graph->matrix_size;
  uint32_t* src_buffer = new uint32_t[total * 4];
  uint64_t total_range = 0;
  uint64_t max_range = 0;
  uint64_t total_card = 0;
  uint64_t max_card = 0;
  double total_density = 0.0;

  for(size_t i = 0; i < total; i++) {
    Set<uinteger> S = graph->get_decoded_row(i, src_buffer);
    uint32_t* data = (uint32_t*) S.data;
    uint64_t range = data[S.cardinality - 1] - data[0];
    total_range += range;
    total_card += S.cardinality;
    max_range = std::max(range, max_range);
    max_card = std::max(S.cardinality, max_card);

    cardinalities.push_back(S.cardinality);

    if(S.cardinality > 1) {
      double density = (double) S.cardinality / (range + 1);

      if(density > 1.0) {
        std::cout << density << " " << data[0] << " " << data[S.cardinality - 1] << " " << S.cardinality << std::endl;
      }

      total_density += std::min(density, 1.0);
      densities.push_back(std::min(density, 1.0));
    }
  }

  std::sort (densities.begin(), densities.end());
  std::sort (cardinalities.begin(), cardinalities.end());

  const size_t num_buckets = 10000;
  vector<size_t> num_elems(num_buckets + 1);
  for(auto density : densities) {
    num_elems.at((size_t) (density * num_buckets))++;
  }

  vector<size_t> num_cards(max_card + 1);
  for(auto card : cardinalities) {
    num_cards.at(card)++;
  }

  double mode = (double) std::distance(num_elems.begin(), std::max_element(num_elems.begin(), num_elems.end())) / num_buckets;
  double card_mode = std::distance(num_cards.begin(), std::max_element(num_cards.begin(), num_cards.end()));
  double median = densities.at(densities.size() / 2);
  double card_median = cardinalities.at(cardinalities.size() / 2);
  double avg_density = ((double) total_density) / total;
  double card_avg = ((double) total_card) / total;

  double variance = 0.0;
  for(auto density : densities) {
    variance += (density - avg_density) * (density - avg_density);
  }
  variance /= total;

  double card_variance = 0.0;
  for(auto card : cardinalities) {
    card_variance += (card - card_avg) * (card - card_avg);
  }
  card_variance /= total;

  std::cout << "Avg. range: " << ((double) total_range) / total << std::endl;
  std::cout << "Max range: " << max_range << std::endl;
  std::cout << "Avg. card: " << ((double) total_card) / total << std::endl;
  std::cout << "Max card: " << max_card << std::endl;
  std::cout << "Avg. density: " << avg_density << std::endl;
  std::cout << "Median. density: " << median << std::endl;
  std::cout << "Density mode: " << mode << std::endl;
  std::cout << "CARD mode: " << card_mode << std::endl;
  std::cout << "CARD variance: " << card_variance << std::endl;
  std::cout << "Density variance: " << variance << std::endl;
  std::cout << "Pearson first skewness: " << 3.0 * (avg_density - mode) / sqrt(variance) << std::endl;
  std::cout << "Pearson second skewness: " << 3.0 * (avg_density - median) / sqrt(variance) << std::endl;
  std::cout << "CARD Pearson first skewness: " << 3.0 * (card_avg - card_mode) / sqrt(card_variance) << std::endl;
  std::cout << "CARD Pearson second skewness: " << 3.0 * (card_avg - card_median) / sqrt(card_variance) << std::endl;

  std::cout << "Stats: " << std::endl;
  std::cout << (double) common::num_bs / total << "\t";
  std::cout << (double) common::num_pshort / total << "\t";
  std::cout << (double) common::num_uint / total << "\t";
  std::cout << (double) common::num_bp / total << "\t";
  std::cout << (double) common::num_v / total << "\t";
  std::cout << std::endl;
}

