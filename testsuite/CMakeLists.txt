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

set(SG_TESTS
u0001_stdint.c
u0002_id.c
u0003_errno.c
u0004_file.c
u0005_string.c
u0007_hex.c
u0008_misc_utils.c
u0010_dbrecord.c
u0012_utf8.c
u0013_fsobj.c
u0014_pathname.c
u0015_dir.c
u0019_xmlwriter.c
u0020_utf8pathnames.c
u0021_workingdir.c
u0023_vcdiff.c
u0024_repo_query_implementation.c
u0025_jsonwriter.c
u0026_jsonparser.c
u0027_pool.c
u0028_vhash.c
u0031_repopath.c
u0032_repo.c
u0034_repo_treenode.c
u0036_committing.c
u0037_dag.c
u0038_closet.c
#### disabled due to dependancy on kilmister u0040_unc.c
u0041_rbtree.c
#### disabled for WC u0042_portability.c
#### disabled for WC u0043_pendingtree.c
u0045_console.c
#### disabled for WC u0046_pendport.c
u0047_repo_blobs.c
u0048_multidag.c
#### disabled for WC u0050_logstuff.c
u0051_hidlookup.c
u0052_zip.c
u0054_repo_encodings.c
u0055_vector.c
u0056_exec.c
u0058_repotx.c
u0061_push.c
u0062_str_utils.c
u0063_error_context.c
u0065_vector_i64.c
u0066_log.c
u0068_filespec.c
u0069_treendx.c
u0070_treenode_search.c
u0072_repo_hash_method.c
u0073_pull.c
u0074_web_utils.c
#### disabled for WC u0075_pendingtree_log.c
u0076_time.c
u0077_jsondb.c
u0078_diff.c
u0079_mutex.c
u0081_varray.c
u0082_rbtreedb.c
u0083_timestampcache.c
# unneeded u0085_crash.c
u0086_sync_hardwired_templates.c
u0087_linediff.c
u0104_treenode_entry.c
u0105_repopath.c
u0106_filetool.c
u0107_hdb.c
u0108_bitvector.c
#### disabled for WC u0109_resolve.c
u0110_validate.c
u0111_echo_argv.c
u0111_fast_import.c
)

file(GLOB PRIVATE_HEADERS ./*.h)

IF (WIN32)

	## The .BAT file needs to be given pathnames with DOS-style backslashes.
	## The following command works, in that it converts the slashes and the
	## pathname gets into the .BAT file properly, but it causes problems for
	## CTest somewhere and we get a "Policy CMP0010" warning because some of
	## the \s in z:\shared\v\veracity... isn't a valid escape sequence.
	##
	## FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR} DOS_CURRENT_SOURCE_DIR)
	##
	## So, we do it the hard way.

	MACRO(MAKE_WINDOWS_PATH pathname)
		## Convert '/' to "\\" (doubled up to get past parser here)
		STRING(REPLACE "/" "\\\\" ${pathname} "${${pathname}}")
	ENDMACRO(MAKE_WINDOWS_PATH)
	SET(DOS_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
	MAKE_WINDOWS_PATH(DOS_CURRENT_SOURCE_DIR)		

	## The .BAT file runs the test, and some of the tests run vv.exe,
	## so we need to be sure that it is in our path.  The NMAKE version
	## puts vv in the cmd directory; the MSVC .sln/.vcproj version puts
	## vv in cmd/Debug.  For Release builds this is cmd/Release.
	##
	## So we need to "fix" things here; we set the relative portion of the path.

	IF(CMAKE_MAKE_PROGRAM MATCHES "nmake")
		SET(MY_RELEXE_DIR ".")
	ELSE()
		SET(MY_RELEXE_DIR ${CMAKE_BUILD_TYPE})
	ENDIF()

	MACRO(ADD_C_TEST testname)
		add_test(${testname} "${CMAKE_CURRENT_SOURCE_DIR}/c_test.bat" "${testname}" "${DOS_CURRENT_SOURCE_DIR}" "${MY_RELEXE_DIR}")
		set_target_properties(${testname} PROPERTIES FOLDER "Tests/C Suite")
		source_group("Private Headers" FILES ${PRIVATE_HEADERS})
	ENDMACRO(ADD_C_TEST)

ELSE()
	# Otherwise, we have a unix-based system with BASH.

	MACRO(ADD_C_TEST testname)
		add_test(${testname} /bin/bash "${CMAKE_CURRENT_SOURCE_DIR}/c_test.sh" "${testname}" "${CMAKE_CURRENT_SOURCE_DIR}")
	ENDMACRO(ADD_C_TEST)

ENDIF()


# Add an executable and test for each test
foreach(ut ${SG_TESTS})
string(REPLACE ".c" "" basename ${ut})
add_executable(${basename} ${ut} ${PRIVATE_HEADERS})
target_link_libraries(${basename} sglib sg_wc sg_fs3 ${SG_THIRDPARTY_LIBS} ${SG_OS_LIBS})
add_dependencies(${basename} template_check)
ADD_C_TEST(${basename} ${EXECUTABLE_OUTPUT_PATH}/${basename} ${CMAKE_CURRENT_SOURCE_DIR})
endforeach(ut ${SG_TESTS})

IF (NOT WIN32)
	add_test(filespec_bash_ref /bin/bash "${CMAKE_CURRENT_SOURCE_DIR}/filespec_bash_ref.sh" "${CMAKE_CURRENT_SOURCE_DIR}/u0068_filespec.c")
ENDIF()

# special case for u0064_mrg, which needs PATH set for vv
add_executable(u0064_mrg u0064_mrg.c)
target_link_libraries(u0064_mrg sglib sg_wc sg_fs3 ${SG_THIRDPARTY_LIBS} ${SG_OS_LIBS})
## add_test(u0064_mrg "${EXECUTABLE_OUTPUT_PATH}/u0064_mrg" "${CMAKE_CURRENT_SOURCE_DIR}")
ADD_C_TEST(u0064_mrg)


# special case for u1100_large_blob so we can pass it the large blob directory as an argument

add_executable(u1100_large_blob u1100_large_blob.c)
set_target_properties(u1100_large_blob PROPERTIES FOLDER "Tests/C Suite")
target_link_libraries(u1100_large_blob sglib sg_wc sg_fs3 ${SG_THIRDPARTY_LIBS} ${SG_OS_LIBS})
if (SG_LONGTESTS)
	#ORIG#	add_test(u1100_large_blob ${EXECUTABLE_OUTPUT_PATH}/u1100_large_blob ${LARGE_BLOB_DIR})
	IF (WIN32)
		add_test(u1100_large_blob "${CMAKE_CURRENT_SOURCE_DIR}/c_test.bat" "u1100_large_blob" "${LARGE_BLOB_DIR}" "${MY_RELEXE_DIR}")
	ELSE()
		add_test(u1100_large_blob /bin/bash "${CMAKE_CURRENT_SOURCE_DIR}/c_test.sh" "u1100_large_blob" "${LARGE_BLOB_DIR}")
	ENDIF()
ENDIF()

# Generate u0000.  This test is another special case.
# It is generated at configure time by cmake here.
# It does not include unittests.h.  It simply calls
# every free function with NULL and returns 0 (success)
# at the end.  It is considered a successful test
# run if it doesn't crash.

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/u0000_free_null.c "#include <sg.h>\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_free_null.c "\nint main(void)\n{\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_free_null.c "\tSG_context * pCtx = NULL;\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_free_null.c "\tSG_context__alloc(&pCtx);\n\n")
file(GLOB hdrs ${PUBLIC_SOURCE_TOP_DIR}/src/libraries/include/*.h) 
set(mall "")
foreach(h ${hdrs})
    file(READ ${h} f)
    string(REGEX MATCHALL "SG[a-z0-9_]+__free" mlist ${f})
    list(APPEND mall ${mlist})
endforeach()
list(REMOVE_DUPLICATES mall)
list(SORT mall)
foreach(m ${mall})
    if(m STREQUAL "SG_context__free")
        file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_free_null.c "\t${m}(NULL);\n")
    elseif(m STREQUAL "SG_stringlist__free")
        file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_free_null.c "\t${m}(pCtx,NULL,0);\n")
    else()
        file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_free_null.c "\t${m}(pCtx,NULL);\n")
    endif()
endforeach()
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_free_null.c "\n\tSG_context__free(pCtx);\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_free_null.c "\treturn 0;\n}\n")
ADD_EXECUTABLE(u0000_free_null ${CMAKE_CURRENT_BINARY_DIR}/u0000_free_null.c)
target_link_libraries(u0000_free_null sglib sg_wc sg_fs3 ${SG_THIRDPARTY_LIBS} ${SG_OS_LIBS})
set_target_properties(u0000_free_null PROPERTIES FOLDER "Tests/C Suite")
add_test(u0000_free_null u0000_free_null)

# Generate u0000_error_messages.. Another special case.

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "#include <sg.h>\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "#include \"${PUBLIC_SOURCE_TOP_DIR}/testsuite/unittests.h\"\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "TEST_MAIN(u0000_error_messages)\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "{\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "\tchar sz[SG_ERROR_BUFFER_SIZE];\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "\tint len;\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "\tTEMPLATE_MAIN_START;\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "\n")
set(mall "")
file(READ ${PUBLIC_SOURCE_TOP_DIR}/src/libraries/include/sg_error_typedefs.h f)
string(REGEX MATCHALL "SG_ERR_SG_LIBRARY\\([1-9][0-9]*\\)" mlist ${f})
list(APPEND mall ${mlist})
foreach(m ${mall})
	if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
		file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "\tSG_error__get_message(${m},SG_TRUE,sz,SG_ERROR_BUFFER_SIZE);\n")
    else()
		file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "\tSG_error__get_message(${m},SG_FALSE,sz,SG_ERROR_BUFFER_SIZE);\n")
	endif()


    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "\tlen = SG_STRLEN(sz);\n")

	if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
		file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "\tVERIFY_COND(\"Descriptive error message should be defined for error code.\", len>20);\n")
    else()
		file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "\tVERIFY_COND(\"Descriptive error message should be defined for error code.\", len>1);\n")
	endif()

	file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "\tVERIFY_COND(\"Error message should not end with a period or a colon.\", sz[len-1]!='.' && sz[len-1]!=':');\n")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "\n")
endforeach()
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "\tTEMPLATE_MAIN_END;\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c "}\n")
ADD_EXECUTABLE(u0000_error_messages ${CMAKE_CURRENT_BINARY_DIR}/u0000_error_messages.c)
set_target_properties(u0000_error_messages PROPERTIES FOLDER "Tests/C Suite")
target_link_libraries(u0000_error_messages sglib sg_wc sg_fs3 ${SG_THIRDPARTY_LIBS} ${SG_OS_LIBS})
add_test(u0000_error_messages u0000_error_messages)

get_target_property(SGLIB_LOCATION sglib LOCATION)
get_filename_component(SGLIB_DIR ${SGLIB_LOCATION} PATH)
set(SGLIB_OBJECT_DIR "${SGLIB_DIR}${CMAKE_FILES_DIRECTORY}/sglib.dir")
#message(${SGLIB_OBJECT_DIR})
#message(${CMAKE_CURRENT_BINARY_DIR})

