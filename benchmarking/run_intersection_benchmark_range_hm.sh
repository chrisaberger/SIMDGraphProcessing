#!/bin/bash

source env.sh

numruns="3"
curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"

odir="${output}/intersection_benchmark_${date}_${curtime}"
mkdir $odir

ranges="10000 20000 40000 80000 160000 320000 640000 1280000"

len="1024"

for i in `seq 1 ${numruns}`; do
  for rangeA in ${ranges}; do
    for rangeB in ${ranges}; do
      ${EMPTY_HEADED_HOME}/bin/intersection_benchmark_and_test ${len} ${len} ${rangeA} ${rangeB} | tee ${odir}/${rangeA}_${rangeB}_${i}.log
    done
  done
done
