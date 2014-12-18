#!/bin/bash

source env.sh

numruns="7"
output="/dfs/scratch0/caberger/bfs_output"
curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"
system="emptyheaded"
# datasets="higgs socLivejournal orkut cid-patents twitter2010"
datasets="twitter2010"
layouts="uint pshort bp v"

odir="${output}/${system}_${date}_${curtime}"
mkdir $odir
echo $odir
for dataset in datasets; do
  for threads in "1" "24" "48"; do
    cd ${system}
    for layout in layouts; do
      ./run_bfs.sh $numruns n_path_perf /dfs/scratch0/caberger/datasets/${dataset}/bin/d_data.bin $threads $layout | tee $odir/${dataset}.${threads}.${layout}.${ordering}.log
    done
    # ./run_bfs.sh $numruns n_path_perf /dfs/scratch0/caberger/datasets/${dataset}/bin/d_data.bin $threads hybrid | tee $odir/${dataset}.${threads}.hybrid_perf.${ordering}.log
    # ./run_bfs.sh $numruns n_path_comp /dfs/scratch0/caberger/datasets/${dataset}/bin/d_data.bin $threads hybrid | tee $odir/${dataset}.${threads}.hybrid_comp.${ordering}.log
    cd ..
  done
done

