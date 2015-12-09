#!/bin/sh
#
#========================================================================
# HEADER
#========================================================================
#% DESCRIPTION
#% scheduler
#%
#% DO NOT call this script directory. This is called by REMORA
#%
#% This script provides extra functionality to interact with
#% the scheduler used by REMORA
#========================================================================
#- IMPLEMENTATION
#-      version     REMORA 1.4
#-      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#-                  Antonio Gomez  (agomez@tacc.utexas.edu)
#-      license     MIT
#
#========================================================================
#  HISTORY
#       2015/12/08: Initial version
#========================================================================

get_node_list()
{
    # Check the scheduler system used and define host list
    if [ -n "$SLURM_NODELIST" ]; then
        REMORA_SCHEDULER=slurm
        REMORA_JOB_ID=$SLURM_JOB_ID
        NODES=`scontrol show hostname $SLURM_NODELIST`
    elif [ -n "$PE_HOSTFILE" ]; then
        REMORA_SHCEDULER=sge
        REMORA_JOB_ID=$JOB_ID
        NODES=`cat $PE_HOSTFILE | awk '{print $1}' | uniq`
    else
        print_error "Unknown scheduler - unable to generate host list"
        exit 1
    fi
}


# Need to check that the hostlist is not empty
check_hostlist_empty()
{
    node_cnt=0; for i in $NODES; do node_cnt=$(( node_cnt + 1 )); done
    if [ "$node_cnt" == "0"  ];then
        print_error "Host list is empty"
        exit 1
    fi
}
