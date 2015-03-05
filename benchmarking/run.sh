#!/bin/bash

# Change the current directory to the directory of the script
cd "$(dirname "$0")"

source env.sh

curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"
numruns="7"

systems="snapr" #"graphlab snapr pgx emptyheaded"
datasets="baidu california higgs g_plus flickr socLivejournal orkut cid-patents pokec wikipedia twitter2010"
num_threads="1 48"

for system in $systems; do
  odir="${output}/${system}_${date}_${curtime}"
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
