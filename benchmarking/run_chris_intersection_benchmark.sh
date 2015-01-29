#!/bin/bash

source env.sh

numruns="5"
curtime="$(date +'%H-%M-%S')"
date="$(date +'%d-%m-%Y')"

odir="${output}/intersection_benchmark_${date}_${curtime}"
mkdir $odir

n="100000000"
densitiesA="0.00002 0.00004 0.00008 0.00016 0.00032 0.00128 0.00256 0.00512 0.01024 0.02048 0.04096 0.16384"
densitiesB="0.00002 0.00004 0.00008 0.00016 0.00032 0.00128 0.00256 0.00512 0.01024 0.02048 0.04096 0.16384"

for i in `seq 0 ${numruns}`; do
  for densityA in ${densitiesA}; do
    for densityB in ${densitiesB}; do
        valid=$(echo "$densityA <= $densityB" | bc)
        if [ $valid == "1" ]; then
          ${EMPTY_HEADED_HOME}/bin/intersection_benchmark_and_test ${n} ${densityA} ${densityB} | tee ${odir}/${densityA}_${densityB}_${i}.log
        fi
    done
  done
done

