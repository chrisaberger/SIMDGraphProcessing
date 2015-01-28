#!/bin/bash

PORT=5477

if [ "${3}" -eq "1" ]; then
  for i in `seq 1 ${1}`; do
    if [ -f $1/attributes.txt ]; then
      echo "Currently no queries with attributes"
    else
       psql -p ${PORT} -v dataset="${2}/glab_undirected/data.txt" -f load_undirected.sql
       for app in "tadpole_counting"; do
          echo "----- ${app} -----"
          psql -p ${PORT} -v dataset=${2} -f ${app}.sql
       done
    fi
  done
else
  echo "Only single threaded execution supported"
fi
