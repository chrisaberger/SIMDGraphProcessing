#!/bin/bash

${SOCIALITE_HOME}/bin/socialite -t$2 all_apps_directed.py $1 | tee all_apps_directed.$2.log | grep -w "time"
${SOCIALITE_HOME}/bin/socialite -t$2 all_apps_undirected.py $1 | tee all_apps_undirected.$2.log | grep -w "time"
