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

add_subdirectory(hexdump)

add_subdirectory(libraries)

add_subdirectory(cmd)

add_subdirectory(script)

add_subdirectory(docs)


IF(NOT SG_IOS)
add_subdirectory(ztcheck)
ENDIF()

FILE(COPY "${CMAKE_CURRENT_SOURCE_DIR}/js_test_lib"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
	
add_subdirectory(js_tests)

IF(WIN32 AND NOT CMAKE_COMPILER_IS_GNUCC)
    add_subdirectory(tortoise)
	add_subdirectory(winsync)
endif () # WIN32 AND NOT CMAKE_COMPILER_IS_GNUCC

add_subdirectory(thirdparty)


FILE(COPY "${CMAKE_CURRENT_SOURCE_DIR}/server_files"
		DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")

add_custom_target(
	update_server_files ALL
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/server_files ${CMAKE_CURRENT_BINARY_DIR} 
)

get_filename_component(SERVER_FILES ${CMAKE_CURRENT_BINARY_DIR}/server_files ABSOLUTE)
set(SERVER_FILES_MIN ${SERVER_FILES}/ui/min)

add_custom_target(minify_server_files ALL
			  COMMAND vscript --no-modules
			  ${CMAKE_CURRENT_SOURCE_DIR}/cmd/renderscripts.js
			  ${SERVER_FILES}
			  ${SERVER_FILES_MIN} 
			  > ${CMAKE_CURRENT_BINARY_DIR}/renderscripts.log
		  	  DEPENDS update_server_files vscript
		  	  COMMENT "Generating Minified JS and CSS")

FILE(COPY ${SERVER_FILES}/ui/css/custom-theme/images
	DESTINATION ${SERVER_FILES_MIN})


INSTALL(
	DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/server_files
	DESTINATION "${Veracity_DATA_DEST}"
	FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	DIRECTORY_PERMISSIONS OWNER_READ OWNER_EXECUTE OWNER_WRITE
                        GROUP_READ GROUP_EXECUTE
                        WORLD_READ WORLD_EXECUTE
)


IF(APPLE)
	SET(DYLIBS
		${VVTHIRDPARTY}/lib/libicudata.46.0.dylib
		${VVTHIRDPARTY}/lib/libicudata.46.dylib
		${VVTHIRDPARTY}/lib/libicudata.dylib
		${VVTHIRDPARTY}/lib/libicui18n.46.0.dylib
		${VVTHIRDPARTY}/lib/libicui18n.46.dylib
		${VVTHIRDPARTY}/lib/libicui18n.dylib
		${VVTHIRDPARTY}/lib/libicuuc.46.0.dylib
		${VVTHIRDPARTY}/lib/libicuuc.46.dylib
		${VVTHIRDPARTY}/lib/libicuuc.dylib
		${VVTHIRDPARTY}/lib/libnspr4.dylib
		${VVTHIRDPARTY}/lib/libplc4.dylib
		${VVTHIRDPARTY}/lib/libplds4.dylib
		${VVTHIRDPARTY}/lib/libmozjs185.1.0.dylib
		${VVTHIRDPARTY}/lib/libmozjs185.1.0.0.dylib
		${VVTHIRDPARTY}/lib/libmozjs185.dylib
	)
	
	foreach(dylib ${DYLIBS})
		INSTALL(FILES "${dylib}"
				DESTINATION "${Veracity_LIB_DEST}"
				PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
				 				 GROUP_READ GROUP_EXECUTE
				 				 WORLD_READ WORLD_EXECUTE)
	endforeach()
ENDIF(APPLE)
