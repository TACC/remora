#!/bin/bash

# DO NOT call this script directly. This is called by REMORA. 
#
#========================================================================
# IMPLEMENTATION
#      version     REMORA 1.8.5
#      authors     Carlos Rosales ( carlos@tacc.utexas.edu)
#                  Antonio Gomez  ( agomez@tacc.utexas.edu)
#      custodian   Kent Milfeld   (milfeld@tacc.utexas.edu)
#      license     MIT
#========================================================================



#This function monitors the memory available on one node
#If the memory goes below REMORA_FREEMEM (0.5GB by default), 
#it kills all the user's processes until this process is killed.

if [[ -z "$REMORA_FREEMEM" ]]; then
    REMORA_FREEMEM=500000 #0.5 GB
fi  
while true; do
    memfree=`grep /proc/meminfo -e MemFree | awk '{print $2;}'`
    if [[ $memfree -le $REMORA_FREEMEM ]]; then
        PIDS=`ps aux | grep $USER | sort -k 2,2 -r | awk '{print $2;}'`
        for PID in $PIDS; do
            kill -9 $PID
        done
        break
    fi  
    sleep 0.01
done
