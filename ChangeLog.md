# Change Log

## [Unreleased 1.4.0]

Improved the format of the output files. The columns have a fixed width so that they are easier to read.

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