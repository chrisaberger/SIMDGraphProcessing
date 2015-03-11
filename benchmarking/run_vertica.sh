#!/bin/bash

# Change the current directory to the directory of the script
cd "$(dirname "$0")"

source env.sh

curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"
numruns="7"
datasets="cid-patents higgs socLivejournal orkut g_plus" # higgs socLivejournal orkut g_plus"
system="vertica"
num_threads="1 48"

odir="${output}/${system}_${date}_${curtime}"
mkdir $odir
echo $odir
for i in `seq 1 $numruns`; do
  for threads in $num_threads; do
    for dataset in $datasets; do
       cd ${system}
       echo "Benchmarking ${system} on ${dataset} with ${threads} threads"
       ./run.sh 1 /dfs/scratch0/caberger/datasets/${dataset} ${threads} |& tee $odir/${system}.${dataset}.${threads}.${i}.log
       cd ..
    done
  done
done
