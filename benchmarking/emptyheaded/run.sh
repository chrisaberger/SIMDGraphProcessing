#!/bin/bash

for i in `seq 1 ${1}`; do
  #timeout 8000 ${EMPTY_HEADED_HOME}/stable_binaries/n_clique --graph=$2/bin/u_degree.bin --t=$3 --input_type=binary --layout=hybrid
  timeout 8000 ${EMPTY_HEADED_HOME}/stable_binaries/undirected_lollipop_counting --graph=$2/bin/u_degree.bin --t=$3 --input_type=binary --layout=hybrid
done
