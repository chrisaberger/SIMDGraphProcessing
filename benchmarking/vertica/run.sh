#!/bin/bash

if [ -f $1/attributes.txt ]; then
  echo "Attributes currently not supported"
else
  for i in `seq 1 ${1}`; do
    echo "triangle"
    timeout 9000 ${VERTICA_HOME}/bin/vsql -v num_threads=$3 -v file=\'$2/glab_undirected/data.txt\' -d benchmarking -f triangle.sql noetzli
    echo "cycle"
    timeout 9000 ${VERTICA_HOME}/bin/vsql -v num_threads=$3 -v file=\'$2/glab_undirected/data.txt\' -d benchmarking -f cycle.sql noetzli
    echo "clique"
    timeout 9000 ${VERTICA_HOME}/bin/vsql -v num_threads=$3 -v file=\'$2/glab_undirected/data.txt\' -d benchmarking -f clique.sql noetzli
    echo "lollipop"
    timeout 9000 ${VERTICA_HOME}/bin/vsql -v num_threads=$3 -v file=\'$2/glab_undirected/data.txt\' -d benchmarking -f lollipop.sql noetzli
  done
fi

