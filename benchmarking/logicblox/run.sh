#!/bin/bash

TIMEOUT=1800

for i in `seq 1 ${1}`; do
  if [ -f $1/attributes.txt ]; then
    echo "Currently no queries with attributes"
  else
     export input_file="${2}/glab_undirected/data.txt"
     envsubst < load_data.template > load_data.lb

     lb create --overwrite benchmarking
     lb addblock -f add_edges.lb benchmarking
     lb exec -f load_data.lb benchmarking

     # for app in "cliqueCounting" "cycleCounting" "lollipopCounting"; do
     for app in "triangleCounting" "cliqueCounting" "lollipopCounting"; do #"similarity" "symbiosity"; do
        echo "----- ${app} -----"
        time timeout $TIMEOUT lb exec --readonly --commit-mode softcommit -f ${app}.lb --print _count benchmarking

        tid=`lb status --active --debug benchmarking | awk '/request_id/ {print $2;}'`

        if [[ $tid ]]; then
          lb aborttransaction benchmarking $tid
        fi
     done

     lb delete benchmarking
  fi
done
