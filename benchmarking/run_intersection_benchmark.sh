#!/bin/bash

source env.sh

numruns="5"
curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"

odir="${output}/intersection_benchmark_${date}_${curtime}"
mkdir $odir

densities="0.00002 0.00004 0.00008 0.00016 0.00032 0.00128 0.00256 0.00512 0.01024 0.02048 0.04096 0.16384"
lens="2 4 8 16 32 64 128 256 512 1024 2048"

for i in `seq 0 ${numruns}`; do
  for density in ${densities}; do
    for gap_len in ${gap_lens}; do
      ${EMPTY_HEADED_HOME}/intersection_benchmark ${n} ${density} ${gap_len} | tee ${odir}/${density}_${gap_len}_${i}.log
    done
  done
done

