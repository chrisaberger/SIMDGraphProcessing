#include "MutableGraph.hpp"

//this is a functor
bool sortPairs(pair<unsigned int,unsigned int> i,pair<unsigned int,unsigned int> j) {
  if(i.first == j.first){
    return i.second > j.second;
  } else{
    return i.first > j.first;
  }
}
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
void build_out_neighborhoods(vector< vector<unsigned int>* > *neighborhoods,vector<pair<unsigned int,unsigned int>> *edges){
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
void build_in_neighborhoods(vector< vector<unsigned int>* > *neighborhoods,vector<pair<unsigned int,unsigned int>> *edges){
  size_t i = 0;
  while(i < edges->size()){
    vector<unsigned int> *cur = new vector<unsigned int>(); //guess a size
    cur->push_back(edges->at(i).second);
    unsigned int prev_src = edges->at(i).second;
    while(i < edges->size() && edges->at(i).second == prev_src){
      cur->push_back(edges->at(i).first);
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
        do{
          string sub;
          iss >> sub;
          if(sub.compare("")){
            cur->push_back(atoi(sub.c_str()));
          }
        } while (iss);
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
  ////////////////////////////////////////////////////////////////////////////////////
  //Place graph into vector of vectors then decide how you want to
  //store the graph.
  vector<pair<unsigned int,unsigned int>> *edges = new vector<pair<unsigned int,unsigned int>>(); //guess a size

  cout << path << endl;
  FILE *pFile = fopen(path.c_str(),"r");
  if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

  // obtain file size:
  fseek(pFile,0,SEEK_END);
  size_t lSize = ftell(pFile);
  rewind(pFile);

  // allocate memory to contain the whole file:
  char *buffer = (char*) malloc (sizeof(char)*lSize);
  edges->reserve(lSize/2);
  if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

  // copy the file into the buffer:
  size_t result = fread (buffer,1,lSize,pFile);
  if (result != lSize) {fputs ("Reading error",stderr); exit (3);}

  char *test = strtok(buffer," \t\n");
  
  while(test != NULL){
    unsigned int src;
    sscanf(test,"%u",&src);
    test = strtok(NULL," \t\n");
    
    unsigned int dst;
    sscanf(test,"%u",&dst);
    test = strtok(NULL," \t\n");

    edges->push_back(make_pair(src,dst));
    edges->push_back(make_pair(dst,src));
  }
  // terminate
  fclose(pFile);
  free(buffer);

  sort(edges->begin(),edges->end());
  edges->erase(unique(edges->begin(),edges->begin()+edges->size()),edges->end());
  //////////////////////////////////////////////////////////////////////////////////////////////////
  size_t num_edges = edges->size();
  size_t num_nodes = 0;
  //Setup for my flat map
  std::sort(edges->begin(),edges->end(),SrcPairComparator());

  //go from edge list to vector of vectors
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();
  build_out_neighborhoods(neighborhoods,edges);
  num_nodes = neighborhoods->size();
  delete edges;

  cout << "num nodes: " << num_nodes << " num_edges: " << num_edges << endl;

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

  vector<pair<unsigned int,unsigned int>> *edges = new vector<pair<unsigned int,unsigned int>>(); //guess a size

  cout << path << endl;
  FILE *pFile = fopen(path.c_str(),"r");
  if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

  // obtain file size:
  fseek(pFile,0,SEEK_END);
  size_t lSize = ftell(pFile);
  rewind(pFile);

  // allocate memory to contain the whole file:
  char *buffer = (char*) malloc (sizeof(char)*lSize);
  edges->reserve(lSize/4);
  if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

  // copy the file into the buffer:
  size_t result = fread (buffer,1,lSize,pFile);
  if (result != lSize) {fputs ("Reading error",stderr); exit (3);}

  printf ("%s\n",buffer);

  char *test = strtok(buffer," \t\n");
  
  while(test != NULL){
    unsigned int src;
    sscanf(test,"%u",&src);
    test = strtok(NULL," \t\n");
    
    unsigned int dst;
    sscanf(test,"%u",&dst);
    test = strtok(NULL," \t\n");

    cout << src << " " << dst << endl;
    edges->push_back(make_pair(src,dst));
  }
  // terminate
  fclose(pFile);
  free(buffer);


  num_edges = edges->size();
  unordered_map<unsigned int,unsigned int> *extern_ids = new unordered_map<unsigned int,unsigned int>();

  ////////////////////////////////////////////////////////////////////
  //out edges
  std::sort(edges->begin(),edges->end(),SrcPairComparator()); //sets us up for flat map operation

  //go from edge list to vector of vectors
  vector< vector<unsigned int>*  > *out_neighborhoods = new vector< vector<unsigned int>* >();
  build_out_neighborhoods(out_neighborhoods,edges); //does flat map

  //Build hash map
  build_hash(out_neighborhoods,extern_ids);

  ////////////////////////////////////////////////////////////////////
  //in edges
  std::sort(edges->begin(),edges->end(),DstPairComparator()); //sets us up for flat map operation

  //go from edge list to vector of vectors
  vector< vector<unsigned int>*  > *in_neighborhoods = new vector< vector<unsigned int>* >();
  build_in_neighborhoods(in_neighborhoods,edges); //does flat map

  //Build hash map
  build_hash(in_neighborhoods,extern_ids);

  //reassign ID's
  reassign_ids(out_neighborhoods,extern_ids);
  reassign_ids(in_neighborhoods,extern_ids);

  std::sort(in_neighborhoods->begin(),in_neighborhoods->end(),NeighborhoodComparator()); //sort by degree
  std::sort(out_neighborhoods->begin(),out_neighborhoods->end(),NeighborhoodComparator());

  num_nodes = extern_ids->size();
  cout << "num nodes: " << num_nodes << " num edges: " << num_edges << endl;
  delete edges;

  return MutableGraph(num_nodes,num_edges,false,extern_ids,out_neighborhoods,in_neighborhoods); 
}