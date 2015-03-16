#define WRITE_VECTOR 0

#include "gtest/gtest.h"
#include "similar_nodes.cpp"

TEST(SimilarNodesTest, SimilarNodesCountingFacebookHybrid) {
  MutableGraph* inputGraph = MutableGraph::undirectedFromBinary("test/data/facebook.bin");
  Parser input_data(4,false,0,0,inputGraph,"hybrid");
  similar_nodes<hybrid,hybrid> similar_nodes_app(input_data);
  similar_nodes_app.run();
  size_t expected_result = 904;
  EXPECT_EQ(expected_result, similar_nodes_app.count);
}
