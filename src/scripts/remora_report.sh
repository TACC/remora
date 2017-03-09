#!/bin/sh
#
#========================================================================
# HEADER
#========================================================================
#% DESCRIPTION
#% remora_report
#%
#% DO NOT call this script directory. This is called by REMORA.
#% This script launches the data collection function for all active 
#% modules
#%
#% remora_report.sh NODE_NAME REMORA_BIN OUTDIR REMORA_PERIOD SYMMETRIC REMORA_MODE REMORA_CUDA
#========================================================================
#- IMPLEMENTATION
#-      version     REMORA 1.8
#-      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#-                  Antonio Gomez  (agomez@tacc.utexas.edu)
#-      license     MIT
#========================================================================

#Initialize variables specific to certain modules here
REMORA_NODE=$1
REMORA_BIN=$2
REMORA_OUTDIR=$3
source $REMORA_OUTDIR/remora_env.txt

#Source the script that has the modules' functionality
source $REMORA_BIN/aux/extra
source $REMORA_BIN/modules/modules_utils


# Create TMPDIR if it si not there
mkdir -p $REMORA_TMPDIR
# Generate unique file for transfer completion check 
# on distributed file systems
for node in $NODES; do
    touch $REMORA_TMPDIR/zz.$node
done

#Read the list of active modules from the configuration file
remora_read_active_modules

#Configure the modules (they might not need it)
remora_configure_modules $REMORA_NODE $REMORA_OUTDIR $REMORA_TMPDIR

# Tag end of file so taht we know when it is OK to source / read
# from a different shell
echo "#EOF" >> $REMORA_OUTDIR/remora_env.txt

while [ 1 ]; do
    remora_execute_modules $REMORA_NODE $REMORA_OUTDIR $REMORA_TMPDIR "${REMORA_MODULES[@]}"
    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "sleep $REMORA_PERIOD"
    fi
    sleep $REMORA_EFFECTIVE_PERIOD
done
