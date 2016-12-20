#!/bin/bash
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Copyright 2010-2013 SourceGear, LLC
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

##################################################################
## Script to compile third-party packages that we use and that
## are not typically installed on the system or available in 
## the package management system.
##
## This script will download and compile each package and
## "install" them into an "sgneeds" directory.
##################################################################
## Usage:
##     ./build_linux.sh [i686 | x86_64]
##
## Build either 32- or 64-bit versions of these libraries.
##
## We default to "i686" (32-bit).
##################################################################
if [ $# -eq 0 ]; then
    ARCH=`uname -m`
elif [ $1 == i686 ]; then
    ARCH=i686
elif [ $1 == x86_64 ]; then
    ARCH=x86_64
else
    echo "Usage: $0 [i686 | x86_64]"
    exit 1
fi

PWD=`pwd`
PARENT_DIR=..
if [ $# -eq 2 ]; then
	# PARENT_DIR is an undocumented extra parameter that you can use to build
	# the "vv-thirdparty" directory in a non-default location. Most likely this
	# is because your source directory and your build directory are not peers
	# (and the fact that you will more likely want "vv-thirdparty" to be a peer
	# of your build directory than a peer of your of your source directory).
	# Simply provide this parameter, giving it the location of the the
	# directory that you want to be the parent containing "vv-thirdparty".
	PARENT_DIR="$2"
fi

# Stop on any error and report failure.
set -e
trap "echo ; echo An error has occurred! Veracity thirdparty libraries did NOT finish building!" ERR

PROCS=`cat /proc/cpuinfo | grep processor | wc -l`
MAKE_PROCS=$((PROCS+1))
MAKE_FLAGS="$MAKE_FLAGS -j$MAKE_PROCS"


SCRIPT_PATH=`dirname $0`
THIRDPARTY_DIR=`cd $SCRIPT_PATH; pwd`
cd $PWD
PATCHES=$THIRDPARTY_DIR/patches
cd "$PARENT_DIR"
OUTPUT=`pwd`/vv-thirdparty/$ARCH
echo "Installing veracity thirdparty dependencies to $OUTPUT"
rm -rf "$OUTPUT"
mkdir -p "$OUTPUT"/include
mkdir -p "$OUTPUT"/lib
mkdir -p "$OUTPUT"/bin

# Intermediate folders. Can be deleted after a successful build, if desired.
DOWNLOADS=`pwd`/vv-thirdparty/downloads
mkdir -p "$DOWNLOADS"
BUILDS=`pwd`/vv-thirdparty/builds-$ARCH
mkdir -p "$BUILDS"

# Helper funciton to download a file if it hasn't already been downloaded.
download () {
	echo "Downloading..."
	cd "$DOWNLOADS"
	if [ -f $2 ]; then
		echo "(Already downloaded.)"
	else
		curl -L $1 -o $2
	fi
}

###############################################################################
## libcurl

## Building curl is an optional step.  It is necessary in some instances 
## in order to include libc-ares as a curl build option.  The stock 
## ubuntu does not include libc-ares support.  If you want to use this,
## be sure to apt-get install libc-ares-dev.  Then do 
## export BUILD_CURL=on
## before calling cmake.  See https://bugs.launchpad.net/ubuntu/+source/curl/+bug/613274
if [ -n "$BUILD_CURL" ]; then

	echo
	echo "libcurl (curl-7.24.0)"
	echo "==========================="
	if [ -f "$BUILDS"/curl-7.24.0/build_finished ]; then
		echo "(Using build output from previous build.)"
	else
		download http://curl.haxx.se/download/curl-7.24.0.tar.gz curl-7.24.0.tar.gz
		
		echo "Unpacking..."
		cd "$BUILDS"
		rm -rf curl-7.24.0 # Clear files from previous build.
		tar xzf "$DOWNLOADS"/curl-7.24.0.tar.gz
		
		echo "Building..."
		cd "$BUILDS"/curl-7.24.0
		LOGFILE="$BUILDS"/curl-7.24.0/build_log.txt
		echo "(Build output logged at $LOGFILE)"
		./configure --prefix="$OUTPUT" --disable-dependency-tracking --enable-ipv6 --with-gssapi=/usr --with-ca-path=/etc/ssl/certs --enable-ares > "$LOGFILE" 2>&1
		make $MAKE_FLAGS >> "$LOGFILE" 2>&1
		
		touch "$BUILDS"/curl-7.24.0/build_finished
	fi

	echo "Copying files..."
	cd "$BUILDS"/curl-7.24.0
	LOGFILE="$BUILDS"/curl-7.24.0/install_log.txt
	echo "(Output logged at $LOGFILE)"
	make install > "$LOGFILE"
fi

###############################################################################
## Done.

echo
echo
echo "Finished building vv-thirdparty for $ARCH!"
echo
echo "It is safe to delete the \"downloads\" and \"builds-$ARCH\" directories."

