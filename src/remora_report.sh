#!/bin/sh
#
#========================================================================
# HEADER
#========================================================================
#% DESCRIPTION
#% remora_report
#%
#% DO NOT call this script directory. This is called by REMORA
#%
#% remora_report.sh NODE_NAME OUTDIR TACC_REMORA_PERIOD SYMMETRIC TACC_REMORA_MODE TACC_REMORA_CUDA
#========================================================================
#- IMPLEMENTATION
#-      version     REMORA 1.1
#-      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#-                  Antonio Gomez  (agomez@tacc.utexas.edu)
#-      license     MIT
#
#========================================================================
#  HISTORY
#       2017/08/12: Initial version
#       2017/08/25: Version 1.1. Improved output format
#========================================================================

#
# rmax_mem : maximum resident memory
# tmax_mem : maximum total memory (sum of rmem and shmem)
# vmax_mem : maximum virtual memory

SYMMETRIC=$4
TACC_REMORA_CUDA=$6

vmem_max_old=0
rmem_max_old=0
tmem_max=0

if [ "$SYMMETRIC" == "0" ]; then
  if [ "$5" = "FULL" ]; then
    PACKAGES=`cat /sys/class/infiniband/mlx4_0/ports/1/counters/port_xmit_packets`

    #Lustre counters
    printf "%-17s %-17s %-17s %-17s %-17s %-17s %-17s %-17s %-17s %-17s %-17s %-17s\n" "time" "msgs_alloc" "msgs_max" "errors" "send_count" "recv_count" "route_count" "drop_count" "send_length" "recv_length" "route_length" "drop_length" > $2/lustre_counters-$1.txt
#    echo "time msgs_alloc msgs_max errors send_count recv_count route_count drop_count send_length recv_length route_length drop_length" > $2/lustre_counters-$1.txt
  fi
fi

USER=`whoami`
printf "%-17s %-17s %-17s %-17s %-17s %-17s %-17s %-17s\n" "#TIME" "VMEM_MAX" "VMEM" "RMEM_MAX" "RMEM" "SHMEM" "MEM_FREE" "TMEM_MAX"> $2/mem_stats_$1.txt
#echo "#TIME VMEM_MAX VMEM RMEM_MAX RMEM SHMEM MEM_FREE TMEM_MAX" > $2/mem_stats_$1.txt
printf "%-17s %-17s %-17s %-17s %-17s %-17s %-17s %-17s %-17s %-17s %-17s %-17s %-17s\n" "#TIME" "Node0_NumaHit" "Node0_NumaMiss" "Node0_LocalNode" "Node0_OtherNode" "Node0_MemFree" "Node0_MemUsed" "Node1_NumaHit" "Node1_NumaMiss" "Node1_LocalNode" "Node1_OtherNode" "Node1_MemFree" "Node1_MemUsed" > $2/numa_stats_$1.txt
while [ 1 ]
do

    #Get a timestamp in seconds for this data sample
    current_time=`date +%s`
    # NUMA statistics
    numStat=`numastat -c`
    node0_numahit=`echo $numStat | awk '{ print $17; }'` #Memory successfully allocated on this node as intended
    node1_numahit=`echo $numStat | awk '{ print $16; }'`
    node0_numamiss=`echo $numStat | awk '{ print $19; }'` #Memory allocated on this node, although the process preferred another
    node1_numamiss=`echo $numStat | awk '{ print $20; }'`
    node0_numalocal=`echo $numStat | awk '{ print $31; }'` #Memory allocated on this node while the process was running on it
    node1_numalocal=`echo $numStat | awk '{ print $32; }'`
    node0_numaother=`echo $numStat | awk '{ print $35; }'` #Memory allocated on this node while the process was running on another node
    node1_numaother=`echo $numStat | awk '{ print $36; }'`
    numMem=`numastat -m | grep Mem`
    node0_memfree=`echo $numMem | awk '{ print $6; }'`  #Memory available on this node
    node1_memfree=`echo $numMem | awk '{ print $7; }'`
    node0_memused=`echo $numMem | awk '{ print $10; }'` #Memory used on this node
    node1_memused=`echo $numMem | awk '{ print $11; }'`
    printf "%-17d %-17d %-17d %-17d %-17d %-17f %-17f %-17d %-17d %-17d %-17d %-17f %-17f\n" $current_time $node0_numahit $node0_numamiss $node0_numalocal $node0_numaother $node0_memfree $node0_memused $node1_numahit $node1_numamiss $node1_numalocal $node1_numaother $node1_memfree $node1_memused >> $2/numa_stats_$1.txt
#    echo $current_time $node0_numahit $node0_numamiss $node0_numalocal $node0_numaother $node0_memfree $node0_memused $node1_numahit $node1_numamiss $node1_numalocal $node1_numaother $node1_memfree $node1_memused >> $2/numa_stats_$1.txt
    
    # Memory statistics
    #Get space used in /dev/shm
    shmem_used=`du /dev/shm 2>/dev/null | tail -n 1 | awk '{print $1} '`
    shmem=$(echo "scale=4; $shmem_used/(1024*1024)" | bc)
    
    mem_free=`grep MemFree /proc/meminfo | awk '{ print $2/1024/1024 }'`
    vmem_max=$(for i in `ps -u $USER | awk 'NR > 1 {print $1}'`; do cat /proc/$i/status 2> /dev/null | grep VmPeak ; done | awk '{sum+=$2} END {print sum/1024/1024}') 
    if [ $(echo " $vmem_max > $vmem_max_old" | bc) -eq 1 ]; then
      vmem_max_old=$vmem_max
    fi
    vmem_max=$vmem_max_old
    vmem=$(for i in `ps -u $USER | awk 'NR > 1 {print $1}'`; do cat /proc/$i/status 2> /dev/null | grep VmSize ; done | awk '{sum+=$2} END {print sum/1024/1024}') 
    rmem_max=$(for i in `ps -u $USER | awk 'NR > 1 {print $1}'`; do cat /proc/$i/status 2> /dev/null | grep VmHWM ; done | awk '{sum+=$2} END {print sum/1024/1024}')
    if [ $(echo " $rmem_max > $rmem_max_old" | bc) -eq 1 ]; then
      rmem_max_old=$rmem_max
    fi
    rmem_max=$rmem_max_old
    rmem=$(for i in `ps -u $USER | awk 'NR > 1 {print $1}'`; do cat /proc/$i/status 2> /dev/null | grep VmRSS ; done | awk '{sum+=$2} END {print sum/1024/1024}')

    tmem=$(echo "$rmem + $shmem" | bc)
    if [ $(echo "$tmem > $tmem_max" | bc) -eq 1 ]; then
        tmem_max=$tmem
    fi

    printf "%-17d %-17f %-17f %-17f %-17f %-17f %-17f %-17f\n" $current_time $vmem_max $vmem $rmem_max $rmem $shmem $mem_free $tmem_max >> $2/mem_stats_$1.txt
#    echo $current_time $vmem_max $vmem $rmem_max $rmem $shmem $mem_free $tmem_max >> $2/mem_stats_$1.txt

if [ "$SYMMETRIC" == "0" ]; then
  if [ "$5" = "FULL" ]; then
    NEWPACKAGES=`cat /sys/class/infiniband/mlx4_0/ports/1/counters/port_xmit_packets`
    printf "%d %10d\n" $current_time $((NEWPACKAGES-PACKAGES)) >> $2/ib_xmit_packets-$1.txt
#    echo $current_time $((NEWPACKAGES-PACKAGES)) >> $2/ib_xmit_packets-$1.txt
    PACKAGES=$NEWPACKAGES


    #Lustre counters
    #msgs_alloc msgs_max errors send_count recv_count route_count drop_count send_length recv_length route_length drop_length
    #   msgs_alloc   ->  Number of currently active messages.
    #   msgs_max     ->  Highwater of msgs_alloc.
    #   errors       ->  Unused.
    #   send_count   ->  Messages sent.
    #   recv_count   ->  Messages dropped.
    #   route_count  ->  Only used on routers? Unused?
    #   drop_count   ->  Messages dropped
    #   send_length  ->  Bytes sent.
    #   recv_length  ->  Bytes received.
    #   route_length ->  Bytes of routed messages.  Routers only? Unused?
    #   drop_length  ->  Bytes dropped
    lnetstats=`cat /proc/sys/lnet/stats`
    printf "%-17d %-17d %-17d %-17d %-17d %-17d %-17d %-17d %-17d %-17d %-17d %-17d\n" $current_time $lnetstats >> $2/lustre_counters-$1.txt
#    echo $current_time $lnetstats >> $2/lustre_counters-$1.txt
  fi

  #Get CPU utilization
mpstat -P ALL 1 1 | grep Average| awk '
{ 
    for (i=2; i<=NF; i++)  {
        a[NR,i] = $i
    }
}
NF>p { p = NF }
END {
    print " %time " systime();
    for(j=3; j<=p; j++) {
        if (j==2 || j==3 || j==5 || j==11) {
            str=""
        for(i=1; i<=NR; i++){
                if (i!=2) {
            str=str" "a[i,j];
            }
        }
        print str
        }
    }
}' >> $2/cpu-$1.txt  
fi

# Get GPU utilization data
if [ "$TACC_REMORA_CUDA" == "1" ]; then
	gpumem=$(nvidia-smi | grep MiB | awk '{print $9}'); gpumem=${gpumem::-3}
	gpumax=$(nvidia-smi | grep MiB | awk '{print $11}'); gpumax=${gpumax::-3}
	gpumem=$(echo $gpumem | awk '{print $1/1000}')
	gpumax=$(echo $gpumax | awk '{print $1/1000}')
	gpufree=$(echo "$gpumax $gpumem" | awk '{ print $1-$2 }')
	echo "$current_time $gpumem $gpufree" >> $2/mem_stats_$1-gpu.txt
fi

  sleep $3

done
