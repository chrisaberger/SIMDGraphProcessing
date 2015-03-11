#!/bin/bash

if [ -f $1/attributes.txt ]; then
  echo "Attributes currently not supported"
else
  for app in "clique" "lollipop"; do #"triangle" "clique" "lollipop"; do
    /opt/vertica/bin/admintools -t start_db -d benchmarking
    echo NUM_THREADS ${3}
    ${VERTICA_HOME}/bin/vsql -v num_threads=$3 -v file=\'$2/glab_undirected/data.txt\' -d benchmarking -f load.sql noetzli
    echo "------------ ${app} ---------"
    timeout 1800 ${VERTICA_HOME}/bin/vsql -v num_threads=$3 -v file=\'$2/glab_undirected/data.txt\' -d benchmarking -f ${app}.sql noetzli
    /opt/vertica/bin/admintools -t stop_db -d benchmarking --force
  done
fi

