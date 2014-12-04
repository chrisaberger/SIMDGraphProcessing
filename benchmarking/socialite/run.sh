#!/bin/bash

if [ -f $1/attributes.txt ]; then
   ${SOCIALITE_HOME}/bin/socialite -t$2 selections_directed.py $1
   ${SOCIALITE_HOME}/bin/socialite -t$2 selections_undirected.py $1
else
   ${SOCIALITE_HOME}/bin/socialite -t$2 all_apps_directed.py $1
   ${SOCIALITE_HOME}/bin/socialite -t$2 all_apps_undirected.py $1
fi
