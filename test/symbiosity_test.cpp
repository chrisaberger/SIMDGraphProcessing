#define WRITE_VECTOR 0

#include "gtest/gtest.h"
#include "symbiosity.cpp"

TEST(SymbiosityTest, FacebookSymbiosityCountingHybrid) {
  MutableGraph* inputGraph = MutableGraph::directedFromBinary("test/data/dfacebook.bin");
  Parser input_data(4,false,0,0,inputGraph,"hybrid");
  symbiosity<hybrid,hybrid> symbiosity_app(input_data);
  symbiosity_app.run();
  EXPECT_EQ((size_t) 0, symbiosity_app.count);
}

TEST(SymbiosityTest, FacebookSymbiosityCountingHybridUndirected) {
  MutableGraph* inputGraph = MutableGraph::undirectedFromBinary("test/data/facebook.bin");
  Parser input_data(4,false,0,0,inputGraph,"hybrid");
  symbiosity<hybrid,hybrid> symbiosity_app(input_data);
  symbiosity_app.run();
  EXPECT_EQ((size_t) 4039, symbiosity_app.count);
}
