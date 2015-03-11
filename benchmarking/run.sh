#!/bin/bash

# Change the current directory to the directory of the script
cd "$(dirname "$0")"

source env.sh

curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"
numruns="1"
datasets="cid-patents higgs socLivejournal orkut g_plus" # higgs socLivejournal orkut g_plus"

for system in $systems; do
  odir="${output}/DM_${system}_${date}_${curtime}"
  mkdir $odir
  echo $odir
   for dataset in $datasets; do
      for threads in $num_threads; do
         cd ${system}
         echo "Benchmarking ${system} on ${dataset} with ${threads} threads"
         ./run.sh $numruns /dfs/scratch0/caberger/datasets/${dataset} $threads |& tee $odir/${system}.${dataset}.${threads}.log
         #./run_symbiosity.sh $numruns /dfs/scratch0/caberger/datasets/${dataset} $threads |& tee $odir/SYMB_${system}.${dataset}.${threads}.log
         #./run_similarity.sh $numruns /dfs/scratch0/caberger/datasets/${dataset} $threads |& tee $odir/SIM_${system}.${dataset}.${threads}.log
         cd ..
      done
   done
done
