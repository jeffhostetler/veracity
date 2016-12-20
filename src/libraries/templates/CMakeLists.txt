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

set(JSON_SOURCES
    sg_ztemplate__hooks.json
    sg_ztemplate__locks.json
    sg_ztemplate__branches.json
    sg_ztemplate__oldbranches.json
    sg_ztemplate__comments.json
    sg_ztemplate__stamps.json
    sg_ztemplate__tags.json
    sg_ztemplate__areas.json
    sg_ztemplate__users.json
    sg_ztemplate__certs.json
    sg_ztemplate__sigs.json
)

foreach(tj ${JSON_SOURCES})
string(REPLACE ".json" "" basename ${tj})
ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${basename}.c
    COMMAND sg_hexdump ${CMAKE_CURRENT_SOURCE_DIR}/${basename}.json ${CMAKE_CURRENT_BINARY_DIR}/${basename}.c
    DEPENDS sg_hexdump ${CMAKE_CURRENT_SOURCE_DIR}/${basename}.json
    )
set(C_SOURCES ${C_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/${basename}.c
)
endforeach(tj ${JSON_SOURCES})

add_library(sgtemplates STATIC ${C_SOURCES} ${JSON_SOURCES})
source_group("Json Sources" FILES ${JSON_SOURCES})
set_target_properties(sgtemplates PROPERTIES FOLDER "Libraries")
target_link_libraries(sgtemplates ${SG_OS_LIBS})

IF (NOT SG_IOS)
add_custom_target(
    template_check
    COMMAND ztcheck
    DEPENDS ztcheck
    )
set_target_properties(template_check PROPERTIES FOLDER "Other")
ENDIF()

