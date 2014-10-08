#include "MutableGraph.hpp"

//this is a functor
struct AdjComparator {
  bool operator()(vector<unsigned int> *i,vector<unsigned int> *j) const { 
    return (i->size() > j->size()); 
  }
};
struct NeighborhoodComparator {
  bool operator()(vector<unsigned int> *i,vector<unsigned int> *j) const { 
    return (i->at(0) < j->at(0)); 
  }
};
struct SrcPairComparator {
  bool operator()(pair<unsigned int,unsigned int> i,pair<unsigned int,unsigned int> j) const { 
    return (i.first > j.first); 
  }
};
struct DstPairComparator {
  bool operator()(pair<unsigned int,unsigned int> i,pair<unsigned int,unsigned int> j) const { 
    return (i.second > j.second); 
  }
};
void build_neighborhoods(vector< vector<unsigned int>* > *neighborhoods,vector<pair<unsigned int,unsigned int>> *edges){
  size_t i = 0;
  while(i < edges->size()){
    vector<unsigned int> *cur = new vector<unsigned int>(); //guess a size
    cur->push_back(edges->at(i).first);
    unsigned int prev_src = edges->at(i).first;
    while(i < edges->size() && edges->at(i).first == prev_src){
      cur->push_back(edges->at(i).second);
      i++;
    }
    neighborhoods->push_back(cur);
  }
}
void build_hash(vector< vector<unsigned int>* > *neighborhoods,unordered_map<unsigned int,unsigned int> *extern_ids){
  extern_ids->reserve(neighborhoods->size());
  //#pragma omp parallel for default(none) shared(neighborhoods,extern_ids)
  for(size_t i = 0; i < neighborhoods->size(); ++i) {
    vector<unsigned int> *hood = neighborhoods->at(i);
    if(extern_ids->find(hood->at(0)) == extern_ids->end()){
      extern_ids->insert(make_pair(hood->at(0),i));
    }
  }
}
void reassign_ids(vector< vector<unsigned int>* > *neighborhoods,unordered_map<unsigned int,unsigned int> *extern_ids){
  for(size_t i = 0; i < neighborhoods->size(); ++i) {
    vector<unsigned int> *hood = neighborhoods->at(i);
    for(size_t j = 0; j < hood->size(); ++j) {
      hood->at(j) = extern_ids->at(hood->at(j));
    }
    sort(hood->begin()+1,hood->end());
  }
}
/*
File format

Edges must be duplicated.  If you have edge (0,1) you must
also have (1,0) listed.

src0 dst0 dst1 dst2 dst3 ...
src1 dst0 dst1 dst2 dst3 ...
...

*/
MutableGraph MutableGraph::undirectedFromAdjList(const string path,const int num_files) {
  vector< vector<unsigned int>* >* *graph_in = new vector< vector<unsigned int>* >*[num_files];

  size_t num_edges = 0;
  size_t num_nodes = 0;
  //Place graph into vector of vectors then decide how you want to
  //store the graph.

  //#pragma omp parallel for default(none) shared(graph_in,path) reduction(+:num_edges) reduction(+:num_nodes)
  for(size_t i=0; i < (size_t) num_files;++i){
    vector< vector<unsigned int>*  > *file_adj = new vector< vector<unsigned int>* >();

    string file_path = path;
    if(num_files!=1) file_path.append(to_string(i));

    ifstream myfile (file_path);
    string line;
    if (myfile.is_open()){
      while ( getline (myfile,line) ){
        vector<unsigned int> *cur = new vector<unsigned int>(); //guess a size
        cur->reserve(line.length());
        istringstream iss(line);

        unsigned int sub;
        while(iss >> sub)
          cur->push_back(sub);
        cur->resize(cur->size());
        num_edges += cur->size()-1;
        num_nodes++;
        file_adj->push_back(cur);
      }
      graph_in[i] = file_adj;
    }
  }

  //Serial Merge: Could actually do a merge sort if I cared enough.
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();
  neighborhoods->reserve(num_nodes);
  for(size_t i=0; i < (size_t) num_files;++i){
    neighborhoods->insert(neighborhoods->end(),graph_in[i]->begin(),graph_in[i]->end());
    graph_in[i]->erase(graph_in[i]->begin(),graph_in[i]->end());
  }
  delete[] graph_in;

  //Sort the neighborhoods by degree.
  std::sort(neighborhoods->begin(),neighborhoods->end(),AdjComparator());

  //Build hash map
  unordered_map<unsigned int,unsigned int> *extern_ids = new unordered_map<unsigned int,unsigned int>();
  build_hash(neighborhoods,extern_ids);

  reassign_ids(neighborhoods,extern_ids);

  return MutableGraph(num_nodes,num_edges,true,extern_ids,neighborhoods,neighborhoods); 
  //stopClock("Reassigning ids");
}

/*
File format

Edges must be duplicated.  If you have edge (0,1) you must
also have (1,0) listed.

src0 dst1
dst1 src0
...

*/
MutableGraph MutableGraph::undirectedFromEdgeList(const string path,const int num_files) {
  size_t num_edges = 0;
  size_t num_nodes = 0;

  //Place graph into vector of vectors then decide how you want to
  //store the graph.

  cout << "Reading File: " << path << endl;

  vector<pair<unsigned int,unsigned int>> *edges = new vector<pair<unsigned int,unsigned int>>(); //guess a size

  for(size_t i=0; i < (size_t) num_files;++i){
    string file_path = path;
    if(num_files!=1) file_path.append(to_string(i));

    ifstream myfile (file_path);
    string line;
    if (myfile.is_open()){
      while ( getline (myfile,line) ){
        istringstream iss(line);

        string sub;
        iss >> sub;
        unsigned int src = atoi(sub.c_str());

        iss >> sub;
        unsigned int dst = atoi(sub.c_str());

        edges->push_back(make_pair(src,dst));
      }
    }
  }

  num_edges = edges->size();
  std::sort(edges->begin(),edges->end(),SrcPairComparator());

  //go from edge list to vector of vectors
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();
  build_neighborhoods(neighborhoods,edges);
  num_nodes = neighborhoods->size();
  delete edges;

  //order by degree
  std::sort(neighborhoods->begin(),neighborhoods->end(),AdjComparator());

  //Build hash map
  unordered_map<unsigned int,unsigned int> *extern_ids = new unordered_map<unsigned int,unsigned int>();
  build_hash(neighborhoods,extern_ids);

  //reassign ID's
  reassign_ids(neighborhoods,extern_ids);

  return MutableGraph(num_nodes,num_edges,true,extern_ids,neighborhoods,neighborhoods); 
}
/*
File format

src0 dst0
src1 dst1
...

*/
MutableGraph MutableGraph::directedFromEdgeList(const string path,const int num_files) {
  size_t num_edges = 0;
  size_t num_nodes = 0;
  
  //Place graph into vector of vectors then decide how you want to
  //store the graph.

  cout << "Reading File: " << path << endl;

  vector<pair<unsigned int,unsigned int>> *edges = new vector<pair<unsigned int,unsigned int>>(); //guess a size
  
  for(size_t i=0; i < (size_t) num_files;++i){
    string file_path = path;
    if(num_files!=1) file_path.append(to_string(i));

    ifstream myfile (file_path);
    string line;
    if (myfile.is_open()){
      while ( getline (myfile,line) ){
        istringstream iss(line);
        
        string sub;
        iss >> sub;
        unsigned int src = atoi(sub.c_str());
          
        iss >> sub;
        unsigned int dst = atoi(sub.c_str());

        edges->push_back(make_pair(src,dst));
      }
    }
  }

  num_edges = edges->size();
  unordered_map<unsigned int,unsigned int> *extern_ids = new unordered_map<unsigned int,unsigned int>();

  ////////////////////////////////////////////////////////////////////
  //out edges
  std::sort(edges->begin(),edges->end(),SrcPairComparator());

  //go from edge list to vector of vectors
  vector< vector<unsigned int>*  > *out_neighborhoods = new vector< vector<unsigned int>* >();
  build_neighborhoods(out_neighborhoods,edges);

  //Build hash map
  build_hash(out_neighborhoods,extern_ids);

  ////////////////////////////////////////////////////////////////////
  //in edges
  std::sort(edges->begin(),edges->end(),DstPairComparator());

  //go from edge list to vector of vectors
  vector< vector<unsigned int>*  > *in_neighborhoods = new vector< vector<unsigned int>* >();
  build_neighborhoods(in_neighborhoods,edges);

  //Build hash map
  build_hash(in_neighborhoods,extern_ids);

  //reassign ID's
  reassign_ids(out_neighborhoods,extern_ids);
  reassign_ids(in_neighborhoods,extern_ids);

  std::sort(out_neighborhoods->begin(),out_neighborhoods->end(),NeighborhoodComparator());
  std::sort(in_neighborhoods->begin(),in_neighborhoods->end(),NeighborhoodComparator());

  num_nodes = extern_ids->size();

  delete edges;

  return MutableGraph(num_nodes,num_edges,false,extern_ids,out_neighborhoods,in_neighborhoods); 
}
