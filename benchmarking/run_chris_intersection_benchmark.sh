#!/bin/bash

source env.sh

numruns="4"
curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"

odir="${output}/intersection_benchmark_${date}_${curtime}"
mkdir $odir

skews="0.00002 0.00004 0.00008 0.00016 0.00032 0.00128 0.00256 0.00512 0.01024 0.02048"
lenA="10000000"
lenB="100000"

range="1000000000000000"

for i in `seq 0 ${numruns}`; do
  for skew in ${skews}; do
    ${EMPTY_HEADED_HOME}/bin/intersection_benchmark_and_test ${range} ${range} ${lenA} ${lenB} ${skew} ${skew} | tee ${odir}/skew_${skews}_${i}.log
  done
done

