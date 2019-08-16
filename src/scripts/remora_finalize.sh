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
#      version     REMORA 1.8.3
#      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#                  Antonio Gomez  (agomez@tacc.utexas.edu)
#      license     MIT
#========================================================================

# --- Show a final report on screen

source $REMORA_BIN/aux/extra
source $REMORA_BIN/aux/report
source $REMORA_OUTDIR/remora_env.txt

# PRELOAD is not needed and may cause problem.
unset LD_PRELOAD

function remora_finalize() {
    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo ""
        echo "REMORA: Starting REMORA finalize"
    fi
    END=$1
    START=$2

    local remora_timeout=120

    # Copy data from temporary location to output dir
    # This assumes OUTDIR is in a shared location
    if [ "$REMORA_TMPDIR" != "$REMORA_OUTDIR" ]; then
        for NODE in $NODES
            do
                if [ "$REMORA_VERBOSE" == "1" ]; then
                    echo "REMORA: Copying files from temporary location to output folder"
                    echo "scp $NODE:$REMORA_TMPDIR/* $REMORA_OUTDIR"
                fi  
                scp $NODE:$REMORA_TMPDIR/* $REMORA_OUTDIR 2> /dev/null 1> /dev/null
            done
    fi

    # Ensure all data has been copied over or issue warning
    NodeCount=`wc -l $REMORA_OUTDIR/remora_nodes.txt | awk '{print $1}'`
    waiting=1; completed=0
    while [ "$waiting" -lt $remora_timeout ] && [ "$completed" -lt "$NodeCount" ]; do
        completed=0
        for node in $NODES; do
            if [ -a $REMORA_OUTDIR/zz.$node ]; then
                completed=$((completed+1))
            fi  
        done
        sleep 1
    done

    if [ "$waiting" -ge $remora_timeout ]; then
        printf "\n*** REMORA: WARNING - Slow file system response. Post-processing may be incomplete\n\n"
        printf "*** REMORA: WARNING - %s out of %s nodes successfully processed\n" "$completed" "$NodeCount"
    fi

    # Kill remote remora processes running in the backgroud
    PID=(); PID_MIC=(); FINAL_PID=()
    idx=0; for elem in `cat $REMORA_OUTDIR/remora_pid.txt`; do PID[$idx]=$elem; idx=$((idx+1)); done
    idx=0; for elem in `cat $REMORA_OUTDIR/remora_pid_mic.txt`; do PID_MIC[$idx]=$elem; idx=$((idx+1)); done
    idx=0
    for NODE in $NODES; do
        REMORA_NODE_ID=$idx
        ssh -f $NODE 'kill '${PID[$idx]} 
        if [ "$REMORA_SYMMETRIC" == "1" ]; then
            ssh -q -f $NODE-mic0 'kill '${PID_MIC[$idx]}
        fi  
        COMMAND="$REMORA_BIN/scripts/remora_remote_post.sh $NODE $REMORA_OUTDIR $REMORA_BIN $REMORA_VERBOSE $REMORA_NODE_ID >> $REMORA_OUTDIR/.remora_out_$NODE  &  echo \$! "
        if [ "$REMORA_VERBOSE" == "1" ]; then
            echo "REMORA: launching remote postprocessing (plotting, etc)"
            echo "ssh -q -n $NODE $COMMAND"
        fi  
        #Right now this is putting the command in the background and continuing (so the remora can finish, therefore epilog might  #kill everything! We need to fix it
        FINAL_PID+=(`ssh -q -n $NODE $COMMAND | tail -n 1`)
        idx=$((idx+1))
    done

    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo ""
        echo "REMORA: Waiting for postprocesses to finish"
    fi

    #Wait until all remora_remote_post processes have finished

    idx=0
    for pid in "${FINAL_PID[@]}"; do
        NODE=${NODES[$idx]}
        while [ `ssh $NODE "[ -e /proc/$pid ] && echo 1"` ]; do
            sleep 0.05
            if [ "$REMORA_VERBOSE" == "1" ]; then
                printf "."
            fi
        done
        idx=$((idx+1))
    done

    #Add a small delay so that files are transferred (deal with Lustre latency)
    sleep 0.5

    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo ""
        echo "REMORA: All REMORA postprocesses have finished"
    fi

    rm $REMORA_OUTDIR/remora_pid.txt
    rm $REMORA_OUTDIR/remora_pid_mic.txt

    # Clean up the instance of remora summary running on the master node
    if [ "$REMORA_MODE" == "MONITOR" ]; then
        idx=0; PID_MON=()
        for elem in `cat $REMORA_OUTDIR/remora_pid_mon.txt`; do PID_MON[$idx]=$elem; idx=$((idx+1)); done
        idx=0   
        for NODE in $NODES; do
            ssh -f $NODE 'kill '${PID_MON[$idx]} 
            idx=$((idx+1))
        done
        rm $REMORA_OUTDIR/remora_pid_mon.txt
    fi

    show_final_report $END $START

    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "REMORA: Cleaning. Moving everything to the correct place"
    fi

    sleep 1.0 

    rm -f $REMORA_OUTDIR/*.tmp

    # Should write name-based loop
    mv $REMORA_OUTDIR/{remora*,runtime*} $REMORA_OUTDIR/INFO

    source $REMORA_BIN/aux/extra
    source $REMORA_BIN/modules/modules_utils
       # PRELOAD is not needed and may cause problem.
    unset LD_PRELOAD
    remora_read_active_modules

    #Move output files to their folders based on the configuration file
    #If some files are missing, don't output the error message
    for i in "${!REMORA_MODULES[@]}"; do
        if [ "$REMORA_VERBOSE" == "1" ]; then
            echo "REMORA: Moving output files for ${REMORA_MODULES[$i]}"
        fi
        mv $REMORA_OUTDIR/${REMORA_MODULES[$i]}* $REMORA_OUTDIR/${REMORA_MODULES_OUTDIRS[$i]} 2> /dev/null
        sleep 0.2
    done

    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "REMORA: Generating base HTML file"
    fi
    #We simply create an HTML file with links to all the different results
    if [ "$REMORA_PLOT_RESULTS" != "0" ] ; then
        printf "%s \n" "<html lang=\"en\">" > $REMORA_OUTDIR/remora_summary.html
        printf "%s \n" "<head><title>REMORA TACC</title></head><body>" >> $REMORA_OUTDIR/remora_summary.html
        printf "%s \n" "<a href=\"https://github.com/TACC/remora\" target=\"_blank\"><img src=\"https://raw.githubusercontent.com/TACC/remora/master/docs/logos/Remora-logo-300px.png\" alt=\"REMORA Logo\" style=\"max-width:100%;\"></a>" >> $REMORA_OUTDIR/remora_summary.html
        printf "<h1>REMORA REPORT - JOB %s </h1>\n" "$REMORA_JOB_ID" >> $REMORA_OUTDIR/remora_summary.html
        for i in "${!REMORA_MODULES[@]}"; do
            printf "<h2>%s utilization</h2> \n" ${REMORA_MODULES[$i]} >> $REMORA_OUTDIR/remora_summary.html
            if [ "${REMORA_MODULES[$i]}" == "lustre" ]; then
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/lustre_aggregated.html" "Aggregated" >> $REMORA_OUTDIR/remora_summary.html
            fi
            if [ "${REMORA_MODULES[$i]}" == "impi" ]; then
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/impi_fraction.html"  "Fraction"  >> $REMORA_OUTDIR/remora_summary.html
                printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/impi_breakdown.html" "Breakdown" >> $REMORA_OUTDIR/remora_summary.html
            fi
            for node in $NODES; do
                if [ -f  $REMORA_OUTDIR/${REMORA_MODULES_OUTDIRS[$i]}/${REMORA_MODULES[$i]}_${node}.html ]; then
                    printf "<a href="%s" target="_blank">%s</a><p/>\n" "${REMORA_MODULES_OUTDIRS[$i]}/${REMORA_MODULES[$i]}_${node}.html" ${node} >> $REMORA_OUTDIR/remora_summary.html
                fi
            done
        done
        #Add the summary at the end
        printf "<h2>Summary for job %s</h2>\n" "$REMORA_JOB_ID" >> $REMORA_OUTDIR/remora_summary.html 
        input="$REMORA_OUTDIR/INFO/remora_summary.txt"
        while IFS= read -r line
        do
            printf "<p>%s</p>\n" "$line" >> $REMORA_OUTDIR/remora_summary.html 
        done < "$input"

        printf "%s \n" "</body></html>" >> $REMORA_OUTDIR/remora_summary.html
    fi

    #Continue after generating the HTML file
    if [ "$REMORA_MODE" == "MONITOR" ]; then
        if [ "$REMORA_VERBOSE" == "1" ]; then
            echo "REMORA: Handling MONITOR files"
        fi
        rm $REMORA_TMPDIR/.monitor
        mv $REMORA_OUTDIR/monitor* $REMORA_OUTDIR/MONITOR/
    fi

    # Clean up TMPDIR if necessary
    if [ "$REMORA_TMPDIR" != "$REMORA_OUTDIR" ]; then
        if [ "$REMORA_VERBOSE" == "1" ]; then
            echo "REMORA: Removing $REMORA_TMPDIR"
        fi
        rm -rf $REMORA_TMPDIR
    fi

    #Clean the zz files (files used to make sure all transfers have finished)
    rm -f $REMORA_OUTDIR/zz.*

    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo ""
        echo "REMORA finalize finished"
    fi
}
