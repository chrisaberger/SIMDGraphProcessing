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

  double avg_nx = 0;
  double avg_n2x = 0;
  double avg_n2x_I_nx = 0;
  double avg_n2i = 0;
  double avg_ny = 0;
  double avg_i = 0;
  double avg_t = 0;

  //cout << "NUMBER OF NODES: " << num_nodes << " NUMBER OF EDGES: " << num_edges << endl;
  cout << "#x\tN(x)\tN2(x)\tN2^N\t^N" << endl;

  for(size_t i = 0; i < num_nodes; ++i) {
    vector<unsigned int> *nx = vg->neighborhoods->at(i);
    if(nx->size() == 0){
      //cout << i << "\t" << nx->size() << "\t" << nx->size() << "\t" << nx->size() << "\t" << nx->size() << endl;
    }else{
      vector<unsigned int> *n2x = vg->neighborhoods->at(nx->at(0));
      vector<unsigned int> *n2i = vg->neighborhoods->at(nx->at(0));
      double ny_size = 0;
      double t_size = 0;
      for(size_t j=1; j < nx->size(); ++j){
    
        vector<unsigned int> *nbr = vg->neighborhoods->at(nx->at(j));
        
        ny_size += nbr->size();

        vector<unsigned int> *un = new vector<unsigned int>(); 
        set_union(nbr->begin(), nbr->end(), n2x->begin(), n2x->end(), back_inserter(*un));

        vector<unsigned int> *in = new vector<unsigned int>(); 
        set_intersection(nbr->begin(), nbr->end(), n2i->begin(), n2i->end(), back_inserter(*in));
 
        vector<unsigned int> *t = new vector<unsigned int>(); 
        set_intersection(nx->begin(), nx->end(), nbr->begin(), nbr->end(), back_inserter(*t));
        t_size += t->size();
        t->clear();

        if(j > 1){
          n2i->clear();
          n2x->clear();
        }

        n2i = in;
        n2x = un;
        std::sort(n2x->begin(), n2x->end()); 
        std::sort(n2i->begin(), n2i->end()); 
      }
      avg_ny += (ny_size/nx->size());
      avg_t += (t_size/nx->size());
      avg_i += t_size;

      vector<unsigned int> *n2x_I_nx = new vector<unsigned int>(); 
      set_intersection(nx->begin(), nx->end(), n2x->begin(), n2x->end(), back_inserter(*n2x_I_nx));

      avg_nx += nx->size();
      avg_n2x += n2x->size();
      avg_n2x_I_nx += n2x_I_nx->size();
      avg_n2i += n2i->size();

      //cout << i << "\t" << nx->size() << "\t" << n2x->size() << "\t" << n2x_I_nx->size() << "\t" << n2i->size() << endl;

      n2x_I_nx->clear();
      if(nx->size() > 1)
        n2x->clear();
        n2i->clear();
    }
  }

  cout << "AVGs:\t" << (avg_nx/num_nodes) << "\t" << (avg_n2x/num_nodes) << "\t" << (avg_n2x_I_nx/num_nodes) << "\t" << (avg_n2i/num_nodes) << "\t" << (avg_ny/num_nodes) << "\t" << (avg_i/num_nodes) << "\t" << (avg_t/num_nodes) << endl;

}

int main (int argc, char* argv[]) { 
  if(argc != 4){
    cout << "Please see usage below: " << endl;
    cout << "\t./main <adjacency list file/folder> <# of files> <# of threads>" << endl;
    exit(0);
  } 

  omp_set_num_threads(atoi(argv[3]));      

  VectorGraph *vg = ReadFile(argv[1],atoi(argv[2]));
  runStats(vg);

  return 0;
}
