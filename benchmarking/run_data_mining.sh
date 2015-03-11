#!/bin/bash

# Change the current directory to the directory of the script
cd "$(dirname "$0")"

output=/dfs/scratch0/noetzli/output
datasets="g_plus higgs socLivejournal orkut cid-patents twitter2010 wikipedia"
STABLE_BIN_DIR=/afs/cs.stanford.edu/u/noetzli/tmp/EmptyHeaded/stable_binaries

curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"
numruns="7"

for system in "data_mining"; do
  odir="${output}/${system}_${date}_${curtime}"
  mkdir $odir
  echo $odir
  for run in `seq $numruns`; do
   for dataset in $datasets; do
     echo ${STABLE_BIN_DIR}/similar_nodes --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=48 --layout=hybrid | tee $odir/similar_nodes_${dataset}_${run}_hybrid.log
     ${STABLE_BIN_DIR}/similar_nodes --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=48 --layout=hybrid | tee $odir/similar_nodes_${dataset}_${run}_hybrid.log

     echo ${STABLE_BIN_DIR}/similar_nodes_no_sra --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=48 --layout=uint | tee $odir/similar_nodes_${dataset}_${run}_uint.log
     ${STABLE_BIN_DIR}/similar_nodes_no_sra --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/u_degree.bin --input_type=binary --t=48 --layout=uint | tee $odir/similar_nodes_${dataset}_${run}_uint.log

     echo ${STABLE_BIN_DIR}/symbiocity --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/d_data.bin --input_type=binary --t=48 --layout=hybrid | tee $odir/symbiocity_${dataset}_${run}_hybrid.log
     ${STABLE_BIN_DIR}/symbiocity --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/d_data.bin --input_type=binary --t=48 --layout=hybrid | tee $odir/symbiocity_${dataset}_${run}_hybrid.log

     echo ${STABLE_BIN_DIR}/symbiocity_no_sra --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/d_data.bin --input_type=binary --t=48 --layout=uint | tee $odir/symbiocity_${dataset}_${run}_uint.log
     ${STABLE_BIN_DIR}/symbiocity_no_sra --graph=/dfs/scratch0/caberger/datasets/${dataset}/bin/d_data.bin --input_type=binary --t=48 --layout=uint | tee $odir/symbiocity_${dataset}_${run}_uint.log
   done
  done
done
