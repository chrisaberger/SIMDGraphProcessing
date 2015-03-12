#define WRITE_VECTOR 0

#include "emptyheaded.hpp"

template<class T, class R>
SparseMatrix<T,R>* materialize_graph(MutableGraph *input_graph, const size_t num_threads){
  auto node_selection = [](uint32_t node, uint32_t attribute){
    (void) node; (void) attribute;
    return true;
  };
  auto edge_selection = [](uint32_t node, uint32_t nbr, uint32_t attribute){
    (void) attribute;
    return nbr < node;
  };
  return SparseMatrix<T,R>::from_symmetric_graph(input_graph,node_selection,edge_selection,num_threads);
}

template<class T, class R>
size_t application(Parser input_data){
  const size_t num_threads = input_data.num_threads;

  //Construct the graph, materializes specialized representations.
  SparseMatrix<T,R>* graph = materialize_graph<T,R>(input_data.input_graph,num_threads);

  //Actual application starts here.
  double start_time = common::startClock();

  long num_triangles = 0;

  //One buffer for writing the intersection output, one for holding the counts
  ParallelBuffer<uint8_t> buffers(num_threads,512*graph->max_nbrhood_size*sizeof(uint32_t));
  size_t *t_count = new size_t[num_threads * PADDING];

  //The heart of the application.
  common::par_for_range(num_threads, 0, graph->matrix_size, 100,
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

  cout << "Number of Triangles: " << num_triangles << endl;
  return num_triangles;
}