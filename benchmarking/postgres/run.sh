#!/bin/bash

psql -p 5433 -v dataset="${1}" -f load_directed.sql
for app in "num_nbrs" "path_counting"; do
   echo "----- ${app} -----"
   psql -p 5433 -v dataset=${1} -f ${app}.sql
done

psql -p 5433 -v dataset="${1}" -f load_undirected.sql
for app in "triangle_counting" "cycle_counting" "clique_counting"; do
   echo "----- ${app} -----"
   psql -p 5433 -v dataset=${1} -f ${app}.sql
done
