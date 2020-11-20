#!/bin/bash
#
#========================================================================
# HEADER
#========================================================================
# DESCRIPTION
# remora_collect
#
# DO NOT call this script directly. This is called by REMORA. 
# This script starts the REMORA data collection.
#
# remora_collect "$@"
#========================================================================
# IMPLEMENTATION
#      version     REMORA 1.8.4
#      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#                  Antonio Gomez  (agomez@tacc.utexas.edu)
#      license     MIT
#========================================================================

# -- Collect required data in backgroud

function remora_collect() {
    if [[ "$REMORA_VERBOSE" == "1" ]]; then
        echo ""
        echo "REMORA: inside remora_collect function. Launching remora_report.sh on nodes.  Possibly remora_monitor.sh and remora_report_mic.sh."
        echo "" 
    fi

    source $REMORA_OUTDIR/remora_env.txt

    idx=0
    [[ "$REMORA_VERBOSE"   == "1" ]] && echo " REMORA: REPORT background processes (remora_report.sh) launching on nodes: ${NODES}."
    [[ "$REMORA_MONITOR"   == "1" ]] && [[ "$REMORA_VERBOSE"   == "1" ]] && echo " REMORA:  MONITOR background processes (remora_monitor.sh)."
    [[ "$REMORA_SYMMETRIC" == "1" ]] && [[ "$REMORA_VERBOSE"   == "1" ]] && echo " REMORA:  MIC     background processes (remora_report_mic.sh)."
    for NODE in $NODES
    do
        # This is the core of REMORA. It connects to all the nodes allocated to this job and runs the remora_report.sh script
        # remora_report.sh will run an infinite loop where, in each iteration of the loop, it calls the different modules
        # that are available (specified in the configuration file)

        COMMAND="$REMORA_BIN/scripts/remora_report.sh $NODE $REMORA_BIN $REMORA_OUTDIR"
        if [[ "$REMORA_VERBOSE" == "1" ]]; then
            echo " REMORA: Launching background REPORT process on $NODE. --watch for FINISHED"; echo ""
            echo "ssh -f -n $NODE $COMMAND >> $REMORA_OUTDIR/.remora_out_$NODE & echo \$!"
        fi
                                # Capture the PID of the remote process running remora_report.sh
                                # so that we can kill it once the job has finished
                                # We export the PATH to make sure that the required tools are found (i.e. mpstat)

       ssh  -f -n $NODE PATH=$PATH "$COMMAND >> $REMORA_OUTDIR/.remora_out_$NODE & echo \$!"  2>&1 >>$REMORA_OUTDIR/remora_pid.txt &

       [[ "$REMORA_VERBOSE" == "1" ]] && echo " REMORA: FINISHED background REPORT process launch on  $NODE."

        # Only do this if MONITOR mode is active
        if [[ "$REMORA_MODE" == "MONITOR" ]]; then
           COMMAND="$REMORA_BIN/scripts/remora_monitor.sh $NODE $REMORA_BIN $REMORA_OUTDIR"
           if [[ "$REMORA_VERBOSE" == "1" ]]; then
               echo " REMORA: Launching background MONITOR process on $NODE. --watch for FINISHED"
               echo ""; echo "ssh -f -n $NODE $COMMAND >> $REMORA_OUTDIR/.remora_out_$NODE & echo \$!"
           fi

           ssh  -f -n $NODE PATH=$PATH "$COMMAND >> $REMORA_OUTDIR/.remora_out_$NODE & echo \$!"  2>&1 >>$REMORA_OUTDIR/remora_pid_mon.txt &

           [[ "$REMORA_VERBOSE" == "1" ]] && echo " REMORA: FINISHED background MONITOR process launch on $Node."
        fi

        # Repeat the same for the MIC
        if [[ "$REMORA_SYMMETRIC" == "1" ]]; then
            COMMAND="$REMORA_BIN/scripts/remora_report_mic.sh ${NODE}-mic0 $REMORA_OUTDIR $REMORA_EFFECTIVE_PERIOD $REMORA_SYMMETRIC $REMORA_MODE $REMORA_PARALLEL $REMORA_VERBOSE $REMORA_BIN" 

            if [[ "$REMORA_VERBOSE" == "1" ]]; then
                echo " REMORA: Launching background MIC process on $NODE. --watch for FINISHED"
                echo "ssh -q -f -n $NODE-mic0 $COMMAND >> $REMORA_OUTDIR/.remora_out_$NODE-mic0  &  echo \$! "
            fi  

            ssh -q -f -n $NODE-mic0 "$COMMAND >> $REMORA_OUTDIR/.remora_out_$NODE-mic0  &  echo \$!" 2>&1 >>$REMORA_OUTDIR/remora_pid_mic.txt

            [[ "$REMORA_VERBOSE" == "1" ]] && echo " REMORA: FINISHED background MIC process launch on $Node."
        fi  

        # Go to the next node
        idx=$((idx+1))
    done

    if [[ "$REMORA_VERBOSE" == "1" ]]; then
        echo ""
        echo "REMORA: all collection processes launched."
    fi
}
