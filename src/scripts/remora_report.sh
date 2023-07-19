#!/bin/bash
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
#-      version     REMORA 2.0
#-      authors     Carlos Rosales ( carlos@tacc.utexas.edu)
#-                  Antonio Gomez  ( agomez@tacc.utexas.edu)
#-      custodian   Kent Milfeld   (milfeld@tacc.utexas.edu)
#-      license     MIT
#========================================================================

#Initialize variables specific to certain modules here
REMORA_NODE=$1
REMORA_BIN=$2
REMORA_OUTDIR=$3

VERB_FILE=$REMORA_OUTDIR/REMORA_VERBOSE.out  # use for debugging

#Source the script that has the modules' functionality
   source $REMORA_BIN/aux/extra
   source $REMORA_BIN/modules/modules_utils
   
   source $REMORA_OUTDIR/remora_env.txt
   REMORA_MODULES=(  ${REMORA_ACTIVE_MODULES[@]}  )
   export REMORA_MODULES
   
# Create TMPDIR if it is not there
   mkdir -p $REMORA_TMPDIR     ##KFM ##KM isn't this done in create_folders routine?

# Generate unique file for transfer completion check on distributed file systems
   touch $REMORA_TMPDIR/zz.$REMORA_NODE
   

   [[ $REMORA_BINARIES == 1 ]] && rm -f /dev/shm/remora_*     #things may be left behind from a previous run
   
#Configure the modules even if binaries.  Various scripts initialize ("configure") modules here.

   for MODULE in ${REMORA_MODULES[@]}; do

      [[ "$REMORA_VERBOSE" == "1" ]] && echo init_module_$MODULE $REMORA_NODE $REMORA_OUTDIR $REMORA_TMPDIR
     
      source $REMORA_BIN/modules/$MODULE
      init_module_$MODULE $REMORA_NODE $REMORA_OUTDIR $REMORA_TMPDIR

   done
   
# Tag end of file so that we know when it is OK to source / read
# from a different shell
   echo "#EOF" >> $REMORA_OUTDIR/remora_env.txt
  
   collect_limit=0.100  # (sec)  If REMORA_PERIOD - collection time < collect_limit, warn user

   reported_warn=no reported_crit=no period_cntr=1 info=""

   while [[ 1 ]]; do

     tm_0=$( date +%s.%3N )
     for MODULE in ${REMORA_MODULES[@]}; do
       if [[ -e $REMORA_BIN/binary_data_collectors/data_collect_$MODULE ]] && [[ $REMORA_BINARIES == 1 ]]; then
  
         [[ "$REMORA_VERBOSE" == "1" ]] && echo "Binary Module Data Collection: $MODULE $NODE $OUTDIR $TMPDIR"
         eval $REMORA_BIN/binary_data_collectors/data_collect_$MODULE $REMORA_NODE $REMORA_OUTDIR $REMORA_TMPDIR
  
       else
         [[ "$REMORA_VERBOSE" == "1" ]] && echo "Script-only Module data Collection: $MODULE $NODE $OUTDIR $TMPDIR"
         eval collect_data_$MODULE $REMORA_NODE $REMORA_OUTDIR $REMORA_TMPDIR
       fi
     done
     tm_1=$(date +%s.%3N)
  
     sleep_time=$( bc<<<"scale=4; $REMORA_PERIOD - ($tm_1-$tm_0)" )
  
     ## Just checking here: make sure collection time is not exceeding REMORA_PERIOD.
     ##                     Will not worry about 2ms if test (8ms if warning/critical printed).
     if (( $(bc <<< "$sleep_time < $collect_limit") == 1 )) ; then
        collect_tm=$( bc<<<"scale=4; $tm_1-$tm_0" )
  
        if (( $(bc <<< "$sleep_time > 0.0") == 1 )); then
           if [[ $reported_warn != "yes" ]]; then
              echo "  ***** WARNING  ******* Sleep time between collections is less than $collect_limit, period=$period_cntr." >>$VERB_FILE
              echo "                         REMORA_PERIOD=$REMORA_PERIOD and Collection Time=$collect_tm." >>$VERB_FILE
              echo "                         You should use a larger PERIOD for this system."               >>$VERB_FILE
              reported_warn=yes
           fi  
        fi  
  
        if (( $(bc <<< "$sleep_time < 0.0") == 1 )); then  #Houston, we have a problem.
  
           if [[ $reported_crit != "yes" ]]; then
              echo "  ***** CRITICAL ******* Sleep time between collections is less than $collect_limit, period=$period_cntr." >>$VERB_FILE
              echo "  *****          ******* TIMING RESULTS MAY BE INACCURATE."                            >>$VERB_FILE
              echo "                        REMORA_PERIOD=$REMORA_PERIOD and Collection Time=$collect_tm." >>$VERB_FILE
              echo "                        Use a larger PERIOD for this system."                          >>$VERB_FILE
              reported_crit=yes
           fi  
           info=" -- zero if negative"
           sleep_time=0        #sleep cannot handle negative number.
        fi  
  
  
     fi  
  
     sleep $sleep_time   #sleep time = REMORA_PERIOD - Collection Time  (see exception above)
  
     [[ ! -z $REMORA_VERBOSE ]] && collect_tm=$( bc<<<"scale=4; $tm_1-$tm_0" )
  
     period_cntr=$(( period_cntr+1 ))

   done
