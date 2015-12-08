#!/bin/sh
#
#========================================================================
# HEADER
#========================================================================
#% DESCRIPTION
#% extra
#%
#% DO NOT call this script directory. This is called by REMORA
#%
#% This script provides extra functionality used by REMORA
#========================================================================
#- IMPLEMENTATION
#-      version     REMORA 1.4
#-      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#-                  Antonio Gomez  (agomez@tacc.utexas.edu)
#-      license     MIT
#
#========================================================================
#  HISTORY
#       2015/12/04: Initial version
#========================================================================


usage() { printf '\033[0;33mREMORA Howto\n\033[0m'; head -${SCRIPT_HEADSIZE:-99} ${0} | grep -e "^#+" | sed -e "s/^#+[ ]*//g" -e "s/\${SCRIPT_NAME}/${SCRIPT_NAME}/g" ; }
usagefull() { head -${SCRIPT_HEADSIZE:-99} ${0} | grep -e "^#[%+-]" | sed -e "s/^#[%+-]//g" -e "s/\${SCRIPT_NAME}/${SCRIPT_NAME}/g" ; }
scriptinfo() { head -${SCRIPT_HEADSIZE:-99} ${0} | grep -e "^#-" | sed -e "s/^#-//g" -e "s/\${SCRIPT_NAME}/${SCRIPT_NAME}/g"; }
print_error() { printf '\033[0;31mREMORA Error: \033[0;34m '"$1"' \033[0m \n'; }

function show_time () {
  num=$1/1000000000
  ((milisec=($1/1000000)%1000))
  min=0
  hour=0
  day=0
  if((num>59));then
    ((sec=num%60))
    ((num=num/60))
    if((num>59));then
      ((min=num%60))
      ((num=num/60))
      if((num>23));then
        ((hour=num%24))
        ((day=num/24))
      else
        ((hour=num))
      fi
    else
      ((min=num))
    fi
  else
    ((sec=num))
  fi
  echo "REMORA: Total Elapsed Time       : "$day"d "$hour"h "$min"m "$sec"s "$milisec"ms"
}
