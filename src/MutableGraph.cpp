#include "MutableGraph.hpp"

struct OrderNeighborhoodByDegree{
  vector< vector<uint32_t>*  > *g;
  OrderNeighborhoodByDegree(vector< vector<uint32_t>*  > *g_in){
    g = g_in;
  }
  bool operator()(uint32_t i, uint32_t j) const {
    size_t i_size = g->at(i)->size();
    size_t j_size = g->at(j)->size();
    if(i_size == j_size)
      return i < j;
    return i_size > j_size;
  }
};

struct OrderNeighborhoodByRevDegree{
  vector< vector<uint32_t>*  > *g;
  OrderNeighborhoodByRevDegree(vector< vector<uint32_t>*  > *g_in){
    g = g_in;
  }
  bool operator()(uint32_t i, uint32_t j) const { 
    return (g->at(i)->size() < g->at(j)->size()); 
  }
};

struct OrderByID{
  OrderByID(){}
  bool operator()(pair<uint32_t,uint32_t> i, pair<uint32_t,uint32_t> j) const {
    return i.first < j.first;
  }
};

void MutableGraph::reassign_ids(vector< vector<uint32_t>* > *neighborhoods,vector< vector<uint32_t>* > *new_neighborhoods,uint32_t *new2old_ids,uint32_t *old2new_ids){
  vector<uint64_t> *new_id_map = new vector<uint64_t>();

  for(size_t i = 0; i < neighborhoods->size(); ++i) {
    vector<uint32_t> *hood = neighborhoods->at(new2old_ids[i]);
    new_id_map->push_back(id_map->at(new2old_ids[i]));
    for(size_t j = 0; j < hood->size(); ++j) {
      hood->at(j) = old2new_ids[hood->at(j)];
    }
    sort(hood->begin(),hood->end());
    new_neighborhoods->push_back(hood);
  }

  delete id_map;
  id_map = new_id_map;
}
void MutableGraph::reorder_bfs(){
  //Pull out what you are going to reorder.
  vector< vector<uint32_t>*  > *neighborhoods = new vector< vector<uint32_t>* >();
  neighborhoods = out_neighborhoods;

  uint32_t *tmp_new2old_ids = new uint32_t[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    tmp_new2old_ids[i] = i;
  }

  std::sort(&tmp_new2old_ids[0],&tmp_new2old_ids[num_nodes],OrderNeighborhoodByDegree(neighborhoods));
  
  uint32_t *new2old_ids = new uint32_t[num_nodes];
  std::unordered_set<uint32_t> *visited = new unordered_set<uint32_t>();
  
  size_t num_added = 0;
  uint32_t *cur_level = new uint32_t[num_nodes];
  size_t cur_level_size = 1;
  size_t bfs_i = 0;
  cur_level[bfs_i] = tmp_new2old_ids[bfs_i];
  new2old_ids[num_added++] = bfs_i;
  visited->insert(bfs_i);
  bfs_i++;

  uint32_t *next_level = new uint32_t[num_nodes];
  while(num_added != neighborhoods->size()){
    //cout << "here: " << num_added << " " << neighborhoods->size() << endl;
    size_t cur_level_i = 0;
    size_t next_level_size = 0;
    while(cur_level_i < cur_level_size){
      vector<uint32_t> *hood = neighborhoods->at(cur_level[cur_level_i++]);
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
    uint32_t *tmp = cur_level;
    cur_level = next_level;
    next_level = tmp;
    cur_level_size = next_level_size;
  }

  delete visited;
  delete[] cur_level;
  delete[] next_level;
  delete[] tmp_new2old_ids;

  uint32_t *old2new_ids = new uint32_t[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    old2new_ids[new2old_ids[i]] = i;
  }

  vector< vector<uint32_t>*  > *new_neighborhoods = new vector< vector<uint32_t>* >();
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
  vector< vector<uint32_t>*  > *neighborhoods = new vector< vector<uint32_t>* >();
  neighborhoods = out_neighborhoods;

  uint32_t *tmp_new2old_ids = new uint32_t[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    tmp_new2old_ids[i] = i;
  }

  std::sort(&tmp_new2old_ids[0],&tmp_new2old_ids[num_nodes],OrderNeighborhoodByDegree(neighborhoods));
  
  uint32_t *new2old_ids = new uint32_t[num_nodes];
  std::unordered_set<uint32_t> *visited = new unordered_set<uint32_t>();
  size_t num_added = 0;
  for(size_t i = 0; i < neighborhoods->size(); ++i) {
    vector<uint32_t> *hood = neighborhoods->at(tmp_new2old_ids[i]);
    for(size_t j = 0; j < hood->size(); ++j) {
      if(visited->find(hood->at(j)) == visited->end()){
        new2old_ids[num_added++] = hood->at(j);
        visited->insert(hood->at(j));
      }
    }
  }
  delete[] tmp_new2old_ids;

  uint32_t *old2new_ids = new uint32_t[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    old2new_ids[new2old_ids[i]] = i;
  }

  vector< vector<uint32_t>*  > *new_neighborhoods = new vector< vector<uint32_t>* >();
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
  vector< vector<uint32_t>*  > *neighborhoods = new vector< vector<uint32_t>* >();
  neighborhoods = out_neighborhoods;

  uint32_t *new2old_ids = new uint32_t[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    new2old_ids[i] = i;
  }

  std::random_shuffle(&new2old_ids[0],&new2old_ids[num_nodes]);
  //std::sort(&new2old_ids[0],&new2old_ids[num_nodes],OrderNeighborhoodByDegree(neighborhoods));
  
  uint32_t *old2new_ids = new uint32_t[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    old2new_ids[new2old_ids[i]] = i;
  }

  vector< vector<uint32_t>*  > *new_neighborhoods = new vector< vector<uint32_t>* >();
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
  vector< vector<uint32_t>*  > *neighborhoods = new vector< vector<uint32_t>* >();
  neighborhoods = out_neighborhoods;

  uint32_t *new2old_ids = new uint32_t[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    new2old_ids[i] = i;
  }

  std::sort(&new2old_ids[0],&new2old_ids[num_nodes],OrderNeighborhoodByDegree(neighborhoods));
  
  uint32_t *old2new_ids = new uint32_t[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    old2new_ids[new2old_ids[i]] = i;
  }

  vector< vector<uint32_t>*  > *new_neighborhoods = new vector< vector<uint32_t>* >();
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
  vector< vector<uint32_t>*  > *neighborhoods = new vector< vector<uint32_t>* >();
  neighborhoods = out_neighborhoods;

  uint32_t *new2old_ids = new uint32_t[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    new2old_ids[i] = i;
  }

  std::sort(&new2old_ids[0],&new2old_ids[num_nodes],OrderNeighborhoodByRevDegree(neighborhoods));
  
  uint32_t *old2new_ids = new uint32_t[num_nodes];
  for(size_t i = 0; i < num_nodes; i++){
    old2new_ids[new2old_ids[i]] = i;
  }

  vector< vector<uint32_t>*  > *new_neighborhoods = new vector< vector<uint32_t>* >();
  new_neighborhoods->reserve(num_nodes);
  reassign_ids(neighborhoods,new_neighborhoods,new2old_ids,old2new_ids);
  
  delete neighborhoods;
  delete[] new2old_ids;
  delete[] old2new_ids;

  //Reassign what you reordered.
  out_neighborhoods = new_neighborhoods;
  in_neighborhoods = new_neighborhoods;
}

void MutableGraph::reorder_by_the_game() {
  reorder_bfs();
  vector<uint64_t> id_map_after_bfs = *id_map;
  std::cout << id_map_after_bfs.size() << std::endl;
  reorder_by_degree();
  vector<uint64_t> id_map_after_degree = *id_map;
  this->id_map->clear();
  std::cout << id_map_after_degree.size() << std::endl;
  for(size_t i = 0; i < num_nodes; i++) {
    this->id_map->push_back(id_map_after_degree.at(id_map_after_bfs.at(i)));
  }
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
    vector<uint32_t> *row = out_neighborhoods->at(i);
    size_t rsize = row->size();
    outfile.write((char*)&id_map->at(i),sizeof(uint64_t));
    outfile.write((char *)&rsize, sizeof(rsize)); 
    outfile.write((char *)row->data(),sizeof(uint32_t)*rsize);
  }
  outfile.close();
}
MutableGraph* MutableGraph::undirectedFromBinary(const string path) {
  ifstream infile; 
  infile.open(path, ios::binary | ios::in); 

  vector<uint64_t> *id_map = new vector<uint64_t>();
  vector<uint32_t> *id_attributes = new vector<uint32_t>();
  vector< vector<uint32_t>*  > *neighborhoods = new vector< vector<uint32_t>* >();
  vector< vector<uint32_t>*  > *edge_attributes = new vector< vector<uint32_t>* >();

  size_t num_edges = 0;
  size_t num_nodes = 0;

  size_t max_nbrhood_size = 0;

  infile.read((char *)&num_nodes, sizeof(num_nodes)); 
  for(size_t i = 0; i < num_nodes; ++i){
    uint64_t external_id;
    infile.read((char *)&external_id, sizeof(uint64_t)); 
    id_map->push_back(external_id);

    size_t row_size = 0;
    infile.read((char *)&row_size, sizeof(row_size)); 
    num_edges += row_size;

    if(row_size > max_nbrhood_size)
      max_nbrhood_size = row_size;

    vector<uint32_t> *row = new vector<uint32_t>();
    row->reserve(row_size);
    uint32_t *tmp_data = new uint32_t[row_size];
    infile.read((char *)&tmp_data[0], sizeof(uint32_t)*row_size); 
    row->assign(&tmp_data[0],&tmp_data[row_size]);
    delete[] tmp_data;
    //TODO: can you delete row?
    neighborhoods->push_back(row);
  }
  infile.close();

  return new MutableGraph(neighborhoods->size(),num_edges,max_nbrhood_size,true,id_map,id_attributes,neighborhoods,neighborhoods,edge_attributes,edge_attributes); 
}
MutableGraph* MutableGraph::undirectedFromAttributeList(const string path, const string node_path) {
  ////////////////////////////////////////////////////////////////////////////////////
  //Place graph into vector of vectors then decide how you want to
  //store the graph.
  unordered_map<uint64_t,uint32_t> *extern_ids = new unordered_map<uint64_t,uint32_t>();
  vector<uint64_t> *id_map = new vector<uint64_t>();
  vector< vector<uint32_t>*  > *neighborhoods = new vector< vector<uint32_t>* >();
  vector< vector<uint32_t>*  > *edge_attributes = new vector< vector<uint32_t>* >();

  cout << path << endl;
  FILE *pFile = fopen(path.c_str(),"r");
  if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

  // obtain file size:
  fseek(pFile,0,SEEK_END);
  size_t lSize = ftell(pFile);
  rewind(pFile);

  // allocate memory to contain the whole file:
  char *buffer = (char*) malloc (sizeof(char)*lSize + 1);

  neighborhoods->reserve(lSize/4);
  extern_ids->reserve(lSize/4);
  if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

  // copy the file into the buffer:
  size_t result = fread (buffer,1,lSize,pFile);
  if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
  buffer[result] = '\0';

  std::set<pair<uint32_t,uint32_t>> *edge_set = new std::set<pair<uint32_t,uint32_t>>(); 
  char *test = strtok(buffer," |\t\nA");
  while(test != NULL){
    uint64_t src;
    sscanf(test,"%lu",&src);
    test = strtok(NULL," |\t\nA");
    
    uint64_t dst;
    sscanf(test,"%lu",&dst);
    test = strtok(NULL," |\t\nA");

    uint32_t year;
    sscanf(test,"%u",&year);
    test = strtok(NULL," -|\t\nA");

    if(edge_set->find(make_pair(src,dst)) == edge_set->end()){
      edge_set->insert(make_pair(src,dst));
      edge_set->insert(make_pair(dst,src));

      vector<uint32_t> *src_row;
      vector<uint32_t> *src_attr;
      if(extern_ids->find(src) == extern_ids->end()){
        extern_ids->insert(make_pair(src,extern_ids->size()));
        id_map->push_back(src);
        src_row = new vector<uint32_t>();
        neighborhoods->push_back(src_row);
        src_attr = new vector<uint32_t>();
        edge_attributes->push_back(src_attr);
      } else{
        src_attr = edge_attributes->at(extern_ids->at(src));
        src_row = neighborhoods->at(extern_ids->at(src));
      }

      vector<uint32_t> *dst_row;
      vector<uint32_t> *dst_attr;
      if(extern_ids->find(dst) == extern_ids->end()){
        extern_ids->insert(make_pair(dst,extern_ids->size()));
        id_map->push_back(dst);
        dst_row = new vector<uint32_t>();
        neighborhoods->push_back(dst_row);
        dst_attr = new vector<uint32_t>();
        edge_attributes->push_back(dst_attr);
      } else{
        dst_attr = edge_attributes->at(extern_ids->at(dst));
        dst_row = neighborhoods->at(extern_ids->at(dst));
      }

      src_attr->push_back(year);
      dst_attr->push_back(year);
      src_row->push_back(extern_ids->at(dst));
      dst_row->push_back(extern_ids->at(src));
    }
  }
  // terminate
  fclose(pFile);
  free(buffer);

  cout << node_path << endl;

  //////////////////////////////////////////////////////////////////////////////
  vector<uint32_t> *id_attributes = new vector<uint32_t>();
  id_attributes->resize(neighborhoods->size()); 

  FILE *pFile2 = fopen(node_path.c_str(),"r");
  if (pFile2==NULL) {fputs ("File error",stderr); exit (1);}

  // obtain file size:
  fseek(pFile2,0,SEEK_END);
  lSize = ftell(pFile2);
  rewind(pFile2);

  // allocate memory to contain the whole file:
  buffer = (char*) malloc (sizeof(char)*lSize + 1);
  if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

  // copy the file into the buffer:
  result = fread (buffer,1,lSize,pFile2);
  if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
  buffer[result] = '\0';

  test = strtok(buffer," |\t\nA");
  while(test != NULL){
    uint64_t id;
    sscanf(test,"%lu",&id);
    test = strtok(NULL," |\t\nA");

    uint32_t attr;
    sscanf(test,"%u",&attr);
    test = strtok(NULL," |\t\nA");

    if(extern_ids->find(id) != extern_ids->end()){
      id_attributes->at(extern_ids->at(id)) = attr;
    }
  }
  // terminate
  fclose(pFile2);
  free(buffer);
  //////////////////////////////////////////////////////////////////////////////

  size_t max_nbrhood_size = 0;
  size_t num_edges = 0;
  for(size_t i = 0; i < neighborhoods->size(); i++){
    vector<uint32_t> *row = neighborhoods->at(i);
    vector<uint32_t> *row_attr = edge_attributes->at(i);

    vector<pair<uint32_t,uint32_t>> *pair_list = new vector<pair<uint32_t,uint32_t>>();
    for(size_t j = 0; j < row->size(); j++){
      pair_list->push_back(make_pair(row->at(j),row_attr->at(j)));
    }
    std::sort(pair_list->begin(),pair_list->end(),OrderByID());
    for(size_t j = 0; j < row->size(); j++){
      row->at(j) = pair_list->at(j).first;
      row_attr->at(j) = pair_list->at(j).second;
    }
    delete pair_list;

    if(row->size() > max_nbrhood_size)
      max_nbrhood_size = row->size();

    //row->erase(unique(row->begin(),row->begin()+row->size()),row->end());
    num_edges += row->size();
  }

  return new MutableGraph(neighborhoods->size(),num_edges,max_nbrhood_size,true,id_map,id_attributes,neighborhoods,neighborhoods,edge_attributes,edge_attributes); 
} 
MutableGraph* MutableGraph::undirectedFromEdgeList(const string path) {
  ////////////////////////////////////////////////////////////////////////////////////
  //Place graph into vector of vectors then decide how you want to
  //store the graph.
  unordered_map<uint64_t,uint32_t> *extern_ids = new unordered_map<uint64_t,uint32_t>();
  vector<uint64_t> *id_map = new vector<uint64_t>();
  vector<uint32_t> *id_attributes = new vector<uint32_t>();
  vector< vector<uint32_t>*  > *neighborhoods = new vector< vector<uint32_t>* >();
  vector< vector<uint32_t>*  > *edge_attributes = new vector< vector<uint32_t>* >();

  cout << path << endl;
  FILE *pFile = fopen(path.c_str(),"r");
  if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

  // obtain file size:
  fseek(pFile,0,SEEK_END);
  size_t lSize = ftell(pFile);
  rewind(pFile);

  // allocate memory to contain the whole file:
  char *buffer = (char*)malloc(sizeof(char) * lSize + 1);
  neighborhoods->reserve(lSize/4);
  extern_ids->reserve(lSize/4);
  if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

  // copy the file into the buffer:
  size_t result = fread (buffer,1,lSize,pFile);
  if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
  buffer[result] = '\0';

  char *test = strtok(buffer," \t\nA");
  while(test != NULL){
    uint64_t src;
    sscanf(test,"%lu",&src);
    test = strtok(NULL," \t\nA");
    
    uint64_t dst;
    sscanf(test,"%lu",&dst);
    test = strtok(NULL," \t\nA");

    vector<uint32_t> *src_row;
    if(extern_ids->find(src) == extern_ids->end()){
      extern_ids->insert(make_pair(src,extern_ids->size()));
      id_map->push_back(src);
      src_row = new vector<uint32_t>();
      neighborhoods->push_back(src_row);
    } else{
      src_row = neighborhoods->at(extern_ids->at(src));
    }

    vector<uint32_t> *dst_row;
    if(extern_ids->find(dst) == extern_ids->end()){
      extern_ids->insert(make_pair(dst,extern_ids->size()));
      id_map->push_back(dst);
      dst_row = new vector<uint32_t>();
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

  size_t max_nbrhood_size = 0;
  size_t num_edges = 0;
  for(size_t i = 0; i < neighborhoods->size(); i++){
    vector<uint32_t> *row = neighborhoods->at(i);
    std::sort(row->begin(),row->end());

    if(row->size() > max_nbrhood_size)
      max_nbrhood_size = row->size();

    row->erase(unique(row->begin(),row->begin()+row->size()),row->end());
    num_edges += row->size();
  }

  delete extern_ids;
  return new MutableGraph(neighborhoods->size(),num_edges,max_nbrhood_size,true,id_map,id_attributes,neighborhoods,neighborhoods,edge_attributes,edge_attributes); 
}

void MutableGraph::writeDirectedToLigra(const string path) {
  ofstream myfile;
  myfile.open(path);

  myfile << "AdjacencyGraph" << endl;
  myfile << num_nodes << endl;

  size_t numedges = 0;
  for(size_t i = 0; i < num_nodes; ++i){
    /////////////////////////////////////////////////////////////////////
    vector<uint32_t> *row = out_neighborhoods->at(i);
    numedges += row->size();
  }
  myfile << numedges << endl;


  size_t index = 0;
  for(size_t i = 0; i < num_nodes; ++i){
    myfile << index << endl;

    /////////////////////////////////////////////////////////////////////
    vector<uint32_t> *row = out_neighborhoods->at(i);
    size_t rsize = row->size();
    index += rsize;
  }
  for(size_t i = 0; i < num_nodes; ++i){
    vector<uint32_t> *row = out_neighborhoods->at(i);
    size_t rsize = row->size();
    for(size_t j = 0; j < rsize; j++){
      myfile << row->at(j) << endl;
    }
  }
  myfile.close();
}

void MutableGraph::writeDirectedToBinary(const string path) {
  ofstream outfile;
  outfile.open(path, ios::binary | ios::out);

  outfile.write((char *)&num_nodes, sizeof(num_nodes));
  for(size_t i = 0; i < num_nodes; ++i){
    outfile.write((char*)&id_map->at(i),sizeof(uint64_t));

    /////////////////////////////////////////////////////////////////////
    vector<uint32_t> *row = out_neighborhoods->at(i);
    size_t rsize = row->size();
    outfile.write((char *)&rsize, sizeof(rsize)); 
    outfile.write((char *)row->data(),sizeof(uint32_t)*rsize);

    /////////////////////////////////////////////////////////////////////
    vector<uint32_t> *col = in_neighborhoods->at(i);
    size_t csize = col->size();
    outfile.write((char *)&csize, sizeof(csize)); 
    outfile.write((char *)col->data(),sizeof(uint32_t)*csize);
  }
  outfile.close();
}
MutableGraph* MutableGraph::directedFromBinary(const string path) {
  ifstream infile; 
  infile.open(path, ios::binary | ios::in); 

  vector<uint64_t> *id_map = new vector<uint64_t>();
  vector< vector<uint32_t>*  > *out_neighborhoods = new vector< vector<uint32_t>* >();
  vector< vector<uint32_t>*  > *in_neighborhoods = new vector< vector<uint32_t>* >();
  vector< vector<uint32_t>*  > *edge_attributes = new vector< vector<uint32_t>* >();
  vector<uint32_t> *id_attributes = new vector<uint32_t>();

  size_t num_edges = 0;
  size_t num_nodes = 0;
  infile.read((char *)&num_nodes, sizeof(num_nodes)); 

  size_t max_nbrhood_size = 0;

  cout << "num nodes: " << num_nodes << endl;
  for(size_t i = 0; i < num_nodes; ++i){
    uint64_t external_id;
    infile.read((char *)&external_id, sizeof(uint64_t)); 
    id_map->push_back(external_id);

    //////////////////////////////////////////////////////////////////// 
    size_t row_size = 0;
    infile.read((char *)&row_size, sizeof(row_size)); 
    num_edges += row_size;

    if(row_size > max_nbrhood_size)
      max_nbrhood_size = row_size;

    vector<uint32_t> *row = new vector<uint32_t>();
    row->reserve(row_size);
    uint32_t *r_tmp_data = new uint32_t[row_size];
    infile.read((char *)&r_tmp_data[0], sizeof(uint32_t)*row_size); 
    row->assign(&r_tmp_data[0],&r_tmp_data[row_size]);
    out_neighborhoods->push_back(row);

    ////////////////////////////////////////////////////////////////////
    size_t col_size = 0;
    infile.read((char *)&col_size, sizeof(col_size)); 
    num_edges += col_size;

    if(col_size > max_nbrhood_size)
      max_nbrhood_size = col_size;

    vector<uint32_t> *col = new vector<uint32_t>();
    col->reserve(col_size);
    uint32_t *c_tmp_data = new uint32_t[col_size];
    infile.read((char *)&c_tmp_data[0], sizeof(uint32_t)*col_size); 
    col->assign(&c_tmp_data[0],&c_tmp_data[col_size]);
    in_neighborhoods->push_back(col);
  }
  infile.close();

  return new MutableGraph(out_neighborhoods->size(),num_edges,max_nbrhood_size,true,id_map,id_attributes,out_neighborhoods,in_neighborhoods,edge_attributes,edge_attributes); 
} 
/*
File format

src0 dst0
src1 dst1
...

*/
MutableGraph* MutableGraph::directedFromAttributeList(const string path, const string node_path) {  
  //Place graph into vector of vectors then decide how you want to
  //store the graph.
  size_t num_edges = 0;

  vector<uint64_t> *id_map = new vector<uint64_t>();
  unordered_map<uint64_t,uint32_t> *extern_ids = new unordered_map<uint64_t,uint32_t>();
  vector< vector<uint32_t>*  > *in_neighborhoods = new vector< vector<uint32_t>* >();
  vector< vector<uint32_t>*  > *out_neighborhoods = new vector< vector<uint32_t>* >();
  vector< vector<uint32_t>*  > *out_edge_attributes = new vector< vector<uint32_t>* >();
  vector< vector<uint32_t>*  > *in_edge_attributes = new vector< vector<uint32_t>* >();

  FILE *pFile = fopen(path.c_str(),"r");
  if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

  // obtain file size:
  fseek(pFile,0,SEEK_END);
  size_t lSize = ftell(pFile);
  rewind(pFile);

  // allocate memory to contain the whole file:
  char *buffer = (char*) malloc (sizeof(char)*lSize + 1);
  if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

  // copy the file into the buffer:
  size_t result = fread (buffer,1,lSize,pFile);
  if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
  buffer[result] = '\0';

  char *test = strtok(buffer," \t\nA");
  while(test != NULL){
    uint64_t src;
    sscanf(test,"%lu",&src);
    test = strtok(NULL," \t\nA");
    
    uint64_t dst;
    sscanf(test,"%lu",&dst);
    test = strtok(NULL," \t\nA");

    uint32_t year;
    sscanf(test,"%u",&year);
    test = strtok(NULL," -|\t\nA");

    num_edges++;

    vector<uint32_t> *src_row;
    vector<uint32_t> *src_attr;
    if(extern_ids->find(src) == extern_ids->end()){
      extern_ids->insert(make_pair(src,extern_ids->size()));
      id_map->push_back(src);
      src_row = new vector<uint32_t>();
      vector<uint32_t> *new_row = new vector<uint32_t>();
      in_neighborhoods->push_back(new_row);
      out_neighborhoods->push_back(src_row);
      src_attr = new vector<uint32_t>();
      out_edge_attributes->push_back(src_attr);
      in_edge_attributes->push_back(new vector<uint32_t>());
    } else{
      src_attr = out_edge_attributes->at(extern_ids->at(src));
      src_row = out_neighborhoods->at(extern_ids->at(src));
    }

    vector<uint32_t> *dst_row;
    vector<uint32_t> *dst_attr;
    if(extern_ids->find(dst) == extern_ids->end()){
      extern_ids->insert(make_pair(dst,extern_ids->size()));
      id_map->push_back(dst);
      dst_row = new vector<uint32_t>();
      vector<uint32_t> *new_row = new vector<uint32_t>();
      out_neighborhoods->push_back(new_row);
      in_neighborhoods->push_back(dst_row);
      dst_attr = new vector<uint32_t>();
      in_edge_attributes->push_back(dst_attr);
      out_edge_attributes->push_back(new vector<uint32_t>());
    } else{
      dst_row = in_neighborhoods->at(extern_ids->at(dst));
      dst_attr = in_edge_attributes->at(extern_ids->at(src));
    }
    src_row->push_back(extern_ids->at(dst));
    src_attr->push_back(year);
    dst_row->push_back(extern_ids->at(src));
    dst_attr->push_back(year);
  }
  // terminate
  fclose(pFile);
  free(buffer);

  //////////////////////////////////////////////////////////////////////////////
  vector<uint32_t> *id_attributes = new vector<uint32_t>();
  id_attributes->resize(in_neighborhoods->size()); 

  FILE *pFile2 = fopen(node_path.c_str(),"r");
  if (pFile2==NULL) {fputs ("File error",stderr); exit (1);}

  // obtain file size:
  fseek(pFile2,0,SEEK_END);
  lSize = ftell(pFile2);
  rewind(pFile2);

  // allocate memory to contain the whole file:
  buffer = (char*) malloc (sizeof(char)*lSize + 1);
  if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

  // copy the file into the buffer:
  result = fread (buffer,1,lSize,pFile2);
  if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
  buffer[result] = '\0';

  test = strtok(buffer," |\t\nA");
  while(test != NULL){
    uint64_t id;
    sscanf(test,"%lu",&id);
    test = strtok(NULL," |\t\nA");

    uint32_t attr;
    sscanf(test,"%u",&attr);
    test = strtok(NULL," |\t\nA");

    if(extern_ids->find(id) != extern_ids->end()){
      id_attributes->at(extern_ids->at(id)) = attr;
    }
  }
  // terminate
  fclose(pFile2);
  free(buffer);
  //////////////////////////////////////////////////////////////////////////////

  size_t max_nbrhood_size = 0;
  for(size_t i = 0; i < in_neighborhoods->size(); i++){
    vector<uint32_t> *row = in_neighborhoods->at(i);
    std::sort(row->begin(),row->end());

    if(row->size() > max_nbrhood_size)
      max_nbrhood_size = row->size();

    row->erase(unique(row->begin(),row->begin()+row->size()),row->end());
  }
  for(size_t i = 0; i < out_neighborhoods->size(); i++){
    vector<uint32_t> *row = out_neighborhoods->at(i);
    std::sort(row->begin(),row->end());

    if(row->size() > max_nbrhood_size)
      max_nbrhood_size = row->size();

    row->erase(unique(row->begin(),row->begin()+row->size()),row->end());
  }

  delete extern_ids;
  return new MutableGraph(in_neighborhoods->size(),num_edges,max_nbrhood_size,false,id_map,id_attributes,out_neighborhoods,in_neighborhoods,out_edge_attributes,in_edge_attributes); 
}
MutableGraph* MutableGraph::directedFromEdgeList(const string path) {  
  //Place graph into vector of vectors then decide how you want to
  //store the graph.
  size_t num_edges = 0;

  vector<uint64_t> *id_map = new vector<uint64_t>();
  unordered_map<uint64_t,uint32_t> *extern_ids = new unordered_map<uint64_t,uint32_t>();
  vector< vector<uint32_t>*  > *in_neighborhoods = new vector< vector<uint32_t>* >();
  vector< vector<uint32_t>*  > *out_neighborhoods = new vector< vector<uint32_t>* >();
  vector<uint32_t> *id_attributes = new vector<uint32_t>();
  vector< vector<uint32_t>*  > *edge_attributes = new vector< vector<uint32_t>* >();

  FILE *pFile = fopen(path.c_str(),"r");
  if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

  // obtain file size:
  fseek(pFile,0,SEEK_END);
  size_t lSize = ftell(pFile);
  rewind(pFile);

  // allocate memory to contain the whole file:
  char *buffer = (char*) malloc (sizeof(char)*lSize + 1);
  if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

  // copy the file into the buffer:
  size_t result = fread (buffer,1,lSize,pFile);
  if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
  buffer[result] = '\0';

  char *test = strtok(buffer," \t\nA");
  while(test != NULL){
    uint64_t src;
    sscanf(test,"%lu",&src);
    test = strtok(NULL," \t\nA");
    
    uint64_t dst;
    sscanf(test,"%lu",&dst);
    test = strtok(NULL," \t\nA");

    num_edges++;

    vector<uint32_t> *src_row;
    if(extern_ids->find(src) == extern_ids->end()){
      extern_ids->insert(make_pair(src,extern_ids->size()));
      id_map->push_back(src);
      src_row = new vector<uint32_t>();
      vector<uint32_t> *new_row = new vector<uint32_t>();
      in_neighborhoods->push_back(new_row);
      out_neighborhoods->push_back(src_row);
    } else{
      src_row = out_neighborhoods->at(extern_ids->at(src));
    }

    vector<uint32_t> *dst_row;
    if(extern_ids->find(dst) == extern_ids->end()){
      extern_ids->insert(make_pair(dst,extern_ids->size()));
      id_map->push_back(dst);
      dst_row = new vector<uint32_t>();
      vector<uint32_t> *new_row = new vector<uint32_t>();
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

  size_t max_nbrhood_size = 0;
  for(size_t i = 0; i < in_neighborhoods->size(); i++){
    vector<uint32_t> *row = in_neighborhoods->at(i);
    std::sort(row->begin(),row->end());

    if(row->size() > max_nbrhood_size)
      max_nbrhood_size = row->size();

    row->erase(unique(row->begin(),row->begin()+row->size()),row->end());
  }
  for(size_t i = 0; i < out_neighborhoods->size(); i++){
    vector<uint32_t> *row = out_neighborhoods->at(i);
    std::sort(row->begin(),row->end());

    if(row->size() > max_nbrhood_size)
      max_nbrhood_size = row->size();

    row->erase(unique(row->begin(),row->begin()+row->size()),row->end());
  }

  delete extern_ids;
  return new MutableGraph(in_neighborhoods->size(),num_edges,max_nbrhood_size,false,id_map,id_attributes,out_neighborhoods,in_neighborhoods,edge_attributes,edge_attributes); 
}
