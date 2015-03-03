#!/bin/bash

TIMEOUT=1800

if [ -f $1/attributes.txt ]; then
  echo "Queries with attributes are currently not supported"
else
  # Replace the ${input_file} variable in the data loading template with the
  # actual input file name.
  load_data_file=`tempfile`
  export input_file="${1}/glab_undirected/data.txt"
  envsubst < load_data.template > ${load_data_file}

  # Load data
  lb create --overwrite benchmarking
  lb addblock -f add_edges.lb benchmarking
  lb exec -f ${load_data_file} benchmarking

  # Run queries
  for app in "triangleCounting" "cliqueCounting" "lollipopCounting"; do #"similarity" "symbiosity"; do
    echo "----- ${app} -----"
    time timeout $TIMEOUT lb exec --readonly --commit-mode softcommit -f ${app}.lb --print _count benchmarking

    # Check if the transaction is still running (because of a timeout) and
    # abort it.
    tid=`lb status --active --debug benchmarking | awk '/request_id/ {print $2;}'`
    if [[ $tid ]]; then
      lb aborttransaction benchmarking $tid
    fi
  done

  # Delete the workspace
  lb delete benchmarking
fi
