#!/bin/bash

if [ -f $1/attributes.txt ]; then
   #${VERTICA_HOME}/bin/vsql -v num_threads=$2 -v attributes=\'$1/attributes.txt\' -v edgelist=\'$1/glab_undirected/data.txt\' -d benchmarking -f selections_undirected.sql dbadmin
   ${VERTICA_HOME}/bin/vsql -v num_threads=$2 -v attributes=\'$1/attributes.txt\' -v edgelist=\'$1/edgelist/data.txt\' -d benchmarking -f selections_directed.sql dbadmin
else
   ${VERTICA_HOME}/bin/vsql -v num_threads=$2 -v file=\'$1/glab_undirected/data.txt\' -d benchmarking -f undirected.sql dbadmin
   ${VERTICA_HOME}/bin/vsql -v num_threads=$2 -v file=\'$1/edgelist/data.txt\' -d benchmarking -f directed.sql dbadmin
fi

