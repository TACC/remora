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
#-      version     REMORA 1.4
#-      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#-                  Antonio Gomez  (agomez@tacc.utexas.edu)
#-      license     MIT
#
#========================================================================
#  HISTORY
#       2015/08/12: Initial version
#       2015/12/08: Version 1.4. Modular design.
#========================================================================

#Initialize variables specific to certain modules here
REMORA_NODE=$1
REMORA_OUTDIR=$2
REMORA_PERIOD=$3
REMORA_SYMMETRIC=$4
REMORA_MODE=$5
REMORA_CUDA=$6
REMORA_PARALLEL=$7
REMORA_VERBOSE=$8
REMORA_BIN=$9


#Source the script that has the modules' functionality
source $REMORA_BIN/modules/modules_utils

#Read the list of active modules from the configuration file
remora_read_active_modules

#Configure the modules (they might not need it)
remora_configure_modules $REMORA_NODE $REMORA_OUTDIR

while [ 1 ]
do
    remora_execute_modules $REMORA_NODE $REMORA_OUTDIR
    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "sleep $REMORA_PERIOD"
    fi
    sleep $REMORA_PERIOD
done
