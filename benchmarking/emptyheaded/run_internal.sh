#!/bin/bash
EMPTY_HEADED_HOME="/dfs/scratch0/caberger/systems/EmptyHeaded"

for i in `seq 1 ${1}`; do
  echo "COMMAND: ${EMPTY_HEADED_HOME}/stable_binaries/$2 $3 $4 $5"
  ${EMPTY_HEADED_HOME}/stable_binaries/$2 $3 $4 $5
done