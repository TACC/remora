#!/bin/bash
# Installation script for REMORA
#
# Change CC, MPICC and the corresponding flags to match your own compiler in
# file "Makefile.in". You should not have to edit this file at all.
#
# v2_0   (2023-06-29)  Albert Lu & Kent Milfeld
# v1_8_4 (2020-10-19)  Kent Milfeld
# v1_8_3 (2018-04-25)  Kent Milfeld/Si Liu
# v1_8_2 (2017-08-08)  Carlos Rosales-Fernandez
#                      Antonio Gomez-Iglesias
#
# Thanks to Kenneth Hoste, from HPC-UGent, for his input
# Changed to use mpiP for intel 2019 and above  (impi_mpiP) (Kent)

# installation directory: use $REMORA_INSTALL_PREFIX if defined, current directory if not
export REMORA_DIR=${REMORA_INSTALL_PREFIX:-$PWD}
export PHI_BUILD=0

# Do not change anything below this line
#--------------------------------------------------------------------------------

REMORA_BUILD_DIR=$PWD

if [[ -d $REMORA_BUILD_DIR/src/C_data_collectors_src ]] &&  [[ "x0" != "x$REMORA_BINARIES" ]]; then
   REMORA_BINARIES=1
else
   REMORA_BINARIES=0
fi

mkdir -p $REMORA_DIR/bin
mkdir -p $REMORA_DIR/include
mkdir -p $REMORA_DIR/lib
mkdir -p $REMORA_DIR/share
mkdir -p $REMORA_DIR/docs

VERSION=2.0
COPYRIGHT1="Copyright 2023 The University of Texas at Austin."
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
#sysfile=`ls -ld sysstat*.tar.gz | awk '{print $9}' | head -n 1`
sysfile=`ls sysstat*.tar.gz`
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

if [[ "$( $CC --version > /dev/null 2>&1 || echo "0")" == "0" ]]; then 
    haveMPICC=0
else 
    haveMPICC=1
fi

if [[ "$( $F77 --version > /dev/null 2>&1 || echo "0")" == "0" ]]; then
     haveMPIFC=0
else 
     haveMPIFC=1 
fi

if [[ $haveMPICC == 1 ]] && [[ $haveMPIFC == 1 ]]; then

   mpicc -v 2>/dev/null | grep 'Intel(R) MPI' > /dev/null 2>&1 
   haveIMPI=$((1-${PIPESTATUS[1]}))           #1=has IMPI, 0=does not have IMPI

   mpicc -v 2>/dev/null | grep 'MVAPICH2'     > /dev/null 2>&1 
   haveMV2=$((1-${PIPESTATUS[1]}))            #1=has MVAPICH2, 0=does not have MV2

   if [[ $haveIMPI == 1 ]]; then
      IMPI_year=$( mpicc -v 2>/dev/null | grep 'Library 20' | sed 's/.*Library 20\([0-9][0-9]\).*/\1/' )
      if [[ $IMPI_year > 18 ]]; then
         IMPI_stats="impi_mpip"
      else
         IMPI_stats=impi
      fi
      #mpicc -v |& grep 'MPI Library 2019' > /dev/null 2>&1 
      #haveIMPI19=$((1-${PIPESTATUS[1]}))      #1=has IMPI 19, 0=does not have IMPI 19
   fi
else
    echo ""
    [[ $haveMPICC == 0 ]] && echo " WARNING : mpicc not found " | tee -a $BUILD_LOG
    [[ $haveMPIFC == 0 ]] && echo " WARNING : mpi77 not found " | tee -a $BUILD_LOG
    echo " WARNING : REMORA will be built without MPI support " | tee -a $BUILD_LOG
    echo ""
fi


if [[ "$haveMPICC" == "1" ]] && [[ "$haveMPIFC"  == "1" ]]; then

   if [[ "$haveMV2"    == "1" || "$IMPI_stats" == "impi_mpip" ]] ; then

      [[ "$haveMV2"    == "1" ]]         && echo " REMORA built with mpiP statistics for Mvapich2"  | tee -a $BUILD_LOG
      [[ "$IMPI_stats" == "impi_mpip" ]] && echo " REMORA built with mpiP statistics for IMPI 20$IMPI_year" | tee -a $BUILD_LOG

      echo "Building mpiP ..." | tee -a $BUILD_LOG
      cd $REMORA_BUILD_DIR/extra
     #mpipfile=`ls -ld mpiP*.tar.gz | awk '{print $9}' | head -n 1`
      mpipfile=`ls  mpiP*.tar.gz`
      mpipdir=`echo  ${mpipfile%%.tar.gz}`
      tar xzvf ${mpipfile}
      [[ -f ./extra/mpiP-3.4.1/make-wrappers.py ]] && sed -i 's@#!/usr/local/bin/python@#!/usr/bin/python2@' ./extra/mpiP-3.4.1/make-wrappers.py

      [[ $(hostname -f) =~ ".ls6.tacc" ]] && 
         sed -i 's/PYTHON  = python$/PYTHON  = python2/'         ./mpiP-3.4.1/Defs.mak.in &&
         sed -i 's/PYTHON  = python$/PYTHON  = python2/'         ./mpiP-3.4.1/Defs.mak    &&    #preserved across install.sh executions
         sed -i 's@#!/usr/local/bin/python@#!/usr/bin/python2@'  ./mpiP-3.4.1/make-wrappers.py

      cd ${mpipdir}
      ./configure CFLAGS="-g" --enable-demangling --disable-bfd --disable-libunwind --prefix=${REMORA_DIR} | tee -a $BUILD_LOG
      make        | tee -a $BUILD_LOG
      make shared | tee -a $BUILD_LOG
      echo "Installing mpiP ..." | tee -a $INSTALL_LOG
      make install

   fi

   if [[ "$IMPI_stats" == "impi" ]]; then
      echo "" 
      echo " REMORA built with support for impi (ipm) statistics" | tee -a $BUILD_LOG
      echo ""
   fi

   if [[ "$haveMV2" == "0" ]] && [[ "$haveIMPI" == "0" ]]; then
      echo "" 
      echo " REMORA only supports statistics for IMPI and MVAPICH2"          | tee -a $BUILD_LOG
      echo " WARNING : REMORA will be built with NO MPI statistics support " | tee -a $BUILD_LOG
      echo ""
   fi

fi

if [[ "$PHI_BUILD" == "1" ]]; then
	echo "Building Xeon Phi affinity script ..."   |  tee -a $BUILD_LOG
	cd $REMORA_BUILD_DIR/extra/
	icc -mmic -o ma ./mic_affinity.c
	echo "Installing Xeon Phi affinity script ..." |  tee -a $INSTALL_LOG
	cp -v ./ma $REMORA_DIR/bin                     |  tee -a $INSTALL_LOG
fi

if [[ $REMORA_BINARIES == 1 ]]; then
  cd $REMORA_BUILD_DIR/src/C_data_collectors_src
  make
fi
cd $REMORA_BUILD_DIR

echo "Copying all scripts to installation folder ..." |  tee -a $INSTALL_LOG

cp -vr ./src/* $REMORA_DIR/bin
rm -rf         $REMORA_DIR/bin/C_data_collectors_src

echo "Copying all scripts to installation folder ..." |  tee -a $INSTALL_LOG

if [[ "$haveMPICC" == "0" ]] || [[ "$haveMPIFC" == "0" ]]; then
    sed -i '/impi,MPI/d'      $REMORA_DIR/bin/config/modules
    sed -i '/impi_mpip,MPI/d' $REMORA_DIR/bin/config/modules
    sed -i '/mv2,MPI/d'       $REMORA_DIR/bin/config/modules
else
    if [[ "$haveIMPI" == "1" ]]; then
                                           sed -i '/mv2,MPI/d'       $REMORA_DIR/bin/config/modules
       [[ $IMPI_stats == "impi_mpip" ]] && sed -i '/impi,MPI/d'      $REMORA_DIR/bin/config/modules
       [[ $IMPI_stats == "impi"      ]] && sed -i '/impi_mpip,MPI/d' $REMORA_DIR/bin/config/modules
    fi
    if [[ "$haveMV2" == "1" ]]; then
       sed -i '/impi,MPI/d'      $REMORA_DIR/bin/config/modules
       sed -i '/impi_mpip,MPI/d' $REMORA_DIR/bin/config/modules
    fi
fi

if [[ "$haveMPICC" == "1" ]] && [[ "$haveMPIFC" == "1" ]]; then
   /sbin/lsmod | grep hfi1 > /dev/null 2>&1 
   if [[ $? == 0 ]]; then
      sed -i 's/ib,NETWORK/opa,NETWORK/' $REMORA_DIR/bin/config/modules
   fi
fi

if [[ $(hostname -f) =~ ".ls6.tacc" ]] || [[ $(hostname -f) =~ ".frontera.tacc" ]]; then
   sed -i '/lustre,IO/d' $REMORA_DIR/bin/config/modules
   sed -i '/lnet,IO/d'   $REMORA_DIR/bin/config/modules
   echo "** WARNING \"lustre,IO\" and \"lnet,IO\" modules are NOT AVAILABLE. **  "  |  tee -a $INSTALL_LOG
   echo "**         Recent security changes only allow root access to IO counters." |  tee -a $INSTALL_LOG
   echo "**         Remora will be updated when a workaround becomes available."    |  tee -a $INSTALL_LOG
fi

if [[ "$REMORA_BUILD_DIR/docs" != $REMORA_DIR/docs ]]; then
        cp $PWD/docs/modules_help          $REMORA_DIR/docs
        cp $PWD/docs/modules_whatis        $REMORA_DIR/docs
        cp $PWD/docs/remora_user_guide.pdf $REMORA_DIR/docs
  echo "Copied modules_help modules_whatis and remora.pdf to $REMORA_DIR/docs dir."
fi

echo $SEPARATOR
echo "Installation of REMORA v$VERSION completed."
echo "For a fully functional installation make sure to:"; echo ""
echo "	export PATH=\$PATH:$REMORA_DIR/bin"
echo "	export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:$REMORA_DIR/lib"
echo "	export REMORA_BIN=$REMORA_DIR/bin"; echo ""
echo "Good Luck!"
echo $SEPARATOR
