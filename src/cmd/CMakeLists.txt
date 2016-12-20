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

set(C_SOURCES vv
sg.c
sg__vv2__do_cmd_cat.c
sg__vv2__do_cmd_comment.c
sg__vv2__do_cmd_export.c
sg__vv2__do_cmd_history.c  
sg__vv2__do_cmd_init_new_repo.c
sg__vv2__do_cmd_leaves.c
sg__vv2__do_cmd_locks.c
sg__vv2__do_cmd_stamp.c
sg__vv2__do_cmd_tag.c
sg__vvwc__do_cmd_diff.c  
sg__vvwc__do_cmd_status.c  
sg__vvwc__do_cmd_mstatus.c 
sg__wc__do_cmd_add.c
sg__wc__do_cmd_addremove.c
sg__wc__do_cmd_checkout.c
sg__wc__do_cmd_commit.c
sg__wc__do_cmd_diffmerge.c
sg__wc__do_cmd_lock.c
sg__wc__do_cmd_merge.c  
sg__wc__do_cmd_move.c
sg__wc__do_cmd_parents.c
sg__wc__do_cmd_rename.c
sg__wc__do_cmd_remove.c
sg__wc__do_cmd_revert_all.c
sg__wc__do_cmd_revert_items.c
sg__wc__do_cmd_scan.c
sg__wc__do_cmd_unlock.c
sg__wc__do_cmd_update.c  
sg_logfile.c
sg__do_cmd_dump_lca.c
sg__do_cmd_merge_preview.c
sg__do_cmd_resolve2.c
sg__do_cmd_serve.c
sg_cmd_util.c
sg_cmd_util__comment.c
sg_cmd_util__report_addremove_journal.c
)

file(GLOB PRIVATE_HEADERS ./*.h)
file(GLOB HEADERS ../libraries/include/*.h)
file(GLOB CORESSJS ../server_files/core/*.js)
file(GLOB CORETEMPLATES ../server_files/core/templates/*.html)
file(GLOB SCRUMSSJS ../server_files/modules/scrum/*.js)
file(GLOB SCRUMTEMPLATES ../server_files/modules/scrum/templates/*.html)
file(GLOB STATICS ../server_files/ui/*.gif ../server_files/ui/*.js ../server_files/ui/*.css ../server_files/ui/*.png ../server_files/ui/*.jpg)

set(ALL_SOURCE ${C_SOURCES} ${HEADERS} ${CORETEMPLATES} ${STATICS} ${PRIVATE_HEADERS} ../server_files/dispatch.js ${CORESSJS} ${SCRUMSSJS} ${SCRUMTEMPLATES} ../libraries/mongoose/sg_mongoose.h ./renderscripts.js)

source_group("Core SSJS" FILES ../server_files/dispatch.js ${CORESSJS})
source_group("Static Web Content" FILES ${STATICS})
source_group("Core Templates" FILES ${CORETEMPLATES})
source_group("Scrum SSJS" FILES ${SCRUMSSJS})
source_group("Scrum Templates" FILES ${SCRUMTEMPLATES})
source_group("Private Headers" FILES ${PRIVATE_HEADERS})

add_executable(${ALL_SOURCE})
set_target_properties(vv PROPERTIES FOLDER "CLI")

target_link_libraries(vv sglib sg_vv2 sg_wc sg_fs3 sgmongoose sghelpsource)
if(WIN32 AND NOT CMAKE_COMPILER_IS_GNUCC)
	# Link the version of _setargv that does wildcard expansion.
	set_target_properties(vv PROPERTIES LINK_FLAGS "Wsetargv.obj")
endif()

macro(FIND_AND_ADD_FRAMEWORK fwname appname)
    find_library(FRAMEWORK_${fwname}
        NAMES ${fwname}
        PATHS ${CMAKE_OSX_SYSROOT}/System/Library
        PATH_SUFFIXES Frameworks
        NO_DEFAULT_PATH)
    if( ${FRAMEWORK_${fwname}} STREQUAL FRAMEWORK_${fwname}-NOTFOUND)
        MESSAGE(ERROR ": Framework ${fwname} not found")
    else()
        TARGET_LINK_LIBRARIES(${appname} ${FRAMEWORK_${fwname}})
        # MESSAGE(STATUS "Framework ${fwname} found at ${FRAMEWORK_${fwname}}")
    endif()
endmacro(FIND_AND_ADD_FRAMEWORK)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   find_and_add_framework(Security vv)
   find_and_add_framework(CoreFoundation vv)
endif()


add_dependencies(vv template_check)

INSTALL_TARGETS(/bin vv)

IF("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
	INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/vv.bash_completion
		DESTINATION /etc/bash_completion.d
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
		RENAME "vv")
ELSEIF(APPLE)
	INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/vv.bash_completion
		DESTINATION "${Veracity_DATA_DEST}"
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
		)
ENDIF()
