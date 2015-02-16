#!/bin/bash

mkdir -p /dev/shm/postgresql/data
postgres -D /dfs/scratch0/noetzli/postgres/ >/dfs/scratch0/noetzli/postgres/logfile 2>&1 &
