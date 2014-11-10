#!/bin/bash

for app in "triangle_counting"; do
   psql -h $3 -p 5432 -U amirabs -v dataset="${1}" -f load.sql andres
   psql -h $3 -p 5432 -U amirabs -v dataset=${1} -f triangle_counting.sql andres
done
