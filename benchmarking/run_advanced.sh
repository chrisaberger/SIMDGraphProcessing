#!/bin/bash

# Change the current directory to the directory of the script
cd "$(dirname "$0")"

output=/dfs/scratch0/noetzli/output
datasets="g_plus higgs socLivejournal orkut cid-patents"
orderings="random degree"
threads="48"
STABLE_BIN_DIR=/afs/cs.stanford.edu/u/noetzli/tmp/EmptyHeaded/stable_binaries

curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"
numruns="7"

for system in "advanced"; do
  odir="${output}/${system}_${date}_${curtime}"
  mkdir $odir
  echo $odir
  for run in `seq $numruns`; do
    for nthread in $threads; do
       for dataset in $datasets; do
         echo ${STABLE_BIN_DIR}/n_clique_no_sra --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=${nthread} --layout=uint
         ${STABLE_BIN_DIR}/n_clique_no_sra --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=${nthread} --layout=uint | tee $odir/clique_${dataset}_${run}_${nthread}_no_sra.log
         echo ${STABLE_BIN_DIR}/undirected_lollipop_counting_no_sra --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=${nthread} --layout=uint
         ${STABLE_BIN_DIR}/undirected_lollipop_counting_no_sra --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=${nthread} --layout=uint | tee $odir/lollipop_${dataset}_${run}_${nthread}_no_sra.log
         echo ${STABLE_BIN_DIR}/undirected_lollipop_counting_slow --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=${nthread} --layout=hybrid
         ${STABLE_BIN_DIR}/undirected_lollipop_counting_slow --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=${nthread} --layout=hybrid | tee $odir/lollipop_${dataset}_${run}_${nthread}_slow.log
       done
    done
  done
done
