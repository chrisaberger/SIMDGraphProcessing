#!/bin/bash

source env.sh

numruns="7"
curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"

odir="${output}/skew_intersection_benchmark_${date}_${curtime}"
mkdir $odir

# Skew experiment
# skews="0.0 0.0001 0.0002 0.0004 0.0008 0.0016 0.0032 0.0064 0.0128 0.0256 0.0512 0.1024"
# lens="512 1024 2048 4096 8192"
# ranges="1000000"

# New skew experiment
# skews="0.00625 0.0125 0.025 0.05 0.1 0.2 0.4 0.8"
# lens="2048"
# ranges="1000000"

skews="0.0"
lens="16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072"
ranges="5000000"

# Range on uint experiment
# skews="0.0"
# lens="2048"
# ranges="1280000 640000 320000 160000 80000 40000 20000 10000"

for i in `seq 1 ${numruns}`; do
  for len in ${lens}; do
    for skew in ${skews}; do
      for range in ${ranges}; do
        ${EMPTY_HEADED_HOME}/stable_binaries/intersection_benchmark_and_test ${range} ${range} ${len} ${len} ${skew} ${skew} | tee ${odir}/${range}_${len}_${skew}_${i}.log
       done
    done
  done
done
