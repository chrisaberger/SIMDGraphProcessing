#define WRITE_VECTOR 1

#include "gtest/gtest.h"
#include "n_clique.cpp"

TEST(CliqueTest, FacebookCliqueCounting) {
  MutableGraph* inputGraph = MutableGraph::undirectedFromBinary("test/data/facebook.bin");
  Parser input_data(4,false,4,0,inputGraph,"hybrid");
  n_clique<hybrid,hybrid> clique_app(input_data);
  clique_app.run();
  EXPECT_EQ(30004668, clique_app.num_cliques);
}
