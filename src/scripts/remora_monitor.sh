#!/bin/sh
#
#========================================================================
# HEADER
#========================================================================
#% DESCRIPTION
#% remora_monitor
#%
#% DO NOT call this script directly. This is called by REMORA. 
#% This script starts real-time REMORA data collection and processing.
#%
#% remora_monitor.sh $NODE $REMORA_BIN $REMORA_OUTDIR
#========================================================================
#- IMPLEMENTATION
#-      version     REMORA 1.7
#-      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#-                  Antonio Gomez  (agomez@tacc.utexas.edu)
#-      license     MIT
#========================================================================

# --- Real-time data monitoring

#Initialize variables specific to certain modules here
REMORA_NODE=$1
REMORA_BIN=$2
REMORA_OUTDIR=$3
source $REMORA_OUTDIR/remora_env.txt

#Source the script that has the modules' functionality
source $REMORA_BIN/aux/extra

#Read the list of active modules from the configuration file
remora_read_active_modules

#Configure the modules (they might not need it)
remora_monitor_summary $REMORA_NODE $REMORA_OUTDIR $REMORA_TMPDIR 

while [ 1 ]; do
	if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "sleep $REMORA_MONITOR_PERIOD"
    fi
    remora_monitor_modules $REMORA_NODE $REMORA_OUTDIR $REMORA_TMPDIR
	remora_monitor_summary $REMORA_NODE $REMORA_OUTDIR $REMORA_TMPDIR 
    sleep $REMORA_MONITOR_PERIOD
done
