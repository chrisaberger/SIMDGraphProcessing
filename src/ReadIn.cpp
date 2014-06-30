// reading a text file
#include "ReadIn.hpp"

using namespace std;

//this is a functor
struct Comparator {
  bool operator()(vector<size_t> *i,vector<size_t> *j) const { 
    return (i->size() > j->size()); 
  }
};

static inline VectorGraph* ReadFile (string path,int num) {
  vector< vector<size_t>*  > *neighborhoods = new vector< vector<size_t>* >();

  neighborhoods->reserve(100000000);

  size_t num_edges = 0;
  string holder = path;
  //Place graph into vector of vectors then decide how you want to
  //store the graph.
  for(size_t i=0; i <= (size_t) num;++i){
    path = holder;
    if(num!=0) path.append(to_string(i));
    string line;
    ifstream myfile (path);
    if (myfile.is_open()){
      while ( getline (myfile,line) ){
        vector<size_t> *cur = new vector<size_t>(); //guess a size
        cur->reserve(line.length());
        istringstream iss(line);
        do{
          string sub;
          iss >> sub;
          if(sub.compare("")){
            cur->push_back(atoi(sub.c_str()));
          }
        } while (iss);
        cur->resize(cur->size());   
        num_edges += cur->size()-1;
        neighborhoods->push_back(cur);
      }
    }
  }
  neighborhoods->resize(neighborhoods->size());

  //Sort the neighborhoods by degree.
  std::sort(neighborhoods->begin(),neighborhoods->end(),Comparator());

  //Build hash map
  unordered_map<size_t,size_t> *external_ids = new unordered_map<size_t,size_t>();
  external_ids->reserve(neighborhoods->size());
  for(size_t i = 0; i < neighborhoods->size(); ++i) {
    vector<size_t> *hood = neighborhoods->at(i);
    external_ids->insert(make_pair(hood->at(0),i));
    hood->erase(hood->begin());
  }
  size_t num_nodes = neighborhoods->size();

  cout << "Finished reading file :)" << endl;
  for(size_t i = 0; i < neighborhoods->size(); ++i) {
    vector<size_t> *hood = neighborhoods->at(i);
    //cout << "Node: " << i << endl;
    for(size_t j = 0; j < hood->size(); ++j) {
      hood->at(j) = external_ids->find(hood->at(j))->second;
    }
    sort(hood->begin(),hood->end());
    /*
    for(size_t j = 0; j < hood->size(); ++j) {
      //cout << hood->at(j) << endl;
    }
    */
  } 

  return new VectorGraph(num_nodes,num_edges,external_ids,neighborhoods);

}

