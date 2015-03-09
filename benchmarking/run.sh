#!/bin/bash

# Change the current directory to the directory of the script
cd "$(dirname "$0")"

source env.sh

curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"
numruns="1"

systems="graphlab snapr pgx emptyheaded"
datasets="g_plus"
num_threads="1 48"

for system in $systems; do
  odir="${output}/advanced_${system}_${date}_${curtime}"
  mkdir $odir
  echo $odir
  echo $datasets
   for dataset in $datasets; do
      for threads in $num_threads; do
         cd ${system}
         echo "Benchmarking ${system} on ${dataset} with ${threads} threads"
         ./run.sh $numruns /dfs/scratch0/caberger/datasets/${dataset} $threads | tee $odir/${system}.${dataset}.${threads}.log
         cd ..
      done
   done
done
