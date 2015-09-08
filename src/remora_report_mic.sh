#!/bin/sh
#========================================================================
# HEADER
#========================================================================
#% DESCRIPTION
#% remora_report_mic
#%
#% DO NOT call this script directory. This is called by REMORA
#%
#% remora_report_mic.sh NODE_NAME OUTDIR TACC_REMORA_PERIOD SYMMETRIC TACC_REMORA_MODE TACC_REMORA_BIN
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


#This will pin the collection to the very last core
$6/ma

SYMMETRIC=$4

vmem_max_old=0
rmem_max_old=0
tmem_max=0

USER=`whoami`
echo "#TIME VMEM_MAX VMEM RMEM_MAX RMEM SHMEM MEM_FREE TMEM_MAX" > $2/mem_stats_$1.txt
while [ 1 ]
do
   current_time=`date +%s`

    #Memory statistics
    #Get space used in /dev/shm
    shmem_used=`du /dev/shm 2>/dev/null | tail -n 1 | awk '{print $1} '`
    shmem=$(echo $shmem_used | awk '{ printf "%6.4f",$1/(1024.0*1024.0)}' )
    
    mem_free=`grep MemFree /proc/meminfo | awk '{ print $2/1024/1024 }'`
    
    vmem_max=$(for i in `ps -u $USER | awk 'NR > 1 {print $1}'`; do cat /proc/$i/status 2> /dev/null | grep VmPeak ; done | awk '{sum+=$2} END {printf "%6.4f\n",sum/1024/1024}') 
    new_max=$(echo "$vmem_max $vmem_max_old" | awk '{res=0} $1 > $2 {res=1} END {print res}')
    if [ "$new_max" -eq 1 ]; then
      vmem_max_old=$vmem_max
    fi
    vmem_max=$vmem_max_old
    vmem=$(for i in `ps -u $USER | awk 'NR > 1 {print $1}'`; do cat /proc/$i/status 2> /dev/null | grep VmSize ; done | awk '{sum+=$2} END {printf "%6.4f\n",sum/1024/1024}') 
    
    rmem_max=$(for i in `ps -u $USER | awk 'NR > 1 {print $1}'`; do cat /proc/$i/status 2> /dev/null | grep VmHWM ; done | awk '{sum+=$2} END {printf "%6.4f\n",sum/1024/1024}')
    new_max=$(echo "$rmem_max $rmem_max_old" | awk '{res=0} $1 > $2 {res=1} END {print res}')
    if [ "$new_max" -eq 1 ]; then
      rmem_max_old=$rmem_max
    fi
    rmem_max=$rmem_max_old
    rmem=$(for i in `ps -u $USER | awk 'NR > 1 {print $1}'`; do cat /proc/$i/status 2> /dev/null | grep VmRSS ; done | awk '{sum+=$2} END {printf "%6.4f\n",sum/1024/1024}')

    tmem=$(echo "$rmem $shmem" | awk '{printf "%6.4f\n",$1+$2}')
    new_max=$(echo "$tmem $tmem_max" | awk '{res=0} $1 > $2 {res=1} END {print res}')
    if [ "$new_max" -eq 1 ]; then
        tmem_max=$tmem
    fi

	echo $current_time $vmem_max $vmem $rmem_max $rmem $shmem $mem_free $tmem_max >> $2/mem_stats_$1.txt

  sleep $3

done
