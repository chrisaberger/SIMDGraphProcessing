#!/bin/bash

mkdir -p /dev/shm/noetzli/postgresql/data
postgres -D /dfs/scratch0/noetzli/postgres/ >/dfs/scratch0/noetzli/postgres/logfile 2>&1 &
