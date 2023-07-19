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
#-      version     REMORA 2.0
#      authors     Carlos Rosales ( carlos@tacc.utexas.edu)
#                  Antonio Gomez  ( agomez@tacc.utexas.edu)
#      custodian   Kent Milfeld   (milfeld@tacc.utexas.edu)
#      license     MIT
#========================================================================

# --- Load basic functions 
# --- Perform basic sanity checks

#Source the auxiliary scripts
source $REMORA_BIN/aux/extra
source $REMORA_BIN/aux/scheduler
source $REMORA_BIN/aux/sql_functions

function remora_init() {
    [[ "$REMORA_VERBOSE" == "1" ]] && echo "REMORA initialization started"

    # Make sure there are no multiple remora instances in each node
    [[ "$REMORA_VERBOSE" == "1" ]] && echo "  Checking if parallel execution"
    check_running_parallel

    # Check that we have some arguments, and print the help if needed
    [[ "$REMORA_VERBOSE" == "1" ]] && echo "  Parsing arguments"
    parse_arguments "$@"

    # --- Basic sanity checks passed
    # --- Start data collection setup

    # Get node list and JOB_ID
    [[ "$REMORA_VERBOSE" == "1" ]] && echo "  Obtaining list of nodes"
    get_node_list

    # Initialize output directories
    [[ "$REMORA_VERBOSE" == "1" ]] && echo "  Initializing folders"
    init_folders   #also executes remora_set_active_modules

    # Check REMORA specific environmental variables
    [[ "$REMORA_VERBOSE" == "1" ]] && echo "  Parsing environment"
    parse_environment

    # Store host list for later reference
    [[ "$REMORA_VERBOSE" == "1" ]] && echo "  Writting list of nodes"
    write_node_list

    # Need to check that the hostlist is not empty
    [[ "$REMORA_VERBOSE" == "1" ]] && echo "  Checking hostlist"
    check_hostlist_empty

    # See if this is an MPI job
    [[ "$REMORA_VERBOSE" == "1" ]] && echo "  Checking if MPI job"
    is_mpi_job "$@"

    # xxxxSave REMORA specific environmental variables to a file
    # Also capture runtime environment
    [[ "$REMORA_VERBOSE" == "1" ]] && echo "  Saving configuration"
    capture_environment

    # Determine active modules from default configuration directory
    # or user specified (REMORA_CONFIG_PATH) directory.
    # User may specify subset of module in REMORA_MODULES env var)
    [[ "$REMORA_VERBOSE" == "1" ]] && echo "  Set active module in REMORA_ACTIVE_MODULES env var."

    # If CPU module is running we reduce sleep by 1 second
    [[ "$REMORA_VERBOSE" == "1" ]] && echo "  Checking CPU module"
    check_cpu

    # Check if we are plotting the results
    [[ "$REMORA_VERBOSE" == "1" ]] && echo "  Checking if plotting is required"
    check_plot

    # See if GPU support is required
    [[ "$REMORA_VERBOSE" == "1" ]] && echo "  Checking if GPU support is required"
    check_gpu

    # Check if Lustre / DVS file systems are present
    [[ "$REMORA_VERBOSE" == "1" ]] && echo "  Checking if parallel file system analysis is required/available."
    check_io

    ## Check TIMER precision (date precision) and if REMORA_DATE_PRECISION is set
    #if [[ "$REMORA_VERBOSE" == "1" ]]; then
    #    echo "  Checking date precision (timer) & if REMORA_DATE_PRECISION is set"
    #fi
    #check_date_precision

    [[ "$REMORA_VERBOSE" == "1" ]] && echo "REMORA initialization finished"
}
