#!/bin/bash

${SOCIALITE_HOME}/bin/socialite -t$2 all_apps.py $1 | tee all_apps.$2.log | grep -w "time"
