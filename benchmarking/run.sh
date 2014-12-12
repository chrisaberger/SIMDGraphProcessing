#!/bin/bash

output="/dfs/scratch0/caberger/output"
curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"
numruns="7"

for system in "graphlab" "galois"; do
  odir="${output}/${system}_${curtime}_${date}"
  mkdir $odir
  echo $odir
   for dataset in "baidu" "california" "higgs" "flickr" "socLivejournal" "orkut" "cid-patents" "pokec" "twitter2010" "wikipedia"; do
      for threads in "1" "24" "48"; do
         cd ${system}
         echo "Benchmarking ${system} on ${dataset} with ${threads} threads"
         ./run.sh $numruns /dfs/scratch0/caberger/datasets/${dataset} $threads | tee $odir/${system}.${dataset}.${threads}.log
         cd ..
      done
   done
done
