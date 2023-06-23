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
#-      version     REMORA 1.8.6
#-      authors     Carlos Rosales ( carlos@tacc.utexas.edu)
#-                  Antonio Gomez  ( agomez@tacc.utexas.edu)
#-      custodian   Kent Milfeld   (milfeld@tacc.utexas.edu)
#-      license     MIT
#========================================================================

#Initialize variables specific to certain modules here
REMORA_NODE=$1
REMORA_BIN=$2
REMORA_OUTDIR=$3

#Source the script that has the modules' functionality
   source $REMORA_BIN/aux/extra
   source $REMORA_BIN/modules/modules_utils
   
   source $REMORA_OUTDIR/remora_env.txt
   
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
   
# Tag end of file so that we know when it is OK to source / read
# from a different shell
   echo "#EOF" >> $REMORA_OUTDIR/remora_env.txt
  
collect_limit=0.100  # (sec)  If REMORA_PERIOD - collection time < collect_limit, warn user

reported_warn=no reported_crit=no period_cntr=1 info=""

while [[ 1 ]]; do

if [[ $REMORA_BINARIES == 1 ]]; then

     for MODULE in "${REMORA_MODULES[@]}"; do
         tm_0=$( date +%s%N )
         if [[ "$REMORA_VERBOSE" == "1" ]]; then
             echo "collect_binary_$MODULE $NODE $OUTDIR $TMPDIR"
         fi
         $REMORA_BIN/binaries/$MODULE
         tm_1=$(date +%s%N)
     done
else
   tm_0=$( date +%s%N )
      remora_execute_modules $REMORA_NODE $REMORA_OUTDIR $REMORA_TMPDIR "${REMORA_MODULES[@]}"
   tm_1=$(date +%s%N)
fi

   sleep_time=$( bc<<<"scale=4; $REMORA_PERIOD - ($tm_1-$tm_0)/1000000000" )

   ## Just checking here: make sure collection time is not exceeding REMORA_PERIOD.
   ##                     Will not worry about 2ms if test (8ms if warning/critical printed).
   if (( $(bc <<< "$sleep_time < $collect_limit") == 1 )) ; then
      collect_tm=$( bc<<<"scale=4; ($tm_1-$tm_0)/1000000000" )

      if (( $(bc <<< "$sleep_time > 0.0") == 1 )); then
         if [[ $reported_warn != "yes" ]]; then
            echo "  ***** WARNING  ******* Sleep time between collections is less than $collect_limit, period=$period_cntr."
            echo "                         REMORA_PERIOD=$REMORA_PERIOD and Collection Time=$collect_tm."
            echo "                         You should use a larger PERIOD for this system."
            reported_warn=yes
         fi  
      fi  

      if (( $(bc <<< "$sleep_time < 0.0") == 1 )); then  #Houston, we have a problem.

         if [[ $reported_crit != "yes" ]]; then
            echo "  ***** CRITICAL ******* Sleep time between collections is less than $collect_limit, period=$period_cntr."
            echo "  *****          ******* TIMING RESULTS MAY BE INACCURATE."
            echo "                        REMORA_PERIOD=$REMORA_PERIOD and Collection Time=$collect_tm."
            echo "                        Use a larger PERIOD for this system."
            reported_crit=yes
         fi  
         info=" -- zero if negative"
         sleep_time=0        #sleep cannot handle negative number.
      fi  


   fi  
   ##

   sleep $sleep_time   #sleep time = REMORA_PERIOD - Collection Time  (see exception above)

   if [[ ! -z $REMORA_VERBOSE ]]; then
      collect_tm=$( bc<<<"scale=4; ($tm_1-$tm_0)/1000000000" )
      echo " Sleeping: $sleep_time = $REMORA_PERIOD-$collect_tm (REMORA_PERIOD-Collection Time $info)."
   fi  

   period_cntr=$(( period_cntr+1 ))
done
