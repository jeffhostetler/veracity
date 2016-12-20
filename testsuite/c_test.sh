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

## usage: c_test.sh c_test_name ${CMAKE_CURRENT_SOURCE_DIR}
set -o xtrace

export TEST=$1
##echo TEST is $TEST

export SRCDIR=$2
##echo SRCDIR is $SRCDIR

export BINDIR=`pwd`
##echo BINDIR is $BINDIR

export TEMPDIR="$BINDIR"/temp/$TEST
##echo TEMPDIR is $TEMPDIR

## We need vv to be in our path.
export PATH=$BINDIR/../src/cmd:$PATH
##echo PATH is $PATH

## Start with a clean (nonexistent) TEMPDIR
if [ -e "$TEMPDIR" ]; then
    rm -rf "$TEMPDIR"
    mkdir -p $TEMPDIR
fi

TIME=`date +%s`
export SGCLOSET="$TEMPDIR/.sgcloset-test.$TIME"
##echo SGCLOSET is $SGCLOSET

## Turn on core dumps.
ulimit -c unlimited

## Report settings.
####ulimit -a

## Dump the localsettings environment.
####vv config

echo "////////////////////////////////////////////////////////////////"
echo "Starting test $TEST..."
echo "////////////////////////////////////////////////////////////////"
echo ""

./$TEST "${SRCDIR}"
RESULT_TEST=$?

if [ $RESULT_TEST -eq 139 ]; then
    ## If the test crashed (SIGSEGV), try to get a stacktrace.
    echo ""
    echo "////////////////////////////////////////////////////////////////"
    echo "Getting stacktrace..."
    echo "////////////////////////////////////////////////////////////////"
    echo ""
    ## the most recent coredump should be ours
    COREFILE=`ls -t /cores | head -1`
    GDBINPUT="$BINDIR"/gdbinput.txt
    echo "thread apply all bt full" >  $GDBINPUT
    echo "quit"                     >> $GDBINPUT
    /usr/bin/gdb -batch -x $GDBINPUT ./$TEST /cores/$COREFILE
    rm $GDBINPUT

elif [ $RESULT_TEST -eq 134 ]; then
    ## If the test crashed with an ASSERT (Abort trap).
    ## I observed this on a MAC.
    ## TODO confirm Linux throws the same signal.
    echo ""
    echo "////////////////////////////////////////////////////////////////"
    echo "Getting stacktrace..."
    echo "////////////////////////////////////////////////////////////////"
    echo ""
    ## the most recent coredump should be ours
    COREFILE=`ls -t /cores | head -1`
    GDBINPUT="$BINDIR"/gdbinput.txt
    echo "thread apply all bt full" >  $GDBINPUT
    echo "quit"                     >> $GDBINPUT
    /usr/bin/gdb -batch -x $GDBINPUT ./$TEST /cores/$COREFILE
    rm $GDBINPUT
fi


exit $RESULT_TEST;
