#!/bin/bash

for i in `seq 1 ${1}`; do
  ${EMPTY_HEADED_HOME}/bin/undirected_triangle_counting --graph=$2/bin/u_degree.bin --t=$3 --input_type=binary --layout=hybrid
  #${EMPTY_HEADED_HOME}/n_path --graph=$2/bin/d_data.bin --t=$3 --input_type=binary --layout=hybrid
done
