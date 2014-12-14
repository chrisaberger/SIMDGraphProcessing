#!/bin/bash

#${EMPTY_HEADED_HOME}/undirected_triangle_counting $1/bin/u_degree.bin $2 hybrid
for i in `seq 1 ${1}`; do
  ${EMPTY_HEADED_HOME}/bfs $2/bin/d_data.bin $3 hybrid
done
