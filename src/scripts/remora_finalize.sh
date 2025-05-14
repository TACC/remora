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

    source $REMORA_OUTDIR/remora_env.txt
    REMORA_MODULES=( $REMORA_ACTIVE_MODULES )
    REMORA_MODULES_OUTDIRS=( $REMORA_ACTIVE_MODULES_OUTDIRS )
    export REMORA_MODULES REMORA_MODULES_OUTDIRS
    VERB_FILE=$REMORA_OUTDIR/REMORA_FINAL.out  #for debugging

    FILE_SUM_HTML=$REMORA_OUTDIR/remora_summary.html

    if [[ "$REMORA_VERBOSE" == "1" ]]; then
        echo ""
        echo "REMORA: Starting REMORA finalize (in remora_finalize function)."
    fi
    END=$1
    START=$2
    local NODES_LIST=( $(cat $REMORA_OUTDIR/remora_nodes.txt) )
    local NODES="${NODES_LIST[@]}"
    local node_cnt=$( wc -w <<<$NODES)

    local remora_timeout=10

if [[ -z $REMORA_SNAPSHOT ]]; then #No need to check if snapshots  run: TMPDIR MUST be same as OUTDIR for snapshots
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

 #vvvv  The master zz.<node>  touched in the init_folders, an other nodes in remora_report.sh immediately.
 #      I don't think this is necessary, KFM.
    # Ensure all data has been copied over or issue warning
  if [[ 0 == 1 ]]; then
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
  fi
 #^^^^
fi

if [[ -z $REMORA_SNAPSHOT ]]; then   #If not snapshots (a remora run) do this
                                      #This is done at end of snapshots with .done_snaps_<node> files. 
    # Kill reporters running in background
    for SSH_NODE in $NODES; do
        ssh -q -f $SSH_NODE "pkill -f remora_report.sh"
    done
fi

    for NODE in $NODES; do
        SSH_NODE=$NODE
        [[ ! -z $REMORA_SSH_NODE     ]] && SSH_NODE=$REMORA_SSH_NODE

        CMD="$REMORA_BIN/scripts/remora_remote_post.sh $NODE $REMORA_OUTDIR $REMORA_BIN $REMORA_VERBOSE >>$REMORA_OUTDIR/.remora_out_$NODE  & "

        if [[ "$REMORA_VERBOSE" == "1" ]]; then
            echo "REMORA: launching remote postprocessing (plotting, etc) on $SSH_NODE for node $NODE"
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

    nodes_list=( ${NODES_LIST[@]} )
    [[ ! -z $REMORA_SSH_NODE     ]] && nodes_list=( $REMORA_SSH_NODE )
   #for NODE in ${NODES_LIST[@]}; do

    for NODE in ${nodes_list[@]}; do
        SSH_NODE=$NODE
        cnt=0
       #CMD="ps -ef |              grep scripts/remora_remote_post.sh | grep -wv grep | wc -l"
        CMD="ps -ef | grep $USER | grep scripts/remora_remote_post.sh | grep -wv grep | wc -l"

        while [[ `ssh -q $SSH_NODE $CMD` -gt 0 ]]; do
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

    if [[ ! -z $REMORA_SNAPS ]]; then  #if snapshot run, just use period*no_of_snapshots in date +%s%N format
      START=1000000000
      TM=$((REMORA_SNAPS*REMORA_PERIOD*1000000000)) 
      END=$((1000000000 + $TM))
    fi
    show_summary_report $END $START

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
        printf "%s \n" "<html lang=\"en\">" > $FILE_SUM_HTML
        printf "%s \n" "<head><title>REMORA TACC</title></head><body>" >> $FILE_SUM_HTML
        printf "%s \n" "<a href=\"https://github.com/TACC/remora\" target=\"_blank\"><img src=\"https://raw.githubusercontent.com/TACC/remora/master/docs/logos/Remora-logo-300px.png\" alt=\"REMORA Logo\" style=\"max-width:100%;\"></a>" >> $FILE_SUM_HTML
        printf "<h1>REMORA REPORT - JOB %s </h1>\n" "$REMORA_JOB_ID" >> $FILE_SUM_HTML
	printf "<pre>\n"   >> $FILE_SUM_HTML
	printf "  ** NOTICE: IO Module (lustre/vast) is not available on most TACC systems.\n" >> $FILE_SUM_HTML
	printf "  **         For security, root access is now required to extract IO data."    >> $FILE_SUM_HTML
	printf "</pre>\n"  >> $FILE_SUM_HTML
        for i in ${!REMORA_MODULES[@]}; do

          R_module="${REMORA_MODULES[$i]}"

          if [[ "$R_module" == "network" ]] && [[  $node_cnt -lt 2  ]]; then
               [[ "$REMORA_VERBOSE" == "1" ]] && echo " NETWORK Pt-2-Pt OUTPUT NOT INCLUDED, only 1 node." >> $VERB_FILE
          else
               [[ "$REMORA_VERBOSE" == "1" ]] && echo " Working on $R_module." >> $VERB_FILE

            if [[ "$R_module" == "gpu" ]] &&  [[ "$REMORA_GPU" == "0" ]]; then
               [[ "$REMORA_VERBOSE" == 1 ]] && echo "  GPU summary not reported, REMORA_GPU=0 or nvidia-smi not detected."
            else
               printf "<h2>%s utilization</h2> \n" ${REMORA_MODULES[$i]} >> $FILE_SUM_HTML
               ngpusM1=$(($REMORA_GPU_CNT - 1))

            fi

            if [[ "${REMORA_MODULES[$i]}" == "power" ]] && [[ $NodeCount -gt 1 ]]; then
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/power_aggregated.html" "Aggregated" >> $FILE_SUM_HTML
            fi

            if [[ "${REMORA_MODULES[$i]}" == "lustre" ]]; then
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/lustre_aggregated.html" "Aggregated" >> $FILE_SUM_HTML
            fi
            if [[ "${REMORA_MODULES[$i]}" == "impi" ]]; then
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/impi_fraction.html"  "Fraction"  >> $FILE_SUM_HTML
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/impi_breakdown.html" "Breakdown" >> $FILE_SUM_HTML
            fi
            if [[ "${REMORA_MODULES[$i]}" == "mv2" ]]; then
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/mv2_fraction.html"  "Fraction"  >> $FILE_SUM_HTML
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/mv2_breakdown.html" "Breakdown" >> $FILE_SUM_HTML
            fi
            if [[ "${REMORA_MODULES[$i]}" == "impi_mpip" ]]; then
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/impi_mpip_fraction.html"  "Fraction"  >> $FILE_SUM_HTML
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/impi_mpip_breakdown.html" "Breakdown" >> $FILE_SUM_HTML
            fi
            if [[ "${REMORA_MODULES[$i]}" == "ompi_mpip" ]]; then
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/ompi_mpip_fraction.html"  "Fraction"  >> $FILE_SUM_HTML
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/ompi_mpip_breakdown.html" "Breakdown" >> $FILE_SUM_HTML
            fi
            for node in $NODES; do

                if [[ -f  $REMORA_OUTDIR/${REMORA_MODULES_OUTDIRS[$i]}/${R_module}_${node}.html ]]; then
                    printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/${R_module}_${node}.html" ${node} >> $FILE_SUM_HTML
                fi

                if [[ "$R_module" == "gpu" ]] &&  [[ "$REMORA_GPU" == "1" ]]; then
                   for gid in `seq 0 $ngpusM1`; do
                     if [[ -f  $REMORA_OUTDIR/${REMORA_MODULES_OUTDIRS[$i]}/${R_module}_${node}_${gid}.html ]]; then
                     printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/${R_module}_${node}_${gid}.html" ${node}_GPUid-${gid} >> $FILE_SUM_HTML
                     fi
                   done
                fi

            done
          fi
        done

        #Add the summary at the end
        printf "<h2>Summary for job %s</h2>\n" "$REMORA_JOB_ID" >> $FILE_SUM_HTML 
        input="$REMORA_OUTDIR/INFO/remora_summary.txt"
        printf "<pre>\n"  >> $FILE_SUM_HTML
        while IFS= read -r line
        do
           #printf "<p>%s</p>\n" "$line" >> $FILE_SUM_HTML 
            printf "%s\n"        "$line" >> $FILE_SUM_HTML 
        done < "$input"

        printf "</pre>\n"  >> $FILE_SUM_HTML

        printf "%s \n" "</body></html>" >> $FILE_SUM_HTML
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
