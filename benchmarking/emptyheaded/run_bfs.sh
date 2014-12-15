#!/bin/bash

for i in `seq 1 ${1}`; do
  echo "COMMAND: ${EMPTY_HEADED_HOME}/stable_binaries/$2 --graph=$3 --input_type=binary --t=$4 --layout=$5"
  ${EMPTY_HEADED_HOME}/stable_binaries/$2 --graph=$3 --input_type=binary --t=$4 --layout=$5
done
