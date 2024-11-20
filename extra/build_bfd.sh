#!/bin/bash
#  Build binutils
#
#  Versions: 2.27 2.23.2 2.35 2.36 2.40 2.43
#
#  Execute with:     Version  Source        Install             Build
#          defaults: 2.43     $PWD/sources  $PWD/binutils-<ver> /tmp/...
#
#    BFD_VER=<> BFD_SRC_DIR=<> BFD_INSTALL_DIR=<> BFD_BUILD_DIR=<> $0
#
#    Uses GNU gcc compiler by default. Change with CC
#
#  wget https://ftp.gnu.org/gnu/binutils/binutils-${VER}.tar.gz
#
# 11/20/24 Kent 

  BFD_VER=${BFD_VER:-2.43}
  TOP_DIR=`pwd`;

  tar_name=binutils
  app_name=binutils

  echo " -> Building Version $BFD_VER"

      BFD_SRC_DIR=${BFD_SRC_DIR:-$TOP_DIR/sources}
     BFD_TAR_FILE=${BFD_SRC_DIR}/${tar_name}-${BFD_VER}.tar.gz
  BFD_INSTALL_DIR=${BFD_INSTALL_DIR:-`pwd`/${tar_name}-$BFD_VER}
    BFD_BUILD_DIR=${BFD_BUILD_DIR:-/tmp/build_${app_name}_$USER}

  echo " -> Building ${app_name} in $BFD_BUILD_DIR."

  echo " -> Untarring in $BFD_BUILD_DIR, may take a minute."
  [[ $BFD_BUILD_DIR =~ ^/tmp ]] && rm -rf $BFD_BUILD_DIR  # if using tmp, clear
  mkdir -p $BFD_BUILD_DIR
  cd       $BFD_BUILD_DIR
  tar -xzf $BFD_TAR_FILE

  echo " -> Building ${app_name} in $BFD_INSTALL_DIR."

  [[ ! -d $BFD_BUILD_DIR ]] && echo " -> Error: No build directory ($BFD_BUILD_DIR) exits." && exit

  cd $BFD_BUILD_DIR/${tar_name}-$BFD_VER

  CC=${CC:-gcc}
  echo " -> Building with $CC compiler. Installing in $BFD_INSTALL_DIR."

  echo "CC=$CC ./configure --enable-shared -prefix=$BFD_INSTALL_DIR"
        CC=$CC ./configure --enable-shared -prefix=$BFD_INSTALL_DIR
  make -j 8
  make install
