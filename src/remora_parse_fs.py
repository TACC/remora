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
#-      version     REMORA 1.4
#-      authors     Carlos Rosales (carlos@tacc.utexas.edu)
#-                  Antonio Gomez  (agomez@tacc.utexas.edu)
#-      license     MIT
#
#========================================================================
#  HISTORY
#       2015/12/15: Doesn't use xltop. Instead, Remora creates a file
#                   for each node with the filesystem load
#       2015/09/09: Python implementation, handles heterogeneous file
#                   system entries in xltop.txt correctly
#       2015/08/12: Initial version
#========================================================================

import glob
import sys
from collections import defaultdict

if (len(sys.argv) != 2):
    print "Error: invalid number of parameters"
    print "This script needs the name of a folder as argument"
    sys.exit()

initialized=False
results=defaultdict(list)
header=list()
for filename in glob.iglob(sys.argv[1]+'/lustre_*'):
    with open(filename) as f:
        idx = 0
        for line in f:
            idx += 1
            if "TIMESTAMP" in line:
                #Only process the first line for the firs file that
                #is processed
                #This is how we collect the different filesystems in
                #the sytem
                if initialized:
                    continue
                initialized = True
                parts = line.split()
                for i in parts:
                    #We don't need the TIMESTAMP name
                    if "TIMESTAMP" in i:
                        continue
                    #Everything that it's not TIMESTAMP in the first
                    #line of the first file, is a filesystem. We append
                    #all the filesystems into the header list
                    header.append(i)
                continue
            parts = line.split()
            idx2=0
            #We now process each line. We have to skip the first
            #column (the timestamp)
            for i in parts:
                if (idx2==0):
                    idx2 += 1
                    continue
                #Now, add or append each value read to the appropriate
                #item in a list. 'results' is a dictionary, where the key
                #is the name of the filesystem (that's why use 'header[i]'
                #to access each element of the dictionary) and the elements
                #are lists
                if ((idx-2)>=len(results[header[idx2-1]])):
                    results[header[idx2-1]].append(int(i))
                else:
                    results[header[idx2-1]][idx-2] += int(i)
                idx2 += 1

#Now we simply format the matrix for a pretty output
out_header=""
numvals=0
max_load=list()
for i in results:
    out_header = out_header + i + "       "
    numvals=len(results[i])
    temp_max=0
    for j in xrange(numvals):
        if results[i][j] > temp_max:
            temp_max = results[i][j]
    max_load.append(temp_max)

fout = open(sys.argv[1]+"/fs_lustre_total.txt", "w")
fout.write(out_header+"\n")
for j in xrange(numvals):
    out_vals = ""
    for i in results:
        out_vals = out_vals + str(results[i][j]) + "       "
    fout.write(out_vals +"\n")
fout.close()

idx=0
for i in results:
    print "REMORA: MAX load in %10s: %10d" % (i, max_load[idx])
    idx += 1
