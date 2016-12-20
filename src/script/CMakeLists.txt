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

set(C_SOURCES vscript vscript.c vscript-usage.c)

file(GLOB HEADERS ../libraries/include/*.h)
file(GLOB JS_TESTS ../js_tests/*.js)

set(ALL_SOURCE ${C_SOURCES} ${HEADERS} ${JS_TESTS})

source_group("js_tests" FILES ${JS_TESTS})

add_executable(${ALL_SOURCE})
set_target_properties(vscript PROPERTIES FOLDER "CLI")

target_link_libraries(vscript sglib sg_wc sg_fs3)

add_dependencies(vscript template_check)

INSTALL_TARGETS(/bin vscript)
