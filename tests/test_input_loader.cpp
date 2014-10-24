#include <stdio.h>
#include <stdlib.h>
#include "Matrix.hpp"
#include "MutableGraph.hpp"

inline bool myNodeSelection(unsigned int node){
  (void)node;
  return true;
}
inline bool myEdgeSelection(unsigned int node, unsigned int nbr){
  (void)node;
  (void)nbr;
  return true;
}

int main (int argc, char* argv[]) {

  /*
  MutableGraph *inputGraph = MutableGraph::undirectedFromEdgeList(argv[1],1);
  cout << "Loaded edge list" << endl;
  
  Matrix *m = new Matrix(inputGraph->out_neighborhoods,
    inputGraph->num_nodes,inputGraph->num_edges,
    &myNodeSelection,&myEdgeSelection,inputGraph->external_ids,common::ARRAY32);
  
  m->print_data("output.txt");
  */

  unsigned int *dat = new unsigned int[10];
  for(size_t i = 0; i < 10; i++){
    dat[i] = i;
  }

  ofstream outfile;
  outfile.open("hello.dat", ios::binary | ios::out);
  // rest of program
  outfile.write((char *)&dat[0],40);
  outfile.write((char *)&dat[0],40);
  outfile.close();

  unsigned int *new_dat = new unsigned int[10];
  ifstream infile; 
  infile.open("hello.dat", ios::binary | ios::in); 
  infile.read((char *)&new_dat[0], 40); // reads 7 bytes into a cell that is either 2 or 4 
  for(size_t i = 0; i < 10; i++){
    cout << new_dat[i] << endl;
  }

  infile.close();
  return 0;
}
