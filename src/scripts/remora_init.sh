#!/bin/bash
#
#========================================================================
# HEADER
#========================================================================
#% DESCRIPTION
#% remora_init
#%
#% DO NOT call this script directly. This is called by REMORA. 
#% This script loads basic functions, performs sanity checks, and sets up
#% the REMORA execution environment.
#%
#% remora_init.sh "$@"
#========================================================================
#- IMPLEMENTATION
#-      version     REMORA 1.6
#-      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#-                  Antonio Gomez  (agomez@tacc.utexas.edu)
#-      license     MIT
#========================================================================

# --- Load basic functions 
# --- Perform basic sanity checks

#Source the auxiliary scripts
source $REMORA_BIN/aux/extra
source $REMORA_BIN/aux/scheduler
source $REMORA_BIN/aux/sql_functions

# Make isure there are no multiple remora instances in each node
check_running_parallel

# Check that we have some arguments, and print the help if needed
parse_arguments "$@"

# --- Basic sanity checks passed
# --- Start data collection setup

# Get node list and JOB_ID
get_node_list

# Initialize output directories
init_folders

# Check REMORA specific environmental variables
parse_environment

# Store host list for later reference
write_node_list

# Need to check that the hostlist is not empty
check_hostlist_empty

# See if this is an MPI job
is_mpi_job "$@"

# See if GPU support is required
check_gpu

# Save REMORA specific environmental variables to a file
# Also capture runtime environment
capture_environment

# If CPU module is running we reduce sleep by 1 second
check_cpu

# Check if Lustre / DVS file systems are present
check_io

