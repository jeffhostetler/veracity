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

IF(WIN32)
	cmake_minimum_required(VERSION 2.8.3)
	set_property(GLOBAL PROPERTY USE_FOLDERS On)
ELSE()
	cmake_minimum_required(VERSION 2.8.4)
ENDIF()

set (PUBLIC_SOURCE_TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (PUBLIC_BINARY_TOP_DIR ${CMAKE_CURRENT_BINARY_DIR})

if(NOT COMMON_INCLUDED)
	include(common-CMakeLists.txt)
endif()

add_subdirectory(src)
add_subdirectory(testsuite)
