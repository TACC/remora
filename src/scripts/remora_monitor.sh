#!/bin/sh
#
#========================================================================
# HEADER
#========================================================================
#% DESCRIPTION
#% remora_report
#%
#% DO NOT call this script directory. This is called by REMORA
#%
#% remora_report.sh NODE_NAME OUTDIR REMORA_PERIOD SYMMETRIC REMORA_MODE REMORA_CUDA
#========================================================================
#- IMPLEMENTATION
#-      version     REMORA 1.5
#-      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#-                  Antonio Gomez  (agomez@tacc.utexas.edu)
#-      license     MIT
#
#========================================================================
#  HISTORY
#       2015/08/12: Initial version
#       2015/12/08: Version 1.4. Modular design.
#		2016/01/24: Version 1.5. Separate dir for tmp files.
#========================================================================

#Initialize variables specific to certain modules here
REMORA_NODE=$1
REMORA_BIN=$2
REMORA_OUTDIR=$3
source $REMORA_OUTDIR/remora_env.txt

#Source the script that has the modules' functionality
source $REMORA_BIN/modules/modules_utils

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
