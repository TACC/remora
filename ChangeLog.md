# Change Log

## [1.6.0] - (2016-03-11)

Introduced real-time monitoring capabilities, enabled by REMORA_MODE=MONITOR.
Fixed minor bug introduced in summary report warnings for systems with over 32 GB or memory per node.
Updated user documentation.

## [1.5.0] - (2016-02-16)

Improved performance by allowing the use of a separate REMORA_TMPDIR which can be a local file system.
Simplified "remora" script itself by having a remora_init / remora_collect / remora_finalize scripts.
New "src/scripts" directory to clean source structure.
Cleaned up output directory and improved summary formatting.
Using "remora_env.txt" file in shared location REMORA_OUTDIR to capture remora options during runtime.  
Added DVS file system data collection and file system blacklist.

## [1.4.0] - 2016-01-25

Improved the format of the output files. The columns have a fixed width so that they are easier to read.
Implemented a modular design, where all the data captured by REMORA is done by independent scripts in the
modules folder. A configuration file, that needs to be configured as part of the installation process,
specifies the active modules for a particular site.

## [1.3.1] - 2015-10-21

Changed remora_post to work with older versions of Python. It was only working with Python 2.7+

## [1.3.0] - 2015-09-19

Added memory monitoring for NVIDIA GPUs

## [1.2.0] - 2015-09-11

Minor bugfixes

## [1.1.0] - 2015-09-09

Fixed Lustre data parser

## [1.0.0] - 2015-08-18

First public release of REMORA (REsource MOnitoring for Remote Applications)
Remora is a tool to monitor runtime resource utilization:

* Memory
* CPU utilization
* Lustre usage
* NUMA memory
* Network topology

To use the tool, modify your batch script and include 'remora' before your script, executable, or MPI launcher.
