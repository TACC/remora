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
# remora_finalize $END $START
#========================================================================
# IMPLEMENTATION
#-      version     REMORA 2.0
#      authors     Carlos Rosales ( carlos@tacc.utexas.edu)
#                  Antonio Gomez  ( agomez@tacc.utexas.edu)
#      custodian   Kent Milfeld   (milfeld@tacc.utexas.edu)
#      license     MIT
#========================================================================

# --- Show a final report on screen

source $REMORA_BIN/aux/extra
source $REMORA_BIN/aux/report

function remora_finalize() {
VERB_FILE=$REMORA_OUTDIR/REMORA_VERBOSE.out  #for debugging

    source $REMORA_OUTDIR/remora_env.txt
    REMORA_MODULES=( $REMORA_ACTIVE_MODULES )
    REMORA_MODULES_OUTDIRS=( $REMORA_ACTIVE_MODULES_OUTDIRS )
    export REMORA_MODULES REMORA_MODULES_OUTDIRS

    if [[ "$REMORA_VERBOSE" == "1" ]]; then
        echo ""
        echo "REMORA: Starting REMORA finalize (in remora_finalize function)."
    fi
    END=$1
    START=$2
    local NODES_LIST=( $(cat $REMORA_OUTDIR/remora_nodes.txt) )
    local NODES="${NODES_LIST[@]}"

    local remora_timeout=10

if [[ -z $REMORA_SNAPSHOT ]]; then        #TMPDIR must same as OUTDIR for snapshots
      # Copy data from temporary location to output dir
      # This assumes OUTDIR is in a shared location
      if [[ "$REMORA_TMPDIR" != "$REMORA_OUTDIR" ]]; then
          for NODE in $NODES
              do
                  if [[ "$REMORA_VERBOSE" == "1" ]]; then
                      echo "REMORA: Copying files from temporary location to output folder"
                      echo "scp $NODE:$REMORA_TMPDIR/* $REMORA_OUTDIR"
                  fi  
                  scp $NODE:$REMORA_TMPDIR/* $REMORA_OUTDIR 2> /dev/null 1> /dev/null
              done
      fi
fi

    # Ensure all data has been copied over or issue warning
    NodeCount=`wc -l $REMORA_OUTDIR/remora_nodes.txt | awk '{print $1}'`
    waiting=1; completed=0
    while [[ "$waiting" -lt $remora_timeout ]] && [[ "$completed" -lt "$NodeCount" ]]; do
        completed=0
        for node in $NODES; do
            if [[ -a $REMORA_OUTDIR/zz.$node ]]; then
                completed=$((completed+1))
            fi  
        done
        [[ "$REMORA_VERBOSE" == "1" ]] && echo " -> Remora_finalize: Checking for zz.<node> completion files."
        sleep 2
    done

    if [[ "$waiting" -ge $remora_timeout ]]; then
        printf "\n*** REMORA: WARNING - Slow file system response. Post-processing may be incomplete\n\n"
        printf "*** REMORA: WARNING - %s out of %s nodes successfully processed\n" "$completed" "$NodeCount"
    fi

if [[ -z $REMORA_SNAPSHOTS ]]; then   #If not snapshots (a remora run) do this
                                      #This is done at end of snapshot with .done_snaps_<node> files. 
    # Kill reporters running in background
    for SSH_NODE in $NODES; do
        ssh -q -f $SSH_NODE "pkill -f remora_report.sh"
    done
fi

    for NODE in $NODES; do
        SSH_NODE=$NODE

        CMD="$REMORA_BIN/scripts/remora_remote_post.sh $NODE $REMORA_OUTDIR $REMORA_BIN $REMORA_VERBOSE >>$REMORA_OUTDIR/.remora_out_$NODE  & "

        if [[ "$REMORA_VERBOSE" == "1" ]]; then
            echo "REMORA: launching remote postprocessing (plotting, etc)"
            echo "        ssh -q -n $SSH_NODE $CMD"
        fi  
        #Right now this is putting the command in the background and continuing 
        #(so the remora can finish, therefore epilog might  #kill everything! We need to fix it

        ssh -q -n $SSH_NODE $CMD

    done

    if [[ "$REMORA_VERBOSE" == "1" ]]; then
        echo ""
        echo " REMORA: Will wait for postprocesses (remora_remorte_post.sh) to finish:"
    fi

    #Wait until all remora_remote_post processes have finished

  ##KFM local NODES_TMP="$NODES"
  ##KFM local NODES_LIST=( $NODES ) #make array list

    [[ ! -z $SNAPSHOT_NODE     ]] && NODES_LIST=($SNAPSHOT_NODE)

    for NODE in ${NODES_LIST[@]}; do
        SSH_NODE=$NODE
        cnt=0
        CMD="ps -ef | grep scripts/remora_remote_post.sh | grep -wv grep | wc -l"

        while [[ `ssh $SSH_NODE $CMD` -gt 0 ]]; do
            sleep 0.05
            if [ "$REMORA_VERBOSE" == "1" ]; then
                [[ $cnt -eq 0 ]] && printf "Post processing waiting on Node: $NODE"
                [[ $cnt -ne 0 ]] && printf "."
            fi
            cnt=$((cnt+1))
        done
        printf "\n" 
    done

    sleep 0.5 #Add a small delay so that files are transferred (deal with Lustre latency)

    [[ "$REMORA_VERBOSE" == "1" ]] && echo -e "\nREMORA: All REMORA postprocesses have finished"

    # Clean up the instance of remora summary running on the master node
    if [[ "$REMORA_MODE" == "MONITOR" ]]; then

        for NODE in ${NODES_LIST[@]}; do
            ssh -q -f $NODE "pkill -f remora_monitor.sh"
        done
    fi

    show_final_report $END $START

    if [[ "$REMORA_VERBOSE" == "1" ]]; then
        echo "REMORA: Cleaning. Moving everything to the correct place"
    fi

    sleep 1.0 

    rm -f $REMORA_OUTDIR/*.tmp

    # Should write name-based loop
    mv $REMORA_OUTDIR/{remora*,runtime*} $REMORA_OUTDIR/INFO

    source $REMORA_BIN/aux/extra
    source $REMORA_BIN/modules/modules_utils

    #Move output files to their folders based on the configuration file
    #If some files are missing, don't output the error message

    for i in ${!REMORA_MODULES[@]}; do
        [[ "$REMORA_VERBOSE" == "1" ]] &&  echo "REMORA: Moving output files for ${REMORA_MODULES[$i]}"
        mv $REMORA_OUTDIR/${REMORA_MODULES[$i]}*    $REMORA_OUTDIR/${REMORA_MODULES_OUTDIRS[$i]} 2> /dev/null

        [[ ${REMORA_MODULES[$i]} == "eth" ]] &&
             mv $REMORA_OUTDIR/network_eth_traffic* $REMORA_OUTDIR/${REMORA_MODULES_OUTDIRS[$i]} 2> /dev/null

        [[ ${REMORA_MODULES[$i]} == "power" ]] &&
             mv $REMORA_OUTDIR/energy_*             $REMORA_OUTDIR/${REMORA_MODULES_OUTDIRS[$i]} 2> /dev/null

        sleep 0.2
    done

    [[ "$REMORA_VERBOSE" == "1" ]] && echo "REMORA: Generating base HTML file"

    #We simply create an HTML file with links to all the different results
    if [[ "$REMORA_PLOT_RESULTS" != "0" ]] ; then
        printf "%s \n" "<html lang=\"en\">" > $REMORA_OUTDIR/remora_summary.html
        printf "%s \n" "<head><title>REMORA TACC</title></head><body>" >> $REMORA_OUTDIR/remora_summary.html
        printf "%s \n" "<a href=\"https://github.com/TACC/remora\" target=\"_blank\"><img src=\"https://raw.githubusercontent.com/TACC/remora/master/docs/logos/Remora-logo-300px.png\" alt=\"REMORA Logo\" style=\"max-width:100%;\"></a>" >> $REMORA_OUTDIR/remora_summary.html
        printf "<h1>REMORA REPORT - JOB %s </h1>\n" "$REMORA_JOB_ID" >> $REMORA_OUTDIR/remora_summary.html
	printf "<pre>\n"   >> $REMORA_OUTDIR/remora_summary.html
	printf "  ** NOTICE: IO Modules are not available on ls6. For security,\n"   >> $REMORA_OUTDIR/remora_summary.html
	printf "  **         root access is now required to extract IO data.     "   >> $REMORA_OUTDIR/remora_summary.html
	printf "</pre>\n"  >> $REMORA_OUTDIR/remora_summary.html
        for i in ${!REMORA_MODULES[@]}; do

            R_module="${REMORA_MODULES[$i]}"

            if [[ "$R_module" == "gpu" ]] &&  [[ "$REMORA_CUDA" == "0" ]]; then
               [[ "$REMORA_VERBOSE" == 1 ]] && echo "  GPU summary not reported, REMORA_CUDA=0 or nvidia-smi not detected."
            else
               printf "<h2>%s utilization</h2> \n" ${REMORA_MODULES[$i]} >> $REMORA_OUTDIR/remora_summary.html
            fi

            if [[ "${REMORA_MODULES[$i]}" == "power" ]] && [[ $NodeCount -gt 1 ]]; then
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/power_aggregated.html" "Aggregated" >> $REMORA_OUTDIR/remora_summary.html
            fi

            if [[ "${REMORA_MODULES[$i]}" == "lustre" ]]; then
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/lustre_aggregated.html" "Aggregated" >> $REMORA_OUTDIR/remora_summary.html
            fi
            if [[ "${REMORA_MODULES[$i]}" == "impi" ]]; then
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/impi_fraction.html"  "Fraction"  >> $REMORA_OUTDIR/remora_summary.html
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/impi_breakdown.html" "Breakdown" >> $REMORA_OUTDIR/remora_summary.html
            fi
            if [[ "${REMORA_MODULES[$i]}" == "mv2" ]]; then
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/mv2_fraction.html"  "Fraction"  >> $REMORA_OUTDIR/remora_summary.html
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/mv2_breakdown.html" "Breakdown" >> $REMORA_OUTDIR/remora_summary.html
            fi
            if [[ "${REMORA_MODULES[$i]}" == "impi_mpip" ]]; then
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/impi_mpip_fraction.html"  "Fraction"  >> $REMORA_OUTDIR/remora_summary.html
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/impi_mpip_breakdown.html" "Breakdown" >> $REMORA_OUTDIR/remora_summary.html
            fi
            if [[ "${REMORA_MODULES[$i]}" == "gpu" ]]; then
               R_module=gpu_memory_stats
            fi
            for node in $NODES; do
               #if [[ -f  $REMORA_OUTDIR/${REMORA_MODULES_OUTDIRS[$i]}/${REMORA_MODULES[$i]}_${node}.html ]]; then
                if [[ -f  $REMORA_OUTDIR/${REMORA_MODULES_OUTDIRS[$i]}/${R_module}_${node}.html ]]; then
                    printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/${R_module}_${node}.html" ${node} >> $REMORA_OUTDIR/remora_summary.html
                fi
            done
        done
        #Add the summary at the end
        printf "<h2>Summary for job %s</h2>\n" "$REMORA_JOB_ID" >> $REMORA_OUTDIR/remora_summary.html 
        input="$REMORA_OUTDIR/INFO/remora_summary.txt"
        printf "<pre>\n"  >> $REMORA_OUTDIR/remora_summary.html
        while IFS= read -r line
        do
           #printf "<p>%s</p>\n" "$line" >> $REMORA_OUTDIR/remora_summary.html 
            printf "%s\n"        "$line" >> $REMORA_OUTDIR/remora_summary.html 
        done < "$input"

        printf "</pre>\n"  >> $REMORA_OUTDIR/remora_summary.html

        printf "%s \n" "</body></html>" >> $REMORA_OUTDIR/remora_summary.html
    fi

    #Continue after generating the HTML file
    if [[ "$REMORA_MODE" == "MONITOR" ]]; then
        [[ "$REMORA_VERBOSE" == "1" ]] && echo "REMORA: Handling MONITOR files"
        rm -f $REMORA_TMPDIR/.monitor
        mv    $REMORA_OUTDIR/monitor* $REMORA_OUTDIR/MONITOR/
    fi

    # Clean up TMPDIR if necessary
    if [[ "$REMORA_TMPDIR" != "$REMORA_OUTDIR" ]]; then
        [[ "$REMORA_VERBOSE" == "1" ]] && echo "REMORA: Removing $REMORA_TMPDIR"
        rm -rf $REMORA_TMPDIR
    fi

    #Clean the zz files (files used to make sure all transfers have finished)
    rm -f $REMORA_OUTDIR/zz.*

    [[ "$REMORA_VERBOSE" == "1" ]] && echo -e "\nREMORA finalize finished"
}

