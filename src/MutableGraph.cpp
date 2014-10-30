#include "MutableGraph.hpp"
#include <time.h>

struct OrderNeighborhoodByDegree{
  vector< vector<unsigned int>*  > *g;
  OrderNeighborhoodByDegree(vector< vector<unsigned int>*  > *g_in){
    g = g_in;
  }
  bool operator()(unsigned int i, unsigned int j) const { 
    return (g->at(i)->size() > g->at(j)->size()); 
  }
};

struct OrderNeighborhoodByRevDegree{
  vector< vector<unsigned int>*  > *g;
  OrderNeighborhoodByRevDegree(vector< vector<unsigned int>*  > *g_in){
    g = g_in;
  }
  bool operator()(unsigned int i, unsigned int j) const { 
    return (g->at(i)->size() < g->at(j)->size()); 
  }
};

void reassign_ids(vector< vector<unsigned int>* > *neighborhoods,vector< vector<unsigned int>* > *new_neighborhoods,unsigned int *new2old_ids,unsigned int *old2new_ids){
  for(size_t i = 0; i < neighborhoods->size(); ++i) {
    vector<unsigned int> *hood = neighborhoods->at(new2old_ids[i]);
    for(size_t j = 0; j < hood->size(); ++j) {
      hood->at(j) = old2new_ids[hood->at(j)];
    }
    sort(hood->begin(),hood->end());
    new_neighborhoods->push_back(hood);
  }
}
void MutableGraph::reorder_bfs(){
  //Pull out what you are going to reorder.
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();
  neighborhoods = out_neighborhoods;

  unsigned int *tmp_new2old_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    tmp_new2old_ids[i] = i;
  }

  std::sort(&tmp_new2old_ids[0],&tmp_new2old_ids[num_nodes],OrderNeighborhoodByDegree(neighborhoods));
  
  unsigned int *new2old_ids = new unsigned int[num_nodes];
  std::unordered_set<unsigned int> *visited = new unordered_set<unsigned int>();
  
  size_t num_added = 0;
  unsigned int *cur_level = new unsigned int[num_nodes];
  size_t cur_level_size = 1;
  size_t bfs_i = 0;
  cur_level[bfs_i] = tmp_new2old_ids[bfs_i];
  new2old_ids[num_added++] = bfs_i;
  visited->insert(bfs_i);
  bfs_i++;

  unsigned int *next_level = new unsigned int[num_nodes];
  while(num_added != neighborhoods->size()){
    //cout << "here: " << num_added << " " << neighborhoods->size() << endl;
    size_t cur_level_i = 0;
    size_t next_level_size = 0;
    while(cur_level_i < cur_level_size){
      vector<unsigned int> *hood = neighborhoods->at(cur_level[cur_level_i++]);
      for(size_t j = 0; j < hood->size(); ++j) {
        if(visited->find(hood->at(j)) == visited->end()){
          new2old_ids[num_added++] = hood->at(j);
          visited->insert(hood->at(j));
          next_level[next_level_size++] = hood->at(j);
        }
      }
    }
    //cout << num_added << endl;
    if(next_level_size == 0 && num_added < neighborhoods->size()){
      next_level_size = 1;
      while(visited->find(bfs_i) != visited->end()){
        bfs_i++;
      }
      cur_level[bfs_i] = tmp_new2old_ids[bfs_i];
      new2old_ids[num_added++] = bfs_i;
      visited->insert(bfs_i);
      bfs_i++;
    }
    unsigned int *tmp = cur_level;
    cur_level = next_level;
    next_level = tmp;
    cur_level_size = next_level_size;
  }

  delete visited;
  delete[] cur_level;
  delete[] next_level;
  delete[] tmp_new2old_ids;

  unsigned int *old2new_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    old2new_ids[new2old_ids[i]] = i;
  }

  vector< vector<unsigned int>*  > *new_neighborhoods = new vector< vector<unsigned int>* >();
  new_neighborhoods->reserve(num_nodes);
  reassign_ids(neighborhoods,new_neighborhoods,new2old_ids,old2new_ids);
  
  delete neighborhoods;
  delete[] new2old_ids;
  delete[] old2new_ids;

  //Reassign what you reordered.
  out_neighborhoods = new_neighborhoods;
  in_neighborhoods = new_neighborhoods;
}
void MutableGraph::reorder_strong_run(){
  //Pull out what you are going to reorder.
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();
  neighborhoods = out_neighborhoods;

  unsigned int *tmp_new2old_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    tmp_new2old_ids[i] = i;
  }

  std::sort(&tmp_new2old_ids[0],&tmp_new2old_ids[num_nodes],OrderNeighborhoodByDegree(neighborhoods));
  
  unsigned int *new2old_ids = new unsigned int[num_nodes];
  std::unordered_set<unsigned int> *visited = new unordered_set<unsigned int>();
  size_t num_added = 0;
  for(size_t i = 0; i < neighborhoods->size(); ++i) {
    vector<unsigned int> *hood = neighborhoods->at(tmp_new2old_ids[i]);
    for(size_t j = 0; j < hood->size(); ++j) {
      if(visited->find(hood->at(j)) == visited->end()){
        new2old_ids[num_added++] = hood->at(j);
        visited->insert(hood->at(j));
      }
    }
  }
  delete[] tmp_new2old_ids;

  unsigned int *old2new_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    old2new_ids[new2old_ids[i]] = i;
  }

  vector< vector<unsigned int>*  > *new_neighborhoods = new vector< vector<unsigned int>* >();
  new_neighborhoods->reserve(num_nodes);
  reassign_ids(neighborhoods,new_neighborhoods,new2old_ids,old2new_ids);
  
  delete neighborhoods;
  delete visited;
  delete[] new2old_ids;
  delete[] old2new_ids;

  //Reassign what you reordered.
  out_neighborhoods = new_neighborhoods;
  in_neighborhoods = new_neighborhoods;
}
void MutableGraph::reorder_random(){
  //Pull out what you are going to reorder.
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();
  neighborhoods = out_neighborhoods;

  unsigned int *new2old_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    new2old_ids[i] = i;
  }

  std::random_shuffle(&new2old_ids[0],&new2old_ids[num_nodes]);
  //std::sort(&new2old_ids[0],&new2old_ids[num_nodes],OrderNeighborhoodByDegree(neighborhoods));
  
  unsigned int *old2new_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    old2new_ids[new2old_ids[i]] = i;
  }

  vector< vector<unsigned int>*  > *new_neighborhoods = new vector< vector<unsigned int>* >();
  new_neighborhoods->reserve(num_nodes);
  reassign_ids(neighborhoods,new_neighborhoods,new2old_ids,old2new_ids);
  
  delete neighborhoods;
  delete[] new2old_ids;
  delete[] old2new_ids;

  //Reassign what you reordered.
  out_neighborhoods = new_neighborhoods;
  in_neighborhoods = new_neighborhoods;
}
void MutableGraph::reorder_by_degree(){
  //Pull out what you are going to reorder.
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();
  neighborhoods = out_neighborhoods;

  unsigned int *new2old_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    new2old_ids[i] = i;
  }

  std::sort(&new2old_ids[0],&new2old_ids[num_nodes],OrderNeighborhoodByDegree(neighborhoods));
  
  unsigned int *old2new_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    old2new_ids[new2old_ids[i]] = i;
  }

  vector< vector<unsigned int>*  > *new_neighborhoods = new vector< vector<unsigned int>* >();
  new_neighborhoods->reserve(num_nodes);
  reassign_ids(neighborhoods,new_neighborhoods,new2old_ids,old2new_ids);
  
  delete neighborhoods;
  delete[] new2old_ids;
  delete[] old2new_ids;

  //Reassign what you reordered.
  out_neighborhoods = new_neighborhoods;
  in_neighborhoods = new_neighborhoods;
}
void MutableGraph::reorder_by_rev_degree(){
  //Pull out what you are going to reorder.
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();
  neighborhoods = out_neighborhoods;

  unsigned int *new2old_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    new2old_ids[i] = i;
  }

  std::sort(&new2old_ids[0],&new2old_ids[num_nodes],OrderNeighborhoodByRevDegree(neighborhoods));
  
  unsigned int *old2new_ids = new unsigned int[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    old2new_ids[new2old_ids[i]] = i;
  }

  vector< vector<unsigned int>*  > *new_neighborhoods = new vector< vector<unsigned int>* >();
  new_neighborhoods->reserve(num_nodes);
  reassign_ids(neighborhoods,new_neighborhoods,new2old_ids,old2new_ids);
  
  delete neighborhoods;
  delete[] new2old_ids;
  delete[] old2new_ids;

  //Reassign what you reordered.
  out_neighborhoods = new_neighborhoods;
  in_neighborhoods = new_neighborhoods;
}

/*
File format

Edges must be duplicated.  If you have edge (0,1) you must
also have (1,0) listed.

src0 dst1
dst1 src0
...

*/
void MutableGraph::writeUndirectedToBinary(const string path) {
  ofstream outfile;
  outfile.open(path, ios::binary | ios::out);

  size_t osize = out_neighborhoods->size();
  outfile.write((char *)&osize, sizeof(osize)); 
  for(size_t i = 0; i < out_neighborhoods->size(); ++i){
    vector<unsigned int> *row = out_neighborhoods->at(i);
    size_t rsize = row->size();
    outfile.write((char *)&rsize, sizeof(rsize)); 
    outfile.write((char *)row->data(),sizeof(unsigned int)*rsize);
  }
  outfile.close();
}
MutableGraph* MutableGraph::undirectedFromBinary(const string path) {
  ifstream infile; 
  infile.open(path, ios::binary | ios::in); 

  unordered_map<unsigned int,unsigned int> *extern_ids = new unordered_map<unsigned int,unsigned int>();
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();
  size_t num_edges = 0;
  size_t num_nodes = 0;
  infile.read((char *)&num_nodes, sizeof(num_nodes)); 
  for(size_t i = 0; i < num_nodes; ++i){
    size_t row_size = 0;
    infile.read((char *)&row_size, sizeof(row_size)); 
    num_edges += row_size;

    vector<unsigned int> *row = new vector<unsigned int>();
    row->reserve(row_size);
    unsigned int *tmp_data = new unsigned int[row_size];
    infile.read((char *)&tmp_data[0], sizeof(unsigned int)*row_size); 
    row->assign(&tmp_data[0],&tmp_data[row_size]);
    delete[] tmp_data;
    //TODO: can you delete row?
    neighborhoods->push_back(row);
  }
  infile.close();

  return new MutableGraph(neighborhoods->size(),num_edges,true,extern_ids,neighborhoods,neighborhoods); 
} 
MutableGraph* MutableGraph::undirectedFromEdgeList(const string path) {
  ////////////////////////////////////////////////////////////////////////////////////
  //Place graph into vector of vectors then decide how you want to
  //store the graph.
  unordered_map<unsigned int,unsigned int> *extern_ids = new unordered_map<unsigned int,unsigned int>();
  vector< vector<unsigned int>*  > *neighborhoods = new vector< vector<unsigned int>* >();

  cout << path << endl;
  FILE *pFile = fopen(path.c_str(),"r");
  if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

  // obtain file size:
  fseek(pFile,0,SEEK_END);
  size_t lSize = ftell(pFile);
  rewind(pFile);

  // allocate memory to contain the whole file:
  char *buffer = (char*) malloc (sizeof(char)*lSize);
  neighborhoods->reserve(lSize/4);
  extern_ids->reserve(lSize/4);
  if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

  // copy the file into the buffer:
  size_t result = fread (buffer,1,lSize,pFile);
  if (result != lSize) {fputs ("Reading error",stderr); exit (3);}

  char *test = strtok(buffer," \t\nA");
  while(test != NULL){
    unsigned int src;
    sscanf(test,"%u",&src);
    test = strtok(NULL," \t\nA");
    
    unsigned int dst;
    sscanf(test,"%u",&dst);
    test = strtok(NULL," \t\nA");

    vector<unsigned int> *src_row;
    if(extern_ids->find(src) == extern_ids->end()){
      extern_ids->insert(make_pair(src,extern_ids->size()));
      src_row = new vector<unsigned int>();
      neighborhoods->push_back(src_row);
    } else{
      src_row = neighborhoods->at(extern_ids->at(src));
    }

    vector<unsigned int> *dst_row;
    if(extern_ids->find(dst) == extern_ids->end()){
      extern_ids->insert(make_pair(dst,extern_ids->size()));
      dst_row = new vector<unsigned int>();
      neighborhoods->push_back(dst_row);
    } else{
      dst_row = neighborhoods->at(extern_ids->at(dst));
    }

    src_row->push_back(extern_ids->at(dst));
    dst_row->push_back(extern_ids->at(src));
  }
  // terminate
  fclose(pFile);
  free(buffer);

  size_t num_edges = 0;
  for(size_t i = 0; i < neighborhoods->size(); i++){
    vector<unsigned int> *row = neighborhoods->at(i);
    std::sort(row->begin(),row->end());
    row->erase(unique(row->begin(),row->begin()+row->size()),row->end());
    num_edges += row->size();
  }

  return new MutableGraph(neighborhoods->size(),num_edges,true,extern_ids,neighborhoods,neighborhoods); 
}

void MutableGraph::writeDirectedToBinary(const string path) {
  ofstream outfile;
  outfile.open(path, ios::binary | ios::out);

  size_t osize = out_neighborhoods->size();
  outfile.write((char *)&osize, sizeof(osize)); 
  cout << osize << endl;
  for(size_t i = 0; i < out_neighborhoods->size(); ++i){
    vector<unsigned int> *row = out_neighborhoods->at(i);
    size_t rsize = row->size();
    outfile.write((char *)&rsize, sizeof(rsize)); 
    outfile.write((char *)row->data(),sizeof(unsigned int)*rsize);
  }
  for(size_t i = 0; i < in_neighborhoods->size(); ++i){
    vector<unsigned int> *row = in_neighborhoods->at(i);
    size_t rsize = row->size();
    outfile.write((char *)&rsize, sizeof(rsize)); 
    outfile.write((char *)row->data(),sizeof(unsigned int)*rsize);
  }
  outfile.close();
}
MutableGraph* MutableGraph::directedFromBinary(const string path) {
  ifstream infile; 
  infile.open(path, ios::binary | ios::in); 

  unordered_map<unsigned int,unsigned int> *extern_ids = new unordered_map<unsigned int,unsigned int>();
  vector< vector<unsigned int>*  > *out_neighborhoods = new vector< vector<unsigned int>* >();
  vector< vector<unsigned int>*  > *in_neighborhoods = new vector< vector<unsigned int>* >();

  size_t num_edges = 0;
  size_t num_nodes = 0;
  infile.read((char *)&num_nodes, sizeof(num_nodes)); 
  cout << num_nodes << endl;
  for(size_t i = 0; i < num_nodes; ++i){
    size_t row_size = 0;
    infile.read((char *)&row_size, sizeof(row_size)); 
    cout << "r: " << row_size << endl;
    num_edges += row_size;

    vector<unsigned int> *row = new vector<unsigned int>();
    row->reserve(row_size);
    unsigned int *tmp_data = new unsigned int[row_size];
    infile.read((char *)&tmp_data[0], sizeof(unsigned int)*row_size); 
    row->assign(&tmp_data[0],&tmp_data[row_size]);
    out_neighborhoods->push_back(row);
  }
  for(size_t i = 0; i < num_nodes; ++i){
    size_t row_size = 0;
    infile.read((char *)&row_size, sizeof(row_size)); 
    num_edges += row_size;

    vector<unsigned int> *row = new vector<unsigned int>();
    row->reserve(row_size);
    unsigned int *tmp_data = new unsigned int[row_size];
    infile.read((char *)&tmp_data[0], sizeof(unsigned int)*row_size); 
    row->assign(&tmp_data[0],&tmp_data[row_size]);
    in_neighborhoods->push_back(row);
  }
  infile.close();

  return new MutableGraph(out_neighborhoods->size(),num_edges,true,extern_ids,out_neighborhoods,in_neighborhoods); 
} 
/*
File format

src0 dst0
src1 dst1
...

*/
MutableGraph* MutableGraph::directedFromEdgeList(const string path) {  
  //Place graph into vector of vectors then decide how you want to
  //store the graph.
  size_t num_edges = 0;

  unordered_map<unsigned int,unsigned int> *extern_ids = new unordered_map<unsigned int,unsigned int>();
  vector< vector<unsigned int>*  > *in_neighborhoods = new vector< vector<unsigned int>* >();
  vector< vector<unsigned int>*  > *out_neighborhoods = new vector< vector<unsigned int>* >();

  FILE *pFile = fopen(path.c_str(),"r");
  if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

  // obtain file size:
  fseek(pFile,0,SEEK_END);
  size_t lSize = ftell(pFile);
  rewind(pFile);

  // allocate memory to contain the whole file:
  char *buffer = (char*) malloc (sizeof(char)*lSize);
  if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

  // copy the file into the buffer:
  size_t result = fread (buffer,1,lSize,pFile);
  if (result != lSize) {fputs ("Reading error",stderr); exit (3);}

  char *test = strtok(buffer," \t\nA");
  while(test != NULL){
    unsigned int src;
    sscanf(test,"%u",&src);
    test = strtok(NULL," \t\nA");
    
    unsigned int dst;
    sscanf(test,"%u",&dst);
    test = strtok(NULL," \t\nA");

    num_edges++;

    vector<unsigned int> *src_row;
    if(extern_ids->find(src) == extern_ids->end()){
      extern_ids->insert(make_pair(src,extern_ids->size()));
      src_row = new vector<unsigned int>();
      vector<unsigned int> *new_row = new vector<unsigned int>();
      in_neighborhoods->push_back(new_row);
      out_neighborhoods->push_back(src_row);
    } else{
      src_row = out_neighborhoods->at(extern_ids->at(src));
    }

    vector<unsigned int> *dst_row;
    if(extern_ids->find(dst) == extern_ids->end()){
      extern_ids->insert(make_pair(dst,extern_ids->size()));
      dst_row = new vector<unsigned int>();
      vector<unsigned int> *new_row = new vector<unsigned int>();
      out_neighborhoods->push_back(new_row);
      in_neighborhoods->push_back(dst_row);
    } else{
      dst_row = in_neighborhoods->at(extern_ids->at(dst));
    }
    src_row->push_back(extern_ids->at(dst));
    dst_row->push_back(extern_ids->at(src));
  }
  // terminate
  fclose(pFile);
  free(buffer);

  for(size_t i = 0; i < in_neighborhoods->size(); i++){
    vector<unsigned int> *row = in_neighborhoods->at(i);
    std::sort(row->begin(),row->end());
    row->erase(unique(row->begin(),row->begin()+row->size()),row->end());
  }
  for(size_t i = 0; i < out_neighborhoods->size(); i++){
    vector<unsigned int> *row = out_neighborhoods->at(i);
    std::sort(row->begin(),row->end());
    row->erase(unique(row->begin(),row->begin()+row->size()),row->end());
  }

  return new MutableGraph(in_neighborhoods->size(),num_edges,false,extern_ids,out_neighborhoods,in_neighborhoods); 
}