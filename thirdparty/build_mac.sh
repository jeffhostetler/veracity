#!/bin/sh
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

###############################################################################
## Script to compile third-party packages that we use and that
## are not typically installed on the system.
##
## This script will download and compile each package and
## "install" them into a "vv-thirdparty" directory.
###############################################################################
## Usage:
##     ./build_mac.sh [universal | i386]
##
## Build either universal or 32-bit versions of these libraries.
## Set CMAKE_OSX_ARCHITECTURES in the main Veracity CMake
## script to whatever value you use here.
##
## If you omit the argument, we will make a guess based on the value of 
## "hw.cpu64bit_capable" in sysctl.  If the platform is 64bit capable, we will
## build universal.  On machines that are not 64bit capable, we will build 32bit.
##
###############################################################################
IS64BIT=`sysctl -n hw.cpu64bit_capable`

if [ $# -eq 0 ]; then
	if [ ${IS64BIT} == "1" ]; then
		ARCH=universal
	else
		ARCH=i386
	fi
elif [ $1 == universal ]; then
	ARCH=universal
elif [ $1 == i386 ]; then
	ARCH=i386
else
   echo "Usage: $0 [universal | i386]"
   exit 1;
fi

PWD=`pwd`
PARENT_DIR=../
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

PROCS=`sysctl hw.ncpu | cut -d" " -f2`
MAKE_PROCS=$((PROCS+1))
MAKE_FLAGS="$MAKE_FLAGS -j$MAKE_PROCS"

# Get the path to this script
SCRIPT_PATH=`dirname $0`
THIRDPARTY_DIR=`cd $SCRIPT_PATH; pwd`
cd $PWD

# Some supporting files are in veracity/thirdparty/patches
PATCHES=$THIRDPARTY_DIR/patches

cd "$PARENT_DIR"
VVTHIRDPATH_PATH=`pwd`/vv-thirdparty
OUTPUT64="$VVTHIRDPATH_PATH"/x86_64
OUTPUT32="$VVTHIRDPATH_PATH"/i386
if [ ${ARCH} == "universal" ]; then
	OUTPUT="$VVTHIRDPATH_PATH"/universal
else
	OUTPUT="$OUTPUT32"
fi

echo "Installing veracity thirdparty dependencies to $VVTHIRDPATH_PATH"
rm -rf "$OUTPUT32"
rm -rf "$OUTPUT64"
rm -rf "$OUTPUT"
mkdir -p "$OUTPUT32"/include "$OUTPUT64"/include "$OUTPUT"/include
mkdir -p "$OUTPUT32"/lib "$OUTPUT64"/lib "$OUTPUT"/lib
mkdir -p "$OUTPUT32"/bin "$OUTPUT64"/bin "$OUTPUT"/bin

# Intermediate folders. Can be deleted after a successful build, if desired.
DOWNLOADS=`pwd`/vv-thirdparty/downloads
mkdir -p "$DOWNLOADS"
BUILDS=`pwd`/vv-thirdparty/builds
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
## ICU (International Components for Unicode)

echo
echo "ICU (icu4c-4_6)"
echo "================="
if [ -f "$BUILDS"/icu4c-4_6-build_finished ]; then
	echo "(Using build output from previous build.)"
else
	download http://download.icu-project.org/files/icu4c/4.6/icu4c-4_6-src.tgz icu4c-4_6-src.tgz
	
	echo "Unpacking..."
	cd "$BUILDS"
	rm -rf icu icu4c-4_6 # Clear files from previous build.
	tar xzf "$DOWNLOADS"/icu4c-4_6-src.tgz
	chmod +x icu/source/runConfigureICU icu/source/configure icu/source/install-sh
	cp -r icu icu4c-4_6-32
	mv icu icu4c-4_6-64
	
	echo "Building 32bit..."
	cd "$BUILDS"/icu4c-4_6-32/source
	LOGFILE="$BUILDS"/icu4c-4_6-32/build_log.txt
	echo "(Build output logged at $LOGFILE)"
	
	./runConfigureICU MacOSX --prefix="$OUTPUT32" --disable-icuio --disable-layout --with-library-bits=32 --with-iostream=none > "$LOGFILE" 2>&1
	make $MAKE_FLAGS >> "$LOGFILE" 2>&1
	
	if [ ${ARCH} == "universal" ]; then
		echo "Building 64bit..."
		cd "$BUILDS"/icu4c-4_6-64/source
		LOGFILE="$BUILDS"/icu4c-4_6-64/build_log.txt
		echo "(Build output logged at $LOGFILE)"
	
		./runConfigureICU MacOSX --prefix="$OUTPUT64" --disable-icuio --disable-layout --with-library-bits=64 --with-iostream=none > "$LOGFILE" 2>&1
		make $MAKE_FLAGS >> "$LOGFILE" 2>&1
	fi
	touch "$BUILDS"/icu4c-4_6-build_finished
fi

echo "Copying 32bit files..."
cd "$BUILDS"/icu4c-4_6-32/source
LOGFILE="$BUILDS"/icu4c-4_6-32/install_log.txt
echo "(Output logged at $LOGFILE)"
make install > "$LOGFILE"

if [ ${ARCH} == "universal" ]; then
	echo "Copying 64bit files..."
	cd "$BUILDS"/icu4c-4_6-64/source
	LOGFILE="$BUILDS"/icu4c-4_6-64/install_log.txt
	echo "(Output logged at $LOGFILE)"
	make install > "$LOGFILE"

	cd "$OUTPUT"/lib

	echo "Creating Universal Binaries"
	lipo -create "$OUTPUT32"/lib/libicudata.46.0.dylib "$OUTPUT64"/lib/libicudata.46.0.dylib -output libicudata.46.0.dylib
	lipo -create "$OUTPUT32"/lib/libicui18n.46.0.dylib "$OUTPUT64"/lib/libicui18n.46.0.dylib -output libicui18n.46.0.dylib
	lipo -create "$OUTPUT32"/lib/libicutest.46.0.dylib "$OUTPUT64"/lib/libicutest.46.0.dylib -output libicutest.46.0.dylib
	lipo -create "$OUTPUT32"/lib/libicutu.46.0.dylib "$OUTPUT64"/lib/libicutu.46.0.dylib -output libicutu.46.0.dylib
	lipo -create "$OUTPUT32"/lib/libicuuc.46.0.dylib "$OUTPUT64"/lib/libicuuc.46.0.dylib -output libicuuc.46.0.dylib

	echo "Creating library symlinks"
	ln -s libicudata.46.0.dylib libicudata.46.dylib
	ln -s libicudata.46.0.dylib libicudata.dylib
	ln -s libicui18n.46.0.dylib libicui18n.46.dylib
	ln -s libicui18n.46.0.dylib libicui18n.dylib
	ln -s libicutest.46.0.dylib libicutest.46.dylib
	ln -s libicutest.46.0.dylib libicutest.dylib
	ln -s libicutu.46.0.dylib libicutu.46.dylib
	ln -s libicutu.46.0.dylib libicutu.dylib
	ln -s libicuuc.46.0.dylib libicuuc.46.dylib
	ln -s libicuuc.46.0.dylib libicuuc.dylib
	
	echo "Copying Includes"
	cp -r "$OUTPUT64"/include/unicode "$OUTPUT"/include/unicode
fi 

echo "Performing quick sanity check on the bitness of the ICU libraries..."
file "$OUTPUT"/lib/libicuuc.dylib
echo "Ok."

echo "Done."

###############################################################################
## NSPR (Netscape Portable Runtime)
##     This library is needed in order to build SpiderMonkey with the
##     JS_THREADSAFE option.

echo
echo "NSPR (nspr-4.8.9)"
echo "================="
if [ -f "$BUILDS"/nspr-4.8.9/build_finished ]; then
	echo "(Using build output from previous build.)"
else
	download http://ftp.mozilla.org/pub/mozilla.org/nspr/releases/v4.8.9/src/nspr-4.8.9.tar.gz nspr-4.8.9.tar.gz
	
	echo "Unpacking..."
	cd "$BUILDS"
	rm -rf nspr-4.8.9 # Clear files from previous build.
	tar xzf "$DOWNLOADS"/nspr-4.8.9.tar.gz
	
	echo "Building Universal Binary..."
	cd "$BUILDS"/nspr-4.8.9/mozilla/nsprpub
	LOGFILE="$BUILDS"/nspr-4.8.9/build_log.txt
	echo "(Build output logged at $LOGFILE)"
	if [ ${ARCH} == "universal" ]; then
		env CFLAGS="-arch i386 -arch x86_64" LDFLAGS="-arch i386 -arch x86_64" ./configure --disable-debug --enable-optimize > "$LOGFILE" 2>&1
	else
		./configure --disable-debug --enable-optimize > "$LOGFILE" 2>&1
	fi
	
	make $MAKE_FLAGS >> "$LOGFILE" 2>&1
	
	touch "$BUILDS"/nspr-4.8.9/build_finished
fi

echo "Copying files..."
cp "$BUILDS"/nspr-4.8.9/mozilla/nsprpub/dist/lib/* "$OUTPUT"/lib/

echo "Done."

###############################################################################
## SpiderMonkey - JavaScript interpreter

echo
echo "Spidermonkey (js185-1.0.0)"
echo "=========================="

if [ -d "$BUILDS"/js185-1.0.0-i386 ]; then
	echo "(Using output from previous build. (32-bit))"
else
	download http://download.sourcegear.com/Veracity/third_party/js/js185-1.0.0-i386.tar.gz js185-1.0.0-i386.tar.gz
	
	echo "Unpacking (32-bit)..."
	cd "$BUILDS"
	rm -rf dist
	tar xzf "$DOWNLOADS"/js185-1.0.0-i386.tar.gz
	mv dist js185-1.0.0-i386
fi


if [ ${ARCH} == "universal" ]; then
	if [ -d "$BUILDS"/js185-1.0.0-x86_64 ]; then
		echo "(Using output from previous build. (64-bit))"
	else
		download http://download.sourcegear.com/Veracity/third_party/js/js185-1.0.0-x86_64.tar.gz js185-1.0.0-x86_64.tar.gz
		
		echo "Unpacking (64-bit)..."
		cd "$BUILDS"
		rm -rf dist
		tar xzf "$DOWNLOADS"/js185-1.0.0-x86_64.tar.gz
		mv dist js185-1.0.0-x86_64
	fi
fi

echo "Copying files..."
cp -r "$BUILDS"/js185-1.0.0-i386/include "$OUTPUT"/include/js
patch -u "$OUTPUT"/include/js/jsapi.h "$PATCHES"/js_1.8.5__jsapi.h.patch
patch -u "$OUTPUT"/include/js/jsval.h "$PATCHES"/mac__js_1.8.5__jsval.h.patch
if [ ${ARCH} == "universal" ]; then
	cp "$BUILDS"/js185-1.0.0-x86_64/include/NativeX64.h "$OUTPUT"/include/js/NativeX64.h
	patch -u "$OUTPUT"/include/js/js-config.h "$PATCHES"/mac__js_1.8.5__js-config.h.patch
	lipo -create \
		"$BUILDS"/js185-1.0.0-i386/lib/libmozjs185.dylib \
		"$BUILDS"/js185-1.0.0-x86_64/lib/libmozjs185.dylib \
		-output "$OUTPUT"/lib/libmozjs185.dylib
else
	cp -r "$BUILDS"/js185-1.0.0-i386/lib/libmozjs185.dylib "$OUTPUT"/lib/libmozjs185.dylib
fi
pushd "$OUTPUT"/lib/
ln -s libmozjs185.dylib libmozjs185.1.0.dylib
ln -s libmozjs185.dylib libmozjs185.1.0.0.dylib
popd

echo "Done."

###############################################################################
## Done.

if [ ${ARCH} == "universal" ]; then
	echo
	echo "Cleaning up..."
	echo "=============="
	rm -r "$OUTPUT32"
	rm -r "$OUTPUT64"
	echo "Done."
fi

echo
echo
echo "Finished building vv-thirdparty!"
echo
echo "It is safe to delete the \"downloads\" and \"builds\" directories."
