#!/bin/bash

psql -p 5433 -v dataset="${1}" -f load.sql
for app in "triangle_counting" "cycle_counting" "clique_counting"; do
   psql -p 5433 -v dataset=${1} -f ${app}.sql
done
