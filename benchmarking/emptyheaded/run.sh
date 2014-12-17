#!/bin/bash

#${EMPTY_HEADED_HOME}/undirected_triangle_counting $1/bin/u_degree.bin $2 hybrid
for i in `seq 1 ${1}`; do
  ${EMPTY_HEADED_HOME}/n_path --graph=$2/bin/d_data.bin --t=$3 --input_type=binary --layout=uint
done
