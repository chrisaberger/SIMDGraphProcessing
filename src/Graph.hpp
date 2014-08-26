#ifndef GRAPH_H
#define GRAPH_H

#include <omp.h>
#include <unordered_map>
#include <xmmintrin.h>
#include <cstring>
#include <immintrin.h>
#include <unordered_map>
#include <setjmp.h>

#include "Partition.hpp"

using namespace std;

struct CompressedGraph {
  const size_t upper_shift;
  const size_t lower_shift;
  const size_t num_nodes;
  const size_t num_edges;
  const size_t edge_array_length;
  const unsigned int *nbr_lengths;
  const size_t *nodes;
  const unsigned short *edges;
  const unordered_map<size_t,size_t> *external_ids;
  unsigned short *unions;
  size_t *union_size;
  CompressedGraph(  
    const size_t upper_shift_in,
    const size_t lower_shift_in,
    const size_t num_nodes_in, 
    const size_t num_edges_in,
    const size_t edge_array_length_in,
    const unsigned int *nbrs_lengths_in, 
    const size_t *nodes_in,
    const unsigned short *edges_in,
    const unordered_map<size_t,size_t> *external_ids_in,
    unsigned short *unions_in,
    size_t *union_size_in): 
      upper_shift(upper_shift_in),
      lower_shift(lower_shift_in),
      num_nodes(num_nodes_in), 
      num_edges(num_edges_in),
      edge_array_length(edge_array_length_in),
      nbr_lengths(nbrs_lengths_in),
      nodes(nodes_in), 
      edges(edges_in),
      external_ids(external_ids_in),
      unions(unions_in),
      union_size(union_size_in){}
    
    inline size_t getEndOfNeighborhood(const size_t node){
      size_t end = 0;
      if(node+1 < num_nodes) end = nodes[node+1];
      else end = edge_array_length;
      return end;
    }
    inline size_t neighborhoodStart(const size_t node){
      return nodes[node];
    }
    inline size_t neighborhoodEnd(const size_t node){
      return nodes[node+1];
    }
    inline void setupInnerPartition(size_t &i, size_t &prefix, size_t &end,bool &isBitSet){
      const size_t header_length = 2;
      const size_t start = i;
      prefix = edges[i++];
      const size_t len = edges[i++];
      isBitSet = len > WORDS_IN_BS;
      if(isBitSet)
        end = start+header_length+WORDS_IN_BS;
      else
        end = start+header_length+len;
    }
    inline long countTriangles(){
      long count = 0;
      unsigned short *result = new unsigned short[num_nodes];

      #pragma omp parallel for default(none) shared(result) schedule(static,150) reduction(+:count)   
      for(size_t i = 0; i < num_nodes; ++i){
        //cout << i << endl;
        //count += foreachNbr(i,&CompressedGraph::intersect_neighborhoods,result);
          const size_t start1 = neighborhoodStart(i);
          const size_t end1 = neighborhoodEnd(i);
          count += intersect_partitioned(result,edges+start1,unions+union_size[i],end1-start1,union_size[i+1]-union_size[i]);
      }
      return count;
    }
    inline long intersect_neighborhoods(const size_t nbr, const size_t n, unsigned short *r) {
      //cout << "Intersecting: " << n << " with " << nbr << endl;

      const size_t start1 = neighborhoodStart(nbr);
      const size_t end1 = neighborhoodEnd(nbr);

      const size_t start2 = neighborhoodStart(n);
      const size_t end2 = neighborhoodEnd(n);

      // /cout << "Length: " << (end1-start1) << endl;

      long result = intersect_partitioned(r,edges+start1,edges+start2,end1-start1,end2-start2);
      //cout << "OUTPUT: " << result << endl;
      return result;
    }
    inline long foreachNbr(size_t node,long (CompressedGraph::*func)(const size_t,const size_t,unsigned short*), unsigned short *r){
      long count = 0;
      const size_t start1 = neighborhoodStart(node);
      const size_t end1 = neighborhoodEnd(node);
      for(size_t j = start1; j < end1; ++j){
        size_t prefix, partition_end; bool isBitSet;
        setupInnerPartition(j,prefix,partition_end,isBitSet);
       
        if(!isBitSet){
          //Traverse partition use prefix to get nbr id.
          for(;j < partition_end;++j){
            const size_t cur = (prefix << lower_shift) | edges[j]; //neighbor node
            long ncount = (this->*func)(cur,node,r);
            count += ncount;
          }
        }else{
          //4096 shorts in every BS
          for(size_t ii=0;(ii < WORDS_IN_BS);++ii){
            //Go through each bit
            for(size_t jj = 0;(jj < 16);++jj){
              unsigned short index = (jj + (ii << 4)); // jj + ii *16; I don't trust compilers.
              if(isSet(index,&edges[j])){
                const size_t cur = (prefix << lower_shift) | index; //neighbor node
                long ncount = (this->*func)(cur,node,r);
                count += ncount;
              }
            }
          } //end outer for
        }//end else */
        j = partition_end-1;   
      }
      return count;
    }
    inline double pagerank(){
      float *pr = new float[num_nodes];
      float *oldpr = new float[num_nodes];
      const double damp = 0.85;
      const int maxIter = 100;
      const double threshold = 0.0001;

      #pragma omp parallel for default(none) schedule(static,150) shared(pr,oldpr)
      for(size_t i=0; i < num_nodes; ++i){
        oldpr[i] = 1.0/num_nodes;
      }
      
      int iter = 0;
      double delta = 1000000000000.0;
      float totalpr = 0.0;
      while(delta > threshold && iter < maxIter){
        totalpr = 0.0;
        delta = 0.0;
        #pragma omp parallel for default(none) shared(pr,oldpr) schedule(static,150) reduction(+:delta) reduction(+:totalpr)
        for(size_t i = 0; i < num_nodes; ++i){
          float sum = 0.0;
          const size_t start1 = neighborhoodStart(i);
          const size_t end1 = neighborhoodEnd(i);
          for(size_t j = start1; j < end1; ++j){
            size_t prefix, partition_end; bool isBitSet;
            setupInnerPartition(j,prefix,partition_end,isBitSet);
           
            if(!isBitSet){
              //Traverse partition use prefix to get nbr id.
              for(;j < partition_end;++j){
                //to be honest this doesn't really speed anything up, looks cool tho :)
                if(j+7 < partition_end){
                  float tmp_pr_holder[8];
                  float tmp_deg_holder[8];
                  for(size_t k = 0; k < 8; k++){
                    size_t cur = (prefix << 16) | edges[j+k];
                    tmp_pr_holder[k] = oldpr[cur];
                    tmp_deg_holder[k] = nbr_lengths[cur];
                  }
                  __m256 pr = _mm256_load_ps(tmp_pr_holder);
                  __m256 deg = _mm256_load_ps(tmp_deg_holder);
                  __m256 d = _mm256_div_ps(pr, deg);
                  _mm256_storeu_ps(tmp_pr_holder, d);
                  for(size_t o = 0; o < 8; o++){
                    sum += tmp_pr_holder[o]; //should be auto vectorized.
                  }
                  j += 7;
                  //const size_t len2 = nbr_lengths[cur];
                  //sum += oldpr[cur]/len2;
                } else{
                    const size_t cur = (prefix << 16) | edges[j]; //neighbor node
                    const size_t len = nbr_lengths[cur];
                    sum += oldpr[cur]/len;
                }
              }
            }else{
              //4096 shorts in every BS
              for(size_t ii=0;(ii < WORDS_IN_BS);++ii){
                //Go through each bit
                for(size_t jj = 0;(jj < 16);++jj){
                  unsigned short index = (jj + (ii << 4)); // jj + ii *16; I don't trust compilers.
                  if(isSet(index,&edges[j])){
                    const size_t cur = (prefix << 16) | index; //neighbor node
                    const size_t len = nbr_lengths[cur];
                    sum += oldpr[cur]/len;
                  }
                }
              } //end outer for
            }//end else */
            j = partition_end-1;  
          }
          pr[i] = ((1.0-damp)/num_nodes) + damp * sum;
          delta += abs(pr[i]-oldpr[i]);
          totalpr += pr[i];
        }

        float *tmp = oldpr;
        oldpr = pr;
        pr = tmp;
        ++iter;
      }
      pr = oldpr;
      cout << "Iter: " << iter << endl;
      /*
      for(size_t i=0; i < num_nodes; ++i){
        cout << "Node: " << i << " PR: " << pr[i] << endl;
      }
      */
      return totalpr;
    }
    /*
    inline void printGraph(){
      for(size_t i = 0; i < num_nodes; ++i){
        size_t start1 = nodes[i];
        
        size_t end1 = 0;
        if(i+1 < num_nodes) end1 = nodes[i+1];
        else end1 = edge_array_length;

        size_t j = start1;
        cout << "Node: " << i <<endl;

        while(j < end1){
          unsigned int prefix = (edges[j] << 16);
          size_t len = edges[j+1];
          j += 2;
          size_t inner_end;
          if(len > WORDS_IN_BS){
            inner_end = j+4096;
            printBitSet(prefix,WORDS_IN_BS,&edges[j]);
            j = inner_end;
          }
          else{
            inner_end = j+len;
            while(j < inner_end){
              unsigned int tmp = prefix | edges[j];
              cout << "Nbr: " << tmp << endl;
              ++j;
            }
            j--;
          }
          j = inner_end;
        }
        cout << endl;
      }
    }
    inline double pagerank(){
      float *pr = new float[num_nodes];
      float *oldpr = new float[num_nodes];
      const double damp = 0.85;
      const int maxIter = 100;
      const double threshold = 0.0001;

      #pragma omp parallel for default(none) schedule(static,150) shared(pr,oldpr)
      for(size_t i=0; i < num_nodes; ++i){
        oldpr[i] = 1.0/num_nodes;
      }
      
      int iter = 0;
      double delta = 1000000000000.0;
      float totalpr = 0.0;
      while(delta > threshold && iter < maxIter){
        totalpr = 0.0;
        delta = 0.0;
        #pragma omp parallel for default(none) shared(pr,oldpr) schedule(static,150) reduction(+:delta) reduction(+:totalpr)
        for(size_t i = 0; i < num_nodes; ++i){
          const size_t start1 = nodes[i];
          const size_t end1 = getEndOfNeighborhood(i);
          
          //float *tmp_degree_holder = new float[len];
          //size_t k = 0;

          size_t j = start1;
          float sum = 0.0;
          while(j < end1){
            size_t prefix, inner_end;
            traverseInnerPartition(j,prefix,inner_end);
            while(j < inner_end){
              if(j+7 < inner_end){
                float tmp_pr_holder[8];
                float tmp_deg_holder[8];
                for(size_t k = 0; k < 8; k++){
                  size_t cur = (prefix << 16) | edges[j+k];
                  tmp_pr_holder[k] = oldpr[cur];
                  tmp_deg_holder[k] = nbr_lengths[cur];
                }
                __m256 pr = _mm256_load_ps(tmp_pr_holder);
                __m256 deg = _mm256_load_ps(tmp_deg_holder);
                __m256 d = _mm256_div_ps(pr, deg);
                _mm256_storeu_ps(tmp_pr_holder, d);
                for(size_t o = 0; o < 8; o++){
                  sum += tmp_pr_holder[o]; //should be auto vectorized.
                }
                j += 8;
                //const size_t len2 = nbr_lengths[cur];
                //sum += oldpr[cur]/len2;
              }
              else{
                const size_t cur = (prefix << 16) | edges[j];
                const size_t len2 = nbr_lengths[cur];
                sum += oldpr[cur]/len2;
                ++j;
              }
            }
          }
          pr[i] = ((1.0-damp)/num_nodes) + damp * sum;
          delta += abs(pr[i]-oldpr[i]);
          totalpr += pr[i];
        }

        float *tmp = oldpr;
        oldpr = pr;
        pr = tmp;
        ++iter;
      }
      pr = oldpr;
    
      cout << "Iter: " << iter << endl;
      
      for(size_t i=0; i < num_nodes; ++i){
        cout << "Node: " << i << " PR: " << pr[i] << endl;
      }

      return totalpr;
    }
    */
};

#endif
