#!/bin/bash

# Change the current directory to the directory of the script
cd "$(dirname "$0")"

output=/dfs/scratch0/noetzli/output
datasets="g_plus higgs socLivejournal orkut cid-patents"
orderings="random degree"
STABLE_BIN_DIR=/afs/cs.stanford.edu/u/noetzli/tmp/EmptyHeaded/stable_binaries

curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"
numruns="7"

for system in "features"; do
  odir="${output}/${system}_${date}_${curtime}"
  mkdir $odir
  echo $odir
  for run in `seq $numruns`; do
   for dataset in $datasets; do
     echo ${STABLE_BIN_DIR}/undirected_triangle_counting --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=hybrid
     ${STABLE_BIN_DIR}/undirected_triangle_counting --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=hybrid
     echo ${STABLE_BIN_DIR}/undirected_triangle_counting_no_p --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=hybrid
     ${STABLE_BIN_DIR}/undirected_triangle_counting_no_p --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=hybrid | tee $odir/${dataset}_${run}_no_p.log

     echo ${STABLE_BIN_DIR}/undirected_triangle_counting_no_s --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=hybrid
     ${STABLE_BIN_DIR}/undirected_triangle_counting_no_s --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=hybrid
     echo ${STABLE_BIN_DIR}/undirected_triangle_counting_no_sp --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=hybrid
     ${STABLE_BIN_DIR}/undirected_triangle_counting_no_sp --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=hybrid

     echo ${STABLE_BIN_DIR}/undirected_triangle_counting --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=uint
     ${STABLE_BIN_DIR}/undirected_triangle_counting --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=uint
     echo ${STABLE_BIN_DIR}/undirected_triangle_counting_no_p --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=uint
     ${STABLE_BIN_DIR}/undirected_triangle_counting_no_p --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=uint

     echo ${STABLE_BIN_DIR}/undirected_triangle_counting_no_a --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=hybrid
     ${STABLE_BIN_DIR}/undirected_triangle_counting_no_a --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=hybrid
     echo ${STABLE_BIN_DIR}/undirected_triangle_counting_no_ap --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=hybrid
     ${STABLE_BIN_DIR}/undirected_triangle_counting_no_ap --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=hybrid

     echo ${STABLE_BIN_DIR}/undirected_triangle_counting_no_sra --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=uint
     ${STABLE_BIN_DIR}/undirected_triangle_counting_no_sra --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=uint
     echo ${STABLE_BIN_DIR}/undirected_triangle_counting_no_srap --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=uint
     ${STABLE_BIN_DIR}/undirected_triangle_counting_no_srap --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=1 --layout=uint
   done
  done
done
