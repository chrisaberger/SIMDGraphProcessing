#!/bin/bash

psql -p 5433 -v dataset="${1}" -f load_attr.sql
for app in "triangle_counting"; do
   psql -p 5433 -v attr_dataset=${1}/attributes.txt -v edge_dataset=${1}/edgelist.txt -f ${app}_sel.sql
done
