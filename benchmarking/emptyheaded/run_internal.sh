#!/bin/bash
EMPTY_HEADED_HOME="/afs/cs.stanford.edu/u/caberger/SpaceGraph"

for i in `seq 1 ${1}`; do
  echo "COMMAND: ${EMPTY_HEADED_HOME}/stable_binaries/$2 $3 $4 $5"
  ${EMPTY_HEADED_HOME}/stable_binaries/$2 $3 $4 $5
done