#!/bin/sh
#========================================================================
# HEADER
#========================================================================
#% DESCRIPTION
#% remora_summary
#%
#% DO NOT call this script directory. This is a postprocessing tool
#% used by REMORA
#%
#% remora_summary.sh NODE_NAME OUTDIR SYMMETRIC TACC_REMORA_CUDA
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
#

currenthost=$1
vmax_mem=`tail -n 1 $2/mem_stats_$currenthost.txt | awk '{printf "%6.4f\n",$2}'`
tmax_mem=`tail -n 1 $2/mem_stats_$currenthost.txt | awk '{printf "%6.4f\n",$8}'`
free_mem=$(awk ' NR == 1 {max=$7; min=$7} NR > 1 && $7 < min {min=$7} END {printf "%6.4f\n",min }' $2/mem_stats_$currenthost.txt)
echo "$currenthost $vmax_mem $tmax_mem $free_mem" >> $2/mem_all_nodes.txt

if [ "$3" == "1" ]; then
	currenthost=$1-mic0
	vmax_mem=`tail -n 1 $2/mem_stats_$currenthost.txt | awk '{printf "%6.4f\n",$2}'`
	tmax_mem=`tail -n 1 $2/mem_stats_$currenthost.txt | awk '{printf "%6.4f\n",$8}'`
	free_mem=$(awk ' NR == 1 {max=$7; min=$7} NR > 1 && $7 < min {min=$7} END {printf "%6.4f\n",min }' $2/mem_stats_$currenthost.txt)
	echo "$currenthost $vmax_mem $tmax_mem $free_mem" >> $2/mem_all_nodes_mic.txt
fi

if [ "$4" == "1" ]; then
    currenthost=$1-gpu
    max_mem=$(awk ' NR == 1 {max=$2; min=$2} NR > 1 && $2 > max {max=$2} END {printf "%6.4f\n",max }' $2/mem_stats_$currenthost.txt)
    free_mem=$(awk ' NR == 1 {max=$3; min=$3} NR > 1 && $3 < min {min=$3} END {printf "%6.4f\n",min }' $2/mem_stats_$currenthost.txt)
    echo "$currenthost $max_mem $free_mem" >> $2/mem_all_nodes_gpu.txt
fi
