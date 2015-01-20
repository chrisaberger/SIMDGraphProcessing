#!/bin/bash
PGX_HOME="/afs/cs.stanford.edu/u/caberger/pgx-release-0.8.1"

python write_threads.py $3

source ${PGX_HOME}/set_env.sh
export JAVA_OPTS="$JAVA_OPTS -Xmx512g"

for i in `seq 1 ${1}`; do
  echo "COMMAND: java -cp ${PGX_HOME}/lib/*:${PGX_HOME}/third-party/*:${PGX_HOME}/classes demos.UndirectedTriangleCounting $2/pgx/sample.edge.json"
  java -Dpgx.server.config.filename=${PGX_HOME}/server.config -cp ${PGX_HOME}/lib/*:${PGX_HOME}/third-party/*:${PGX_HOME}/classes demos.UndirectedTriangleCounting $2/pgx/sample.edge.json
done