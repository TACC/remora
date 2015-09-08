#!/bin/bash
#
#========================================================================
# HEADER
#========================================================================
#% DESCRIPTION
#% remora_parse_fs
#%
#% DO NOT call this script directory. This is a postprocessing
#% tool called by REMORA
#%
#========================================================================
#- IMPLEMENTATION
#-      version     REMORA 0.1
#-      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#-                  Antonio Gomez  (agomez@tacc.utexas.edu)
#-      license     MIT
#
#========================================================================
#  HISTORY
#       2015/08/12: Initial version
#========================================================================

hfs=stampede-home
sfs=stampede-scratch
wfs=stockyard-work

fs=()
fs_loc=()

fs[1]=$(cat $1 | awk 'NR==2 {print $1}')
fs[2]=$(cat $1 | awk 'NR==2 {print $5}')
fs[3]=$(cat $1 | awk 'NR==2 {print $9}')

fail=0
if [ "x${fs[1]}" == "x" ]; then
  fail=$((fail+1))
fi
if [ "x${fs[2]}" == "x" ]; then
  fail=$((fail+1))
fi
if [ "x${fs[3]}" == "x" ]; then
  fail=$((fail+1))
fi
fscount=$((3-fail))

for i in `seq 1 $fscount`
do
  if [ "x${fs[$i]}" != "x" ]; then
    if [ "x${fs[$i]}" == "x$hfs" ]; then
      fs_loc[$i]=$(echo "4+($i-1)*4" | bc)
    fi
    if [ "x${fs[$i]}" == "x$wfs" ]; then
      fs_loc[$i]=$(echo "4+($i-1)*4" | bc)
    fi
    if [ "x${fs[$i]}" == "x$sfs" ]; then
      fs_loc[$i]=$( echo "4+($i-1)*4" | bc)
    fi
  fi
done


hreqs="-.--"
wreqs="-.--"
sreqs="-.--"
for i in `seq 1 $fscount`
do
  if [ "x${fs[$i]}" == "x$hfs" ]; then
    hreqs=$(awk -v idx="${fs_loc[$i]}" ' NR == 2 {max=$idx; min=$idx} NR > 2 && $idx > max {max=$idx} END {printf "%4.2f\n",max}' $1)
  fi
  if [ "x${fs[$i]}" == "x$wfs" ]; then
    wreqs=$(awk -v idx="${fs_loc[$i]}" ' NR == 2 {max=$idx; min=$idx} NR > 2 && $idx > max {max=$idx} END {printf "%4.2f\n",max}' $1)
  fi
  if [ "x${fs[$i]}" == "x$sfs" ]; then
    sreqs=$(awk -v idx="${fs_loc[$i]}" ' NR == 2 {max=$idx; min=$idx} NR > 2 && $idx > max {max=$idx} END {printf "%4.2f\n",max}' $1)
  fi
done

echo "TACC: MDS Load (IO REQ/S)      : $hreqs (HOME) / $wreqs (WORK) / $sreqs (SCRATCH)"
