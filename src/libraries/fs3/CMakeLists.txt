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

set(C_SOURCES
sg_dag_sqlite3.c
sg_dbndx_create.c
sg_dbndx_query.c
sg_dbndx_update.c
sg_treendx.c
sg_fs3_update.c
sg_repo_vtable__fs3.c
)

file(GLOB PRIVATE_HEADERS ./*.h)

set(ALL_SOURCE ${C_SOURCES} ${PRIVATE_HEADERS})

source_group("Private Headers" FILES ${PRIVATE_HEADERS})

add_library(sg_fs3 STATIC ${ALL_SOURCE})
target_link_libraries(sg_fs3 sglib)

set_target_properties(sg_fs3 PROPERTIES FOLDER "Libraries")

