#!/bin/bash

source env.sh

numruns="3"
curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"
system="emptyheaded"

datasets="cid-patents socLivejournal higgs orkut wikipedia twitter2010"
num_threads="1 48"


odir="${output}/${system}_${date}_${curtime}"
mkdir $odir
echo $odir
  for dataset in $datasets; do
    for threads in $num_threads; do
      for ordering in "u_the_game"; do
        cd ${system}
        for layout in "uint" "hybrid"; do
          ./run_internal.sh $numruns up_non_simd_tcount /dfs/scratch0/caberger/datasets/${dataset}/bin/${ordering}.bin $threads $layout | tee $odir/up_non_simd.${dataset}.${threads}.${ordering}.${layout}.log
          ./run_internal.sh $numruns up_simd_tcount /dfs/scratch0/caberger/datasets/${dataset}/bin/${ordering}.bin $threads $layout | tee $odir/up_simd.${dataset}.${threads}.${ordering}.${layout}.log
          ./run_internal.sh $numruns non_simd_tcount /dfs/scratch0/caberger/datasets/${dataset}/bin/${ordering}.bin $threads $layout | tee $odir/non_simd.${dataset}.${threads}.${ordering}.${layout}.log
          ./run_internal.sh $numruns simd_tcount /dfs/scratch0/caberger/datasets/${dataset}/bin/${ordering}.bin $threads $layout | tee $odir/simd.${dataset}.${threads}.${ordering}.${layout}.log
        done
        #./run_internal.sh $numruns undirected_triangle_counting /dfs/scratch0/caberger/datasets/${dataset}/bin/${ordering}.bin $threads hybrid | tee $odir/${dataset}.${threads}.hybrid_perf.${ordering}.log
        #for layout in "bp" "v"; do
        #  ./run_internal.sh $numruns $comp_bin /dfs/scratch0/caberger/datasets/${dataset}/bin/${ordering}.bin $threads $layout | tee $odir/${dataset}.${threads}.${layout}.${ordering}.log
        #done
        #./run_internal.sh $numruns $comp_bin /dfs/scratch0/caberger/datasets/${dataset}/bin/${ordering}.bin $threads hybrid | tee $odir/${dataset}.${threads}.hybrid_comp.${ordering}.log
        #SIMD vrs NON-SIMD
        #./run_internal.sh $numruns $nosimd_bin /dfs/scratch0/caberger/datasets/${dataset}/bin/${ordering}.bin $threads hybrid | tee $odir/nonsimd_${dataset}.${threads}.hybrid_perf.${ordering}.log
        cd ..
      done
    done
  done
