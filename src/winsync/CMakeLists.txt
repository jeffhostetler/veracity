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

# set the application's name
set(APP_NAME vvWinSync)

# vv-thirdparty directories
set(VVTHIRDPARTY_BIN "${VVTHIRDPARTY}/bin")
set(VVTHIRDPARTY_INCLUDE "${VVTHIRDPARTY}/include")
set(VVTHIRDPARTY_LIB "${VVTHIRDPARTY}/lib")

# add the lib dir in vv-thirdparty to the lib path
link_directories(${VVTHIRDPARTY_LIB})

# set the CPP flags to match the C flags
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} /EHsc")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" )
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
#set(CMAKE_CXX_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${VVTHIRDPARTY}/lib)
#set(CMAKE_CXX_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${VVTHIRDPARTY}/include)

# basic file lists
file(GLOB SOURCES *.cpp)
file(GLOB HEADERS *.h)
file(GLOB WIN_RESOURCES *.rc *.rc2)
file(GLOB GRAPHICS *.ico *.png)

# precompiled header files
#set(PC_NAME stdafx)
#set(PC_HEADER ${PC_NAME}.h)
#set(PC_SOURCE ${PC_NAME}.cpp)
#file(GLOB PC_HEADER_PATH ${PC_HEADER})
#file(GLOB PC_SOURCE_PATH ${PC_SOURCE})

# groups of source files, for easy navigation in Visual Studio
source_group("Graphics" FILES ${GRAPHICS})
file(GLOB GROUP_APPLICATION ${PC_NAME}.* vvWinSync.* WorkerThread.* Config.*)
source_group("Application" FILES ${GROUP_APPLICATION})
# add groups for new commands here

# external libs that we need to link to
set(EXTERNAL_LIBS
	# system  libs
	comctl32.lib

	# SG libs
	sglib
	sg_fs3
)

# add the app's executable
add_executable(${APP_NAME} WIN32 ${SOURCES} ${HEADERS} ${WIN_RESOURCES} ${GRAPHICS})
set_target_properties(${APP_NAME} PROPERTIES FOLDER "Windows")

# set the linker to use C++
set_target_properties(${APP_NAME} PROPERTIES LINKER_LANGUAGE CXX)

# set the project to use our precompiled header
#set_target_properties(${APP_NAME} PROPERTIES COMPILE_FLAGS "/Yu${PC_HEADER}")

# override the precompiled header's CPP to generate the header, rather than trying to use it
#set_source_files_properties(${PC_SOURCE} PROPERTIES COMPILE_FLAGS "/Yc${PC_HEADER}")

# link with our external libs
target_link_libraries(${APP_NAME} ${EXTERNAL_LIBS})
