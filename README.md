# REMORA
REMORA: REsource MOnitoring for Remote Applications

Remora is a tool to monitor runtime resource utilization:
  - Memory
  - CPU utilization
  - IO usage (Lustre, DVS)
  - NUMA memory
  - Network topology

To use the tool, modify your batch script and include 'remora' before your script, executable, or MPI launcher.

Download and use
-------------------
Please, do not try to use the version available in the master branch. We regularly change the code and it might contain bugs. If you want to download and use remora, have a look at the different tags. [The most recent release can be found here] (https://github.com/TACC/remora/tree/v1.6.0).

Why name it Remora?
-------------------
Apart from a pretty cool acronym, this tools behaves a bit like the remora fish. It attaches to a larger fish (user proces) and travels with it wherever it goes, while offering very little in the way of resistance to the motion (overhead) as well as providing some benefits (resource usage information).

Do you use Remora?
-------------------
Remora is an open-source project. Funding to keep researchers working on Remora depends on the value of this tool to the scientific community. We would appreciate if you could include the following citation in your scientific articles:

C. Rosales, A. Gómez-Iglesias, A. Predoehl. "REMORA: a Resource Monitoring Tool for Everyone". HUST2015 November 15-20, 2015, Austin, TX, USA. DOI: 10.1145/2834996.2834999

Comments? Suggestions?
-------------------
Fell free to create new issues here in GitHub. You can also send us an email.
