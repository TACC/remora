#Show a final report on screen
source $REMORA_BIN/aux/extra
source $REMORA_BIN/aux/report
source $REMORA_BIN/aux/scheduler
source $REMORA_BIN/aux/sql_functions
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

# Kill remote remora processes running in the backgroud
PID=(); PID_MIC=()
idx=0; for elem in `cat $REMORA_OUTDIR/remora_pid.txt`; do PID[$idx]=$elem; idx=$((idx+1)); done
idx=0; for elem in `cat $REMORA_OUTDIR/remora_pid_mic.txt`; do PID_MIC[$idx]=$elem; idx=$((idx+1)); done
idx=0;
for NODE in $NODES
do
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
mv $REMORA_OUTDIR/cpu* $REMORA_OUTDIR/CPU/
mv $REMORA_OUTDIR/{dvs*,lustre*,lnet*} $REMORA_OUTDIR/IO
mv $REMORA_OUTDIR/mem* $REMORA_OUTDIR/MEMORY
mv $REMORA_OUTDIR/{ib*,trace_*} $REMORA_OUTDIR/NETWORK/
mv $REMORA_OUTDIR/numa* $REMORA_OUTDIR/NUMA/
