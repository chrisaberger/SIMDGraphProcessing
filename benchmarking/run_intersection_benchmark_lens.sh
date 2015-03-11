#!/bin/bash

source env.sh

numruns="3"
curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"

odir="${output}/intersection_benchmark_${date}_${curtime}"
mkdir $odir

lens="8" #32 256 2048"
sweep_lens="2 4 8 16 32 64 128 256 512 1024 2048 4096"
range="500000"

for i in `seq 1 ${numruns}`; do
  for lenA in ${lens}; do
    for lenB in ${sweep_lens}; do
      ${EMPTY_HEADED_HOME}/bin/intersection_benchmark_and_test ${lenA} ${lenB} ${range} ${range} | tee ${odir}/${lenA}_${lenB}_${i}.log
    done
  done
done
