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


SYMMETRIC=$4
REMORA_CUDA=$6

source modules/modules_utils

remora_read_active_modules
remora_configure_modules $1 $2

while [ 1 ]
do
    remora_read_active_modules $1 $2

  sleep $3

done
