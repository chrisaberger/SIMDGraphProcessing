#!/bin/bash

source env.sh

numruns="7"
curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"

odir="${output}/intersection_benchmark_${date}_${curtime}"
mkdir $odir

#ranges="5000000"
#sweep_ranges="100000 200000 400000 800000 1600000 320000 640000 1280000 2560000 5120000 10240000 20480000"

#lens="100"
#sweep_lens="2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768"

ranges="5000000"
sweep_ranges="3125 6250 12500 25000 50000 100000 200000 400000 800000 1600000 320000"

lens="512"
sweep_lens="65536" # 131072 262144"

for i in `seq 1 ${numruns}`; do
  for range in ${ranges}; do
    for len in ${sweep_lens}; do
      ${EMPTY_HEADED_HOME}/bin/intersection_benchmark_and_test ${len} ${len} ${range} ${range} | tee ${odir}/lens_${len}_${range}_${i}.log
    done
  done

  for len in ${lens}; do
    for range in ${sweep_ranges}; do
      ${EMPTY_HEADED_HOME}/bin/intersection_benchmark_and_test ${len} ${len} ${range} ${range} | tee ${odir}/range_${len}_${range}_${i}.log
    done
  done
done
