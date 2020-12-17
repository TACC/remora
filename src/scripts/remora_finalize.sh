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
#      version     REMORA 1.8.4
#      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#                  Antonio Gomez  (agomez@tacc.utexas.edu)
#      license     MIT
#========================================================================

# --- Show a final report on screen

source $REMORA_BIN/aux/extra
source $REMORA_BIN/aux/report

source $REMORA_OUTDIR/remora_env.txt

function remora_finalize() {

    if [[ "$REMORA_VERBOSE" == "1" ]]; then
        echo ""
        echo "REMORA: Starting REMORA finalize (in remora_finalize function)."
    fi
    END=$1
    START=$2

    local remora_timeout=120

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
        sleep 1
    done

    if [[ "$waiting" -ge $remora_timeout ]]; then
        printf "\n*** REMORA: WARNING - Slow file system response. Post-processing may be incomplete\n\n"
        printf "*** REMORA: WARNING - %s out of %s nodes successfully processed\n" "$completed" "$NodeCount"
    fi

    # Kill remote remora processes running in the backgroud

    for NODE in $NODES; do

        ssh -q -f $NODE "pkill -f remora_report.sh"
        if [[ "$REMORA_SYMMETRIC" == "1" ]]; then
            ssh -q -f $NODE-mic0 "pkill -f remora_report_mic.sh"
        fi  

        COMMAND="$REMORA_BIN/scripts/remora_remote_post.sh $NODE $REMORA_OUTDIR $REMORA_BIN $REMORA_VERBOSE >>$REMORA_OUTDIR/.remora_out_$NODE  & "
        if [[ "$REMORA_VERBOSE" == "1" ]]; then
            echo "REMORA: launching remote postprocessing (plotting, etc)"
            echo "ssh -q -n $NODE $COMMAND"
        fi  
        #Right now this is putting the command in the background and continuing 
        #(so the remora can finish, therefore epilog might  #kill everything! We need to fix it

        ssh -q -n $NODE $COMMAND

    done

    if [[ "$REMORA_VERBOSE" == "1" ]]; then
        echo ""
        echo " REMORA: Will wait for postprocesses (remora_remorte_post.sh) to finish:"
    fi

    #Wait until all remora_remote_post processes have finished

    local NODES_TMP="$NODES"
    local NODES_LIST=( $NODES ) #make array list

    for NODE in ${NODES_LIST[@]}; do
        cnt=0
        CMD="ps -ef | grep scripts/remora_remote_post.sh | grep -wv grep | wc -l"

        while [[ `ssh $NODE $CMD` -gt 0 ]]; do
            sleep 0.05
            if [ "$REMORA_VERBOSE" == "1" ]; then
                [[ $cnt -eq 0 ]] && printf "Post processing waiting on Node # $cnt"
                [[ $cnt -ne 0 ]] && printf "."
            fi
            cnt=$((cnt+1))
        done
        printf "\n" 
    done

    sleep 0.5 #Add a small delay so that files are transferred (deal with Lustre latency)

    if [[ "$REMORA_VERBOSE" == "1" ]]; then
        echo ""
        echo "REMORA: All REMORA postprocesses have finished"
    fi

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

    remora_read_active_modules

    #Move output files to their folders based on the configuration file
    #If some files are missing, don't output the error message
    for i in "${!REMORA_MODULES[@]}"; do
        if [[ "$REMORA_VERBOSE" == "1" ]]; then
            echo "REMORA: Moving output files for ${REMORA_MODULES[$i]}"
        fi
        mv $REMORA_OUTDIR/${REMORA_MODULES[$i]}* $REMORA_OUTDIR/${REMORA_MODULES_OUTDIRS[$i]} 2> /dev/null
        [[ ${REMORA_MODULES[$i]} == "eth" ]] &&
             mv $REMORA_OUTDIR/network_eth_traffic* $REMORA_OUTDIR/${REMORA_MODULES_OUTDIRS[$i]} 2> /dev/null
        sleep 0.2
    done

    if [[ "$REMORA_VERBOSE" == "1" ]]; then
        echo "REMORA: Generating base HTML file"
    fi
    #We simply create an HTML file with links to all the different results
    if [[ "$REMORA_PLOT_RESULTS" != "0" ]] ; then
        printf "%s \n" "<html lang=\"en\">" > $REMORA_OUTDIR/remora_summary.html
        printf "%s \n" "<head><title>REMORA TACC</title></head><body>" >> $REMORA_OUTDIR/remora_summary.html
        printf "%s \n" "<a href=\"https://github.com/TACC/remora\" target=\"_blank\"><img src=\"https://raw.githubusercontent.com/TACC/remora/master/docs/logos/Remora-logo-300px.png\" alt=\"REMORA Logo\" style=\"max-width:100%;\"></a>" >> $REMORA_OUTDIR/remora_summary.html
        printf "<h1>REMORA REPORT - JOB %s </h1>\n" "$REMORA_JOB_ID" >> $REMORA_OUTDIR/remora_summary.html
        for i in "${!REMORA_MODULES[@]}"; do

            R_module="${REMORA_MODULES[$i]}"

            if [[ "$R_module" == "gpu" ]] &&  [[ "$REMORA_CUDA" == "0" ]]; then
               [[ "$REMORA_VERBOSE" == 1 ]] && echo "  GPU summary not reported, REMORA_CUDA=0 or nvidia-smi not detected."
            else
               printf "<h2>%s utilization</h2> \n" ${REMORA_MODULES[$i]} >> $REMORA_OUTDIR/remora_summary.html
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
        if [[ "$REMORA_VERBOSE" == "1" ]]; then
            echo "REMORA: Handling MONITOR files"
        fi
        rm $REMORA_TMPDIR/.monitor
        mv $REMORA_OUTDIR/monitor* $REMORA_OUTDIR/MONITOR/
    fi

    # Clean up TMPDIR if necessary
    if [[ "$REMORA_TMPDIR" != "$REMORA_OUTDIR" ]]; then
        if [[ "$REMORA_VERBOSE" == "1" ]]; then
            echo "REMORA: Removing $REMORA_TMPDIR"
        fi
        rm -rf $REMORA_TMPDIR
    fi

    #Clean the zz files (files used to make sure all transfers have finished)
    rm -f $REMORA_OUTDIR/zz.*

    if [[ "$REMORA_VERBOSE" == "1" ]]; then
        echo ""
        echo "REMORA finalize finished"
    fi
}
