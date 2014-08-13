// class templates
#include "Common.hpp"
#include <math.h>

using namespace std;

template<class T> bool vectorExist (vector<T> c, T item){
  return (std::find(c.begin(), c.end(), item) != c.end());
}

template<class T> vector<T> vectorUnion (vector<T> a, vector<T> b){
  vector<T> c;

  std::sort(a.begin(), a.end());
  std::sort(b.begin(), b.end());

  auto i = a.begin();
  auto j = b.begin();

  while (i != a.end() || j != b.end())
  {
    if (j == b.end() || *i < *j)
    {
      if(!exist(c,*i)) c.insert(*i);
      i++;
    }
    else
    {
      if(!exist(c,*j)) c.insert(*j);
      j++;
    }
  }

  return c;
}

void runStats(VectorGraph *vg){
  const size_t num_nodes = vg->num_nodes;
  const size_t num_edges = vg->num_edges;

  cout << "NUMBER OF NODES: " << num_nodes << " NUMBER OF EDGES: " << num_edges << endl;

  size_t NUM_LEVELS = 2;
  size_t power = (size_t) log2(num_nodes/4);
  size_t numNodesInTrie = pow(2,power);

  vector< vector<size_t>* >* *vector_trie = new vector< vector<size_t>* >*[NUM_LEVELS];
  
  //Setup the first level.
  vector< vector<size_t>* > *level = new vector< vector<size_t>* >();
  for(size_t i=0;i<numNodesInTrie;++i){
    vector<size_t> *partition = vg->neighborhoods->at(i);
    level->push_back(partition);
  }
  vector_trie[0] = level;

  //Do all the levels except the very bottom.
  int prev_num_packets = numNodesInTrie;
  for(size_t i=1;i<NUM_LEVELS;++i){
    cout << "LEVEL: " << i << endl;
    size_t levelpower = power*i/(NUM_LEVELS-1); 
    size_t numPackets = pow(2,power-levelpower);
    cout << "LEVELPOWER: " << levelpower << " NUM PACKETS: " << numPackets << endl;
    vector< vector<size_t>* >* level = new vector< vector<size_t>* >();
    for(size_t j=0;j<numPackets;++j){
      size_t packetStart = prev_num_packets*j/numPackets;
      size_t packetEnd = prev_num_packets*(j+1)/numPackets;

      vector<size_t> *partition = vector_trie[i-1]->at(packetStart); //guess a size
      for(size_t k=packetStart+1;k<packetEnd;k++){
        vector<size_t> *new_part = new vector<size_t>();

        vector<size_t> *cur = vector_trie[i-1]->at(k); //guess a size
        cout << "CUR SIZE: " << cur->size() << " PARITION SIZE: " << partition->size() << endl;
        set_intersection(partition->begin(),partition->end(),cur->begin(),cur->end(),back_inserter(*new_part));
        cout << "Level " << i << " SIZE: " << new_part->size() << endl;

        //partition = new_part;
      }
      level->push_back(partition);

      vector< vector<size_t>* >* prev_level = new vector< vector<size_t>* >();
      for(size_t k=packetStart;k<packetEnd;k++){
        vector<size_t> new_prev_part;
        vector<size_t> *cur = vector_trie[i-1]->at(k); //guess a size
        set_difference(partition->begin(),partition->end(),cur->begin(),cur->end(),back_inserter(new_prev_part));
        prev_level->push_back(&new_prev_part);
      }

      //cout << "PACKET START: " << packetStart << " PACKET END: " << packetEnd << endl;
    }
    prev_num_packets = numPackets;
    vector_trie[i] = level;
    cout << endl;
  }

  /*
  vector<size_t> un;
  size_t nbrhoodSize = 0;
  //Used to be stats
  for(size_t i = 0; i < cutoff; ++i) {
    vector<size_t> *hood = vg->neighborhoods->at(i);
    nbrhoodSize += hood->size(); 
  }
  cout << "Raw: " << nbrhoodSize << " Union: " << un.size() << endl;
  */

}

int main (int argc, char* argv[]) { 
  if(argc != 4){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of files> <# of threads>" << endl;
    exit(0);
  } 

  cout << "Number of threads: " << atoi(argv[3]) << endl;
  omp_set_num_threads(atoi(argv[3]));      

  VectorGraph *vg = ReadFile(argv[1],atoi(argv[2]));
  runStats(vg);

  return 0;
}
