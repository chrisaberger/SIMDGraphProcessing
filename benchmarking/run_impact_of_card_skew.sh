#!/bin/bash

source env.sh

numruns="7"
curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"

odir="/lfs/local/0/noetzli/card_skew_${date}_${curtime}"
mkdir $odir

# Skew experiment
# skews="0.0 0.0001 0.0002 0.0004 0.0008 0.0016 0.0032 0.0064 0.0128 0.0256 0.0512 0.1024"
# lens="512 1024 2048 4096 8192"
# ranges="1000000"

skews="0.0"
lens="64 128 256 512 1024 2048 4096 8192 16384 32768"
ranges="1000000"

for i in `seq 1 ${numruns}`; do
  for len in ${lens}; do
    for skew in ${skews}; do
      for range in ${ranges}; do
        echo ${EMPTY_HEADED_HOME}/stable_binaries/intersection_benchmark_and_test ${range} ${range} 64 ${len} ${skew} ${skew}
        ${EMPTY_HEADED_HOME}/stable_binaries/intersection_benchmark_and_test ${range} ${range} 64 ${len} ${skew} ${skew} | tee ${odir}/${range}_${len}_${skew}_${i}.log
       done
    done
  done
done
