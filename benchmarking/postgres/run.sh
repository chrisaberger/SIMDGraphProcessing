#!/bin/bash

PORT=5477

if [ -f $1/attributes.txt ]; then
   psql -p ${PORT} -v attributes="${1}/attributes.txt" -v edgelist="${1}/edgelist/data.txt" -f load_directed_sel.sql
   for app in "path_counting"; do
      echo "----- ${app} -----"
      psql -p ${PORT} -v dataset=${1} -f ${app}_sel.sql
   done

   psql -p ${PORT} -v attributes="${1}/attributes.txt" -v edgelist="${1}/glab_undirected/data.txt" -f load_undirected_sel.sql
   for app in "triangle_counting" "cycle_counting" "clique_counting"; do
      echo "----- ${app} -----"
      psql -p ${PORT} -f ${app}_sel.sql
   done
else
   psql -p ${PORT} -v dataset="${1}/edgelist/data.txt" -f load_directed.sql
   for app in "num_nbrs" "path_counting"; do
      echo "----- ${app} -----"
      psql -p ${PORT} -v dataset=${1} -f ${app}.sql
   done

   psql -p ${PORT} -v dataset="${1}/glab_undirected/data.txt" -f load_undirected.sql
   for app in "triangle_counting" "cycle_counting" "clique_counting"; do
      echo "----- ${app} -----"
      psql -p ${PORT} -v dataset=${1} -f ${app}.sql
   done
fi


