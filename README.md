# remora
REMORA: REsource MOnitoring for Remote Applications

Remora is a tool to monitor runtime resource utilization:
  - Memory
  - CPU utilization
  - Lustre usage
  - NUMA memory
  - Network topology

To use the tool, modify your batch script and include 'remora' before your script, executable, or MPI launcher.

Why name it Remora?
-------------------
Apart from a pretty cool acronym, this tools behaves a bit like the remora fish. It attaches to a larger fish (user proces) and travels with it wherever it goes, while offering very little in the way of resistance to the motion (overhead) as well as providing some benefits (resource usage information).

