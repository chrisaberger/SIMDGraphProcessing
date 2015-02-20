#!/bin/bash

source env.sh

numruns="4"
curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"

output="/dfs/scratch0/caberger/output/"
odir="${output}/intersection_benchmark_${date}_${curtime}"
mkdir $odir

skews="0.00002 0.00004 0.00008 0.00016 0.00032 0.00128 0.00256 0.00512 0.01024 0.02048"
lensA="20 40 80 160 320 1280 2560 5120 10240 20480 40960 163840 327680 655360 1310720"
lensB="20 40 80 160 320 1280 2560 5120 10240 20480 40960 163840 327680 655360 1310720"

range="10000000"

for i in `seq 0 ${numruns}`; do
  for skew in ${skews}; do
    for lenA in ${lensA}; do
      for lenB in ${lensB}; do
        echo "${EMPTY_HEADED_HOME}/stable_binaries/intersection_benchmark_and_test ${range} ${range} ${lenA} ${lenB} ${skew} ${skew} | tee ${odir}/${skews}.${lenA}.${lenB}.${range}.${i}.log"
        ${EMPTY_HEADED_HOME}/stable_binaries/intersection_benchmark_and_test ${range} ${range} ${lenA} ${lenB} ${skew} ${skew} | tee ${odir}/${skew}.${lenA}.${lenB}.${range}.${i}.log
      done
    done
  done
done

