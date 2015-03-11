#!/bin/bash

export LB_DEPLOYMENT_HOME=/lfs/local/0/noetzli/lb/lb_deployment
export LB_WEBSERVER_HOME=/afs/cs.stanford.edu/u/noetzli/share/logicblox-4.1.6/share/lb_web/
export LB_LOGDIR=/lfs/local/0/noetzli/lb/logs
export LB_MEM=400G
export LB_CONNECTBLOX_ENABLE_ADMIN=1

TIMEOUT=9999

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
  lb exec -f ${load_data_file} --commit-mode softcommit benchmarking

  # Run queries
  for app in "triangleCounting"; do #"cliqueCounting" "lollipopCounting"; do #"triangleCounting" "cliqueCounting" "lollipopCounting"; do #"similarity" "symbiosity"; do
    echo "----- ${app} -----"
    time timeout $TIMEOUT lb exec --readonly --commit-mode softcommit -f ${app}.lb --print _count benchmarking

    # Check if the transaction is still running (because of a timeout) and
    # abort it.
    tid=`lb status --active --debug benchmarking | awk '/request_id/ {print $2;}'`
    if [[ $tid ]]; then
      echo "Aborting $tid"
      lb aborttransaction benchmarking $tid
    fi
    sleep 120
  done

  # Delete the workspace
  lb services restart
  sleep 120
  lb services restart
  sleep 120
fi
