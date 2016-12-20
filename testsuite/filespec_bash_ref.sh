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

# This script goes through a list of patterns and paths, checking for matches
# and failures.  It's intended to "keep up honest" in the way Veracity handles
# wildcards.

# Usage: ./filespec.sh <path_to_source>
#
# The source is expected be a tab-delimited data list of the form:
# [1|0]	"quoted_pattern"	"quoted_path"
#
# If the source file has a .c suffix, then it is assumed to be the actual
# test code, in which case it will be parsed, and the data list will
# be generated on the fly.

# NOTE: source_parse doesn't handle embedded spaces

function BuildDataFile
{
	# $1 is input_file - $2 is output_file
	echo "- Generating test data file: $2 from source: $1 ..."
	[ -e "$2" ] && rm "$2"
	# ensure Unixy EOLs; grep just the TRUE|FALSE lines; eliminate lines with '\' 'NULL' and '""'
	# also eliminate lines with the keyword 'NOBASH'
	tr -d '\r' <"${1}" | grep '{ SG_TRUE,  ".*", 0u \|{ SG_FALSE, ".*", 0u ' | grep -v '\\\|""\|NULL\|NOBASH' > $TEMPFILE
	echo "# The following lines were removed from the SOURCE file ($1)">>$SKIPLOG
	tr -d '\r' <"${1}" | grep '{ SG_TRUE,  ".*", 0u \|{ SG_FALSE, ".*", 0u ' | grep '\\\|""\|NULL\|NOBASH' >>$SKIPLOG
	while read x a b c d y
	do
		MATCH="${a:3:$(( ${#a} - 4 ))}"
		MASK="${b:1:$(( ${#b} - 3 ))}"
		SAMPLE="${c:1:$(( ${#c} - 3 ))}"
		[ "$MATCH" == "TRUE" ] && MATCH=1 || MATCH=0

		if [ "$d" == "0u" ] ; then
			echo -e "${MATCH}\tx\t${MASK}\t${SAMPLE}" >> "$2"
		else
			echo "## ERROR parsing: $x $a $b $c $d $y" >> "$ERRORLOG"
			echo "## ERROR parsing: $x $a $b $c $d $y"
			EXITCODE=1
		fi
	done < $TEMPFILE
}

function FilterInputFile
{
	# $1 is input_file - $2 is output_file
	echo "- Filtering test data file: $2 from input: $1 ..."
	[ -e "$2" ] && rm "$2"
	# ensure Unixy EOLs; eliminate comments and lines with '\' 'NULL' and '""'
	# also eliminate lines with the keyword 'NOBASH'
	if [ "${BASH_VERSINFO[0]}" -ge "4" ] ; then
		tr -d '\r' <"${1}" | grep -v '^#\|\\\|""\|NULL\|NOBASH' > $TEMPFILE
		echo "# The following lines were removed from the TEST DATA file ($1)">>$SKIPLOG
		tr -d '\r' <"${1}" | grep '^#\|\\\|""\|NULL\|NOBASH' >> $SKIPLOG
	else
		# for older BASH versions, skip the star-star
		tr -d '\r' <"${1}" | grep -v '^#\|\\\|""\|\*\*\|NULL\|NOBASH' > $TEMPFILE
		echo "# The following lines were removed from the TEST DATA file ($1)">>$SKIPLOG
		echo "# tests using globstar wildcards (/**/) are removed for pre-4 versions of BASH">>$SKIPLOG
		echo "# Your bash version is $BASH_VERSION"
		tr -d '\r' <"${1}" | grep '^#\|\\\|""\|\*\*\|NULL\|NOBASH' >> $SKIPLOG
	fi
	
	# remove surrounding quotes
	IFS=$' \n'
	while read line
	do
		if [ -n "$line" ] ; then
			MATCH=$(echo $line | cut -f 1)
			MASK=$(echo $line | cut -f 3)
			SAMPLE=$(echo $line | cut -f 4)
			overflow=$(echo $line | cut -f 5)
			if [ -z "$SAMPLE" ] || [ -n "$overflow" ] && [ "${overflow:0:1}" != "#" ] ; then
				echo "## ERROR parsing: MATCH=[$MATCH]" >> "$ERRORLOG"
				echo "                  MASK=[$MASK]" >> "$ERRORLOG"
				echo "                  SAMPLE=[$SAMPLE]" >> "$ERRORLOG"
				echo "                  overflow=[$overflow]" >> "$ERRORLOG"
				echo "## ERROR parsing: MATCH=[$MATCH]"
				echo "                  MASK=[$MASK]"
				echo "                  SAMPLE=[$SAMPLE]"
				echo "                  overflow=[$overflow]"
				EXITCODE=1
			else
				[ "${MATCH:0:1}" == "\"" ] && MATCH="${MATCH:1}"
				[ "${MATCH:(-1)}" == "\"" ] && MATCH="${MATCH:0:$(( ${#MATCH} - 1 ))}"
				[ "${MASK:0:1}" == "\"" ] && MASK="${MASK:1}"
				[ "${MASK:(-1)}" == "\"" ] && MASK="${MASK:0:$(( ${#MASK} - 1 ))}"
				[ "${SAMPLE:0:1}" == "\"" ] && SAMPLE="${SAMPLE:1}"
				[ "${SAMPLE:(-1)}" == "\"" ] && SAMPLE="${SAMPLE:0:$(( ${#SAMPLE} - 1 ))}"
				echo -e "${MATCH}\tx\t${MASK}\t${SAMPLE}" >> "$2"
			fi
		fi
	done < $TEMPFILE
}

function ProcessDataFile
{
	# $1 is test_data_file
	# ensure Unixy EOLs
	tr -d '\r' <"${1}" > $TEMPFILE
	IFS=$' \n'
	while read line
	do
		MATCH=$(echo $line | cut -f 1)
		MASK=$(echo $line | cut -f 3)
		SAMPLE=$(echo $line | cut -f 4)
		overflow=$(echo $line | cut -f 5)
		if [ -z "$SAMPLE" ] || [ -n "$overflow" ] ; then
			echo "## ERROR parsing: MATCH=[$MATCH]" >> "$ERRORLOG"
			echo "                  MASK=[$MASK]" >> "$ERRORLOG"
			echo "                  SAMPLE=[$SAMPLE]" >> "$ERRORLOG"
			echo "                  overflow=[$overflow]" >> "$ERRORLOG"
			echo "## ERROR parsing: MATCH=[$MATCH]"
			echo "                  MASK=[$MASK]"
			echo "                  SAMPLE=[$SAMPLE]"
			echo "                  overflow=[$overflow]"
			EXITCODE=1
		else
			TestSample
		fi
	done < $TEMPFILE
	IFS=$' \t\n'
}

function TestSample
{
	if [ "$MATCH" == "1" ] ; then
		expect="=="
	else
		expect="<>"
	fi
	mkdir -p "$SAMPLE"
	if [ ! -d "$SAMPLE" ] ; then
		LogFsError creating directory
		return
	fi
	TestMask dir
	rm -rf * .?* >/dev/null 2>&1
	if [ -d "$SAMPLE" ] ; then
		[ "$(basename $SAMPLE)" == "." ] || LogFsError deleting directory
		return
	fi
	mkdir -p "$(dirname $SAMPLE)"
	touch "$SAMPLE"
#	if [ ! -f "$SAMPLE" ] ; then ### this reports an error when checking for foo/. - can't create as file
	if [ ! -e "$SAMPLE" ] ; then
		LogFsError creating file
		return
	fi
	TestMask file
	rm -rf * .?* >/dev/null 2>&1
	if [ -f "$SAMPLE" ] ; then
		LogFsError deleting file
		return
	fi
}

function LogFsError
{
	echo "## ERROR $1 $2: $SAMPLE" >> "$ERRORLOG"
	echo "## ERROR $1 $2: $SAMPLE"
	EXITCODE=1
}

function TestMask
{
# set non-matching patterns to return null, rather than the pattern
shopt -s nullglob
	localresult="0"
	for f in $MASK
	do
		# ignore trailing "/"
		[ "${f:(-1)}" == "/" ] && f="${f:0:$(( ${#f} - 1 ))}"
		[ "$f" == "$SAMPLE" ] && localresult="1"
	done
	if [ "$localresult" == "$MATCH" ] ; then
		# passed
		echo "Passed: \"$MASK\" $expect \"$SAMPLE\" ($1)" >>"$OUTLOG"
		echo "Passed: \"$MASK\" $expect \"$SAMPLE\" ($1)"
	else
		# failed
		EXITCODE=1
		echo "FAILED: \"$MASK\" $expect \"$SAMPLE\" ($1)" >>"$OUTLOG"
		echo "       (" ${MASK} ")" >>"$OUTLOG"
		echo "FAILED: \"$MASK\" $expect \"$SAMPLE\" ($1)"
		echo "       (" ${MASK} ")"
		echo "FAILED: \"$MASK\" $expect \"$SAMPLE\" ($1)" >>"$ERRORLOG"
		echo "       (" ${MASK} ")" >>"$ERRORLOG"
	fi
shopt -u nullglob
}


################## main() ###################

HOMEDIR="${PWD}/wildbash"
TESTDIR="${HOMEDIR}/testdir"
INTERFILE="${HOMEDIR}/filespec_intermediate_data.txt"
DATAFILE="${HOMEDIR}/filespec_test_data.txt"
OUTLOG="${HOMEDIR}/fs_output.txt"
ERRORLOG="${HOMEDIR}/fs_errors.txt"
SKIPLOG="${HOMEDIR}/fs_skipped.txt"
TEMPFILE="${HOMEDIR}/filespec_temp.txt"
EXITCODE=0

if [ -z "$1" ] ; then
	echo "# ERROR: filespec.sh was called with no source argument"
	echo "#"
	echo "# Usage: ./filespec.sh <path_to_source>"
	echo "#"
	echo "# The source is expected be a tab-delimited data list of the form:"
	echo '# [1|0]	"quoted_pattern"	"quoted_path"'
	echo "#"
	echo "# If the source file has a .c suffix, then it is assumed to be the actual"
	echo "# test code, in which case it will be parsed, and the data list will"
	echo "# be generated on the fly."
	exit 1
fi

# convert the input file to an absolute path
D=`dirname "$1"`
B=`basename "$1"`
INPUTFILE="`cd \"$D\" 2>/dev/null && pwd || echo \"$D\"`/$B"

# enable /**/
shopt -s globstar

[ -e "$HOMEDIR" ] && rm -rf "$HOMEDIR"
mkdir -p "$TESTDIR"

pushd "$TESTDIR" >/dev/null
if [ "${INPUTFILE:(-2)}" == ".c" ] ; then
	BuildDataFile "$INPUTFILE" "$INTERFILE"
	FilterInputFile "$INTERFILE" "$DATAFILE"
else
	FilterInputFile "$INPUTFILE" "$DATAFILE"
fi
ProcessDataFile "$DATAFILE"
popd >/dev/null
echo "== SKIPPED LINES: =================================================="
[ -e "$SKIPLOG" ] && cat "$SKIPLOG" || echo "(none)"
echo "== ERROR SUMMARY: =================================================="
[ -e "$ERRORLOG" ] && cat "$ERRORLOG" || echo "(no errors)"
exit $EXITCODE
