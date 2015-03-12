#define WRITE_VECTOR 1

#include "emptyheaded.hpp"

template<class T, class R>
class n_clique: public application<T,R> {
  public:
    long num_cliques;
    size_t query_depth;
    SparseMatrix<T,R>* graph;

    n_clique(Parser input_data):
      application<T,R>(input_data) {
      num_cliques = 0;
      query_depth = input_data.n;
    }

    SparseMatrix<T,R>* materialize_graph(){
      auto node_selection = [](uint32_t node, uint32_t attribute) -> bool {
        (void) node; (void) attribute;
        return true;
      };
      auto edge_selection = [](uint32_t node, uint32_t nbr, uint32_t attribute) -> bool {
        (void) attribute;
        return nbr < node;
      };
      return SparseMatrix<T,R>::from_symmetric_graph(this->input_graph,node_selection,edge_selection,this->num_threads);
    }

    inline size_t apply_function(size_t node, size_t depth, Set<R> **set_buffers, Table<uint32_t>* decode_buffers, Table<uint64_t>* output) {
      Set<R>* A = set_buffers[depth-1];
      Set<R> B = graph->get_decoded_row(node,decode_buffers->data[depth]);

      Set<R> *C = ops::set_intersect(set_buffers[depth],A,&B);
      set_buffers[depth] = C;

      size_t count = 0;
      if(++depth == query_depth){
        #if WRITE_TABLE == 1
        output->write_table(*C,graph->id_map);
        #endif
        return C->cardinality;
      } else{
        C->foreach([this,&count,depth,set_buffers,output,decode_buffers] (uint32_t i){
          output->tuple[depth-1] = graph->id_map[i];
          count += this->apply_function(i,depth,set_buffers,decode_buffers,output);
        });
      }
      return count;
    }

    void run() {
      // Construct the graph, materializes specialized representations.
      graph = materialize_graph();

      //Actual application starts here.
      double start_time = common::startClock();

      size_t num_nodes = graph->matrix_size;
      const size_t estimated_table_size = ((graph->cardinality*40)/this->num_threads);
      ParallelTable<uint64_t> output = ParallelTable<uint64_t>(this->num_threads,query_depth,estimated_table_size);
      ParallelTable<uint32_t> decode_buffers = ParallelTable<uint32_t>(this->num_threads,query_depth,graph->max_nbrhood_size*sizeof(uint32_t));
      Set<R> **set_buffers = new Set<R>*[PADDING*query_depth*this->num_threads];
      uint8_t* common_buffer = new uint8_t[this->num_threads * query_depth * num_nodes];

      common::par_for_range(this->num_threads,0,num_nodes,100,
        ///////////////////////////////////////////////////////////
        [&](size_t tid){
          for(size_t j = 0; j < query_depth; j++){
            set_buffers[PADDING*tid*query_depth+j] = new Set<R>(common_buffer + (tid * query_depth + j) * num_nodes, num_nodes);
          }
          decode_buffers.allocate(tid);
          output.allocate(tid);
        },
        //////////////////////////////////////////////////////////
        [&](size_t tid, size_t i) {
          Table<uint32_t> *thread_decode_buffers = decode_buffers.table[tid];
          Table<uint64_t> *thread_output = output.table[tid];
          Set<R> **thread_set_buffers = &set_buffers[PADDING*tid*query_depth];

          Set<R> A = this->graph->get_decoded_row(i,thread_decode_buffers->data[query_depth*tid]);
          thread_output->tuple[0] = graph->id_map[i];
          thread_set_buffers[1] = &A;

          A.foreach([&] (uint32_t j){
            thread_output->tuple[1] = graph->id_map[j];
            thread_output->cardinality += this->apply_function(j,2,thread_set_buffers,thread_decode_buffers, thread_output);
          });
        },
        [&](size_t tid){
        /////////////////////////////////////////////////////////
          Table<uint64_t> *thread_output = output.table[tid];
          output.cardinality += thread_output->cardinality;
        }
      );

      num_cliques = output.cardinality;

      common::stopClock("n_cliques",start_time);
      cout << "Number of cliques: " << num_cliques << endl;
    }
};

template<class T, class R>
static application<T,R>* compute(Parser input_data) {
  return new n_clique<T,R>(input_data);
}
