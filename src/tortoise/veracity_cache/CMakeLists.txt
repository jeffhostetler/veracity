## Copyright 2010-2013 SourceGear, LLC
## 
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
## 
## http://www.apache.org/licenses/LICENSE-2.0
## 
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.


set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} /EHsc")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")

set(VVTHIRDPARTY_BIN "${VVTHIRDPARTY}/bin")
set(VVTHIRDPARTY_INCLUDE "${VVTHIRDPARTY}/include")
set(VVTHIRDPARTY_LIB "${VVTHIRDPARTY}/lib")

link_directories(${VVTHIRDPARTY_LIB})

file(GLOB GROUP_READ_DIRECTORY_CHANGES
	ReadDirectoryChanges/*
)
source_group("ReadDirectoryChanges" FILES ${GROUP_READ_DIRECTORY_CHANGES})

add_executable(veracity_cache 
	targetver.h stdafx.cpp stdafx.h
	sg_cache_application.h sg_cache_application.cpp
	sg_status_cache.cpp sg_status_cache.h
	sg_read_dir_manager.cpp sg_read_dir_manager.h
	${GROUP_READ_DIRECTORY_CHANGES}
	)

set_target_properties(veracity_cache PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(veracity_cache
	sglib sghash sg_fs3 
	debug ${VVTHIRDPARTY_LIB}/wxbase29ud.lib
	optimized ${VVTHIRDPARTY_LIB}/wxbase29u.lib
	shell32
	)

set_target_properties(veracity_cache PROPERTIES FOLDER "Windows/Tortoise")
