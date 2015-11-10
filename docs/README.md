=== REMORA: REsource MOnitoring for Remote Applications ===

Remora is a tool to monitor runtime resource utilization:
  - Memory (CPU, Xeon Phi, GPU)
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

It is also possible to get plots of those files for an easier analysis. Use the tool
'remora_post'. Within the batch script, 'remora_post' does not need any parameter.
From the login node, you can cd to the location that contains the remora_JOBID folder, 
and once there execute 'remora_post -j JOBID'.

The following environment variables control the behavior of the tool:

  - REMORA_PERIOD  - How often memory usage is checked. Default is 10 seconds.
  - REMORA_VERBOSE - Verbose mode will save all information to a file. Default is 0 (off).
  - REMORA_MODE    - FULL (default) for all stats, BASIC for memory and cpu only.
  
  
=== Installation ===
If you intend to install remora outside TACC, please keep in mind that we are still 
working on its portability. Remora depends on a modified version of xltop that writes 
output directly to a file in order to collect IO data.

Use the provided install.sh script to install REMORA. Modify the variables
XLTOP_PORT, REMORA_DIR and PHI_BUILD at the top of the script so that they 
point to the port xltop should be listening in, to the installation directory, 
and select if a Xeon Phi build is necessary (1) or not (0).

The installation script will create build and log files that you can check
in case anything fails during hte installation process.

=== AUTHORS ===

2015-10-21 \ Carlos Rosales-Fernandez \ carlos@tacc.utexas.edu
2015-10-21 \ Antonio Gomez-Iglesias   \ agomez@tacc.utexas.edu
