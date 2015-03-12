#define WRITE_VECTOR 0

#include "emptyheaded.hpp"

template<class T, class R>
class undirected_triangle_counting: public application<T,R> {
  public:
    long num_triangles;

    undirected_triangle_counting(Parser input_data): 
      application<T,R>(input_data) {
      num_triangles = 0;
    }

    SparseMatrix<T,R>* materialize_graph(){
      auto node_selection = [](uint32_t node, uint32_t attribute){
        (void) node; (void) attribute;
        return true;
      };
      auto edge_selection = [](uint32_t node, uint32_t nbr, uint32_t attribute){
        (void) attribute;
        return nbr < node;
      };
      return SparseMatrix<T,R>::build(this->input_graph,node_selection,edge_selection,this->num_threads);
    }

    void run(){
      //Construct the graph, materializes specialized representations.
      SparseMatrix<T,R>* graph = materialize_graph();

      //Actual application starts here.
      double start_time = common::startClock();

      //One buffer for writing the intersection output, one for holding the counts
        ParallelBuffer<uint8_t> buffers(this->num_threads,512*graph->max_nbrhood_size*sizeof(uint32_t));
        size_t *t_count = new size_t[this->num_threads * PADDING];

        //The heart of the application.
        common::par_for_range(this->num_threads, 0, graph->matrix_size, 100,
          [&](size_t tid){
            buffers.allocate(tid);
            t_count[tid*PADDING] = 0;
          },
          ////////////////////////////////////////////////////
          [&](size_t tid, size_t i) {
             long t_num_triangles = 0;
             Set<R> A = graph->get_row(i);
             Set<R> C(buffers.data[tid]);

             A.foreach([&] (uint32_t j){
              Set<R> B = graph->get_row(j);
              t_num_triangles += ops::set_intersect(&C,&A,&B)->cardinality;
             });

             t_count[tid*PADDING] += t_num_triangles;
          },
          ////////////////////////////////////////////////////////////
          [&](size_t tid){
            num_triangles += t_count[tid*PADDING];
          }
        );
        common::stopClock("UNDIRECTED TRIANGLE COUNTING",start_time);

        cout << "Number of triangles: " << num_triangles << endl;
    }
};

template<class T, class R>
static application<T,R>* compute(Parser input_data){
  return new undirected_triangle_counting<T,R>(input_data); 
}
