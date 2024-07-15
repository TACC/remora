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

FILE_ADVICE=$REMORA_OUTDIR/advisory.txt  # can be used for debugging
FILE_REMOTE_VEBOSE=$REMORA_OUTDIR/remote_verbose.txt  # used for debugging #TODO: remove this and echo below

#Function to cleanup /dev/shm when this script is terminated to stop collection.
exit_clean() {
  rm -f /dev/shm/remora_*
}
#Trap the SIGTERM and SIGINT signals and call exit_clean
trap "exit_clean" EXIT


#Source the script that has the modules' functionality
   source $REMORA_BIN/aux/extra
   source $REMORA_BIN/modules/modules_utils
   
   source $REMORA_OUTDIR/remora_env.txt
## REMORA_MODULES=( ${REMORA_ACTIVE_MODULES[@]} )
   REMORA_MODULES=( $REMORA_ACTIVE_MODULES )
   export REMORA_MODULES
   
# Create TMPDIR if it is not there
   mkdir -p $REMORA_TMPDIR     

# Generate unique file for transfer completion check on distributed file systems
   touch $REMORA_TMPDIR/zz.$REMORA_NODE           ##KFM TODO: Maybe deprecated/rename tmp_dir.REMORA_NODE
   

   [[ $REMORA_BINARIES == 1 ]] && rm -f /dev/shm/remora_*     #things may be left behind from a previous run
   
#Configure the modules even if binaries.  Various scripts initialize ("configure") modules here.

  for MODULE in ${REMORA_MODULES[@]}; do

     [[ "$REMORA_VERBOSE" == "1" ]] && echo init_module_$MODULE $REMORA_NODE $REMORA_OUTDIR $REMORA_TMPDIR >> $FILE_REMOTE_VEBOSE
    
     source $REMORA_MODULE_PATH/$MODULE
     init_module_$MODULE $REMORA_NODE $REMORA_OUTDIR $REMORA_TMPDIR

  done

  # Generate unique node file to let remora/snapshot know init_module_x completed
  touch $REMORA_OUTDIR/.remora_out_$REMORA_NODE  ##KFM now remora looks for this.
  
  # Tag end of file so that we know when it is OK to source / read
  # from a different shell
   echo "#EOF" >> $REMORA_OUTDIR/remora_env.txt
 
  collect_limit=0.100  # (sec)  If REMORA_PERIOD - collection time < collect_limit, warn user

  reported_warn=no reported_crit=no period_cntr=1 info=""

  # sleep_time correction reset
  reset=1

  # sleep_time correction factor
  alpha=0.6


  # Remove any collectors that were rejected during remora_init
  if [[ $REMORA_POWER_IGNORE -ne 0 ]]; then
    REMORA_MODULES=( "${REMORA_MODULES[@]/power}" )
  fi

  period_no=1
  while [[ 1 ]]; do

     tm_0=$( date +%s.%3N )
     for MODULE in ${REMORA_MODULES[@]}; do
       if [[ -e $REMORA_BIN/binary_data_collectors/data_collect_$MODULE ]] && [[ $REMORA_BINARIES == 1 ]]; then
  
         [[ "$REMORA_VERBOSE" == "1" ]] && echo "Binary Module Data Collection: $MODULE $NODE $OUTDIR $TMPDIR" >> $FILE_REMOTE_VEBOSE
         eval $REMORA_BIN/binary_data_collectors/data_collect_$MODULE $REMORA_NODE $REMORA_OUTDIR $REMORA_TMPDIR
  
       else
         [[ "$REMORA_VERBOSE" == "1" ]] && echo "Script-only Module data Collection: $MODULE $NODE $OUTDIR $TMPDIR" >> $FILE_REMOTE_VEBOSE
         eval collect_data_$MODULE $REMORA_NODE $REMORA_OUTDIR $REMORA_TMPDIR
       fi
     done
     tm_1=$(date +%s.%3N)

     if [[ $reset == 1 ]]; then
       sleep_time=$( bc<<<"scale=4; $REMORA_PERIOD - ($tm_1-$tm_0)")
       reset=0
     else
       sleep_time=$( bc<<<"scale=4; $sleep_time - $alpha*$tm_d" )
     fi  

     ## Just checking here: make sure collection time is not exceeding REMORA_PERIOD.
     ##                     Will not worry about 2ms if test (8ms if warning/critical printed).
     if (( $(bc <<< "$sleep_time < $collect_limit") == 1 )) ; then
        collect_tm=$( bc<<<"scale=4; $tm_1-$tm_0" )
  
        if (( $(bc <<< "$sleep_time > 0.0") == 1 )); then
           if [[ $reported_warn != "yes" ]]; then
              echo "  ***** WARNING  ******* Sleep time between collections is less than $collect_limit, period=$period_cntr." >>$FILE_ADVICE
              echo "                         REMORA_PERIOD=$REMORA_PERIOD and Collection Time=$collect_tm." >>$FILE_ADVICE
              echo "                         You should use a larger PERIOD for this system."               >>$FILE_ADVICE
              reported_warn=yes
           fi  
        fi  
  
        if (( $(bc <<< "$sleep_time < 0.0") == 1 )); then  #Houston, we have a problem.
  
           if [[ $reported_crit != "yes" ]]; then
              echo "  ***** CRITICAL ******* Sleep time between collections is less than $collect_limit, period=$period_cntr." >>$FILE_ADVICE
              echo "  *****          ******* TIMING RESULTS MAY BE INACCURATE."                            >>$FILE_ADVICE
              echo "                        REMORA_PERIOD=$REMORA_PERIOD and Collection Time=$collect_tm." >>$FILE_ADVICE
              echo "                        Use a larger PERIOD for this system."                          >>$FILE_ADVICE
              reported_crit=yes
           fi  
           info=" -- negative sleep adjusted to 0"
           sleep_time=0        #sleep cannot handle negative number.
           reset=1
        fi  
  
  
     fi  

     sleep $sleep_time   #sleep time = REMORA_PERIOD - Collection Time  (see exception above)
  
     if [[ "$REMORA_VERBOSE" == "1" ]]; then
         collect_tm=$( bc<<<"scale=4; $tm_1-$tm_0" )
         echo " Sleeping: $sleep_time = $REMORA_PERIOD-$collect_tm (R_PERIOD-Collect. tm $info)." >> $FILE_REMOTE_VEBOSE
     fi
  
     if [[ $reset == 0 ]]; then   #Normal run: find tm_d(time_delta) of  "background" consumed time  up to this point.
       tm_2=$(date +%s.%3N)       #Will use in adjusting time in next iteration.
       tm_d=$( bc<<<"$tm_2-$tm_0-$REMORA_PERIOD" )
     fi
     # vvv for snapshot to know when to stop collecting data -- /dev/shm/remora_* files are removed by trap
     [[ ! -z  $REMORA_SNAPS ]] && [[ "$period_no" == "$REMORA_SNAPS" ]] && touch $REMORA_OUTDIR/.done_snaps_${REMORA_NODE} && exit 0
     period_no=$(( period_no+1 ))

  done
