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
    sqlite3 $REMORADB "CREATE TABLE cpu_usage (FOREIGN KEY(job_id) REFERENCES jobs(id), timestamp INTEGER PRIMARY KEY, cpud_id INTEGER PRIMARY KEY, usage REAL)";
    sqlite3 $REMORADB "CREATE TABLE filesystem (FOREIGN KEY(job_id) REFERENCES jobs(id), timestamp INTEGER, fs_name TEXT, requests INTEGER)";
    # Do we want to have a table with all the nodes in the system?
    # And with all the switches?
    sqlite3 $REMORADB "CREATE TABLE network (FOREIGN KEY(job_id) REFERENCES jobs(id))"; # Finish this
    sqlite3 $REMORADB "CREATE TABLE ib (FOREIGN KEY(job_id) REFERENCES jobs(id))";  # Finish this
    sqlite3 $REMORADB "CREATE TABLE numa (FOREIGN KEY(job_id) REFERENCES jobs(id), timestamp INTEGER PRIMARY KEY, node INTEGER PRIMARY KEY, local_mem REAL, remote_mem REAL, local_miss REAL, remote_miss REAL)";
}

# This function will need to be implemented: if the version of the database is older than
# the current version used, some tables will have changed.
update_database()
{
}