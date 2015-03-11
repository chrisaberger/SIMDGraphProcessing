#!/bin/bash

export LB_DEPLOYMENT_HOME=/lfs/local/0/noetzli/lb/lb_deployment
export LB_WEBSERVER_HOME=/afs/cs.stanford.edu/u/noetzli/share/logicblox-4.1.6/share/lb_web/
export LB_LOGDIR=/lfs/local/0/noetzli/lb/logs
export LB_MEM=400G
export LB_CONNECTBLOX_ENABLE_ADMIN=1

lb services start
