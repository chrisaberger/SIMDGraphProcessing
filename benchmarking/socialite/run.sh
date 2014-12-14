#!/bin/bash

snode=`cat ${2}/largest_degree_external_id.txt`

if [ -f $2/attributes.txt ]; then
  echo "Socalite selections"
  ${SOCIALITE_HOME}/bin/socialite -t$3 selections_directed.py $2 $1 $snode
  ${SOCIALITE_HOME}/bin/socialite -t$3 selections_undirected.py $2 $1
else
  echo "Vanilla Socalite"
  ${SOCIALITE_HOME}/bin/socialite -t$3 all_apps_directed.py $2 $1 $snode
  ${SOCIALITE_HOME}/bin/socialite -t$3 all_apps_undirected.py $2 $1
fi
