#!/bin/bash
SNAPR="/afs/cs.stanford.edu/u/caberger/snapr"

for i in `seq 1 ${1}`; do
  echo "${SNAPR}/examples/tcount/centrality -i:$2/edgelist/data.txt $3"
  ${SNAPR}/examples/tcount/centrality -i:$2/edgelist/data.txt $3
done