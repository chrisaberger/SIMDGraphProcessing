#define WRITE_VECTOR 0

#include "gtest/gtest.h"
#include "undirected_triangle_counting.cpp"

TEST(TEST1, FACEBOOK_TRIANGLES_HYBRID) {
  MutableGraph* inputGraph = MutableGraph::undirectedFromBinary("test/data/facebook.bin");
  Parser input_data(4,false,0,0,inputGraph,"hybrid");
  undirected_triangle_counting<hybrid,hybrid> triangle_app(input_data);
  triangle_app.run();
  size_t expected_result = 1612010;
  EXPECT_EQ(expected_result, triangle_app.num_triangles);
}
