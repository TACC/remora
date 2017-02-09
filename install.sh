#!/bin/sh
# Installation script for REMORA
#
# Change CC, MPICC and the corresponding flags to match your own compiler in
# file "Makefile.in". You should not have to edit this file at all.
#
# v1.8 (2016-11-10)  Carlos Rosales-Fernandez
#                    Antonio Gomez-Iglesias
#
# Thanks to Kenneth Hoste, from HPC-UGent, for his input

# installation directory: use $REMORA_INSTALL_PREFIX if defined, current directory if not
export REMORA_DIR=${REMORA_INSTALL_PREFIX:-$PWD}
export PHI_BUILD=0

# Do not change anything below this line
#--------------------------------------------------------------------------------

mkdir -p $REMORA_DIR/bin
mkdir -p $REMORA_DIR/include
mkdir -p $REMORA_DIR/lib
mkdir -p $REMORA_DIR/share

REMORA_BUILD_DIR=$PWD

VERSION=1.8
COPYRIGHT1="Copyright 2017 The University of Texas at Austin."
COPYRIGHT2="License: MIT <http://opensource.org/licenses/MIT>"
COPYRIGHT3="This is free software: you are free to change and redistribute it."
COPYRIGHT4="There is NO WARRANTY of any kind"

BUILD_LOG="$REMORA_BUILD_DIR/remora_build.log"
INSTALL_LOG="$REMORA_BUILD_DIR/remora_install.log"

SEPARATOR="======================================================================"
PKG="Package  : REMORA"
VER="Version  : $VERSION"
DATE="Date     : `date +%Y.%m.%d`"
SYSTEM="System   : `uname -sr`"

# Record the local conditions for the compilation
echo
echo $SEPARATOR  | tee $BUILD_LOG
echo $PKG        | tee -a $BUILD_LOG
echo $VER        | tee -a $BUILD_LOG
echo $DATE       | tee -a $BUILD_LOG
echo $SYSTEM     | tee -a $BUILD_LOG
echo $SEPARATOR  | tee -a $BUILD_LOG
echo $COPYRIGHT1 | tee -a $BUILD_LOG
echo $COPYRIGHT2 | tee -a $BUILD_LOG
echo $COPYRIGHT3 | tee -a $BUILD_LOG
echo $COPYRIGHT4 | tee -a $BUILD_LOG
echo $SEPARATOR  | tee -a $BUILD_LOG
echo             | tee -a $BUILD_LOG

#Now build mpstat
echo "Building mpstat ..." | tee -a $BUILD_LOG
cd $REMORA_BUILD_DIR/extra
sysfile=`ls -ld sysstat*.tar.gz | awk '{print $9}' | head -n 1`
sysdir=`echo  ${sysfile%%.tar.gz}`
tar xzvf ${sysfile}
cd ${sysdir}
./configure | tee -a $BUILD_LOG
make mpstat |  tee -a $BUILD_LOG
echo "Installing mpstat ..." | tee -a $INSTALL_LOG
cp mpstat $REMORA_DIR/bin

#Now build mpiP
# Check if MPI compiler is available. If not, disable MPI module and skip mpiP build
export CC=mpicc
export F77=mpif77
export ARCH=x86_64
if [ "$( $CC --version >& /dev/null || echo "0")" == "0" ]; then 
    haveMPICC=0
else 
    haveMPICC=1
fi
if [ "$( $F77 --version >& /dev/null || echo "0")" == "0" ]; then
     haveMPIFC=0
else 
     haveMPIFC=1 
fi
#haveMPICC=1; haveMPICC=$( $CC --version >& /dev/null || echo "0" )
#haveMPIFC=1; haveMPIFC=$( $F77 --version >& /dev/null || echo "0" )
mpiexec --version | grep Intel >& /dev/null
haveIMPI=$((1-$?))
if [ "$haveMPICC" == "1" ] && [ "$haveMPIFC" == "1" ] && [ "$haveIMPI" == "0" ]; then
    echo " REMORA built with support for Mvapich2 MPI statistics" | tee -a $BUILD_LOG
    echo "Building mpiP ..." | tee -a $BUILD_LOG
    cd $REMORA_BUILD_DIR/extra
    mpipfile=`ls -ld mpiP*.tar.gz | awk '{print $9}' | head -n 1`
    mpipdir=`echo  ${mpipfile%%.tar.gz}`
    tar xzvf ${mpipfile}
    cd ${mpipdir}
    ./configure CFLAGS="-g" --enable-demangling --disable-bfd --disable-libunwind --prefix=${REMORA_DIR} | tee -a $BUILD_LOG
    make        | tee -a $BUILD_LOG
    make shared | tee -a $BUILD_LOG
    echo "Installing mpiP ..." | tee -a $INSTALL_LOG
    make install
elif [ "$haveMPICC" == "1" ] && [ "$haveMPIFC" == "1" ] && [ "$haveIMPI" == "1" ]; then
    echo "" 
    echo " REMORA built with support for Intel MPI statistics" | tee -a $BUILD_LOG
    echo ""
else
    echo ""
    echo " WARNING : mpicc / mpif77 not found " | tee -a $BUILD_LOG
    echo " WARNING : REMORA will be built without MPI support" | tee -a $BUILD_LOG
    echo ""
fi

if [ "$PHI_BUILD" == "1" ]; then
	echo "Building Xeon Phi affinity script ..."   |  tee -a $BUILD_LOG
	cd $REMORA_BUILD_DIR/extra/
	icc -mmic -o ma ./mic_affinity.c
	echo "Installing Xeon Phi affinity script ..." |  tee -a $INSTALL_LOG
	cp -v ./ma $REMORA_DIR/bin                     |  tee -a $INSTALL_LOG
fi

echo "Copying all scripts to installation folder ..." |  tee -a $INSTALL_LOG
cd $REMORA_BUILD_DIR
cp -vr ./src/* $REMORA_DIR/bin
if [ "$haveMPICC" == "0" ] || [ "$haveMPIFC" == "0" ]; then
    sed '/impi,MPI/d' $REMORA_DIR/bin/config/modules > $REMORA_DIR/remora.tmp
    sed '/mv2,MPI/d' $REMORA_DIR/remora.tmp > $REMORA_DIR/bin/config/modules
else
    if [ "$haveIMPI" == "1" ]; then
        sed '/mv2,MPI/d' $REMORA_DIR/bin/config/modules > $REMORA_DIR/remora.tmp
        mv $REMORA_DIR/remora.tmp $REMORA_DIR/bin/config/modules
	else
        sed '/impi,MPI/d' $REMORA_DIR/bin/config/modules > $REMORA_DIR/remora.tmp
        mv $REMORA_DIR/remora.tmp $REMORA_DIR/bin/config/modules
    fi
fi

echo $SEPARATOR
echo "Installation of REMORA v$VERSION completed."
echo "For a fully functional installation make sure to:"; echo ""
echo "	export PATH=\$PATH:$REMORA_DIR/bin"
echo "	export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:$REMORA_DIR/lib"
echo "	export REMORA_BIN=$REMORA_DIR/bin"; echo ""
echo "Good Luck!"
echo $SEPARATOR
