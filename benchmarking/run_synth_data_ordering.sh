#!/bin/bash

source env.sh

numruns="5"
curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"
system="emptyheaded"

synth_p_data="1.5 2.3 3"

odir="${output}/synth_data_ordering_${date}_${curtime}"
mkdir $odir
echo $odir
for dataset in $synth_p_data; do
  for ordering in "u_shingles" "u_the_game" "u_degree" "u_bfs" "u_random" "u_rev_degree" "u_strong_run"; do
    cd ${system}
    for layout in "uint"; do
      ./run_internal.sh $numruns bin/undirected_triangle_counting /dfs/scratch0/caberger/datasets/power_law_synthetic/${dataset}/${ordering}.bin 1 $layout | tee $odir/${dataset}.${layout}.${ordering}.log
    done
    cd ..
  done
done
