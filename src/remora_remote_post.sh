#!/bin/sh
#
#========================================================================
# HEADER
#========================================================================
#% DESCRIPTION
#% remora_remote_post
#%
#% DO NOT call this script directory. This is called by REMORA
#%
#% remora_remote_post.sh NODE_NAME OUTDIR REMORA_BIN
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
#       2016/01/24: Version 1.5. Separate dir for tmp files.
#========================================================================

#Initialize variables specific to certain modules here
REMORA_NODE=$1
REMORA_OUTDIR=$2
REMORA_BIN=$3
REMORA_VERBOSE=$4
REMORA_NODE_ID=$5

#Source the script that has the modules' functionality
if [ "$REMORA_VERBOSE" == "1" ]; then
  echo "source $REMORA_BIN/modules/modules_utils"
fi
source $REMORA_BIN/modules/modules_utils

#Read the list of active modules from the configuration file
remora_read_active_modules

remora_finalize_modules $REMORA_NODE $REMORA_OUTDIR $REMORA_NODE_ID
