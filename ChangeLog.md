# Change Log

## [1_8_4] - (2020-l0-19)
* MPI
  1.) Reporting MPI functions was fixed at 5. (Broke codes with less.) (Fixed)
      Changed to gathering top 20 or all greater than 0.5%.
  2.) Long jobs broke because mpiP stats returned time in sci. notation. (Fixed)
  3.) MPI fraction for mpiP now reports coefficient of variation.
  4.) Changed reporting values of MAX (%) to Average.
  5.) Fixed recent change to broke reporting memory in summary for multi-nodes runs

*NUMA
  1.) Incorrectly stated 4MB pages instead of 2MB. (Fixed)

*GPU
  1.) On dual system (nodes with/without GPU) report GPU is active, but not
      used on non-GPU nodes.
  2.) No longer necessary to set REMORA_CUDA to get GPU information on gpu nodes.
      Can turn off by setting REMORA_CUD to any value except 1.

*MEM
  1.) Summary now reports MAX VIRT, MAX PHYS and AVAIL at beginning of job.
  

## [1_8_3] - (2019-04-25)

### Codes Fixes
* numa
  1. Corrected 4MB to 2MB in description of pages.

* remora_finalize.sh
  1. Create local nodes array, NODES_LIST, for accessing node and pid simultaneous as list
     (e.g.  ${PID[$idx]} and ${NODES_LIST[$idx]}  in loop)
* install.sh
   1. config/modules must be changed from ib,NETWORK opa,NETWORK if OPA is detected

* scripts/remora_finalize.sh
   1. Include impi fraction and breakdown links under "impi utilization".
   2. impi_fraction.html and impi_breakdown.html links in remora_summary.html were not
   3. included. Now, it is included if impi is found in REMORA_MODULES[@] in remora_finalize.sh.
   4. (This works similar to including the lustre_aggregated.html under "lustre utilization".

* modules/numa
   1. numastats does not show THPs. 
   2. Replace misses with foreign hits--since it is the real miss. 
   3. Now uses vmstats to give Top-Right plot with allocated memory from thp and (thp+4k) pages. How to read text was placed at top.

* modules/opa
   1. Fixed modules/opa.  
   2. packs and bytes strings were never initialized.  pack was used in string names instead of packs.

* modules/network
   1. ibtracert not found in default path.  
   2. Added /sbin

* modules/ib
   1. was not set up to handle dev_hfi1_0 device for opa

* docs
   1. added modules_help and modules_whatis files for supporting mkmod



## [1_8_2] - (2017-08-08)

Small issue fix (#45). Also ensures that there are extra checks when no scheduler is present.

## [1_8_1] - (2017-06-30)

Several issues fixed (#37, #40, #41, #42, #43) and partial fix for issue #39.
For #39, the CPU collection is now correct when mpstats is not in the path of all nodes

## [1_8_0] - (2017-03-09)

Added MPI statistics collection for Intel MPI and Mvapich2
Modified how top level functions are called in order to improve overall handling
Improved verbose mode information

## [1_7_1] - (2016-12-07)

Now collection and post-processing work for users with default csh/tsh shells.

Thanks to Mark Reed (UNC) for the suggested changes.

## [1_7_0] - (2016-12-01)

Added power and temperature modules.
Added automatic discovery of NUMA nodes.
Supports PBS scheduler.
Supports running on a local machine (no scheduler needed).
Multiple jobs for the same run: new folders are created.
Use Google Charts to create plots automatically (no need for post-processing Python script except for network trace).
Added a script to monitor free memory that kill the application before the OOM does.
Added a script to generate post-processing files in case the node crashes.

Contributions from Jacob Pollack at Shell (Eth monitoring, CPU utilization post-processing, and 64-bit counters for Infiniband monitoring), Kevin Manalo (suggestion of automation of mlx device discovery) and wpoely86 (PBS support).

## [bp-1_6_0] - (2016-08-09)

Added "eth" module which tracks Ethernet statistics in a similar fashion to how "ib" module tracks InfiniBand statistics.
Added support for use of 64 bit InfiniBand counters if they exist.
Added average CPU usage information to CPU plots.

## [1_6_0] - (2016-03-11)

Introduced real-time monitoring capabilities, enabled by REMORA_MODE=MONITOR.
Fixed minor bug introduced in summary report warnings for systems with over 32 GB or memory per node.
Updated user documentation.

## [1_5_0] - (2016-02-16)

Improved performance by allowing the use of a separate REMORA_TMPDIR which can be a local file system.
Simplified "remora" script itself by having a remora_init / remora_collect / remora_finalize scripts.
New "src/scripts" directory to clean source structure.
Cleaned up output directory and improved summary formatting.
Using "remora_env.txt" file in shared location REMORA_OUTDIR to capture remora options during runtime.  
Added DVS file system data collection and file system blacklist.

## [1_4_0] - 2016-01-25

Improved the format of the output files. The columns have a fixed width so that they are easier to read.
Implemented a modular design, where all the data captured by REMORA is done by independent scripts in the
modules folder. A configuration file, that needs to be configured as part of the installation process,
specifies the active modules for a particular site.

## [1_3_1] - 2015-10-21

Changed remora_post to work with older versions of Python. It was only working with Python 2.7+

## [1_3_0] - 2015-09-19

Added memory monitoring for NVIDIA GPUs

## [1_2_0] - 2015-09-11

Minor bugfixes

## [1_1_0] - 2015-09-09

Fixed Lustre data parser

## [1_0_0] - 2015-08-18

First public release of REMORA (REsource MOnitoring for Remote Applications)
Remora is a tool to monitor runtime resource utilization:

* Memory
* CPU utilization
* Lustre usage
* NUMA memory
* Network topology

To use the tool, modify your batch script and include 'remora' before your script, executable, or MPI launcher.
