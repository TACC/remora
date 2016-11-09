#!/bin/sh
#
#========================================================================
# HEADER
#========================================================================
#% DESCRIPTION
#% remora_remote_post
#%
#% DO NOT call this script directory. This is called by REMORA.
#% This script launches the finalize function for all active modules.
#%
#% remora_remote_post.sh $NODE $REMORA_OUTDIR $REMORA_BIN $REMORA_VERBOSE $REMORA_NODE_ID
#========================================================================
#- IMPLEMENTATION
#-      version     REMORA 1.7
#-      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#-                  Antonio Gomez  (agomez@tacc.utexas.edu)
#-      license     MIT
#========================================================================

#Initialize variables specific to certain modules here
REMORA_NODE=$1
REMORA_OUTDIR=$2
REMORA_BIN=$3
REMORA_VERBOSE=$4
REMORA_NODE_ID=$5

#Source the script that has the modules' functionality
if [ "$REMORA_VERBOSE" == "1" ]; then
  echo "source $REMORA_BIN/aux/extra"
  echo "source $REMORA_BIN/modules/modules_utils"
fi
source $REMORA_BIN/aux/extra
source $REMORA_BIN/modules/modules_utils
source $REMORA_OUTDIR/remora_env.txt

#Read the list of active modules from the configuration file
remora_read_active_modules

remora_finalize_modules $REMORA_NODE $REMORA_OUTDIR $REMORA_TMPDIR
