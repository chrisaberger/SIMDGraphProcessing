#!/bin/bash

source env.sh

numruns="0"
curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"

odir="${output}/new_intersection_benchmark_${date}_${curtime}"
mkdir $odir

skews="0.02048" # 0.00032 0.02048" #"0.0 0.00002 0.00032 0.02048"
lens="8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144"
ranges="1000000"

for i in `seq 0 ${numruns}`; do
  for len in ${lens}; do
    for skew in ${skews}; do
      for range in ${ranges}; do
        ${EMPTY_HEADED_HOME}/stable_binaries/intersection_benchmark_and_test ${range} ${range} 16 ${len} 0.0 ${skew} | tee ${odir}/${range}_${len}_${skew}_${i}.log
       done
    done
  done
done
