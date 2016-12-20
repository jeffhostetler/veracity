/*
Copyright 2010-2013 SourceGear, LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC6MERGE__PRIVATE_PROTOTYPES_H
#define H_SG_WC6MERGE__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////
// TODO 2012/01/20 Change the "SG_" to "sg_" to make these routines
// TODO            look like private routines.

void SG_mrg__free(SG_context * pCtx, SG_mrg * pMrg);

#define SG_MRG_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_mrg__free)

/**
 * Allocate a pMrg structure for computing a merge on the version control tree.
 * This only allocates the structure and used the given pending-tree to find the
 * WD, REPO, BASELINE, and determine if the WD is dirty.
 *
 * Use SG_mrg__compute_merge() to actually compute the merge in memory
 * and then SG_mrg__apply_merge() to actually update the WD.
 */
void SG_mrg__alloc(SG_context * pCtx,
				   SG_wc_tx * pWcTx,
				   const SG_wc_merge_args * pMergeArgs,
				   SG_mrg ** ppMrg_New);

#if defined(DEBUG)
#define SG_MRG__ALLOC(pCtx,pWcTx,pMergeArgs,ppMrg_New)					\
	SG_STATEMENT(	SG_mrg * _pNew = NULL;								\
					SG_mrg__alloc(pCtx,pWcTx,pMergeArgs,&_pNew);		\
					_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"SG_mrg");	\
					*(ppMrg_New) = _pNew;								)
#else
#define SG_MRG__ALLOC(pCtx,pWcTx,pMergeArgs,ppMrg_New)					\
	SG_mrg__alloc(pCtx,pWcTx,pMergeArgs,ppMrg_New)
#endif

//////////////////////////////////////////////////////////////////

void SG_mrg_cset_entry__free(SG_context * pCtx, SG_mrg_cset_entry * pMrgCSetEntry);

/**
 * Load the contents of the SG_treenode_entry into a new SG_mrg_cset_entry
 * and add it to the flat list of entries in the SG_mrg_cset.
 */
void SG_mrg_cset_entry__load__using_repo(SG_context * pCtx,
										 SG_mrg * pMrg,
										 SG_mrg_cset * pMrgCSet,
										 const char * pszGid_Parent,		// will be null for actual-root directory
										 const char * pszGid_Self,
										 const SG_treenode_entry * pTne_Self,
										 SG_mrg_cset_entry ** ppMrgCSetEntry_New);

/**
 * Load the SG_treenode for a directory and convert each SG_treenode_entry
 * into a SG_mrg_cset_entry and load it into the flat list of entries in
 * the SG_mrg_cset.
 */
void SG_mrg_cset_entry__load_subdir__using_repo(SG_context * pCtx,
												SG_mrg * pMrg,
												SG_mrg_cset * pMrgCSet,
												const char * pszGid_Self,
												SG_mrg_cset_entry * pMrgCSetEntry_Self,
												const char * pszHid_Blob);

void SG_mrg_cset_entry__load__using_wc(SG_context * pCtx,
											 SG_mrg * pMrg,
											 SG_mrg_cset * pMrgCSet,
											 const char * pszGid_Parent,
											 const char * pszGid_Self,
											 sg_wc_liveview_item * pLVI_Self,
											 SG_mrg_cset_entry ** ppMrgCSetEntry_New);

void SG_mrg_cset_entry__load_subdir__using_wc(SG_context * pCtx,
													SG_mrg * pMrg,
													SG_mrg_cset * pMrgCSet,
													const char * pszGid_Self,
													sg_wc_liveview_item * pLVI_Self,
													SG_mrg_cset_entry * pMrgCSetEntry_Self);

/**
 * Compare 2 entries for equality.  For files and symlinks we compare everything.
 * For directories, we omit the contents of the directory (the HID-BLOB).
 */
void SG_mrg_cset_entry__equal(SG_context * pCtx,
							  const SG_mrg_cset_entry * pMrgCSetEntry_1,
							  const SG_mrg_cset_entry * pMrgCSetEntry_2,
							  SG_mrg_cset_entry_neq * pNeq);

/**
 * Clone the given entry and add it to the flat list of entries in the
 * given SG_mrg_cset.
 *
 * We do not copy the HID-BLOB for directories because it is a computed
 * field and the value from the source entry probably won't match the
 * value that will be computed on the directory in the destination CSET.
 *
 * If the entry does not have a parent, we assume that it is the root "@/"
 * directory and set the _Root fields in the destination CSET.
 *
 * You DO NOT own the returned pointer.
 */
void SG_mrg_cset_entry__clone_and_insert_in_cset(SG_context * pCtx,
												 SG_mrg * pMrg,
												 const SG_mrg_cset_entry * pMrgCSetEntry_Src,
												 SG_mrg_cset * pMrgCSet_Result,
												 SG_mrg_cset_entry ** ppMrgCSetEntry_Result);

/**
 * We get called once for each entry which is a member of a collision
 * equivalence class.
 *
 * We want to assign a unique entryname to this file/symlink/directory
 * so that we can completely populate the directory during the merge.
 *
 * That is, if we have 2 branches that each created a new file
 * called "foo", we can't put both in the directory.  So create
 * something like "foo~<gid7>".
 * and let the user decide the names when they RESOLVE things.
 */
SG_rbtree_foreach_callback SG_mrg_cset_entry__make_unique_entrynames;

void SG_mrg_cset_entry__forget_inherited_hid_blob(SG_context * pCtx,
												  SG_mrg_cset_entry * pMrgCSetEntry);

void SG_mrg_cset_entry__check_for_outstanding_issue(SG_context * pCtx,
													SG_mrg * pMrg,
													sg_wc_liveview_item * pLVI,
													SG_mrg_cset_entry * pMrgCSetEntry);

//////////////////////////////////////////////////////////////////

void SG_mrg_cset__free(SG_context * pCtx, SG_mrg_cset * pMrgCSet);

void SG_mrg_cset__alloc(SG_context * pCtx,
						const char * pszMnemonicName,
						const char * pszCSetLabel,
						const char * pszAcceptLabel,
						SG_mrg_cset_origin origin,
						SG_daglca_node_type nodeType,
						SG_mrg_cset ** ppMrgCSet);

#if defined(DEBUG)
#define SG_MRG_CSET__ALLOC(pCtx,p1,p2,p3,o,n,pp)						\
	SG_STATEMENT(	SG_mrg_cset * _pNew = NULL;							\
					SG_mrg_cset__alloc(pCtx,p1,p2,p3,o,n,&_pNew);		\
					_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"SG_mrg_cset"); \
					*(pp) = _pNew;										)
#else
#define SG_MRG_CSET__ALLOC(pCtx,p1,p2,p3,o,n,pp)	\
	SG_mrg_cset__alloc(pCtx,p1,p2,p3,o,n,pp)
#endif

/**
 * Load the entire contents of the version control tree as of
 * this CSET into memory.
 *
 * This loads all of the entries (files/symlinks/directories) into
 * a flat list.  Each SG_treenode_entry is converted to a SG_mrg_cset_entry
 * and they are added to SG_mrg_cset.prbEntries.
 *
 * This version of __load_entire_cset__ uses the REPO to get a
 * clean view of the CSET as it was recorded.
 * 
 */
void SG_mrg_cset__load_entire_cset__using_repo(SG_context * pCtx,
											   SG_mrg * pMrg,
											   SG_mrg_cset * pMrgCSet,
											   const char * pszHid_CSet);

/**
 * This version loads the CSET using the current/live view
 * as it exists in the WC (and the WC TX).
 *
 */
void SG_mrg_cset__load_entire_cset__using_wc(SG_context * pCtx,
											 SG_mrg * pMrg,
											 SG_mrg_cset * pMrgCSet,
											 const char * pszHid_CSet);

/**
 * Load a specific CSet into the AUX table.  These
 * are used when we need an alternate per-item ancestor.
 *
 * You do not own the result.
 *
 */
void sg_mrg_cset__load_aux__using_repo(SG_context * pCtx,
									   SG_mrg * pMrg,
									   const char * pszHid_CSet,
									   SG_mrg_cset **ppMrgCSet);

/**
 * Set the value of SG_mrg_cset_entry.markerValue to the given newValue
 * on all entries in the tree.
 */
void SG_mrg_cset__set_all_markers(SG_context * pCtx,
								  SG_mrg_cset * pMrgCSet,
								  SG_int64 newValue);

#if TRACE_WC_MERGE
/**
 * Create a debug string containing the full repo-path of the entry
 * as it appeared in this version of this version control tree.
 *
 * The caller must free the returned string.
 */
void _sg_mrg_cset__compute_repo_path_for_entry(SG_context * pCtx,
											   SG_mrg_cset * pMrgCSet,
											   const char * pszGid_Entry,
											   SG_string ** ppStringResult);		// caller must free this
#endif

/**
 * Create a flat list of the directories in the CSET.  Within each directory, create
 * a flat list of the entries within the directory.  This is not a true hierarchy
 * (because we don't have links between the directories), but it can simulate one
 * when needed.
 *
 * This populates the SG_mrg_cset.prbDirs and .pMrgCSetDir_Root members.  We use the
 * flat list of entries in .prbEntries to drive this.
 *
 * Once this is computed and bound to the SG_mrg_cset, we must consider the
 * SG_mrg_cset frozen.
 */
void SG_mrg_cset__compute_cset_dir_list(SG_context * pCtx,
										SG_mrg_cset * pMrgCSet);

/**
 * Inspect all of the directories and look for real and potential collisions.
 */
void SG_mrg_cset__check_dirs_for_collisions(SG_context * pCtx,
											SG_mrg_cset * pMrgCSet,
											SG_mrg * pMrg);

/**
 * Register a conflict on the merge.  That is, add the conflict info for an entry
 * (the ancestor entry and all of the leaf versions that changed it in divergent
 * ways) to the conflict-list.
 *
 * Note that we do not distinguish between hard-conflicts (that can't be automatically
 * resolved) and candiate-conflicts (where we just need to do diff3 or something before
 * we can decide what to do).
 *
 * We take ownership of the conflict object given, if there are no errors.
 */
void SG_mrg_cset__register_conflict(SG_context * pCtx,
									SG_mrg_cset * pMrgCSet,
									SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict);

/**
 * Register that an entry was deleted on the merge.  We do this by adding the GID-ENTRY
 * of the entry in a deleted-list in the result-cset.  (We can't store the entry as it
 * appeared in the source-cset that deleted it, because it, well, deleleted it.)
 *
 * As a convenience, we remember the original entry from the ancestor in the deleted-list
 * in case it would be helpful.  (<gid-entry>, <entry-from-ancestor>).
 *
 * Also, since the entry may have been deleted in multiple branches, we DON'T try to store
 * (<gid-entry>,<cset-leaf>) because we'd just get duplicates.
 *
 * We do not own the entry (but then you didn't own it either).
 */
void SG_mrg_cset__register_delete(SG_context * pCtx,
								  SG_mrg_cset * pMrgCSet,
								  SG_mrg_cset_entry * pMrgCSetEntry_Ancestor);

/**
 * We allow entries to be added to the CSET at random.
 * But as soon as you compute the directory list, we consider it frozen.
 */
#define SG_MRG_CSET__IS_FROZEN(pMrgCSet)	((pMrgCSet)->prbDirs != NULL)

/**
 * Look at all entries in the (result) cset and make unique entrynames
 * for any that have issues.  This includes:
 * [] members of a collision (multiple entries in different branches
 *    that want to use the same entryname (such as independent ADDS
 *    with the same name))
 * [] divergent renames (an entry given different names in different
 *    branches (and where we have to arbitrarily pick a name))
 * [] optionally, entries that had portability warnings (such as
 *    "README" and "readme" comming together in a directory for the
 *    first time).
 */
void SG_mrg_cset__make_unique_entrynames(SG_context * pCtx,
										 SG_mrg_cset * pMrgCSet);

//////////////////////////////////////////////////////////////////

void SG_mrg_cset_dir__free(SG_context * pCtx,
						   SG_mrg_cset_dir * pMrgCSetDir);

void SG_mrg_cset_dir__alloc__from_entry(SG_context * pCtx,
										SG_mrg_cset * pMrgCSet,
										SG_mrg_cset_entry * pMrgCSetEntry,
										SG_mrg_cset_dir ** ppMrgCSetDir);

void SG_mrg_cset_dir__add_child_entry(SG_context * pCtx,
									  SG_mrg_cset_dir * pMrgCSetDir,
									  SG_mrg_cset_entry * pMrgCSetEntry);

SG_rbtree_foreach_callback SG_mrg_cset_dir__check_for_collisions;

//////////////////////////////////////////////////////////////////

void SG_mrg_cset_entry_collision__alloc(SG_context * pCtx,
										SG_mrg_cset_entry_collision ** ppMrgCSetEntryCollision);

void SG_mrg_cset_entry_collision__free(SG_context * pCtx,
									   SG_mrg_cset_entry_collision * pMrgCSetEntryCollision);

void SG_mrg_cset_entry_collision__add_entry(SG_context * pCtx,
											SG_mrg_cset_entry_collision * pMrgCSetEntryCollision,
											SG_mrg_cset_entry * pMrgCSetEntry);

//////////////////////////////////////////////////////////////////

void SG_mrg_cset_entry_conflict__alloc(SG_context * pCtx,
									   SG_mrg_cset * pMrgCSet,
									   SG_mrg_cset_entry * pMrgCSetEntry_Ancestor,
									   SG_mrg_cset_entry * pMrgCSetEntry_Baseline,
									   SG_mrg_cset_entry_conflict ** ppMrgCSetEntryConflict);

void SG_mrg_cset_entry_conflict__free(SG_context * pCtx,
									  SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict);

/**
 * Add this leaf to the list of branches where this entry was deleted.
 */
void SG_mrg_cset_entry_conflict__append_delete(SG_context * pCtx,
											   SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
											   SG_mrg_cset * pMrgCSet_Leaf_k);

/**
 * Add a this leaf's entry as a CHANGED-ENTRY to the (potential) conflict.
 * We also store the NEQ flags relative to the ancestor entry.
 */
void SG_mrg_cset_entry_conflict__append_change(SG_context * pCtx,
											   SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
											   SG_mrg_cset_entry * pMrgCSetEntry_Leaf_k,
											   SG_mrg_cset_entry_neq neq);


void SG_mrg_cset_entry_conflict__count_deletes(SG_context * pCtx,
											   SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
											   SG_uint32 * pCount);

void SG_mrg_cset_entry_conflict__count_changes(SG_context * pCtx,
											   SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
											   SG_uint32 * pCount);

void SG_mrg_cset_entry_conflict__count_unique_attrbits(SG_context * pCtx,
													   SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
													   SG_uint32 * pCount);

void SG_mrg_cset_entry_conflict__count_unique_entryname(SG_context * pCtx,
														SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
														SG_uint32 * pCount);

void SG_mrg_cset_entry_conflict__count_unique_gid_parent(SG_context * pCtx,
														 SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
														 SG_uint32 * pCount);

void SG_mrg_cset_entry_conflict__count_unique_symlink_hid_blob(SG_context * pCtx,
															   SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
															   SG_uint32 * pCount);

void SG_mrg_cset_entry_conflict__count_unique_submodule_hid_blob(SG_context * pCtx,
																 SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
																 SG_uint32 * pCount);

void SG_mrg_cset_entry_conflict__count_unique_file_hid_blob(SG_context * pCtx,
															SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
															SG_uint32 * pCount);

//////////////////////////////////////////////////////////////////

/**
 * Compute what NEEDS TO HAPPEN to perform a simple merge
 * of CSETs L0 and L1 relative to common ancestor A and
 * producing M.  (We do not actually produce M.)
 *
 * We assume a clean WD.
 *
 *     A
 *    / \.
 *  L0   L1
 *    \ /
 *     M
 *
 * The ancestor may or may not be the absolute LCA.  Our
 * computation may be part of a larger merge that will use
 * our result M as a leaf in another portion of the graph.
 *
 * We return a "candidate SG_mrg_cset" that relects what
 * *should* be in the merge result if we decide to apply it.
 * This HAS NOT be added to any container in pMrg; you are
 * responsible for freeing it.
 *
 * pszInterimCSetName is a label for your convenience (and
 * may be used in debug/log messages).
 */
void SG_mrg__compute_simple2(SG_context * pCtx,
							 SG_mrg * pMrg,
							 const char * pszInterimCSetName,
							 SG_mrg_cset * pMrgCSet_Ancestor,
							 SG_mrg_cset * pMrgCSet_Leaf_0,
							 SG_mrg_cset * pMrgCSet_Leaf_1,
							 SG_mrg_cset ** ppMrgCSet_Result);

/**
 * TODO 2012/01/20 remove this routine since n-way never happened.
 * 
 * Compute what NEEDS TO HAPPEN to perform a simple merge
 * of N CSets relative to a common ancestor A and producing
 * a single M.  (We do not actually produce M.)
 *
 * We assume a clean WD.
 *
 * When N > 2, this is conceptually similar to GIT's Octopus Merge.
 * And like them, we may complain and stop if there are conflicts
 * that would require too much manual involvement; that is, for
 * large values of N, we only attempt to merge orthogonal changes.
 *
 *               A
 *    __________/ \__________
 *   /   /                   \.
 * L0  L1       ...           Ln-1
 *   \___\______   __________/
 *              \ /
 *               M
 *
 * The ancestor may or may not be the absolute LCA.  Our
 * computation may be part of a larger merge that will use
 * our result M as a leaf in another portion of the graph.
 *
 * We return a "candidate SG_mrg_cset" that relects what
 * *should* be in the merge result if we decide to apply it.
 * This HAS NOT be added to any container in pMrg; you are
 * responsible for freeing it.
 *
 * pszInterimCSetName is a label for your convenience (and
 * may be used in debug/log messages).
 */
void SG_mrg__compute_simpleV(SG_context * pCtx,
							 SG_mrg * pMrg,
							 const char * pszInterimCSetName,
							 SG_mrg_cset * pMrgCSet_Ancestor,
							 SG_vector * pVec_MrgCSets_Leaves,
							 SG_mrg_cset ** ppMrgCSet_Result);

//////////////////////////////////////////////////////////////////

/**
 * Resolve all of the __FILE_EDIT_TBD conflicts in the given result-cset.
 * Update the stats if we were able to auto-merge any.
 */
void SG_mrg__automerge_files(SG_context * pCtx, SG_mrg * pMrg, SG_mrg_cset * pMrgCSet);

//////////////////////////////////////////////////////////////////

void _sg_mrg__create_pathname_for_conflict_file(SG_context * pCtx,
												SG_mrg * pMrg,
												SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict,
												const char * pszPrefix,
												SG_pathname ** ppPathReturned);

void _sg_mrg__export_to_temp_file(SG_context * pCtx, SG_mrg * pMrg,
								  const char * pszHidBlob,
								  const SG_pathname * pPathTempFile);

void _sg_mrg__copy_wc_to_temp_file(SG_context * pCtx, SG_mrg * pMrg,
								   SG_mrg_cset_entry * pMrgCSetEntry,
								   const SG_pathname * pPathTempFile);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__merge__determine_target_cset(SG_context * pCtx,
											SG_mrg * pMrg);

void sg_wc_tx__merge__get_starting_conditions(SG_context * pCtx,
											  SG_mrg * pMrg);

void sg_wc_tx__merge__compute_lca(SG_context * pCtx,
								  SG_mrg * pMrg);

void sg_wc_tx__merge__load_csets(SG_context * pCtx,
								 SG_mrg * pMrg);

void sg_wc_tx__merge__compute_simple2(SG_context * pCtx,
									  SG_mrg * pMrg);

void sg_wc_tx__merge__prepare_issue_table(SG_context * pCtx,
										  SG_mrg * pMrg);

void sg_wc_tx__merge__prepare_issues(SG_context * pCtx,
									 SG_mrg * pMrg);

void sg_wc_tx__merge__queue_plan(SG_context * pCtx,
								 SG_mrg * pMrg);

void sg_wc_tx__merge__queue_plan__add_special(SG_context * pCtx,
											  SG_mrg * pMrg,
											  const char * pszGid,
											  SG_mrg_cset_entry * pMrgCSetEntry_FinalResult);

void sg_wc_tx__merge__queue_plan__delete(SG_context * pCtx,
										SG_mrg * pMrg);

void sg_wc_tx__merge__queue_plan__entry(SG_context * pCtx,
										SG_mrg * pMrg);

void sg_wc_tx__merge__queue_plan__peer(SG_context * pCtx, SG_mrg * pMrg,
									   const char * pszKey_GidEntry,
									   SG_mrg_cset_entry * pMrgCSetEntry_FinalResult,
									   const SG_mrg_cset_entry * pMrgCSetEntry_Baseline);

void sg_wc_tx__merge__queue_plan__park(SG_context * pCtx,
									   SG_mrg * pMrg,
									   SG_mrg_cset_entry * pMrgCSetEntry);

void sg_wc_tx__merge__queue_plan__undo_delete(SG_context * pCtx,
											  SG_mrg * pMrg,
											  const char * pszGid,
											  SG_mrg_cset_entry * pMrgCSetEntry_FinalResult);

void sg_wc_tx__merge__queue_plan__undo_lost(SG_context * pCtx,
											SG_mrg * pMrg,
											const char * pszGid,
											SG_mrg_cset_entry * pMrgCSetEntry_FinalResult);

void sg_wc_tx__merge__queue_plan__unpark(SG_context * pCtx,
										 SG_mrg * pMrg);

void sg_wc_tx__merge__queue_plan__kill_list(SG_context * pCtx,
											SG_mrg * pMrg);

void sg_wc_tx__merge__make_stats(SG_context * pCtx,
								 SG_mrg * pMrg,
								 SG_vhash ** ppvhStats);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__postmerge__commit_cleanup(SG_context * pCtx,
										 SG_wc_tx * pWcTx);

//////////////////////////////////////////////////////////////////

#define SG_MRG_CSET_ENTRY_COLLISION_NULLFREE(pCtx,p)        SG_STATEMENT( SG_context__push_level(pCtx);      SG_mrg_cset_entry_collision__free(pCtx, p);    SG_ASSERT(!SG_CONTEXT__HAS_ERR(pCtx)); SG_context__pop_level(pCtx); p=NULL; )
#define SG_MRG_CSET_ENTRY_CONFLICT_NULLFREE(pCtx,p)         SG_STATEMENT( SG_context__push_level(pCtx);       SG_mrg_cset_entry_conflict__free(pCtx, p);    SG_ASSERT(!SG_CONTEXT__HAS_ERR(pCtx)); SG_context__pop_level(pCtx); p=NULL; )
#define SG_MRG_CSET_ENTRY_NULLFREE(pCtx,p)                  SG_STATEMENT( SG_context__push_level(pCtx);                SG_mrg_cset_entry__free(pCtx, p);    SG_ASSERT(!SG_CONTEXT__HAS_ERR(pCtx)); SG_context__pop_level(pCtx); p=NULL; )
#define SG_MRG_CSET_DIR_NULLFREE(pCtx,p)                    SG_STATEMENT( SG_context__push_level(pCtx);                  SG_mrg_cset_dir__free(pCtx, p);    SG_ASSERT(!SG_CONTEXT__HAS_ERR(pCtx)); SG_context__pop_level(pCtx); p=NULL; )
#define SG_MRG_CSET_NULLFREE(pCtx,p)                        SG_STATEMENT( SG_context__push_level(pCtx);                      SG_mrg_cset__free(pCtx, p);    SG_ASSERT(!SG_CONTEXT__HAS_ERR(pCtx)); SG_context__pop_level(pCtx); p=NULL; )

//////////////////////////////////////////////////////////////////

#if TRACE_WC_MERGE
void _sg_wc_tx__merge__trace_msg__dump_entry_conflict_flags(SG_context * pCtx,
															SG_mrg_cset_entry_conflict_flags flags,
															SG_uint32 indent);

void _sg_wc_tx__merge__trace_msg__dir_list_as_hierarchy(SG_context * pCtx,
														const char * pszLabel,
														SG_mrg_cset * pMrgCSet);

void _sg_wc_tx__merge__trace_msg__dump_raw_stats(SG_context * pCtx,
												 SG_mrg * pMrg);

#endif

#if 0 && TRACE_WC_MERGE

static SG_rbtree_foreach_callback _sg_wc_tx__merge__trace_msg__dir_list_as_hierarchy1;



static void _sg_wc_tx__merge__trace_msg__v_input(SG_context * pCtx,
												 const char * pszLabel,
												 const char * pszInterimCSetName,
												 SG_mrg_cset * pMrgCSet_Ancestor,
												 SG_vector * pVec_MrgCSet_Leaves);
static void _sg_wc_tx__merge__trace_msg__v_ancestor(SG_context * pCtx,
													const char * pszLabel,
													SG_int32 marker_value,
													SG_mrg_cset * pMrgCSet_Ancestor,
													const char * pszGid_Entry);
static void _sg_wc_tx__merge__trace_msg__v_leaf_eq_neq(SG_context * pCtx,
													   const char * pszLabel,
													   SG_int32 marker_value,
													   SG_uint32 kLeaf,
													   SG_mrg_cset * pMrgCSet_Leaf_k,
													   const char * pszGid_Entry,
													   SG_mrg_cset_entry_neq neq);
static void _sg_wc_tx__merge__trace_msg__v_leaf_deleted(SG_context * pCtx,
														const char * pszLabel,
														SG_int32 marker_value,
														SG_mrg_cset * pMrgCSet_Ancestor,
														SG_uint32 kLeaf,
														const char * pszGid_Entry);
static void _sg_wc_tx__merge__trace_msg__v_leaf_added(SG_context * pCtx,
													  const char * pszLabel,
													  SG_int32 marker_value,
													  SG_mrg_cset * pMrgCSet_Leaf_k,
													  SG_uint32 kLeaf,
													  SG_mrg_cset_entry * pMrgCSetEntry_Leaf_k);
static void _sg_wc_tx__merge__trace_msg__dump_delete_list(SG_context * pCtx,
														  const char * pszLabel,
														  SG_mrg_cset * pMrgCSet_Result);

#endif

//////////////////////////////////////////////////////////////////

void sg_wc_tx__merge__delete_automerge_tempfiles_on_abort(SG_context * pCtx,
														  SG_wc_tx * pWcTx);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__fake_merge__update__build_pc(SG_context * pCtx,
											SG_mrg * pMrg);

void sg_wc_tx__fake_merge__update__get_starting_conditions(SG_context * pCtx,
														   SG_mrg * pMrg,
														   const char * pszHid_StartingBaseline,
														   const char * pszBranchName_Starting);

void sg_wc_tx__fake_merge__update__determine_target_cset(SG_context * pCtx,
														 SG_mrg * pMrg,
														 const char * pszHidTarget);

void sg_wc_tx__fake_merge__update__compute_lca(SG_context * pCtx, SG_mrg * pMrg);

void sg_wc_tx__fake_merge__update__load_csets(SG_context * pCtx, SG_mrg * pMrg);

void sg_wc_tx__fake_merge__update__status(SG_context * pCtx,
										  SG_mrg * pMrg,
										  SG_varray ** ppvaStatus);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__fake_merge__revert_all__get_starting_conditions(SG_context * pCtx, SG_mrg * pMrg);

void sg_wc_tx__fake_merge__revert_all__determine_target_cset(SG_context * pCtx, SG_mrg * pMrg);

void sg_wc_tx__fake_merge__revert_all__compute_lca(SG_context * pCtx, SG_mrg * pMrg);

void sg_wc_tx__fake_merge__revert_all__load_csets(SG_context * pCtx, SG_mrg * pMrg);

void sg_wc_tx__fake_merge__revert_all__cleanup_db(SG_context * pCtx, SG_mrg * pMrg);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__merge__add_to_kill_list(SG_context * pCtx,
									   SG_mrg * pMrg,
									   SG_uint64 uiAliasGid);

void sg_wc_tx__merge__remove_from_kill_list(SG_context * pCtx,
											SG_mrg * pMrg,
											const SG_uint64 uiAliasGid);

//////////////////////////////////////////////////////////////////

void sg_wc_tx__merge__main(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   const SG_wc_merge_args * pMergeArgs,
						   SG_vhash ** ppvhStats,
						   char ** ppszHidTarget);

void sg_wc_tx__merge__compute_preview_target(SG_context * pCtx,
											 SG_wc_tx * pWcTx,
											 const SG_wc_merge_args * pMergeArgs,
											 char ** ppszHidTarget);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC6MERGE__PRIVATE_PROTOTYPES_H
