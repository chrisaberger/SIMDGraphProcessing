#!/bin/bash

for system in "graphx"; do
   for dataset in "com-amazon"; do
      for threads in "1" "24" "48"; do
         cd ${system}
         echo "Benchmarking ${system} on ${dataset} with ${threads} threads"
         ./run.sh /dfs/scratch0/noetzli/datasets/${dataset}/edgelist/data.txt $threads | tee ../${system}.${dataset}.${threads}.log
         cd ..
      done
   done
done
