#!/bin/bash
EMPTY_HEADED_HOME="/dfs/scratch0/caberger/systems/benchmarking_eh/EmptyHeaded"

echo $1
for i in `seq 1 ${1}`; do
  echo "COMMAND: ${EMPTY_HEADED_HOME}/stable_binaries/$2 --graph=$3 --t=$4 --layout=$5 --input_type=binary"
  ${EMPTY_HEADED_HOME}/stable_binaries/$2 --graph=$3 --t=$4 --layout=$5 --input_type=binary
done