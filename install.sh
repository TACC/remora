#!/bin/sh
# Installation script for REMORA
#
# Change CC, MPICC and the corresponding flags to match your own compiler in
# file "Makefile.in". You should not have to edit this file at all.
#
# v1.0 (2015-08-25)  Carlos Rosales-Fernandez
#                    Antonio Gomez-Iglesias

export REMORA_DIR=`pwd`
export XLTOP_PORT=XXXX
export PHI_BUILD=0

# Do not change anything below this line 
#--------------------------------------------------------------------------------

mkdir -p $REMORA_DIR/bin
mkdir -p $REMORA_DIR/include
mkdir -p $REMORA_DIR/lib
mkdir -p $REMORA_DIR/share
mkdir -p $REMORA_DIR/python

REMORA_BUILD_DIR=`pwd`

VERSION=1.0
COPYRIGHT1="Copyright 2015 The University of Texas at Austin."
COPYRIGHT2="License: MIT <http://opensource.org/licenses/MIT>"
COPYRIGHT3="This is free software: you are free to change and redistribute it."
COPYRIGHT4="There is NO WARRANTY of any kind"

BUILD_LOG="$REMORA_BUILD_DIR/remora_build.log"
INSTALL_LOG="$REMORA_BUILD_DIR/remora_install.log"

SEPARATOR="======================================================================"
PKG="Package  : REMORA"
VER="Version  : 1.0"
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

# Build libraries in extra directory
echo "Building auxiliary libraries for xltop ..." |  tee -a $BUILD_LOG
echo "Building CONFUSE library ..."               |  tee -a $BUILD_LOG
cd $REMORA_BUILD_DIR/extra 
tar xzf ./confuse*.tar.gz 
cd confuse* 
./configure --prefix=$REMORA_DIR                    | tee -a $BUILD_LOG
make                                                | tee -a $BUILD_LOG
echo "Installing auxiliary libraries for xltop ..." | tee -a $INSTALL_LOG
echo "Installing CONFUSE library ..."               | tee -a $INSTALL_LOG
make install                                        | tee -a $INSTALL_LOG

echo "Building LIBEV library ..."                   |  tee -a $BUILD_LOG
cd $REMORA_BUILD_DIR/extra
tar xzf ./libev*.tar.gz
cd libev*
./configure --prefix=$REMORA_DIR                    | tee -a $BUILD_LOG
make                                                | tee -a $BUILD_LOG
echo "Installing auxiliary libraries for xltop ..." | tee -a $INSTALL_LOG
echo "Installing LIBEV library ..."                 | tee -a $INSTALL_LOG
make install                                        | tee -a $INSTALL_LOG


echo "Building XLTOP library ..."                   |  tee -a $BUILD_LOG
cd $REMORA_BUILD_DIR/extra/xltop/source
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$REMORA_DIR/lib
./autogen.sh
./configure CFLAGS="-I$REMORA_DIR/include" LDFLAGS="-L$REMORA_DIR/lib -lev -lconfuse" --prefix=$REMORA_DIR | tee -a $BUILD_LOG
make                         | tee -a $BUILD_LOG
echo "Installing XLTOP ..."  | tee -a $INSTALL_LOG
make install                 | tee -a $INSTALL_LOG

#Now build mpstat
echo "Building mpstat ..." | tee -a $BUILD_LOG
cd $REMORA_BUILD_DIR/extra
git clone https://github.com/sysstat/sysstat | tee -a $BUILD_LOG
cd sysstat
./configure | tee -a $BUILD_LOG
make mpstat |  tee -a $BUILD_LOG
echo "Installing mpstat ..."
cp mpstat $REMORA_DIR/bin

if [ "$PHI_BUILD" == "1" ]; then
	echo "Building Xeon Phi affinity script ..."   |  tee -a $BUILD_LOG
	cd $REMORA_BUILD_DIR/extra/
	icc -mmic -o ma ./mic_affinity.c
	echo "Installing Xeon Phi affinity script ..." |  tee -a $INSTALL_LOG
	cp -v ./ma $REMORA_DIR/bin                     |  tee -a $INSTALL_LOG
fi

echo "Copying all scripts to installation folder ..." |  tee -a $INSTALL_LOG
cd $REMORA_BUILD_DIR
cp -v ./src/* $REMORA_DIR/bin

echo "Installing python module blockdiag ..." | tee -a $INSTALL_LOG
module load python
pip install blockdiag --target=$REMORA_DIR/python

echo $SEPARATOR
echo "Installation of REMORA v$VERSION completed."
echo "For a fully functional installation make sure to:"; echo ""
echo "	export PATH=\$PATH:$REMORA_DIR/bin"
echo "	export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:$REMORA_DIR/lib"
echo "	export PYTHONPATH=\$PYTHONPATH:$REMORA_DIR/python"
echo "	export TACC_REMORA_BIN=$REMORA_DIR/bin"; echo ""
echo "Good Luck!"
echo $SEPARATOR
