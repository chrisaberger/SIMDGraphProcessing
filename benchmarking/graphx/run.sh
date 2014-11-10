#!/bin/bash

sbt package
for app in "TriangleCounting" "CliqueCounting"; do
   echo -e $app\\t`${SPARK_HOME}/bin/spark-submit --class "${app}" --master local[$2] --driver-memory 100g  target/scala-2.10/graphx-clique-counting_2.10-1.0.jar $1 | grep "Time" | sed -n -e 's/^Time:\s\+\([0-9\.]*\).*/\1/p'`
done
