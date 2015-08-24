=== REMORA: REsource MOnitoring for Remote Applications ===

Remora is a tool to monitor runtime resource utilization:
  - Memory
  - CPU utilization
  - Lustre usage
  - NUMA memory
  - Network topology

To use the tool, modify your batch script and include 'remora' before your script, 
executable, or MPI launcher.

=== USE EXAMPLES ===

Examples for utilization in Stampede @ TACC (after "module load remora"):

1. Parallel execution
...
#SBATCH -n 16
#SBATCH -A my_project

remora ibrun my_parallel_program [arguments]

2. Sequential execution (may be threaded)
...
#SBATCH -n 1
#SBATCH -A my_project
remora ./my_program [arguments]

Remora will create a folder with a number of files that contain the values for the 
resources indicated above.

It is also possible to get plots of those files for an easier analysis. Use the tool 'job
statsplot'. Within the batch script, 'jobstatsplot' does not need any parameter. From the 
login node, you can cd to the location that contains the remora_JOBID folder, and once t
here execute 'remoraplot JOBID'.

The following environment variables control the behaviour of the tool:

  - REMORA_PERIOD  - How often memory usage is checked. Default is 10 seconds.
  - REMORA_VERBOSE - Verbose mode will save all information to a file. Default is 0 (off).
  - REMORA_MODE    - FULL (default) for all stats, BASIC for memory and cpu only.
  
  
=== Installation ===
If you intend to install remora outside TACC, please keep in mind that we are still 
working on its portability. Remora depends on a modified version of xltop that writes 
output directly to a file in order to collect IO data.

==== Step 1: Build dependencies ====
First, build the two libraries in the 'extra' folder (libconfuse, libev). Assuming you 
install them in $WORK/foo you can install the modified xltop version by following these
steps:

% module purge
% export XLTOP_PORT=YOUR_OWN_PORT
% export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:foo/lib
% ./autogen.sh
% ./configure CFLAGS="-Ifoo/include" LDFLAGS="-Lfoo/lib -lev -lconfuse" --prefix=MY_PREFIX
% make
% make install

Note that xltop needs SLURM_JOB_ID to be set to a given job id

Call it like this:
./xltop --master=master 

If you intend to use Xeon Phi co-processors you must also compile a small C code that 
will pin the background process in the Phi to the last core to avoid excessive overhead 
during data collection. To do this:

% cd extras
% gcc ./mic_affinity.c -o ma
% mv ./ma MY_PREXFIX

==== Step 2: Copy all necessary scripts to the final destination ====

cp remor* MY_PREFIX

=== CHANGELOG ===
Revision 1.0: (2015-08-18) First public release of REMORA

2015-08-18 \ Carlos Rosales-Fernandez \ carlos@tacc.utexas.edu
2015-08-18 \ Antonio Gomez-Iglesias   \ agomez@tacc.utexas.edu
