#!/bin/bash

numruns="5"
output="/dfs/scratch0/caberger/internal_output"
curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"
system="emptyheaded"

odir="${output}/${system}_${curtime}_${date}"
mkdir $odir
echo $odir
  for dataset in "higgs" "flickr" "socLivejournal" "orkut" "cid-patents" "pokec" "twitter2010" "wikipedia"; do
    for threads in "1" "24" "48"; do
      for ordering in "u_degree" "u_bfs" "u_random" "u_rev_degree" "u_strong_run"; do
        cd ${system}
        for layout in "a32" "a16"; do
          ./run_internal.sh $numruns undirected_triangle_counting /dfs/scratch0/caberger/datasets/${dataset}/bin/${ordering}.bin $threads $layout | tee $odir/${dataset}.${threads}.${layout}.${ordering}.log
        done
        ./run_internal.sh $numruns undirected_triangle_counting /dfs/scratch0/caberger/datasets/${dataset}/bin/${ordering}.bin $threads hybrid | tee $odir/${dataset}.${threads}.hybrid_perf.${ordering}.log
        for layout in "bp" "v"; do
          ./run_internal.sh $numruns undirected_triangle_counting_comp /dfs/scratch0/caberger/datasets/${dataset}/bin/${ordering}.bin $threads $layout | tee $odir/${dataset}.${threads}.${layout}.${ordering}.log
        done
        ./run_internal.sh $numruns undirected_triangle_counting_comp /dfs/scratch0/caberger/datasets/${dataset}/bin/${ordering}.bin $threads hybrid | tee $odir/${dataset}.${threads}.hybrid_comp.${ordering}.log
        #SIMD vrs NON-SIMD
        ./run_internal.sh $numruns undirected_triangle_counting_nonsimd /dfs/scratch0/caberger/datasets/${dataset}/bin/${ordering}.bin $threads hybrid | tee $odir/nonsimd_${dataset}.${threads}.hybrid_perf.${ordering}.log
        cd ..
      done
    done
  done