#!/bin/bash

snode=`cat ${2}/largest_degree_external_id.txt`

for i in `seq 1 ${1}`; do
  echo "COMMAND:   ${GALOIS_HOME}/build/release/apps/bfs/bfs -t $3 -startNode=$snode $4/galois/data.bin"
  ${GALOIS_HOME}/build/release/apps/bfs/bfs -t $3 -startNode=$snode $4/galois/data.bin
done
