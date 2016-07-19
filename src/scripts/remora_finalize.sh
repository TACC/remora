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
#      version     REMORA 1.6
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
# Remove any temp files leftover
rm $REMORA_OUTDIR/*.tmp
# Give time for metadata to be updated
sleep 5

# Kill remote remora processes running in the backgroud
PID=(); PID_MIC=()
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
  FINAL_PID=`ssh -q -n $NODE $COMMAND 2> /dev/null`
  idx=$((idx+1))
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

# Wait for files to be available in cached shared file systems
waiting=1
while [ "$waiting" -lt "10" ] && [ ! -r $REMORA_OUTDIR/trace_network.txt ]; do
  sleep 2
  waiting=$((waiting+1))
done
if [ "$waiting" -gt "1" ] && [ "$REMORA_WARNING" -gt "1" ]; then
  printf "*** REMORA: WARNING - Slow file system response.\n"
  printf "*** REMORA: WARNING - It took %d seconds to reach the output files.\n" $((waiting*2))
fi

show_final_report $END $START

# Should write name-based loop
mv $REMORA_OUTDIR/{remora*,runtime*} $REMORA_OUTDIR/INFO
mv $REMORA_OUTDIR/cpu* $REMORA_OUTDIR/CPU
mv $REMORA_OUTDIR/mem* $REMORA_OUTDIR/MEMORY
if [ "$REMORA_MODE" == "FULL" ] || [ "$REMORA_MODE" == "MONITOR" ]; then
  if [ "$REMORA_LUSTRE" == "1" ]; then mv $REMORA_OUTDIR/{lustre*,lnet*} $REMORA_OUTDIR/IO; fi
  if [ "$REMORA_DVS" == "1" ]; then mv $REMORA_OUTDIR/dvs* $REMORA_OUTDIR/IO; fi
  mv $REMORA_OUTDIR/{ib*,trace_*,eth*} $REMORA_OUTDIR/NETWORK/
  mv $REMORA_OUTDIR/numa* $REMORA_OUTDIR/NUMA
fi
if [ "$REMORA_MODE" == "MONITOR" ]; then
	rm $REMORA_TMPDIR/.monitor
	mv $REMORA_OUTDIR/monitor* $REMORA_OUTDIR/MONITOR/
fi

