#!/bin/bash
#
#========================================================================
# HEADER
#========================================================================
# DESCRIPTION
# remora_finalize
#
# DO NOT call this script directly. This is called by REMORA. 
# This script finalizes the REMORA execution and kills all backgrounded
# remora scripts.
#
# remora_finalize.sh $END $START
#========================================================================
# IMPLEMENTATION
#      version     REMORA 1.7
#      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#                  Antonio Gomez  (agomez@tacc.utexas.edu)
#      license     MIT
#========================================================================

# --- Show a final report on screen

source $REMORA_BIN/aux/extra
source $REMORA_BIN/aux/report
source $REMORA_OUTDIR/remora_env.txt

END=$1
START=$2

# Copy data from temporary location to output dir
# This assumes OUTDIR is in a shared location
if [ "$REMORA_TMPDIR" != "$REMORA_OUTDIR" ]; then
  for NODE in $NODES
  do
    if [ "$REMORA_VERBOSE" == "1" ]; then
      echo "scp $NODE:$REMORA_TMPDIR/* $REMORA_OUTDIR"
    fi  
    scp $NODE:$REMORA_TMPDIR/* $REMORA_OUTDIR 2> /dev/null 1> /dev/null
  done
fi

# Ensure all data has been copied over or issue warning
NodeCount=`wc -l $REMORA_OUTDIR/remora_nodes.txt | awk '{print $1}'`
waiting=1; completed=0
while [ "$waiting" -lt 60 ] && [ "$completed" -lt "$NodeCount" ]; do
    completed=0
    for node in $NODES; do
        if [ -a $REMORA_OUTDIR/zz.$node ]; then
            completed=$((completed+1))
        fi  
    done
    sleep 1
done
if [ "$waiting" -ge 60 ]; then
    printf "*** REMORA: WARNING - Slow file system response. Post-processing may be incomplete\n"
fi

# Kill remote remora processes running in the backgroud
PID=(); PID_MIC=(); FINAL_PID=()
idx=0; for elem in `cat $REMORA_OUTDIR/remora_pid.txt`; do PID[$idx]=$elem; idx=$((idx+1)); done
idx=0; for elem in `cat $REMORA_OUTDIR/remora_pid_mic.txt`; do PID_MIC[$idx]=$elem; idx=$((idx+1)); done
idx=0
for NODE in $NODES; do
  REMORA_NODE_ID=$idx
  ssh -f $NODE 'kill '${PID[$idx]} 2> /dev/null
  if [ "$REMORA_SYMMETRIC" == "1" ]; then
    ssh -q -f $NODE-mic0 'kill '${PID_MIC[$idx]}
  fi  
  COMMAND="$REMORA_BIN/scripts/remora_remote_post.sh $NODE $REMORA_OUTDIR $REMORA_BIN $REMORA_VERBOSE $REMORA_NODE_ID >> $REMORA_OUTDIR/.remora_out_$NODE  &  echo \$! "
  if [ "$REMORA_VERBOSE" == "1" ]; then
    echo "ssh -q -n $NODE $COMMAND"
  fi  
  #Right now this is putting the command in the background and continuing (so the remora can finish, therefore epilog might  #kill everything! We need to fix it
  FINAL_PID+=(`ssh -q -n $NODE $COMMAND 2> /dev/null`)
  idx=$((idx+1))
done

#Wait until all remora_remote_post processes have finished
for pid in "${FINAL_PID[@]}"; do
  while [ -e /proc/$pid ]; do
    sleep 0.1
  done
done

rm $REMORA_OUTDIR/remora_pid.txt
rm $REMORA_OUTDIR/remora_pid_mic.txt

# Clean up the instance of remora summary running on the master node
if [ "$REMORA_MODE" == "MONITOR" ]; then
    idx=0; PID_MON=()
    for elem in `cat $REMORA_OUTDIR/remora_pid_mon.txt`; do PID_MON[$idx]=$elem; idx=$((idx+1)); done
    idx=0   
    for NODE in $NODES; do
        ssh -f $NODE 'kill '${PID_MON[$idx]} 2> /dev/null
        idx=$((idx+1))
    done
    rm $REMORA_OUTDIR/remora_pid_mon.txt
fi

show_final_report $END $START
rm -f $REMORA_OUTDIR/*.tmp

# Should write name-based loop
mv $REMORA_OUTDIR/{remora*,runtime*} $REMORA_OUTDIR/INFO

source $REMORA_BIN/aux/extra
source $REMORA_BIN/modules/modules_utils
remora_read_active_modules

#Move output files to their folders based on the configuration file
#If some files are missing, don't output the error message
for i in "${!REMORA_MODULES[@]}"; do
    mv $REMORA_OUTDIR/${REMORA_MODULES[$i]}* $REMORA_OUTDIR/${REMORA_MODULES_OUTDIRS[$i]} 2> /dev/null
done

if [ "$REMORA_MODE" == "MONITOR" ]; then
    rm $REMORA_TMPDIR/.monitor
    mv $REMORA_OUTDIR/monitor* $REMORA_OUTDIR/MONITOR/
fi
# Clean up TMPDIR if necessary
if [ "$REMORA_TMPDIR" != "$REMORA_OUTDIR" ]; then
    rm -rf $REMORA_TMPDIR
fi

#Clean the zz files (files used to make sure all transfers have finished)
rm -f $REMORA_OUTDIR/zz.*
