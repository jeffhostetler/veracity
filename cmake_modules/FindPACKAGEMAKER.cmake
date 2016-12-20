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

SET (PACKAGEMAKER_ROOT_DIRS
	"/Developer/usr/bin"
	"/usr/bin"
	"/bin"
)

IF (OSX_DEVELOPER_ROOT)
	SET(PACKAGEMAKER_ROOT_DIRS "${OSX_DEVELOPER_ROOT}/../Applications/PackageMaker.app/Contents/MacOS" ${PACKAGEMAKER_ROOT_DIRS} )
ENDIF(OSX_DEVELOPER_ROOT)

FIND_FILE(PACKAGEMAKER_ROOT
	NAMES 
	packagemaker PackageMaker  
PATHS ${PACKAGEMAKER_ROOT_DIRS})

SET(PACKAGEMAKER_FOUND OFF)
IF(PACKAGEMAKER_ROOT)
  SET(PACKAGEMAKER_FOUND ON)
  SET(PACKAGEMAKER_PATH "${PACKAGEMAKER_ROOT}" CACHE STRING "PackageMaker Path")
ENDIF(PACKAGEMAKER_ROOT)

IF(NOT PACKAGEMAKER_FOUND)
	MESSAGE(STATUS
      "PackageMaker required but not found. Please see the instructions for installing XCode and the Auxiliary Tools in @/docs/BUILD-mac.txt.")
ELSE()
	MESSAGE(STATUS "PackageMaker path: ${PACKAGEMAKER_PATH}")
ENDIF()
