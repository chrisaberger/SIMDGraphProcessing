#!/bin/bash

${VERTICA_HOME}/bin/vsql -v num_threads=$2 -v file=\'$1\' -d benchmarking -f directed.sql dbadmin
${VERTICA_HOME}/bin/vsql -v num_threads=$2 -v file=\'$1\' -d benchmarking -f undirected.sql dbadmin
