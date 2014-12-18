#!/bin/bash

PORT=5477

if [ "${3}" -eq "1" ]; then
  for i in `seq 1 ${1}`; do
    if [ -f $1/attributes.txt ]; then
       psql -p ${PORT} -v attributes="${2}/attributes.txt" -v edgelist="${2}/edgelist/data.txt" -f load_directed_sel.sql
       for app in "path_counting"; do
          echo "----- ${app} -----"
          psql -p ${PORT} -v dataset=${2} -f ${app}_sel.sql
       done

       psql -p ${PORT} -v attributes="${2}/attributes.txt" -v edgelist="${2}/glab_undirected/data.txt" -f load_undirected_sel.sql
       for app in "triangle_counting" "clique_counting"; do
          echo "----- ${app} -----"
          psql -p ${PORT} -f ${app}_sel.sql
       done
    else
       psql -p ${PORT} -v dataset="${2}/edgelist/data.txt" -f load_directed.sql
       for app in "num_nbrs" "path_counting"; do
          echo "----- ${app} -----"
          psql -p ${PORT} -v dataset=${2} -v start_node=222074 -f ${app}.sql
       done

       psql -p ${PORT} -v dataset="${2}/glab_undirected/data.txt" -f load_undirected.sql
       for app in "triangle_counting" "clique_counting"; do
          echo "----- ${app} -----"
          psql -p ${PORT} -v dataset=${2} -f ${app}.sql
       done
    fi
  done
else
  echo "Only single threaded execution supported"
fi
