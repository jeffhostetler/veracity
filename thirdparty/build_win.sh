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
## Script to download and build third-party packages needed by Veracity.
###############################################################################
## Usage:
##     sh build_win.sh
###############################################################################
PWD=`pwd`
PARENT_DIR=..
if [[ (( $# -eq 1 )) && (( -d "$1" )) ]]; then
	# PARENT_DIR is an undocumented parameter that you can use to build the
	# "vv-thirdparty" directory in a non-default location. Most likely this is
	# because your source directory and your build directory are not peers (and
	# the fact that you will more likely want "vv-thirdparty" to be a peer of
	# your build directory than a peer of your of your source directory).
	# Simply provide this parameter, giving it the location of the the
	# directory that you want to be the parent containing "vv-thirdparty".
	PARENT_DIR="$1"
elif [ $# -ne 0 ]; then
	echo "Usage: sh build_win.sh"
	exit 1
fi

# Determine which architecture we are building for:
if [ -z "`cl.exe 2>&1 | grep x64`" ]; then
	# "x86" means a 32-bit (i386 aka x86) executable, which should be able to run
	# on both 32-bit and 64-bit versions of Windows. This will use the 32-bit
	# version of the compiler, so any machine should be able to create it.
	ARCH=x86
else
	# "amd64" means a 64-bit (x86_64 aka amd64) executable, which can only be run
	# on 64-bit versions of Windows. This will use the 64-bit version of the
	# compiler, so only 64-bit machines should be able to create it.
	ARCH=amd64
fi
echo "Building veracity third-party dependencies for architecture: $ARCH"

# Stop on any error and report failure.
set -e
trap "echo ; echo An error has occurred! Veracity third-party dependencies did NOT finish building!" ERR

# Get the path to this script
SCRIPT_PATH=`dirname $0`
THIRDPARTY_DIR=`cd $SCRIPT_PATH; pwd`
cd $PWD

# Some supporting files are in veracity/thirdparty/patches
PATCHES=$THIRDPARTY_DIR/patches

cd "$PARENT_DIR"
OUTPUT=`pwd`/vv-thirdparty/$ARCH
echo "Installing veracity third-party dependencies to $OUTPUT"
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
## curl

echo
LIBCURL_VERSION=7.23.1
echo "curl (curl-${LIBCURL_VERSION})"
echo "=================="
if [ -f "$BUILDS"/libcurl_ssl_${LIBCURL_VERSION}/build_finished ]; then
	echo "(Using build output from previous build.)"
else
	download http://download.sourcegear.com/Veracity/third_party/libcurl/libcurl_ssl_${LIBCURL_VERSION}_$ARCH.zip  libcurl_ssl_${LIBCURL_VERSION}_$ARCH.zip
	
	echo "Unpacking..."
	cd "$BUILDS"
	rm -rf libcurl_ssl_${LIBCURL_VERSION} # Clear files from previous build.
	unzip -q $DOWNLOADS/libcurl_ssl_${LIBCURL_VERSION}_$ARCH.zip
	
	echo "Building..."
	
	touch "$BUILDS"/libcurl_ssl_${LIBCURL_VERSION}/build_finished
fi

echo "Copying files..."
cp -r "$BUILDS"/libcurl_ssl_${LIBCURL_VERSION}/include/curl   "$OUTPUT"/include/
cp    "$BUILDS"/libcurl_ssl_${LIBCURL_VERSION}/lib/*.lib    "$OUTPUT"/lib/
cp    "$BUILDS"/libcurl_ssl_${LIBCURL_VERSION}/bin/*.dll    "$OUTPUT"/bin/
cp    "$BUILDS"/libcurl_ssl_${LIBCURL_VERSION}/bin/curl.exe "$OUTPUT"/bin/
chmod 755 "$OUTPUT"/bin/curl.exe
chmod 755 "$OUTPUT"/bin/*.dll

echo "Done."

###############################################################################
## ICU (International Components for Unicode)

echo
echo "ICU (icu4c-4_6)"
echo "================="
if [ -f "$BUILDS"/icu4c-4_6/build_finished ]; then
	echo "(Using build output from previous build.)"
else
	download http://download.icu-project.org/files/icu4c/4.6/icu4c-4_6-src.tgz icu4c-4_6-src.tgz
	
	echo "Unpacking..."
	cd "$BUILDS"
	rm -rf icu icu4c-4_6 # Clear files from previous build.
	tar xzfo "$DOWNLOADS"/icu4c-4_6-src.tgz
	mv icu icu4c-4_6
	
	echo "Building..."
	unset MAKEFLAGS
	unset MAKELEVEL
	unset MAKEOVERRIDES
	LOGFILE="$BUILDS"/icu4c-4_6/build_log.txt
	echo "(Build output logged at $LOGFILE)"
	cd "$BUILDS"/icu4c-4_6/source/allinone
	msbuild.exe allinone.sln /p:Configuration=Release > "$LOGFILE" 2>&1
	
	touch "$BUILDS"/icu4c-4_6/build_finished
fi

echo "Copying files..."
cp -r "$BUILDS"/icu4c-4_6/include/unicode "$OUTPUT"/include/
if [ $ARCH == x86 ]; then
	cp "$BUILDS"/icu4c-4_6/lib/icudt.lib "$OUTPUT"/lib/
	cp "$BUILDS"/icu4c-4_6/lib/icuuc.lib "$OUTPUT"/lib/
	cp "$BUILDS"/icu4c-4_6/lib/icuin.lib "$OUTPUT"/lib/
	cp "$BUILDS"/icu4c-4_6/bin/icudt46.dll "$OUTPUT"/bin/
	cp "$BUILDS"/icu4c-4_6/bin/icuuc46.dll "$OUTPUT"/bin/
	cp "$BUILDS"/icu4c-4_6/bin/icuin46.dll "$OUTPUT"/bin/
else
	cp "$BUILDS"/icu4c-4_6/lib64/icudt.lib "$OUTPUT"/lib/
	cp "$BUILDS"/icu4c-4_6/lib64/icuuc.lib "$OUTPUT"/lib/
	cp "$BUILDS"/icu4c-4_6/lib64/icuin.lib "$OUTPUT"/lib/
	cp "$BUILDS"/icu4c-4_6/bin64/icudt46.dll "$OUTPUT"/bin/
	cp "$BUILDS"/icu4c-4_6/bin64/icuuc46.dll "$OUTPUT"/bin/
	cp "$BUILDS"/icu4c-4_6/bin64/icuin46.dll "$OUTPUT"/bin/
fi

echo "Done."

###############################################################################
## Spidermonkey - JavaScript interpreter

JSVERSION=1.8.5
echo
echo "Spidermonkey (js_${JSVERSION})"
echo "=========================="
if [ -d "$BUILDS"/js_${JSVERSION} ]; then
	echo "(Using output from previous build.)"
else
	download http://download.sourcegear.com/Veracity/third_party/js/js_${JSVERSION}-win-$ARCH.zip js_${JSVERSION}-win-$ARCH.zip
	
	echo "Unpacking..."
	cd "$BUILDS"
	rm -rf js_${JSVERSION}-win-$ARCH
	unzip -q "$DOWNLOADS"/js_${JSVERSION}-win-$ARCH.zip
	mv js_${JSVERSION} js_${JSVERSION}-win-$ARCH
fi

#javascript no longer uses static libs.  It uses a dll.
echo "Copying files..."
cp -r "$BUILDS"/js_${JSVERSION}-win-$ARCH/include/* "$OUTPUT"/include/
patch -u "$OUTPUT"/include/js/jsapi.h "$PATCHES"/js_${JSVERSION}__jsapi.h.patch
patch -u "$OUTPUT"/include/js/jsapi.h "$PATCHES"/win__js_${JSVERSION}__jsapi.h.patch
patch -u "$OUTPUT"/include/js/jsval.h "$PATCHES"/win__js_${JSVERSION}__jsval.h.patch
cp "$BUILDS"/js_${JSVERSION}-win-$ARCH/lib/*.lib "$OUTPUT"/lib/
cp "$BUILDS"/js_${JSVERSION}-win-$ARCH/bin/*.dll "$OUTPUT"/bin/

echo "Done."

###############################################################################
## wxWidgets - GUI toolkit
##     This is used for the Tortoise interface (plus more to come...?).

echo
echo "wxWidgets (wxWidgets-2.9.1)"
echo "==========================="
if [ -f "$BUILDS"/wxWidgets-2.9.1/build_finished ]; then
	echo "(Using build output from previous build.)"
else
	download http://download.sourcegear.com/Veracity/third_party/wxWidgets/wxWidgets-2.9.1.zip wxWidgets-2.9.1.zip

	echo "Unpacking..."
	rm -rf "$BUILDS"/wxWidgets-2.9.1 # Clear files from previous build.
	mkdir "$BUILDS"/wxWidgets-2.9.1
	cd "$BUILDS"/wxWidgets-2.9.1
	unzip -q "$DOWNLOADS"/wxWidgets-2.9.1.zip
	cp "$PATCHES"/wxWidgets-wxrc.cpp utils/wxrc/wxrc.cpp # We need a more recent version of this file than the zip had.
	cp "$PATCHES"/wxWidgets-propgrid.cpp src/propgrid/propgrid.cpp # We need a more recent version of this file than the zip had.
	cp "$PATCHES"/wxWidgets-datavgen.cpp src/generic/datavgen.cpp # Contains a bug fix.  If we update the wxWidgets version past 2.9.1, we need to retest double-click and checkbox selection in the commit dialog
	cp "$PATCHES"/wxWidgets-window.cpp src/msw/window.cpp # Contains a bug fix for VS2012: removes obsolete include of pbt.h (from Windows Platform SDK)
	
	echo "Building..."
	LOGFILE="$BUILDS"/wxWidgets-2.9.1/build_log.txt
	echo "(Build output logged at $LOGFILE)"
	if [ $ARCH == x86 ]; then
		cd "$BUILDS"/wxWidgets-2.9.1/build/msw
		nmake -f makefile.vc BUILD=debug _ITERATOR_DEBUG_LEVEL=0 >  "$LOGFILE" 2>&1
		nmake -f makefile.vc BUILD=release >> "$LOGFILE" 2>&1
		cd "$BUILDS"/wxWidgets-2.9.1/utils/wxrc
		nmake -f makefile.vc BUILD=release >> "$LOGFILE" 2>&1
	else
		cd "$BUILDS"/wxWidgets-2.9.1/build/msw
		nmake -f makefile.vc BUILD=debug TARGET_CPU=AMD64   >  "$LOGFILE" 2>&1
		nmake -f makefile.vc BUILD=release TARGET_CPU=AMD64 >> "$LOGFILE" 2>&1
		cd "$BUILDS"/wxWidgets-2.9.1/utils/wxrc
		nmake -f makefile.vc BUILD=release TARGET_CPU=AMD64 >> "$LOGFILE" 2>&1
	fi
	
	touch "$BUILDS"/wxWidgets-2.9.1/build_finished
fi

echo "Copying files..."
cp -r "$BUILDS"/wxWidgets-2.9.1/include/* "$OUTPUT"/include/
if [ $ARCH == x86 ]; then
	cp -r "$BUILDS"/wxWidgets-2.9.1/lib/vc_lib/mswud/wx/* "$OUTPUT"/include/wx/
	cp -r "$BUILDS"/wxWidgets-2.9.1/lib/vc_lib/mswu/wx/* "$OUTPUT"/include/wx/
	cp "$BUILDS"/wxWidgets-2.9.1/lib/vc_lib/*.lib "$OUTPUT"/lib/
	cp "$BUILDS"/wxWidgets-2.9.1/lib/vc_lib/*.pdb "$OUTPUT"/lib/
	cp "$BUILDS"/wxWidgets-2.9.1/utils/wxrc/vc_mswu/wxrc.exe "$OUTPUT"/bin/
else
	cp -r "$BUILDS"/wxWidgets-2.9.1/lib/vc_amd64_lib/mswud/wx/* "$OUTPUT"/include/wx/
	cp -r "$BUILDS"/wxWidgets-2.9.1/lib/vc_amd64_lib/mswu/wx/* "$OUTPUT"/include/wx/
	cp "$BUILDS"/wxWidgets-2.9.1/lib/vc_amd64_lib/*.lib "$OUTPUT"/lib/
	cp "$BUILDS"/wxWidgets-2.9.1/lib/vc_amd64_lib/*.pdb "$OUTPUT"/lib/
	cp "$BUILDS"/wxWidgets-2.9.1/utils/wxrc/vc_mswu_amd64/wxrc.exe "$OUTPUT"/bin/
fi
chmod -R 755 "$OUTPUT"/bin/wxrc.exe

echo "Done."

###############################################################################
## zlib - Compression library

ZLIBVER=1.2.7
echo
echo "zlib (zlib-$ZLIBVER)"
echo "================="
if [ -f "$BUILDS"/zlib-$ZLIBVER/build_finished ]; then
	echo "(Using build output from previous build.)"
else
	download http://zlib.net/zlib-$ZLIBVER.tar.gz zlib-$ZLIBVER.tar.gz
	
	echo "Unpacking..."
	cd "$BUILDS"
	rm -rf zlib-$ZLIBVER # Clear files from previous build.
	tar xzfo "$DOWNLOADS"/zlib-$ZLIBVER.tar.gz
	
	echo "Building..."
	cd "$BUILDS"/zlib-$ZLIBVER
	LOGFILE="$BUILDS"/zlib-$ZLIBVER/build_log.txt
	echo "(Build output logged at $LOGFILE)"
	cl /c -D_CRT_SECURE_NO_DEPRECATE /MD /O2 /Ob2 *.c > "$LOGFILE" 2>&1
	lib /out:zlib.lib *.obj                 >> "$LOGFILE" 2>&1
	
	touch "$BUILDS"/zlib-$ZLIBVER/build_finished
fi

echo "Copying files..."
cp "$BUILDS"/zlib-$ZLIBVER/zlib.h   "$OUTPUT"/include/
cp "$BUILDS"/zlib-$ZLIBVER/zconf.h  "$OUTPUT"/include/
cp "$BUILDS"/zlib-$ZLIBVER/zlib.lib "$OUTPUT"/lib/

echo "Done."

###############################################################################
## Done.

echo
echo
echo "Finished building vv-thirdparty for $ARCH!"
echo
echo "It is safe to delete the \"downloads\" and \"builds-$ARCH\" directories."
