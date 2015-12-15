#!/usr/bin/env python
#
#========================================================================
# HEADER
#========================================================================
#% DESCRIPTION
#% remora_parse_fs
#%
#% DO NOT call this script directory. This is a postprocessing
#% tool called by REMORA
#%
#========================================================================
#- IMPLEMENTATION
#-      version     REMORA 0.1
#-      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#-                  Antonio Gomez  (agomez@tacc.utexas.edu)
#-      license     MIT
#
#========================================================================
#  HISTORY
#	2015/09/09: Python implementation, handles heterogeneous file
#                   system entries in xltop.txt correctly
#       2015/08/12: Initial version
#========================================================================

import os.path

if not os.path.isfile("xltop.txt"):
    return 0

# Define the file system names
hfs = "stampede-home"
sfs = "stampede-scratch"
wfs = "stockyard-work"
# File header
hdr = "#TIME(S)\tWR_MB/S\t\tRD_MB/S\t\tREQS/S\n"
# To begin with, we have no reported activity on any FS
fs_present = [0, 0, 0]
fs_max     = [0.0, 0.0, 0.0]
fs_loc     = [20,20,20]
# Available Lustre file systems and associated output file names
fs      = [hfs, sfs, wfs]
fs_name = ['home.txt', 'scratch.txt', 'work.txt']
# Placeholder for the actual files
fs_file = [1,2,3]

# process IO file line by line
with open("xltop.txt","r") as file:
    for line in file:
        columns = line.split()
        for i in range(0,len(fs)):
            fs_loc[i] = 20
            for idx in range( 0, len(columns) ):
                if columns[idx] == fs[i]:
                    fs_loc[i] = idx
            if fs_loc[i] < 20:
                if fs_present[i] == 0:
                    fs_file[i] = open( fs_name[i], "a" )
                    fs_file[i].write( hdr )
                    fs_present[i] = 1
                fs_str = "%s\t%s\t%s\t%s\n" % (columns[0],columns[fs_loc[i]+1],columns[fs_loc[i]+2],columns[fs_loc[i]+3])
                fs_file[i].write( fs_str )
                if float(columns[fs_loc[i]+3]) > fs_max[i]:
                    fs_max[i] = float(columns[fs_loc[i]+3])

# Close the files
for i in range(0,len(fs)):
	if fs_present[i] == 1:
		fs_file[i].close()
		fs_max[i] = "%4.2f" % float(fs_max[i])
	else:
		fs_max[i] = "-.--"

# Report max IOPS values
print "REMORA: MDS Load (IO REQ/S)      : %s (HOME) / %s (WORK) / %s (SCRATCH)" % (fs_max[0],fs_max[2],fs_max[1])
