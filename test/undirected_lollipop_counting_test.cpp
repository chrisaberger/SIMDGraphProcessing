#define WRITE_VECTOR 1

#include "gtest/gtest.h"
#include "undirected_lollipop_counting.cpp"

TEST(LollipopTest, FacebookLollipopCountingHybrid) {
  MutableGraph* inputGraph = MutableGraph::undirectedFromBinary("test/data/facebook.bin");
  Parser input_data(4,false,0,0,inputGraph,"hybrid");
  undirected_lollipop_counting<hybrid,hybrid> undirected_lollipop_counting_app(input_data);
  undirected_lollipop_counting_app.run();
  size_t expected_result = 713455740;
  EXPECT_EQ(expected_result, undirected_lollipop_counting_app.num_lollipops);
}
