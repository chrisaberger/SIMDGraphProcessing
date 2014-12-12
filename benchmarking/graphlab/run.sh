#!/bin/bash
GRAPHLAB_HOME="/afs/cs.stanford.edu/u/caberger/graphlab"

export GRAPHLAB_THREADS_PER_WORKER=$3
snode=`cat ${2}/largest_degree_external_id.txt`

for i in `seq 1 ${1}`; do
  #${GRAPHLAB_HOME}/release/toolkits/graph_analytics/undirected_triangle_count --graph=$1/glab_undirected/data.txt --format=snap --ncpus=$2
  echo "COMMAND: ${GRAPHLAB_HOME}/release/toolkits/graph_analytics/sssp --graph=$2/edgelist/data.txt --format=snap --ncpus=$3 --source=$snode"
  ${GRAPHLAB_HOME}/release/toolkits/graph_analytics/sssp --graph=$2/edgelist/data.txt --format=snap --ncpus=$3 --source=$snode
done