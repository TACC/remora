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
#-      version     REMORA 2.0
#      authors     Carlos Rosales ( carlos@tacc.utexas.edu)
#                  Antonio Gomez  ( agomez@tacc.utexas.edu)
#      custodian   Kent Milfeld   (milfeld@tacc.utexas.edu)
#      license     MIT
#========================================================================

# -- Collect required data in backgroud

function remora_collect() {
    [[ "$REMORA_VERBOSE" == "1" ]] && echo -e "\n REMORA: collection started (in remora_collect)\n"

    source $REMORA_OUTDIR/remora_env.txt

    [[ "$REMORA_VERBOSE"   == "1" ]] && echo " REMORA: REPORT background processes (remora_report.sh) launching on nodes: ${NODES}."
    [[ "$REMORA_MONITOR"   == "1" ]] && [[ "$REMORA_VERBOSE"   == "1" ]] && echo " REMORA: MONITOR daemon processes (remora_monitor.sh)."
    [[ "$REMORA_SYMMETRIC" == "1" ]] && [[ "$REMORA_VERBOSE"   == "1" ]] && echo " REMORA: REPORT MIC daemon processes (remora_report_mic.sh)."

VERB_FILE=$REMORA_OUTDIR/REMORA_VERBOSE.out    #for debugging
[[ "$REMORA_VERBOSE" == "1" ]] && echo " -> DBG: Collection Nodes found in remora_collect:  NODES=${NODES[@]}"
    for NODE in ${NODES[@]} ; do

[[ "$REMORA_VERBOSE" == "1" ]] && echo " -> DBG: remora_collect launching collector on node=$NODE of  NODES=${NODES[@]}"
        # This is the core of REMORA. It runs the remora_report.sh daemon on each node allocated to the job.
        # remora_report.sh will run an infinite loop. In each iteration, it sequentially calls each collection
        # specificied in the configuration file or specified in REMORA_MODULES env var.

        COMMAND="$REMORA_BIN/scripts/remora_report.sh $NODE $REMORA_BIN $REMORA_OUTDIR >> $REMORA_OUTDIR/.remora_out_$NODE & "

        [[ "$REMORA_VERBOSE" == "1" ]] && echo " REMORA: remora_collect: START LAUNCH of remora_report.sh background process on $NODE."
        [[ "$REMORA_VERBOSE" == "1" ]] && echo "         ssh -f -n $NOE $COMMAND"

            ssh -f -n $NODE PATH=$PATH $COMMAND  # PATH is included to make so that tools are found (i.e. mpstat)

        [[ "$REMORA_VERBOSE" == "1" ]] && echo " REMORA: remora_collect: FINISHED LAUNCH of remora_report.sh background process on $NODE."

        # Only do this if MONITOR mode is active  TODO: explain what monitor.sh does.
        if [[ "$REMORA_MODE" == "MONITOR" ]]; then
            COMMAND="$REMORA_BIN/scripts/remora_monitor.sh $NODE $REMORA_BIN $REMORA_OUTDIR >> $REMORA_OUTDIR/.remora_out_${NODE} & "

            [[ "$REMORA_VERBOSE" == "1" ]] && echo " REMORA: START LAUNCH remora_monitor.sh background process on $NODE."
            [[ "$REMORA_VERBOSE" == "1" ]] && echo "         ssh -f -n $NODE $COMMAND"

            ssh -f -n $NODE $COMMAND

            [[ "$REMORA_VERBOSE" == "1" ]] && echo " REMORA: FINISHED LAUNCH remora_monitor.sh background process on $NODE."
        fi

        # Repeat the same for the MIC
        if [[ "$REMORA_SYMMETRIC" == "1" ]]; then
            COMMAND="$REMORA_BIN/scripts/remora_report_mic.sh ${NODE}-mic0 $REMORA_OUTDIR $REMORA_EFFECTIVE_PERIOD $REMORA_SYMMETRIC $REMORA_MODE $REMORA_PARALLEL $REMORA_VERBOSE $REMORA_BIN > $REMORA_OUTDIR/.remora_out_$NODE-mic0  & "

            if [[ "$REMORA_VERBOSE" == "1" ]]; then
                echo "REMORA: LAUNCHING remora daemon remora_report_mic.sh process on $NODE"
                echo "        ssh -q -f -n $NODE-mic0 $COMMAND"
            fi

            ssh -q -f -n $NODE-mic0 $COMMAND 

            [[ "$REMORA_VERBOSE" == "1" ]] && echo " REMORA: FINISHED  remora daemon remora_report_mic.sh launch on $Node."
        fi  

    done

    [[ "$REMORA_VERBOSE" == "1" ]] && echo -e " REMORA: All remote remora_report.sh collection processes launched.\n"
}
