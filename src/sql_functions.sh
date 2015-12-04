#!/bin/bash

#========================================================================
# HEADER
#========================================================================
#% DESCRIPTION
#% sql_functions
#%
#% This file implements a set of auxiliary functions for managing SQLITE
#% databases
#%
#========================================================================
#- IMPLEMENTATION
#-      version     REMORA 1.4
#-      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#-                  Antonio Gomez  (agomez@tacc.utexas.edu)
#-      license     MIT
#
#========================================================================
#  HISTORY
#       2015/11/29: Initial version
#========================================================================
#

# Location of the REMORA database
REMORADB=$HOME/.remora.db

# Current version of the database
REMORAVERSION=1.4

# Function that returns the version stored in the main table of the database
check_db_version()
{
    version=sqlite3 $REMORADB "SELECT version FROM remora_info";
    return version
}

# Function that returns a boolean indicating whether the database already exists
check_db_exists()
{
    if [ -f $REMORADB ];
    then
        return 1
    else
        return 0
    fi
}

# This function creates the database for REMORA. It assumes that the database doesn't
# already exist
# For version 1.4, the tables are the following (this is documented in the REMORA
# repository (https://github.com/TACC/remora):
#
#           remora_info                         jobs
#                                                |
#         ___________________________________________________________________________________
#           |                |                   |               |              |           |
#         numa          memory_usage        cpu_usage       filesystem      network         IB
create_database()
{
    sqlite3 $REMORADB "CREATE TABLE remora_info (version INTEGER, date INTEGER)";
    sqlite3 $REMORADB "INSERT INTO remora_info VALUES ($REMORAVERSION, `date +'%s'`)";
    sqlite3 $REMORADB "CREATE TABLE jobs (id INTEGER PRIMARY KEY, time_start INTEGER, time_end INTEGER, commmand TEXT, path TEXT, nodes TEXT)";
    sqlite3 $REMORADB "CREATE TABLE memory_usage (FOREIGN KEY(job_id) REFERENCES jobs(id), timestamp INTEGER, shmem TEXT, virtual TEXT, resident TEXT)";
    sqlite3 $REMORADB "CREATE TABLE cpu_usage (FOREIGN KEY(job_id) REFERENCES jobs(id), timestamp INTEGER PRIMARY KEY, cpu_id INTEGER PRIMARY KEY, usage REAL)";
    sqlite3 $REMORADB "CREATE TABLE filesystem (FOREIGN KEY(job_id) REFERENCES jobs(id), timestamp INTEGER, fs_name TEXT, requests INTEGER)";
    # Do we want to have a table with all the nodes in the system?
    # And with all the switches?
    sqlite3 $REMORADB "CREATE TABLE network (FOREIGN KEY(job_id) REFERENCES jobs(id))"; # Finish this
    sqlite3 $REMORADB "CREATE TABLE ib (FOREIGN KEY(job_id) REFERENCES jobs(id))";  # Finish this
    sqlite3 $REMORADB "CREATE TABLE numa_usage (FOREIGN KEY(job_id) REFERENCES jobs(id), timestamp INTEGER PRIMARY KEY, node_id INTEGER PRIMARY KEY, local_mem REAL, remote_mem REAL, local_miss REAL, remote_miss REAL)";
}

# This functions takes as arguments the following values:
#   - job_id
#   - timestamp
#   - shmem
#   - virtual
#   - resident
insert_memory_usage() {
    sqlite3 $REMORADB "INSERT INTO cpu_usage (job_id, timestamp, shmem, virtual, resident) VALUES ($1, $2, '$3', '$4', $5)";
}

# This functions takes as arguments the following values:
#   - job_id
#   - timestamp
#   - fs_name
#   - cpu_id
#   - usage
insert_cpu_usage() {
    sqlite3 $REMORADB "INSERT INTO cpu_usage (job_id, timestamp, fs_name, cpu_id, usage) VALUES ($1, $2, '$3', '$4', $5)";
}

# This functions takes as arguments the following values:
#   - job_id
#   - timestamp
#   - filesystem name (HOME, WORK, SCRATCH,...)
#   - requests
insert_memory_usage() {
    sqlite3 $REMORADB "INSERT INTO filesystem (job_id, timestamp, fsname, requests) VALUES ($1, $2, '$3', '$4')";
}

# This functions takes as arguments the following values:
#   - job_id
#   - timestamp
#   - node_id (name of the node)
#   - local_mem (mem used in local memory)
#   - remote_mem (mem used in remote remory)
#   - local_miss (misses in local memory)
#   - remote_miss (misses in remote memory)
insert_numa_usage() {
    if [ "$#" -ne 1 ]; then
        echo "REMORA
    else
        sqlite3 $REMORADB "INSERT INTO numa_usage (job_id, timestamp, node_id, local_mem, remote_mem, local_miss, remote_miss) VALUES ($1, $2, '$3', $4, $5, $6, $7)";
    fi
}

# This function will need to be implemented: if the version of the database is older than
# the current version used, some tables will have changed.
#update_database()
#{
#
#}
