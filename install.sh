#!/bin/bash
#--------------------------------------------------------------------------------
  
  # Default installation directory is PWD unless defined by  $REMORA_INSTALL_PREFIX
  export REMORA_DIR=${REMORA_INSTALL_PREFIX:-$PWD}
  
  # For now 3.4.1 impi used with x86_64 and 3.5 with aarch64
  export ARCH=x86_64  MPIP_VER=3.4.1
  [[ $(uname -a) =~ aarch64 ]] &&  export ARCH=aarch64 MPIP_VER=3.5
  echo $ARCH
  
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

  echo " ==> Building mpstat ..." | tee -a $BUILD_LOG
  cd $REMORA_BUILD_DIR/extra
  
  sysfile=`ls sysstat*.tar.gz`
  sysdir=`echo  ${sysfile%%.tar.gz}`
  tar xzvf ${sysfile}
  cd ${sysdir}
  sed -i 's/-pipe //g' Makefile
  sed -i 's/-pipe //g' Makefile.in
    
  ./configure | tee -a $BUILD_LOG
  make mpstat | tee -a $BUILD_LOG
    
  echo " ==> Installing mpstat ..." | tee -a $INSTALL_LOG
  cp mpstat $REMORA_DIR/bin

  #MPI
  # Check if MPI compiler is available. If not, disable MPI module and skip mpiP build
  export CC=mpicc
  export F77=mpif77
  
  echo " ==> Checking on MPI ..." | tee -a $INSTALL_LOG

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
  
     mpicc -v 2>&1 >/dev/null | grep 'NVCOMPILER' > /dev/null 2>&1
     haveNVHPC=$((1-${PIPESTATUS[1]}))          #1=has NVHPC, 0=does not have NVHPC
     [[ $haveNVHPC == 1 ]] &&  [[ $MPI_ROOT =~ openmpi ]] && haveOMPI=1

     mpicc -v 2>&1 >/dev/null | grep 'GCC' > /dev/null 2>&1
     haveGCC=$((1-${PIPESTATUS[1]}))          #1=has GCC, 0=does not have GCC
     if [[ $haveGCC == 1 ]] &&  [[ $MPI_ROOT =~ openmpi ]]; then
       haveOMPI=1
       FLAGS="LDFLAGS=-g -Wno-implicit-int -Wno-implicit-function-declaration"
     fi
  
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
     echo " ==> MPI: IMPI=$haveIMPI MV2=$haveMV2 OMPII=$haveOMPI ..." | tee -a $INSTALL_LOG
  else
      echo ""
      [[ $haveMPICC == 0 ]] && echo " WARNING : mpicc not found " | tee -a $BUILD_LOG
      [[ $haveMPIFC == 0 ]] && echo " WARNING : mpi77 not found " | tee -a $BUILD_LOG
      echo " WARNING : REMORA will be built without MPI support " | tee -a $BUILD_LOG
      echo ""
  fi

  # BFD binutils and unwind for Vista (OpenMPI aarch64)  do this before building mpiP
  # libunwind-1.3.0.tar.gz  libunwind-1.3.2.tar.gz  libunwind-1.3-rc1.tar.gz  libunwind-1.8.1.tar.gz
  # binutils-2.23.2.tar.gz  binutils-2.27.tar.gz  binutils-2.35.1.tar.gz  binutils-2.36.tar.gz  binutils-2.40.tar.gz

 #if [[ $ARCH == aarch64 ]] &&  [[ "$haveOMPI" == "1" ]] && [[ "$haveGCC" == 1 ]] ; then
  if [[ $ARCH == aarch64 ]] &&  [[ "$haveOMPI" == "1" ]]; then
      # For Vista  (aarch64 and OpenMPI) use BFD (binutils) for extracting prg. function names
      # Used by mpiP.  All tested unwind builds break with a combo of bfd and mpiP.
      # DON'T INCLUDE UNWIND in vista remora build

      echo " ==> Building BFD (binutils) ..." | tee -a $INSTALL_LOG
  
      export UNW_VER=1.8.1  # libUNWind version
      export BFD_VER=2.43   # libUNWind version
  
  
      cd $REMORA_BUILD_DIR/extra
      mkdir -p $REMORA_DIR/binutils-$BFD_VER
      CC=gcc CXX=g++ BFD_SRC_DIR=$REMORA_BUILD_DIR/extra  BFD_INSTALL_DIR=$REMORA_DIR/binutils-$BFD_VER  ./build_bfd.sh
     #export LD_LIBRARY_PATH=$REMORA_DIR/binutils-${BFD_VER}:$LD_LIBRARY_PATH
  
      # cd $REMORA_BUILD_DIR/extra
      # CC=gcc CXX=g++ UNW_SRC_DIR=$REMORA_BUILD_DIR/extra  UNW_INSTALL_DIR=$REMORA_DIR/unwind-$UNW_VER    ./build_unwind.sh
      #export LD_LIBRARY_PATH=$REMORA_DIR/unwind-$UNW_VER:$LD_LIBRARY_PATH
  fi  

  # mpiP
  if [[ "$haveMPICC" == "1" ]] && [[ "$haveMPIFC"  == "1" ]]; then

     if [[ "$haveMV2" == 1 || "$haveOMPI" == 1 ||  "$IMPI_stats" == "impi_mpip" ]] ; then
  
        echo " ==> Building mpiP ..." | tee -a $INSTALL_LOG

        [[ "$haveMV2"    == 1 ]]         && echo " REMORA built with mpiP statistics for Mvapich2"          | tee -a $BUILD_LOG
        [[ "$haveOMPI"   == 1 ]]         && echo " REMORA built with mpiP statistics for OMPI"              | tee -a $BUILD_LOG
        [[ "$IMPI_stats" == impi_mpip ]] && echo " REMORA built with mpiP statistics for IMPI 20$IMPI_year" | tee -a $BUILD_LOG

        echo "Building mpiP ..." | tee -a $BUILD_LOG
        cd $REMORA_BUILD_DIR/extra

        FLAGS=${FLAGS:-CFLAGS=-g}    # default is -g unless FLAGS is defined above
  
        mpipfile=mpiP-${MPIP_VER}.tar.gz
        mpipdir=${mpipfile%%.tar.gz}
        tar xzvf ${mpipfile}
        [[ -f ./extra/mpiP-3.4.1/make-wrappers.py ]] && sed -i 's@#!/usr/local/bin/python@#!/usr/bin/python2@' ./extra/mpiP-3.4.1/make-wrappers.py
        [[ -f ./extra/mpiP-3.5/make-wrappers.py   ]] && sed -i 's@#!/usr/local/bin/python@#!/usr/bin/python@'  ./extra/mpiP-3.5/make-wrappers.py
  
        [[ $(hostname -f) =~ ".ls6.tacc" ]] && 
           sed -i 's/PYTHON  = python$/PYTHON  = python2/'         ./mpiP-3.4.1/Defs.mak.in &&
           sed -i 's/PYTHON  = python$/PYTHON  = python2/'         ./mpiP-3.4.1/Defs.mak    &&    #preserved across install.sh executions
           sed -i 's@#!/usr/local/bin/python@#!/usr/bin/python2@'  ./mpiP-3.4.1/make-wrappers.py

        BFD_OPTION="--disable-bfd"

        if [[ ${MPIP_VER} == 3.5 ]] && [[ $ARCH == aarch64 ]] && [[ "$haveGCC" == 1 ]]; then
          cp -p patched_3.5_pc_lookup.c ./extra/mpiP-3.5/pc_lookup.c
        fi
        if [[ ${MPIP_VER} == 3.5 ]] && [[ $ARCH == aarch64 ]]; then
          BFD_OPTION="--with-binutils-dir=$REMORA_DIR/binutils-$BFD_VER"
        fi
  
        cd ${mpipdir}

        #echo " -> ./configure CFLAGS="-g" --enable-demangling  --prefix=${REMORA_DIR}/mpiP-$MPIP_VER  "$BFD_OPTION" --disable-libunwind "
                 # ./configure CFLAGS="-g" --enable-demangling  --prefix=${REMORA_DIR}/mpiP-$MPIP_VER  "$BFD_OPTION" --disable-libunwind \
                 # ./configure LDFLAGS="-g -Wno-implicit-int -Wno-implicit-function-declaration" --enable-demangling  --prefix=${REMORA_DIR}/mpiP-$MPIP_VER  "$BFD_OPTION" --disable-libunwind \

        echo " ==> mpiP ./configure --prefix=${REMORA_DIR}/mpiP-$MPIP_VER  "$FLAGS"  "$BFD_OPTION"  --enable-demangling  --disable-libunwind"

                   # "$FLAGS" \

echo " ==>"        ./configure --prefix=${REMORA_DIR}/mpiP-$MPIP_VER \
                    CFLAGS="-g" \
                    "$BFD_OPTION" \
                    --enable-demangling \
                    --disable-libunwind | tee -a $BUILD_LOG

        ./configure --prefix=${REMORA_DIR}/mpiP-$MPIP_VER \
                    CFLAGS="-g" \
                    "$BFD_OPTION" \
                    --enable-demangling \
                    --disable-libunwind \
                    |& tee -a $BUILD_LOG

  
        make                            | tee -a $BUILD_LOG
        make shared                     | tee -a $BUILD_LOG
        echo " --> Installing mpiP ..." | tee -a $INSTALL_LOG
        make install
  
     fi
  
     if [[ "$IMPI_stats" == "impi" ]]; then
        echo "" 
        echo " ==> REMORA built with support for impi (ipm) statistics" | tee -a $BUILD_LOG
        echo ""
     fi
  
     if [[ "$haveIMPI" == 0 ]] && [[ "$haveMV2" == 0 ]] && [[ "$haveOMPI" == 0 ]]; then
        echo "" 
        echo " ==> REMORA only supports statistics for IMPI, MVAPICH2 and OpenMPI"  | tee -a $BUILD_LOG
        echo " ==> WARNING : REMORA will be built with NO MPI statistics support " | tee -a $BUILD_LOG
        echo ""
     fi
     has_MPI=yes
        echo " ==> Finished Building mpiP ..." | tee -a $INSTALL_LOG
  else
     has_MPI=no
  fi

  # Build Binary Collectors
  if [[ $REMORA_BINARIES == 1 ]]; then
    echo " ==> Building Binary Collectors ..." | tee -a $INSTALL_LOG
    cd $REMORA_BUILD_DIR/src/C_data_collectors_src
    make
  fi  
  cd $REMORA_BUILD_DIR
    
  # Copy scripts to installation folder
  echo " ==> Copying all scripts to installation folder ..." |  tee -a $INSTALL_LOG
  cp -vr ./src/* $REMORA_DIR/bin
  rm -rf         $REMORA_DIR/bin/C_data_collectors_src


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

  echo " ==> Setting up mpip in config/modules ..." | tee -a $INSTALL_LOG
  # Modify config/modules to use appropriate mpi module (mpi,MPI) -> (remove_it, impi, ompi_mpip, mv2, etc.)
  if [[ "$haveMPICC" == "0" ]] || [[ "$haveMPIFC" == "0" ]]; then
      sed -i      '/mpi,MPI/d' $REMORA_DIR/bin/config/modules
  else
      if [[ "$haveIMPI" == "1" ]]; then
         [[ $IMPI_stats == "impi"      ]] && sed -i 's/mpi,MPI/impi,MPI/'     $REMORA_DIR/bin/config/modules
         [[ $IMPI_stats == "impi_mpip" ]] && sed -i 's/mpi,MPI/impi_mpip,MPI/' $REMORA_DIR/bin/config/modules
         [[ $IMPI_stats == "impi_mpip" ]] && echo " ==>  Using impi_mpip ..." | tee -a $INSTALL_LOG
      fi
      if [[ "$haveMV2" == "1" ]]; then
         sed -i 's/mpi,MPI/mv2,MPI/' $REMORA_DIR/bin/config/modules
      echo " ==>  Using mv2 ..." | tee -a $INSTALL_LOG
      fi
      if [[ "$haveOMPI" == "1" ]]; then
         sed -i 's/mpi,MPI/ompi_mpip,MPI/' $REMORA_DIR/bin/config/modules
      echo " ==>  Using ompi_mpip ..." | tee -a $INSTALL_LOG
      fi
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
  [[ $has_MPI == yes ]] &&
  echo "	export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:$REMORA_DIR/mpiP-${MPIP_VER}/lib"
  echo "	export REMORA_BIN=$REMORA_DIR/bin"; echo ""
  echo "Good Luck!"
  echo $SEPARATOR
