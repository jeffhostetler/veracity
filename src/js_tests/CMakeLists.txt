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

## Add "test" target to run the javascript tests.
## This causes them to be run whenever you type "make test".
## If you are in this directory, just the javascript tests will run.
## If you are at top level, then both the JS and C tests will run.
##
## WARNING: We go through a few hurdles so that the tests can reference the SOURCE
## WARNING: directory for the test scripts and the BUILD directory for the .EXEs
## WARNING: and temp space to put the generated REPOs, Working Copies, and
## WARNING: other trash.  The goal here is to NOT dirty up the SOURCE directory.
##
## TODO It'd be nice if we could change vscript and/or vscript_tests.js to take a pathname
## TODO to the SOURCE directory.  (And maybe add the vv and vscript exe directories
## TODO to our PATH.

## Use GLOB to get a list of all of the st*.js test files.  These
## come in as full pathnames rather than entrynames it seems.

FILE(GLOB            SG_JS_TESTS st*.js)
FILE(GLOB   nightly__SG_JS_TESTS nightly__st*.js)
FILE(GLOB longtests__SG_JS_TESTS longtests__st*.js)

FILE(COPY "${CMAKE_CURRENT_SOURCE_DIR}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/..")

IF (SG_REAL_BUILD)
	SET(SG_JS_DEFINES "${SG_JS_DEFINES} -D SG_REAL_BUILD")
ENDIF (SG_REAL_BUILD)
IF (SG_NIGHTLY_BUILD)
	SET(SG_JS_DEFINES "${SG_JS_DEFINES} -D SG_NIGHTLY_BUILD")
	SET(SG_JS_TESTS ${nightly__SG_JS_TESTS} ${SG_JS_TESTS})
ENDIF (SG_NIGHTLY_BUILD)
IF (SG_LONGTESTS)
	SET(SG_JS_DEFINES "${SG_JS_DEFINES} -D SG_LONGTESTS")
	SET(SG_JS_TESTS ${longtests__SG_JS_TESTS} ${SG_JS_TESTS})
ENDIF (SG_LONGTESTS)

IF (SG_SQS_CONFIG_PATH)
	SET(SG_JS_DEFINES "${SG_JS_DEFINES} -D SG_SQS_CONFIG_PATH=${SG_SQS_CONFIG_PATH}")
ENDIF (SG_SQS_CONFIG_PATH)

IF("${ENABLE_SSJS_MODULE}" MATCHES "cirrus")
	SET(SG_JS_DEFINES "${SG_JS_DEFINES} -D SG_CIRRUS")
ENDIF("${ENABLE_SSJS_MODULE}" MATCHES "cirrus")

IF (WIN32)

	## The .BAT file needs to be given pathnames with DOS-style backslashes.
	## The following command works, in that it converts the slashes and the
	## pathname gets into the .BAT file properly, but it causes problems for
	## CTest somewhere and we get a "Policy CMP0010" warning because some of
	## the \s in z:\shared\v\veracity... isn't a valid escape sequence.
	##
	## FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR} DOS_CURRENT_BINARY_DIR)
	##
	## So, we do it the hard way.

	MACRO(MAKE_WINDOWS_PATH pathname)
		## Convert '/' to "\\" (doubled up to get past parser here)
		STRING(REPLACE "/" "\\\\" ${pathname} "${${pathname}}")
	ENDMACRO(MAKE_WINDOWS_PATH)
	SET(DOS_CURRENT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
	MAKE_WINDOWS_PATH(DOS_CURRENT_BINARY_DIR)		

	## The .BAT file runs vscript.exe and some of the JS tests run vv.exe,
	## so we need to be sure that they are in our path.  The NMAKE version
	## puts them in the cmd & script directory; the MSVC .sln/.vcproj version
	## puts them in cmd/Debug and script/Debug.  For Release builds this is
	## cmd/Release and script/Release.
	##
	## So we need to "fix" things here; we set the relative portion of the path.

	IF(CMAKE_MAKE_PROGRAM MATCHES "nmake")
		SET(MY_RELEXE_DIR ".")
	ELSE()
		SET(MY_RELEXE_DIR ${CMAKE_BUILD_TYPE})
	ENDIF()
	
	##SET(DOS_PUBLIC_BINARY_TOP_DIR ${PUBLIC_BINARY_TOP_DIR})
	##MAKE_WINDOWS_PATH(DOS_PUBLIC_BINARY_TOP_DIR)
	
	FILE(TO_NATIVE_PATH ${PUBLIC_BINARY_TOP_DIR} DOS_PUBLIC_BINARY_TOP_DIR)
	# Configure the js_test.bat script with some information from our build environment
	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/js_test.bat.in"
				"${CMAKE_CURRENT_BINARY_DIR}/js_test.bat")
	# Likewise for the debugger version.
	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/js_devenv.bat.in"
				"${CMAKE_CURRENT_BINARY_DIR}/js_devenv.bat")
	
	foreach(js_pathname ${SG_JS_TESTS})
		GET_FILENAME_COMPONENT(js_filename ${js_pathname} NAME)
		add_test(${js_filename} "${CMAKE_CURRENT_BINARY_DIR}/js_test.bat" "${DOS_CURRENT_BINARY_DIR}" "${js_filename}" "${MY_RELEXE_DIR}" "${SG_JS_DEFINES}")
	endforeach(js_pathname ${SG_JS_TESTS})
	
	CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/Ctest_starting.cmake.in
                ${CMAKE_CURRENT_BINARY_DIR}/Ctest_starting.cmake)
	SET_DIRECTORY_PROPERTIES(PROPERTIES TEST_INCLUDE_FILE "Ctest_starting.cmake")
	
	# ADD_CUSTOM_TARGET(update_js_tests ALL
	# 	COMMAND xcopy *.js "${CMAKE_CURRENT_BINARY_DIR}")
	
ELSE()
	# Otherwise, we have a unix-based system with BASH.
	
	# Configure the js_test.sh script with some information from our build environment
	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/js_test.sh.in"
				"${CMAKE_CURRENT_BINARY_DIR}/js_test.sh")
	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/js_gdb.sh.in"
				"${CMAKE_CURRENT_BINARY_DIR}/js_gdb.sh")
		
	foreach(js_pathname ${SG_JS_TESTS})
		GET_FILENAME_COMPONENT(js_filename ${js_pathname} NAME)
		add_test(${js_filename} /bin/bash "${CMAKE_CURRENT_BINARY_DIR}/js_test.sh" "${CMAKE_CURRENT_BINARY_DIR}" "${js_filename}" "${SG_JS_DEFINES}")
	endforeach(js_pathname ${SG_JS_TESTS})
	
	# ADD_CUSTOM_TARGET(update_js_tests ALL
	# 	COMMAND cp *.js "${CMAKE_CURRENT_BINARY_DIR}")
	

ENDIF()

