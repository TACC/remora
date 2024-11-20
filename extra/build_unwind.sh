#!/bin/bash
#  Build unwind library (libunwind)
#
#  UNW : UNWind
#
#  Versions: 2.27 2.23.2 2.35 2.36 2.40 2.43
#
#  Execute with:     Version  Source        Install             Build
#          defaults: 1.8.1    $PWD/sources  $PWD/unwind-<ver> /tmp/...
#
#    UNW_VER=<> UNW_SRC_DIR=<> UNW_INSTALL_DIR=<> UNW_BUILD_DIR=<> $0
#
#    Uses GNU compilers by default. Change with  CC and CCX
#
# 11/20/24 Kent 
#
#
#https://github.com/libunwind/libunwind/releases/tag/v1.8.1

  UNW_VER=${UNW_VER:-1.8.1}
  TOP_DIR=`pwd`; 

  tar_name=libunwind
  app_name=unwind

  echo " -> Building Version $UNW_VER"

      UNW_SRC_DIR=${UNW_SRC_DIR:-$TOP_DIR}
     UNW_TAR_FILE=${UNW_SRC_DIR}/${tar_name}-${UNW_VER}.tar.gz
  UNW_INSTALL_DIR=${UNW_INSTALL_DIR:-`pwd`/${tar_name}-$UNW_VER}
    UNW_BUILD_DIR=${UNW_BUILD_DIR:-/tmp/build_${app_name}_$USER}

         echo "    UNW_SRC_DIR=$UNW_SRC_DIR"
         echo "   UNW_TAR_FILE=$UNW_TAR_FILE"
         echo "UNW_INSTALL_DIR=$UNW_INSTALL_DIR"
         echo "  UNW_BUILD_DIR=$UNW_BUILD_DIR"

  mkdir -p $UNW_INSTALL_DIR

  echo " -> Building ${app_name} in $UNW_BUILD_DIR."

  echo " -> Untarring in $UNW_BUILD_DIR, may take a minute."
  [[ $UNW_BUILD_DIR =~ ^/tmp ]] && rm -rf $UNW_BUILD_DIR  # if using tmp, clear
  mkdir -p $UNW_BUILD_DIR
  cd       $UNW_BUILD_DIR
  tar -xzf $UNW_TAR_FILE

  echo " -> Building ${app_name} in $UNW_INSTALL_DIR."

  [[ ! -d $UNW_BUILD_DIR ]] && echo " -> Error: No build directory ($UNW_BUILD_DIR) exits." && exit

  cd $UNW_BUILD_DIR/${tar_name}-$UNW_VER

  CC=${CC:-gcc} CXX=${CCX:-g++}
  echo " -> Building with $CC and $CCX compilers. Installing in $UNW_INSTALL_DIR."""

  echo CC=$CC CCX=$CXX ./configure -prefix=$UNW_INSTALL_DIR
       CC=$CC CCX=$CXX ./configure -prefix=$UNW_INSTALL_DIR
  make -j 8
  make install
