#define WRITE_VECTOR 1
#define PATH
#include "emptyheaded.hpp"

template<class T, class R>
class n_path: public application<T,R> {
  public:
    size_t path_length;

    n_path(Parser input_data): 
      application<T,R>(input_data) {
      path_length = 0;
    }

    SparseMatrix<T,R>* materialize_graph(){
      auto node_selection = [](uint32_t node, uint32_t attribute) -> bool {
        (void) node; (void) attribute;
        return true;
      };
      auto edge_selection = [](uint32_t node, uint32_t nbr, uint32_t attribute) -> bool {
        (void) node; (void) nbr; (void) attribute;
        return true;
      };
      return SparseMatrix<T,R>::build(this->input_graph,node_selection,edge_selection,this->num_threads);
    }

    void run(){
      //Construct the graph, materializes specialized representations.
      SparseMatrix<T,R>* graph = materialize_graph();

      //Figure out where we want to start from.
      uint32_t internal_start;
      if(this->start_node == -1)
        internal_start = graph->get_max_row_id();
      else
        internal_start = graph->get_internal_id(this->start_node);
      cout << "Start node - External: " << graph->id_map[internal_start] << " Internal: " << internal_start << endl;

      uint8_t *f_data = new uint8_t[graph->matrix_size*sizeof(uint64_t)];
      uint32_t *start_array = new uint32_t[1];
      start_array[0] = internal_start;

      const size_t bs_size = (((graph->matrix_size + 64) / 64) * 8) + 8;

      //allocate a new visited array and set the start node
      Set<bitset> visited(bs_size);
      Set<bitset> old_visited(bs_size);

      bitset::set(internal_start,(uint64_t*)(visited.data+sizeof(uint64_t)),0);
      Set<uinteger> frontier = Set<uinteger>::from_array(f_data,start_array,1);

      Set<bitset> **vis_bufs = new Set<bitset>*[this->num_threads * PADDING];
      for(size_t i = 0; i < this->num_threads; i ++){
        vis_bufs[i * PADDING] = new Set<bitset>(bs_size);
      }

      double pure_bfs_time = common::startClock();
      while(true){
        //cout << endl << " Path: " << path_length << " F-TYPE: " << frontier.type <<  " CARDINALITY: " << frontier.cardinality << endl;
        old_visited.copy_from(visited);

        size_t real_num_threads = this->num_threads;
        real_num_threads = frontier.par_foreach(this->num_threads,
          [&] (size_t tid,uint32_t n){
            Set<T> outnbrs = graph->get_row(n);
            ops::set_union(vis_bufs[tid * PADDING], &outnbrs);
        });

        size_t left_to_merge = real_num_threads;
        while(left_to_merge > 1) {
          for(size_t i = 0; i < left_to_merge / 2; i++){
            ops::set_union(vis_bufs[i * PADDING], vis_bufs[(left_to_merge - i - 1) * PADDING]);
          }
          left_to_merge = left_to_merge / 2 + (left_to_merge % 2);
        }
        ops::set_union(&visited, vis_bufs[0]);

        frontier = *ops::set_difference(&frontier,&visited,&old_visited);

        if(frontier.cardinality == 0 || path_length >= this->query_depth)
          break;
        path_length++;
      }

      common::stopClock("pure bfs time", pure_bfs_time);
      cout << "path length: " << path_length << endl;
    }
};

template<class T, class R>
static application<T,R>* init_app(Parser input_data){
  return new n_path<T,R>(input_data);
}
