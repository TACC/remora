Modified version of xltop that writes the output to a file. It only returns one event each time it is called.

First, you need to build the two libraries in the 'extra' folder (libconfuse, libev). Suppose you install it in $WORK/foo

To install it (Stampede):
  - Remove all the modules
  - export XLTOP_PORT=YOUR_OWN_PORT
  - export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:foo/lib
  - ./autogen.sh
  - ./configure CFLAGS="-Ifoo/include" LDFLAGS="-Lfoo/lib -lev -lconfuse" --prefix=XXX
  - make
  - make install

It needs SLURM_JOB_ID to be set to a given job id

Call it like this:
./xltop --master=master 
