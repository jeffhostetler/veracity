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

vv0util/sg_vv2__util__get_repo.c
vv0util/sg_vv2__util__translate_input_to_gid.c

vv2status/sg_vv2__filtered_status.c
vv2status/sg_vv2__status__alloc.c
vv2status/sg_vv2__status__compare_cset_vs_cset.c
vv2status/sg_vv2__status__compute_flags.c
vv2status/sg_vv2__status__compute_repo_path.c
vv2status/sg_vv2__status__main.c
vv2status/sg_vv2__status__od.c
vv2status/sg_vv2__status__odi.c
vv2status/sg_vv2__status__summarize.c
vv2status/sg_vv2__status__work_queue.c

vv6diff/sg_vv2__diff__diff_to_stream.c
vv6diff/sg_vv2__diff__directory.c
vv6diff/sg_vv2__diff__file.c
vv6diff/sg_vv2__diff__header.c
vv6diff/sg_vv2__diff__symlink.c
vv6diff/sg_vv2__diff__tempdir.c

vv6history/sg_vv2__history__fetch_more.c
vv6history/sg_vv2__history__main.c
vv6history/sg_vv2__history__utils.c

vv6locks/sg_vv2__locks__main.c

vv6mstatus/sg_vv2__mstatus__main.c

vv8api/sg_vv2__cat.c
vv8api/sg_vv2__check_attach_name.c
vv8api/sg_vv2__comment.c
vv8api/sg_vv2__compute_file_hid.c
vv8api/sg_vv2__diff_to_stream.c
vv8api/sg_vv2__export.c
vv8api/sg_vv2__find_cset.c
vv8api/sg_vv2__history.c
vv8api/sg_vv2__init_new_repo.c
vv8api/sg_vv2__leaves.c
vv8api/sg_vv2__line_history.c
vv8api/sg_vv2__locks.c
vv8api/sg_vv2__mstatus.c
vv8api/sg_vv2__stamp.c
vv8api/sg_vv2__status.c
vv8api/sg_vv2__tag.c
vv8api/sg_vv2__whoami.c

vv9js/sg_vv2__jsglue.c
vv9js/sg_vv2__jsglue__sg_vv2.c
)

file(GLOB DIR_HEADERS ./*.h)
list(APPEND C_HEADERS ${DIR_HEADERS})
file(GLOB DIR_FILES ./${dir}/*)
source_group("\\" FILES ${DIR_FILES})
foreach (dir
	vv0util
	vv2status
	vv6diff
	vv6history
	vv6locks
	vv6mstatus
	vv8api
	vv9js
)
	file(GLOB DIR_HEADERS ./${dir}/*.h)
	list(APPEND C_HEADERS ${DIR_HEADERS})
	file(GLOB DIR_FILES ./${dir}/*)
	source_group("${dir}" FILES ${DIR_FILES})
endforeach (dir)

add_library(sg_vv2 STATIC ${C_SOURCES} ${C_HEADERS})
target_link_libraries(sg_vv2 sglib)

set_target_properties(sg_vv2 PROPERTIES FOLDER "Libraries")
