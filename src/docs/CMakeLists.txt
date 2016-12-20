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

FILE(GLOB MD_SOURCES vv*.md)

FILE(COPY "${CMAKE_CURRENT_SOURCE_DIR}/mdparser.js" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
FILE(COPY "${CMAKE_CURRENT_SOURCE_DIR}/md2html.js" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
FILE(COPY "${CMAKE_CURRENT_SOURCE_DIR}/md2c.js" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

foreach( tj ${MD_SOURCES} )
	 GET_FILENAME_COMPONENT(basename ${tj} NAME_WE)
#	string(REPLACE ".md" "" basename ${tj})
	ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${basename}.c
		COMMAND vscript --no-modules ${CMAKE_CURRENT_SOURCE_DIR}/md2c.js ${CMAKE_CURRENT_SOURCE_DIR}/${basename}.md ${CMAKE_CURRENT_BINARY_DIR}/${basename}.c
		DEPENDS vscript ${CMAKE_CURRENT_SOURCE_DIR}/${basename}.md ${CMAKE_CURRENT_SOURCE_DIR}/md2c.js ${CMAKE_CURRENT_SOURCE_DIR}/mdparser.js
    )		  
    set(C_SOURCES ${C_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/${basename}.c)
endforeach(tj ${MD_SOURCES})

add_library(sghelpsource STATIC ${C_SOURCES})
set_target_properties(sghelpsource PROPERTIES FOLDER "Libraries")
target_link_libraries(sghelpsource ${SG_OS_LIBS})

