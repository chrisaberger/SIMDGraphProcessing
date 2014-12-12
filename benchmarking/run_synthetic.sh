#!/bin/bash

output="/dfs/scratch0/caberger/synthetic_output"
curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"
system="emptyheaded"
numruns="5"


odir="${output}/${system}_${date}_${curtime}"
mkdir $odir
echo $odir

cd ${system}
for vertices "200000" in ; do
  #"1000000" "2000000" "3000000" "4000000" "6000000" "8000000" "16000000" "32000000" "64000000"
  for degree in "128000000" "256000000" "512000000" "1250000000"; do
    for threads in "1" "24" "48"; do
      for layout in "uint" "pshort" "bs"; do
        ./run_synthetic.sh $numruns synthetic_perf_2 /dfs/scratch0/caberger/datasets/synthetic/data/node$vertices/synthetic_${vertices}_${degree}.txt $threads $layout | tee $odir/v${vertices}.e${degree}.${layout}.${threads}.log
      done
      ./run_synthetic.sh $numruns synthetic_perf_2 /dfs/scratch0/caberger/datasets/synthetic/data/node$vertices/synthetic_${vertices}_${degree}.txt $threads hybrid | tee $odir/v${vertices}.e${degree}.hybrid_perf.${threads}.log
      for layout in "v" "bp"; do
        ./run_synthetic.sh $numruns synthetic_comp_2 /dfs/scratch0/caberger/datasets/synthetic/data/node$vertices/synthetic_${vertices}_${degree}.txt $threads $layout | tee $odir/v${vertices}.e${degree}.${layout}.${threads}.log
      done
      ./run_synthetic.sh $numruns synthetic_comp_2 /dfs/scratch0/caberger/datasets/synthetic/data/node$vertices/synthetic_${vertices}_${degree}.txt $threads hybrid | tee $odir/v${vertices}.e${degree}.hybrid_comp.${threads}.log
    done
  done
done

for vertices in "1000000"; do
  for degree in "1000000" "2000000" "3000000" "4000000" "5000000" "10000000" "20000000" "25000000" "30000000" "35000000" "40000000"; do
    for threads in "1" "24" "48"; do
      for layout in "uint" "pshort" "bs"; do
        ./run_synthetic.sh $numruns synthetic_perf_100 /dfs/scratch0/caberger/datasets/synthetic/data/node$vertices/synthetic_${vertices}_${degree}.txt $threads $layout | tee $odir/v${vertices}.e${degree}.${layout}.${threads}.log
      done
      ./run_synthetic.sh $numruns synthetic_perf_100 /dfs/scratch0/caberger/datasets/synthetic/data/node$vertices/synthetic_${vertices}_${degree}.txt $threads hybrid | tee $odir/v${vertices}.e${degree}.hybrid_perf.${threads}.log
      for layout in "v" "bp"; do
        ./run_synthetic.sh $numruns synthetic_comp_100 /dfs/scratch0/caberger/datasets/synthetic/data/node$vertices/synthetic_${vertices}_${degree}.txt $threads $layout | tee $odir/v${vertices}.e${degree}.${layout}.${threads}.log
      done
      ./run_synthetic.sh $numruns synthetic_comp_100 /dfs/scratch0/caberger/datasets/synthetic/data/node$vertices/synthetic_${vertices}_${degree}.txt $threads hybrid | tee $odir/v${vertices}.e${degree}.hybrid_comp.${threads}.log
    done
  done
done
cd ..