#!/bin/bash

PORT=5477
TIMEOUT=1800

if [ "${3}" -eq "1" ]; then
  for i in `seq 1 ${1}`; do
    if [ -f $1/attributes.txt ]; then
      echo "Currently no queries with attributes"
    else
       for app in "clique_counting" "lollipop_counting"; do
          ./launch_server.sh
          sleep 30
          echo "----- ${app} -----"
          psql -p ${PORT} -v dataset="${2}/glab_undirected/data.txt" -f load_undirected.sql
          timeout $TIMEOUT psql -p ${PORT} -v dataset=${2} -f ${app}.sql
          killall -u noetzli -9 postgres
          sleep 20
       done
    fi
  done
else
  echo "Only single threaded execution supported"
fi
