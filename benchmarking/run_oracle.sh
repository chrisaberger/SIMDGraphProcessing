#!/bin/bash

# Change the current directory to the directory of the script
cd "$(dirname "$0")"

output=/dfs/scratch0/noetzli/output
datasets="g_plus higgs socLivejournal orkut cid-patents twitter2010 wikipedia"
orderings="random degree"
STABLE_BIN_DIR=/afs/cs.stanford.edu/u/noetzli/tmp/EmptyHeaded/stable_binaries

curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"
numruns="7"

for system in "oracle"; do
  odir="${output}/${system}_${date}_${curtime}"
  mkdir $odir
  echo $odir
  for run in `seq $numruns`; do
   for dataset in $datasets; do
     ${STABLE_BIN_DIR}/cost_undirected_triangle_counting --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=uint | tee $odir/${dataset}_${run}.log
   done
  done
done
