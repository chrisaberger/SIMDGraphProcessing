#define WRITE_VECTOR 0

#include "emptyheaded.hpp"

template<class T, class R>
class similar_nodes: public application<T,R> {
  public:
    size_t count;
    size_t N;

    similar_nodes(Parser input_data):
      application<T,R>(input_data) {
      count = 0;
      N = 10;
    }

    SparseMatrix<T,R>* materialize_graph(){
      auto node_selection = [](uint32_t node, uint32_t attribute) -> bool {
        (void) node; (void) attribute;
        return true;
      };
      auto edge_selection = [](uint32_t node, uint32_t nbr, uint32_t attribute) -> bool {
        (void) attribute; (void) node; (void) nbr;
        return true;
      };
      return SparseMatrix<T,R>::build(this->input_graph,node_selection,edge_selection,this->num_threads);
    }

    void run(){
      // Construct the graph, materializes specialized representations.
      SparseMatrix<T,R>* graph = materialize_graph();
      size_t num_threads = this->num_threads;

      // Actual application starts here.
      double start_time = common::startClock();

      ParallelBuffer<uint8_t> buffers(num_threads,512*graph->max_nbrhood_size*sizeof(uint32_t));

      uint32_t node = graph->get_max_row_id();
      Set<R> A = graph->get_row(node);

      size_t *t_count = new size_t[num_threads * PADDING];
      common::par_for_range(num_threads, 0, graph->matrix_size, 100,
        [&](size_t tid){
          buffers.allocate(tid);
          t_count[tid*PADDING] = 0;
        },
        ////////////////////////////////////////////////////
        [&](size_t tid, size_t i) {
           long t_num_similar_nodes = 0;
           Set<R> B = graph->get_row(i);
           Set<R> C(buffers.data[tid]);
           const size_t common_neighbors = ops::set_intersect(&C,&A,&B)->cardinality;
           if(common_neighbors > N) {
             t_num_similar_nodes++;
           }

           t_count[tid*PADDING] += t_num_similar_nodes;
        },
        ////////////////////////////////////////////////////////////
        [this,t_count](size_t tid){
          count += t_count[tid*PADDING];
        }
      );
    common::stopClock("similar_nodes", start_time);

    cout << "Count: " << count << endl << endl;
  }
};


template<class T, class R>
static application<T,R>* init_app(Parser input_data){
  return new similar_nodes<T,R>(input_data);
}
