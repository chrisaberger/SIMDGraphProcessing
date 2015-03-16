#define WRITE_VECTOR 1

#include "gtest/gtest.h"
#include "n_path.cpp"

TEST(BFSTest, FacebookCliqueCounting) {
  MutableGraph* inputGraph = MutableGraph::directedFromBinary("test/data/dfacebook.bin");
  Parser input_data(4,false,4,0,inputGraph,"uint");
  n_path<uinteger,uinteger> n_path(input_data);
  n_path.run();
  EXPECT_EQ((size_t) 4, n_path.path_length);
}
