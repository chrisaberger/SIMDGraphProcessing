#!/bin/bash

TIMEOUT=9000

for i in `seq 1 ${1}`; do
  if [ -f $1/attributes.txt ]; then
    echo "Currently no queries with attributes"
  else
     ln -f -s "${2}/glab_undirected/data.txt" input.txt
     for app in "cliqueCounting" "cycleCounting" "lollipopCounting"; do
        echo "----- ${app} -----"
        ./launch_server.sh
        timeout $TIMEOUT lb unit --sequential --threads $3 --time --test ${app}.lb
        ./stop_server.sh
     done
  fi
done
