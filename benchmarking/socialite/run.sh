#!/bin/bash

if [ -f $2/attributes.txt ]; then
  echo "Socalite selections"
else
  echo "Vanilla Socalite"
  # ${SOCIALITE_HOME}/bin/socialite -t$3 all_apps_undirected.py $2 $1 triangle_counting
  ${SOCIALITE_HOME}/bin/socialite -t$3 all_apps_undirected.py $2 $1 clique_counting
  # ${SOCIALITE_HOME}/bin/socialite -t$3 all_apps_undirected.py $2 $1 lollipop_counting
  killall -9 java
  sleep 5
fi
