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

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Veracity")
SET(CPACK_PACKAGE_VENDOR "SourceGear")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${PUBLIC_SOURCE_TOP_DIR}/LICENSE.txt")
SET(CPACK_RESOURCE_FILE_LICENSE "${PUBLIC_SOURCE_TOP_DIR}/LICENSE.txt")
SET(CPACK_RESOURCE_FILE_WELCOME "${PUBLIC_SOURCE_TOP_DIR}/")
SET(CPACK_PACKAGE_VERSION "${VV_SHORT_VERSION}")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "Veracity ${VV_SHORT_VERSION}")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "veracity-${VV_FULL_VERSION}")

# Make this explicit here, rather than accepting the CPack default value,
# so we can refer to it:
SET(CPACK_PACKAGE_NAME "Veracity")
SET(CPACK_PACKAGE_FILE_NAME "veracity-${VV_FULL_VERSION}")

SET(CPACK_PACKAGE_CONTACT "veracity@sourcegear.org")

IF(UNIX)
	# Set the default locations for installation on unix platforms
	# NOTE: These values will bo overriden by the debian package creator
	# NOTE: to install in /usr
	set(CPACK_PACKAGING_INSTALL_PREFIX "/")
	set(CPACK_PACKAGE_DEFAULT_LOCATION "/usr/local")
	set(CPACK_INSTALL_PREFIX "/usr/local")
	SET(CPACK_STRIP_FILES "src/cmd/vv" "src/script/vscript")
	SET(CPACK_SOURCE_STRIP_FILES "src/server_files" "docs")
	SET(CPACK_PACKAGE_EXECUTABLES "vv" "Veracity")
	
	IF ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
		IF("${MACHINETYPE}" STREQUAL "x86_64")
			SET(CPACK_PACKAGE_FILE_NAME "veracity_${VV_FULL_VERSION}_amd64")
		else()
			SET(CPACK_PACKAGE_FILE_NAME "veracity_${VV_FULL_VERSION}_i386")
		endif()
	endif()
ENDIF(UNIX)

CONFIGURE_FILE("${PUBLIC_SOURCE_TOP_DIR}/cmake_modules/VeracityCPackOptions.cmake.in"
				"${PUBLIC_BINARY_TOP_DIR}/VeracityCPackOptions.cmake" @ONLY)
SET(CPACK_PROJECT_CONFIG_FILE "${PUBLIC_BINARY_TOP_DIR}/VeracityCPackOptions.cmake")
