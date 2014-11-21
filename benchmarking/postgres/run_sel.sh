#!/bin/bash

psql -p 5433 -v attributes=${1}/attributes.txt -v edgelist=${1}/edgelist.txt -f load_attr.sql
for app in "triangle_counting"; do
   psql -p 5433 -v attributes=${1}/attributes.txt -v edgelist=${1}/edgelist.txt -f ${app}_sel.sql
done
