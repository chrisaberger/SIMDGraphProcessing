#!/bin/bash

for system in "socialite"; do
   for dataset in "california" "higgs" "flickr" "socLivejournal" "orkut" "cid-patents" "pokec" "twitter2010" "wikipedia"; do
      for threads in "1" "24" "48"; do
         cd ${system}
         echo "Benchmarking ${system} on ${dataset} with ${threads} threads"
         ./run.sh /dfs/scratch0/caberger/datasets/${dataset}/glab_undirected/data.txt $threads | tee ../${system}.${dataset}.${threads}.log
         cd ..
      done
   done
done
