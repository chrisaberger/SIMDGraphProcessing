#!/bin/bash

for i in `seq 1 ${1}`; do
  echo "COMMAND: ${EMPTY_HEADED_HOME}/bin/$2 --graph=$3 --t=$4 --layout=$5 --input_type=binary"
  ${EMPTY_HEADED_HOME}/bin/$2 --graph=$3 --t=$4 --layout=$5 --input_type=binary
done
