#!/bin/bash
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
#% remora_remote_post.sh $NODE $REMORA_OUTDIR $REMORA_BIN $REMORA_VERBOSE
#========================================================================
#- IMPLEMENTATION
#-                 version     REMORA 1.8.6
#-
#-up to mid 2017
#-               Carlos Rosales ( carlos@tacc.utexas.edu)
#-               Antonio Gomez  ( agomez@tacc.utexas.edu)
#-after mid 2017
#-               Kent Milfeld     (milfeld@tacc.utexas.edu)
#-               Albert LU        (   cylu@tacc.utexas.edu)
#-
#-      license     MIT
#========================================================================

#Initialize variables specific to certain modules here
   REMORA_NODE=$1
   REMORA_OUTDIR=$2
   REMORA_BIN=$3
   REMORA_VERBOSE=$4
   VERB_FILE=$REMORA_OUTDIR/REMORA_VERBOSE.out    #Write to this file for Debugging.
   
   #Source the script that has the modules' functionality  ##KFM Don't think this is needed anymore
   if [[ "$REMORA_VERBOSE" == "1" ]]; then
     echo "source $REMORA_BIN/aux/extra"
     echo "source $REMORA_BIN/modules/modules_utils"
   fi
   
   source $REMORA_BIN/aux/extra
   source $REMORA_BIN/modules/modules_utils
   
   source $REMORA_OUTDIR/remora_env.txt
   REMORA_MODULES=( $REMORA_ACTIVE_MODULES )
   export REMORA_MODULES

   for MODULE in ${REMORA_MODULES[@]}; do
      source $REMORA_BIN/modules/$MODULE
      [[ "$REMORA_VERBOSE" == "1" ]] && echo finalize_module_$MODULE $REMORA_NODE $REMORA_OUTDIR $REMORA_TMPDIR

      finalize_module_$MODULE $REMORA_NODE $REMORA_OUTDIR $REMORA_TMPDIR

   done
