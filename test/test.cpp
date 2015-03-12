#define WRITE_VECTOR 0

#include "gtest/gtest.h"
#include "undirected_triangle_counting.cpp"

TEST(TEST1, FACEBOOK_TRIANGLES_HYBRID) {
  MutableGraph* inputGraph = MutableGraph::undirectedFromBinary("test/facebook.bin");
  Parser input_data(4,false,0,0,inputGraph,"hybrid");
  int num_triangles = (int)application<hybrid,hybrid>(input_data);
  EXPECT_EQ(1612010, num_triangles);
}
