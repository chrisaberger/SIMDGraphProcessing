#!/bin/bash
LIGRA_HOME="/dfs/scratch0/noetzli/opt/ligra"

export LD_LIBRARY_PATH=/dfs/scratch0/noetzli/downloads/tmp/cilkplus-install/lib:/dfs/scratch0/noetzli/downloads/tmp/cilkplus-install//lib64:$LD_LIBRARY_PATH
export LIBRARY_PATH=/dfs/scratch0/noetzli/downloads/tmp/cilkplus-install/lib:/dfs/scratch0/noetzli/downloads/tmp/cilkplus-install/lib64:$LIBRARY_PATH
export CILK_NWORKERS=$3

snode=`cat ${2}/largest_degree_internal_id.txt`
for i in `seq 1 ${1}`; do
  echo "COMMAND: ${LIGRA_HOME}/BFS -r $snode $2/ligra/data.txt"
  ${LIGRA_HOME}/BFS -r $snode $2/ligra/data.txt
done