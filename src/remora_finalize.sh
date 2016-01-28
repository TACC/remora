#Show a final report on screen
source $REMORA_BIN/aux/extra
source $REMORA_BIN/aux/report
source $REMORA_BIN/aux/scheduler
source $REMORA_BIN/aux/sql_functions
source $REMORA_OUTDIR/remora_env.txt

# Copy data from temporary location to output dir
if [ "$REMORA_TMP" != "$REMORA_OUTDIR" ]; then
  for NODE in $NODES
  do
    if [ "$REMORA_VERBOSE" == "1" ]; then
      echo "ssh $NODE:$REMORA_TMPDIR/* $REMORA_OUTDIR"
    fi  
    scp $NODE:$REMORA_TMPDIR/* $REMORA_OUTDIR 2> /dev/null 1> /dev/null
  done
fi

# Kill remote remora processes running in the backgroud
PID=(); PID_MIC=()
idx=0; for elem in $REMORA_PID; do PID[$idx]=$elem; idx=$((idx+1)); done
idx=0; for elem in $REMORA_PID_MIC; do PID_MIC[$idx]=$elem; idx=$((idx+1)); done
idx=0;
for NODE in $NODES
do
  REMORA_NODE_ID=$idx
  ssh -f $NODE 'kill '${PID[$idx]}
  if [ "$REMORA_SYMMETRIC" == "1" ]; then
    ssh -q -f $NODE-mic0 'kill '${PID_MIC[$idx]}
  fi  
  COMMAND="$REMORA_BIN/remora_remote_post.sh $NODE $REMORA_OUTDIR $REMORA_BIN $REMORA_VERBOSE $REMORA_NODE_ID >> $REMORA_OUTDIR/.remora_out_$NODE  &  echo \$! "
  if [ "$REMORA_VERBOSE" == "1" ]; then
    echo "ssh -q -n $NODE $COMMAND"
  fi  
  #Right now this is putting the command in the background and continuing (so the remora can finish, therefore epilog might  #kill everything! We need to fix it
  FINAL_PID=`ssh -q -n $NODE $COMMAND`
  idx=$((idx+1))
done  


show_final_report $END $START

