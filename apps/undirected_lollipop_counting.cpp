#define WRITE_VECTOR 1

#include "emptyheaded.hpp"

template<class T, class R>
class undirected_lollipop_counting: public application<T,R> {
  public:
    size_t num_lollipops;

    undirected_lollipop_counting(Parser input_data):
      application<T,R>(input_data) {
      num_lollipops = 0;
    }

    SparseMatrix<T,R>* materialize_graph(){
      auto node_selection = [](uint32_t node, uint32_t attribute) -> bool {
        (void) attribute; (void) node;
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

      ParallelBuffer<uint32_t> y_buffers(num_threads,graph->max_nbrhood_size * 8);
      ParallelBuffer<uint32_t> z_buffers(num_threads,graph->max_nbrhood_size * 8);
      ParallelBuffer<uint8_t> r_buffers(num_threads,graph->max_nbrhood_size*sizeof(uint32_t) * 8);

      const size_t matrix_size = graph->matrix_size;
      size_t *t_count = new size_t[num_threads * PADDING];

      common::par_for_range(num_threads, 0, matrix_size, 20,
        [&](size_t tid){
          y_buffers.allocate(tid);
          z_buffers.allocate(tid);
          r_buffers.allocate(tid);
          t_count[tid*PADDING] = 0;
        },
        ////////////////////////////////////////////////////
        [&](size_t tid, size_t x) {
           long t_num_lollipops = 0;
           uint32_t* y_buffer = y_buffers.data[tid];
           uint32_t* z_buffer = z_buffers.data[tid];

           Set<R> ys = graph->get_decoded_row(x, y_buffer);
           ys.foreach([&](uint32_t y) {
             Set<R> rs(r_buffers.data[tid]);
             Set<R> zs = graph->get_decoded_row(y, z_buffer);
             ops::set_intersect(&rs, &ys, &zs);

             rs.foreach([&](uint32_t z) {
               if(z < y)
                 t_num_lollipops += ys.cardinality;
             });
           });

           t_count[tid*PADDING] += t_num_lollipops;
        },
        ////////////////////////////////////////////////////////////
        [&](size_t tid){
          num_lollipops += t_count[tid*PADDING];
        }
      );
    common::stopClock("lollipop",start_time);

    cout << "Count: " << num_lollipops << endl;
  }
};

template<class T, class R>
static application<T,R>* init_app(Parser input_data){
  return new undirected_lollipop_counting<T,R>(input_data);
}
