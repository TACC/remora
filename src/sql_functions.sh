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

REMORADB=$HOME/.remora.db
REMORAVERSION=1.4

check_db_version()
{
    sqlite3 $REMORADB "SELECT version FROM remora_info";
}

check_db_exists()
{
    if [ -f $REMORADB ];
    then
        return 1
    else
        return 0
    fi
}

create_database()
{
    sqlite3 $REMORADB "CREATE TABLE remora_info (version INTEGER, date TEXT)";
    sqlite3 $REMORADB "INSERT INTO remora_info VALUES ($REMORAVERSION, `date +'%s'`)";
    sqlite3 $REMORADB "CREATE TABLE jobs (id INTEGER PRIMARY KEY, timestamp TEXT, commmand TEXT, path TEXT";
    sqlite3 $REMORADB "CREATE TABLE memory_usage(FOREIGN KEY(job_id) REFERENCES jobs(id), time TEXT, shmem TEXT, virtual TEXT, resident TEXT";
}

# This function will need to be implemented: if the version of the database is older than
# the current version used, some tables will have changed.
update_database()
{
}
