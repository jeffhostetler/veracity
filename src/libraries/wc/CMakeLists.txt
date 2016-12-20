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
wc0util/sg_rbtree_ui64.c
wc0util/sg_wc_attrbits.c
wc0util/sg_wc_diff_utils.c
wc0util/sg_wc_park.c
wc0util/sg_wc_path.c
wc0util/sg_wc_port.c
wc0util/sg_wc_readdir.c
wc0util/sg_wc_readdir__row.c
wc0util/sg_wc_sparse.c

wc0db/sg_wc_db.c
wc0db/sg_wc_db__branch.c
wc0db/sg_wc_db__csets.c
wc0db/sg_wc_db__gid.c
wc0db/sg_wc_db__ignores.c
wc0db/sg_wc_db__info.c 
wc0db/sg_wc_db__issue.c 
wc0db/sg_wc_db__path.c
wc0db/sg_wc_db__pc.c
wc0db/sg_wc_db__pc_row.c
wc0db/sg_wc_db__port.c
wc0db/sg_wc_db__state.c
wc0db/sg_wc_db__tne.c
wc0db/sg_wc_db__tsc.c
wc0db/sg_wc_db__tx.c

wc1pt/sg_wc_pctne__foreach_in_dir_by_parent_alias.c
wc1pt/sg_wc_pctne__get_row_by_alias.c
wc1pt/sg_wc_pctne__row.c

wc2scan/sg_wc_prescan_dir.c
wc2scan/sg_wc_prescan_dir__find_row_by_alias.c
wc2scan/sg_wc_prescan_dir__scan_and_match.c
wc2scan/sg_wc_prescan_row.c
wc2scan/sg_wc_prescan_row__root.c
wc2scan/sg_wc_prescan_row__synthesize_replacement_row.c

wc3live/sg_wc_liveview_dir.c
wc3live/sg_wc_liveview_dir__can_add_new_entryname.c
wc3live/sg_wc_liveview_item.c
wc3live/sg_wc_liveview_item__alter_structure.c
wc3live/sg_wc_liveview_item__root.c
wc3live/sg_wc_liveview_item__synthesize_replacement_row.c

wc4status/sg_wc__status__append.c
wc4status/sg_wc__status__classic_format.c
wc4status/sg_wc__status__compute_flags.c
wc4status/sg_wc__status__flags_to_properties.c
wc4status/sg_wc__status__sort.c 
wc4status/sg_wc__status__summary.c 
wc4status/sg_wc__status__tne_type_to_flags.c
wc4status/sg_wc__status__find_status__single_item.c

wc4tx/sg_wc_tx__cache.c
wc4tx/sg_wc_tx__liveview__fetch.c
wc4tx/sg_wc_tx__prescan__fetch.c
wc4tx/sg_wc_tx__tempdir.c

wc5apply/sg_wc_tx__apply.c
wc5apply/sg_wc_tx__apply__add.c  
wc5apply/sg_wc_tx__apply__add_special.c  
wc5apply/sg_wc_tx__apply__clean_pc_but_leave_sparse.c
wc5apply/sg_wc_tx__apply__delete_issue.c
wc5apply/sg_wc_tx__apply__delete_pc.c
wc5apply/sg_wc_tx__apply__delete_tne.c
wc5apply/sg_wc_tx__apply__insert_issue.c
wc5apply/sg_wc_tx__apply__insert_tne.c
wc5apply/sg_wc_tx__apply__kill_pc_row.c
wc5apply/sg_wc_tx__apply__move_rename.c
wc5apply/sg_wc_tx__apply__remove_directory.c
wc5apply/sg_wc_tx__apply__remove_file.c
wc5apply/sg_wc_tx__apply__remove_symlink.c
wc5apply/sg_wc_tx__apply__resolve_issue.c
wc5apply/sg_wc_tx__apply__store_directory.c
wc5apply/sg_wc_tx__apply__store_file.c
wc5apply/sg_wc_tx__apply__store_symlink.c
wc5apply/sg_wc_tx__apply__undo_delete.c
wc5apply/sg_wc_tx__apply__undo_lost.c

wc5queue/sg_wc_tx__journal.c
wc5queue/sg_wc_tx__journal__fake.c
wc5queue/sg_wc_tx__queue__add.c
wc5queue/sg_wc_tx__queue__add_special.c  
wc5queue/sg_wc_tx__queue__delete_issue.c
wc5queue/sg_wc_tx__queue__insert_issue.c
wc5queue/sg_wc_tx__queue__kill_pc_row.c
wc5queue/sg_wc_tx__queue__move_rename.c
wc5queue/sg_wc_tx__queue__overwrite_file.c
wc5queue/sg_wc_tx__queue__overwrite_symlink.c
wc5queue/sg_wc_tx__queue__remove.c
wc5queue/sg_wc_tx__queue__resolve_issue.c
wc5queue/sg_wc_tx__queue__set_attrbits.c
wc5queue/sg_wc_tx__queue__undo_add.c
wc5queue/sg_wc_tx__queue__undo_delete.c

wc6commit/sg_wc_tx__commit__apply.c
wc6commit/sg_wc_tx__commit__dump_marks.c
wc6commit/sg_wc_tx__commit__mark_bubble_up.c
wc6commit/sg_wc_tx__commit__mark_subtree.c
wc6commit/sg_wc_tx__commit__queue.c
wc6commit/sg_wc_tx__commit__queue__blob__non_dir.c
wc6commit/sg_wc_tx__commit__queue__blob__bubble_up_directory.c
wc6commit/sg_wc_tx__commit__queue__blob__dir_contents_participating.c
wc6commit/sg_wc_tx__commit__queue__blob__dir_non_recursive.c
wc6commit/sg_wc_tx__commit__queue__blob__dir_superroot.c
wc6commit/sg_wc_tx__commit__queue__utils.c
wc6commit/sg_wc_tx__rp__commit__mark_bubble_up.c
wc6commit/sg_wc_tx__rp__commit__mark_subtree.c

wc6diff/sg_wc_tx__diff__directory.c 
wc6diff/sg_wc_tx__diff__file.c 
wc6diff/sg_wc_tx__diff__header.c
wc6diff/sg_wc_tx__diff__setup.c
wc6diff/sg_wc_tx__diff__symlink.c

wc6ffmerge/sg_wc_tx__ffmerge__main.c

wc6merge/sg_wc_tx__fake_merge__revert_all__cleanup_db.c
wc6merge/sg_wc_tx__fake_merge__update__build_pc.c
wc6merge/sg_wc_tx__fake_merge__update__status.c
wc6merge/sg_wc_tx__merge__alloc.c
wc6merge/sg_wc_tx__merge__automerge_files.c
wc6merge/sg_wc_tx__merge__compute_lca.c
wc6merge/sg_wc_tx__merge__compute_simple2.c
wc6merge/sg_wc_tx__merge__cset.c 
wc6merge/sg_wc_tx__merge__cset_dir.c 
wc6merge/sg_wc_tx__merge__cset_entry.c  
wc6merge/sg_wc_tx__merge__cset_entry_collision.c   
wc6merge/sg_wc_tx__merge__cset_entry_conflict.c   
wc6merge/sg_wc_tx__merge__delete_automerge_tempfiles_on_abort.c
wc6merge/sg_wc_tx__merge__determine_target_cset.c
wc6merge/sg_wc_tx__merge__get_starting_conditions.c
wc6merge/sg_wc_tx__merge__load_csets.c 
wc6merge/sg_wc_tx__merge__main.c
wc6merge/sg_wc_tx__merge__make_stats.c
wc6merge/sg_wc_tx__merge__prepare_issues.c  
wc6merge/sg_wc_tx__merge__queue_plan.c  
wc6merge/sg_wc_tx__merge__queue_plan__add_special.c  
wc6merge/sg_wc_tx__merge__queue_plan__delete.c 
wc6merge/sg_wc_tx__merge__queue_plan__entry.c 
wc6merge/sg_wc_tx__merge__queue_plan__kill_list.c
wc6merge/sg_wc_tx__merge__queue_plan__park.c 
wc6merge/sg_wc_tx__merge__queue_plan__peer.c  
wc6merge/sg_wc_tx__merge__queue_plan__undo_delete.c
wc6merge/sg_wc_tx__merge__queue_plan__undo_lost.c
wc6merge/sg_wc_tx__merge__queue_plan__unpark.c  
wc6merge/sg_wc_tx__merge__temp_dir.c
wc6merge/sg_wc_tx__postmerge__cleanup.c

wc6mstatus/sg_wc_tx__mstatus__main.c

wc6resolve/sg_resolve.c

wc6rpapi/sg_wc_tx__rp__add.c 
wc6rpapi/sg_wc_tx__rp__add_special.c 
wc6rpapi/sg_wc_tx__rp__move_rename.c
wc6rpapi/sg_wc_tx__rp__overwrite_file_from_file.c
wc6rpapi/sg_wc_tx__rp__overwrite_file_from_repo.c
wc6rpapi/sg_wc_tx__rp__overwrite_symlink.c
wc6rpapi/sg_wc_tx__rp__remove.c
wc6rpapi/sg_wc_tx__rp__set_attrbits.c 
wc6rpapi/sg_wc_tx__rp__status.c
wc6rpapi/sg_wc_tx__rp__undo_delete.c
wc6rpapi/sg_wc_tx__rp__undo_lost.c

wc6status1/sg_wc_tx__status1__main.c

wc6update/sg_wc_tx__update__apply.c
wc6update/sg_wc_tx__update__attach.c
wc6update/sg_wc_tx__update__determine_target_cset.c
wc6update/sg_wc_tx__update__fake_merge.c

wc7txapi/sg_wc_tx__add.c 
wc7txapi/sg_wc_tx__add_special.c 
wc7txapi/sg_wc_tx__addremove.c
wc7txapi/sg_wc_tx__begin.c 
wc7txapi/sg_wc_tx__branch.c 
wc7txapi/sg_wc_tx__diff.c 
wc7txapi/sg_wc_tx__flush_timestamp_cache.c
wc7txapi/sg_wc_tx__get_item_dirstatus_flags.c 
wc7txapi/sg_wc_tx__get_item_gid.c 
wc7txapi/sg_wc_tx__get_item_gid_path.c 
wc7txapi/sg_wc_tx__get_item_issues.c 
wc7txapi/sg_wc_tx__get_item_repo_path.c 
wc7txapi/sg_wc_tx__get_item_resolve_info.c
wc7txapi/sg_wc_tx__get_item_status_flags.c
wc7txapi/sg_wc_tx__get_wc_csets.c 
wc7txapi/sg_wc_tx__get_wc_info.c
wc7txapi/sg_wc_tx__get_wc_parents.c 
wc7txapi/sg_wc_tx__get_wc_top.c 
wc7txapi/sg_wc_tx__journal.c
wc7txapi/sg_wc_tx__move.c 
wc7txapi/sg_wc_tx__move_rename.c 
wc7txapi/sg_wc_tx__mstatus.c 
wc7txapi/sg_wc_tx__overwrite_file_from_file.c
wc7txapi/sg_wc_tx__overwrite_file_from_repo.c
wc7txapi/sg_wc_tx__overwrite_symlink.c
wc7txapi/sg_wc_tx__remove.c
wc7txapi/sg_wc_tx__rename.c
wc7txapi/sg_wc_tx__revert_all.c
wc7txapi/sg_wc_tx__revert_items.c
wc7txapi/sg_wc_tx__resolve.c
wc7txapi/sg_wc_tx__set_attrbits.c
wc7txapi/sg_wc_tx__status.c
wc7txapi/sg_wc_tx__status1.c
wc7txapi/sg_wc_tx__undo_delete.c
wc7txapi/sg_wc_tx__undo_lost.c

wc8api/sg_wc__add.c
wc8api/sg_wc__addremove.c
wc8api/sg_wc__branch.c
wc8api/sg_wc__checkout.c
wc8api/sg_wc__commit.c 
wc8api/sg_wc__diff.c 
wc8api/sg_wc__flush_timestamp_cache.c
wc8api/sg_wc__get_item_dirstatus_flags.c
wc8api/sg_wc__get_item_gid.c
wc8api/sg_wc__get_item_gid_path.c
wc8api/sg_wc__get_item_issues.c
wc8api/sg_wc__get_item_resolve_info.c
wc8api/sg_wc__get_item_status_flags.c
wc8api/sg_wc__get_wc_info.c
wc8api/sg_wc__get_wc_parents.c
wc8api/sg_wc__get_wc_top.c
wc8api/sg_wc__initialize.c 
wc8api/sg_wc__lock.c 
wc8api/sg_wc__line_history.c 
wc8api/sg_wc__merge.c
wc8api/sg_wc__move.c
wc8api/sg_wc__move_rename.c 
wc8api/sg_wc__mstatus.c 
wc8api/sg_wc__remove.c
wc8api/sg_wc__rename.c
wc8api/sg_wc__revert_all.c
wc8api/sg_wc__revert_items.c
wc8api/sg_wc__status.c
wc8api/sg_wc__status1.c
wc8api/sg_wc__unlock.c 
wc8api/sg_wc__update.c

wc9js/sg_wc__jsglue.c
wc9js/sg_wc__jsglue__safeptr.c
wc9js/sg_wc__jsglue__sg_wc.c
wc9js/sg_wc__jsglue__sg_wc_tx.c

)

# run through each folder in the lib and generate:
# 1) a list of headers in the library in C_HEADERS
# 2) a source_group for all the source files in each folder
file(GLOB DIR_HEADERS ./*.h)
list(APPEND C_HEADERS ${DIR_HEADERS})
file(GLOB DIR_FILES ./${dir}/*)
source_group("\\" FILES ${DIR_FILES})
foreach (dir
	wc0util
	wc0db
	wc1pt
	wc2scan
	wc3live
	wc4status
	wc4tx
	wc5apply
	wc5queue
	wc6commit
	wc6ffmerge
	wc6merge
	wc6mstatus
	wc6resolve
	wc6rpapi
	wc6status1
	wc6update
	wc7txapi
	wc8api
	wc9js
)
	file(GLOB DIR_HEADERS ./${dir}/*.h)
	list(APPEND C_HEADERS ${DIR_HEADERS})
	file(GLOB DIR_FILES ./${dir}/*)
	source_group("${dir}" FILES ${DIR_FILES})
endforeach (dir)

add_library(sg_wc STATIC ${C_SOURCES} ${C_HEADERS})
target_link_libraries(sg_wc sglib)

set_target_properties(sg_wc PROPERTIES FOLDER "Libraries")

