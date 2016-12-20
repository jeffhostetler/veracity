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

IF (NOT SG_IOS)
add_subdirectory(mongoose)
ENDIF()
add_subdirectory(sghash)
add_subdirectory(fs3)
add_subdirectory(ut)
add_subdirectory(wc)
add_subdirectory(vv2)
add_subdirectory(templates)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/doxygen.conf DC)
string(REPLACE "SG__OUTPUT_DIRECTORY__SG" "${CMAKE_BINARY_DIR}/doxygen" DC2 "${DC}")
string(REPLACE "SG__INPUT__SG" "${CMAKE_CURRENT_SOURCE_DIR}/include" DC3 "${DC2}")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/doxygen.conf "${DC3}")

add_custom_target(
    doxygen
    COMMAND doxygen ${CMAKE_CURRENT_BINARY_DIR}/doxygen.conf
    )

set_target_properties(doxygen PROPERTIES FOLDER "Other")
