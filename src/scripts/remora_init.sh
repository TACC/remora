#!/bin/bash
#
#========================================================================
# HEADER
#========================================================================
# DESCRIPTION
# remora_init
#
# DO NOT call this script directly. This is called by REMORA. 
# This script loads basic functions, performs sanity checks, and sets up
# the REMORA execution environment.
#
# remora_init "$@"
#========================================================================
# IMPLEMENTATION
#      version     REMORA 1.8
#      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#                  Antonio Gomez  (agomez@tacc.utexas.edu)
#      license     MIT
#========================================================================

# --- Load basic functions 
# --- Perform basic sanity checks

#Source the auxiliary scripts
source $REMORA_BIN/aux/extra
source $REMORA_BIN/aux/scheduler
source $REMORA_BIN/aux/sql_functions

function remora_init() {
    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "REMORA initialization started"
    fi
    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "  Checking if parallel execution"
    fi
    # Make sure there are no multiple remora instances in each node
    check_running_parallel

    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "  Parsing arguments"
    fi
    # Check that we have some arguments, and print the help if needed
    parse_arguments "$@"

    # --- Basic sanity checks passed
    # --- Start data collection setup

    # Get node list and JOB_ID
    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "  Obtaining list of nodes"
    fi
    get_node_list

    # Initialize output directories
    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "  Initializing folders"
    fi
    init_folders

    # Check REMORA specific environmental variables
    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "  Parsing environment"
    fi
    parse_environment

    # Store host list for later reference
    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "  Writting list of nodes"
    fi
    write_node_list

    # Need to check that the hostlist is not empty
    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "  Checking hostlist"
    fi
    check_hostlist_empty

    # See if this is an MPI job
    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "  Checking if MPI job"
    fi
    is_mpi_job "$@"

    # Save REMORA specific environmental variables to a file
    # Also capture runtime environment
    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "  Saving configuration"
    fi
    capture_environment

    # If CPU module is running we reduce sleep by 1 second
    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "  Checking CPU module"
    fi
    check_cpu

    # Check if we are plotting the results
    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "  Checking if plotting is required"
    fi
    check_plot

    # See if GPU support is required
    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "  Checking if GPU support is required"
    fi
    check_gpu

    # Check if Lustre / DVS file systems are present
    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "  Checking if parallel file system analysis is required"
    fi
    check_io

    if [ "$REMORA_VERBOSE" == "1" ]; then
        echo "REMORA initialization finished"
    fi
}
