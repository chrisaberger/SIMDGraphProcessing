#!/bin/bash

if [ -f $1/attributes.txt ]; then
   ./selections_undirected.sh $1
   ./selections_directed.sh $1
else
   ./directed.sh $1
   ./undirected.sh $1
fi

