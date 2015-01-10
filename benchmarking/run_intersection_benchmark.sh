#!/bin/bash

source env.sh

numruns="5"
curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"

odir="${output}/intersection_benchmark_${date}_${curtime}"
mkdir $odir

n="100000"
lens="2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768"

for i in `seq 0 ${numruns}`; do
  for run_len in ${lens}; do
    for gap_len in ${lens}; do
      ${EMPTY_HEADED_HOME}/intersection_benchmark ${n} ${run_len} ${gap_len} | tee ${odir}/${run_len}_${gap_len}_${i}.log
    done
  done
done

