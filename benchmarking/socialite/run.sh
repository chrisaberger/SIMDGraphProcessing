#!/bin/bash

for app in "triangle_counting" "clique_counting" "cycle_counting"; do
   ${SOCIALITE_HOME}/bin/socialite -t$2 ${app}.py $1 | grep "Time" | sed -n -e 's/^Time:\s\+\([0-9\.]*\).*/\1/p'
done
