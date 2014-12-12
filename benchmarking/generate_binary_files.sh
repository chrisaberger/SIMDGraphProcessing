#!/bin/bash

for dataset in "baidu" "california" "higgs" "flickr" "socLivejournal" "orkut" "cid-patents" "pokec" "twitter2010" "wikipedia"; do
  ../bin/undirectedEdgeListToBinary /dfs/scratch0/caberger/datasets/${dataset}/edgelist/data.txt /dfs/scratch0/caberger/datasets/${dataset}/bin
done
