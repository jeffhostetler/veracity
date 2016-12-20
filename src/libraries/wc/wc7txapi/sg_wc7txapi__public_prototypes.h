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

/**
 *
 * @file sg_wc7txapi__public_prototypes.h
 *
 * @details This routines in this API layer define the public TX-based
 * routines.  All paths are "raw inputs" (see below).
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC7TXAPI__PUBLIC_PROTOTYPES_H
#define H_SG_WC7TXAPI__PUBLIC_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_wc_tx__free(SG_context * pCtx, SG_wc_tx * pWcTx);

#define SG_WC_TX__NULLFREE(pCtx,p)    _sg_generic_nullfree(pCtx,p,SG_wc_tx__free)

/**
 * We create the TX using a pathname anywhere within
 * the working directory.  It doesn't need to be the
 * WD-top.  We will find the WD-top and the repo-name
 * for you.  If no path is given, we will use the CWD
 * and start searching from there.
 *
 */
void SG_wc_tx__alloc__begin(SG_context * pCtx,
							SG_wc_tx ** ppWcTx,
							const SG_pathname * pPathInWD,
							SG_bool bReadOnly);

#if defined(DEBUG)

#define SG_WC_TX__ALLOC__BEGIN(pCtx,ppNew,pPath,bReadOnly)				\
	SG_STATEMENT(														\
		SG_wc_tx * _pNew = NULL;										\
		SG_wc_tx__alloc__begin(pCtx,&_pNew,pPath,bReadOnly);			\
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"SG_wc_tx");	\
		*(ppNew) = _pNew;												\
		)

#else

#define SG_WC_TX__ALLOC__BEGIN(pCtx,ppNew,pPath,bReadOnly)	\
	SG_wc_tx__alloc__begin(pCtx,ppNew,pPath,bReadOnly)

#endif

//////////////////////////////////////////////////////////////////

/**
 * Create a Working Directory.  Either create a new root
 * directory or use an existing (uncontrolled) directory
 * and create/initialize .sgdrawer and its contents.
 *
 */
void SG_wc_tx__alloc__begin__create(SG_context * pCtx,
									SG_wc_tx ** ppWcTx,
									const char * pszRepoName,
									const char * pszPath,
									SG_bool bRequireNewDirOrEmpty,
									const char * pszHidCSet);

#if defined(DEBUG)

#define SG_WC_TX__ALLOC__BEGIN__CREATE(pCtx,ppNew,pszRepoName,pPath,bRequire,pszHid) \
	SG_STATEMENT(														\
		SG_wc_tx * _pNew = NULL;										\
		SG_wc_tx__alloc__begin__create(pCtx,&_pNew,pszRepoName,pPath,bRequire,pszHid); \
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"SG_wc_tx");	\
		*(ppNew) = _pNew;												\
		)

#else

#define SG_WC_TX__ALLOC__BEGIN__CREATE(pCtx,ppNew,pszRepoName,pPath,bRequire,pszHid) \
	SG_wc_tx__alloc__begin__create(pCtx,ppNew,pszRepoName,pPath,bRequire,pszHid)

#endif

//////////////////////////////////////////////////////////////////

void SG_wc_tx__apply(SG_context * pCtx, SG_wc_tx * pWcTx);

void SG_wc_tx__cancel(SG_context * pCtx, SG_wc_tx * pWcTx);

void SG_wc_tx__abort_create_and_free(SG_context * pCtx, SG_wc_tx ** ppWcTx);

//////////////////////////////////////////////////////////////////

void SG_wc_tx__journal(SG_context * pCtx,
					   SG_wc_tx * pWcTx,
					   const SG_varray ** ppvaJournal);

void SG_wc_tx__journal_length(SG_context * pCtx,
							  SG_wc_tx * pWcTx,
							  SG_uint32 * pCount);

void SG_wc_tx__journal__alloc_copy(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   SG_varray ** ppvaJournal);

#if defined(DEBUG)

#define SG_WC_TX__JOURNAL__ALLOC_COPY(pCtx,pWcTx,ppNew)					\
	SG_STATEMENT(														\
		SG_varray * _pNew = NULL;										\
		SG_wc_tx__journal__alloc_copy(pCtx,pWcTx,&_pNew);				\
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"SG_wc_tx__journal");	\
		*(ppNew) = _pNew;												\
		)

#else

#define SG_WC_TX__JOURNAL__ALLOC_COPY(pCtx,pWcTx,ppNew)					\
	SG_wc_tx__journal__alloc_copy(pCtx,pWcTx,ppNew)

#endif

//////////////////////////////////////////////////////////////////

/**
 * In all of the following public API routines we take
 * one or more "pszInputPaths".  These are intended to
 * be "raw inputs" from the user.  These can be any of
 * the following:
 * 
 * [] absolute paths
 * [] relative paths
 * [] repo-paths
 *
 * Depending on the API routine, we can either substitute
 * "@/" or the CWD or throw an error when an input is
 * NULL or zero-length.
 *
 * Each of these routines will parse the raw input and
 * create a validated repo-path for internal use.  If an
 * input cannot be parsed, they will throw.
 *
 * In all cases, the input path refers to a live/in-TX
 * item.
 * 
 */

//////////////////////////////////////////////////////////////////

void SG_wc_tx__add(SG_context * pCtx,
				   SG_wc_tx * pWcTx,
				   const char * pszInput,
				   SG_uint32 depth,
				   SG_bool bNoIgnores);

void SG_wc_tx__add__stringarray(SG_context * pCtx,
								SG_wc_tx * pWcTx,
								const SG_stringarray * psaInputs,
								SG_uint32 depth,
								SG_bool bNoIgnores);

//////////////////////////////////////////////////////////////////

void SG_wc_tx__addremove(SG_context * pCtx,
						 SG_wc_tx * pWcTx,
						 const char * pszInput,
						 SG_uint32 depth,
						 SG_bool bNoIgnores);

void SG_wc_tx__addremove__stringarray(SG_context * pCtx,
									  SG_wc_tx * pWcTx,
									  const SG_stringarray * psaInputs,
									  SG_uint32 depth,
									  SG_bool bNoIgnores);

//////////////////////////////////////////////////////////////////

void SG_wc_tx__get_item_status_flags(SG_context * pCtx,
									 SG_wc_tx * pWcTx,
									 const char * pszInput,
									 SG_bool bNoIgnores,
									 SG_bool bNoTSC,
									 SG_wc_status_flags * pStatusFlags,
									 SG_vhash ** ppvhProperties);

void SG_wc_tx__get_item_dirstatus_flags(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										const char * pszInput,
										SG_wc_status_flags * pStatusFlagsDir,
										SG_vhash ** ppvhProperties,
										SG_bool * pbDirContainsChanges);

//////////////////////////////////////////////////////////////////

void SG_wc_tx__move(SG_context * pCtx,
					SG_wc_tx * pWcTx,
					const char * pszInput_Src,
					const char * pszInput_DestDir,
					SG_bool bNoAllowAfterTheFact);

void SG_wc_tx__move__stringarray(SG_context * pCtx,
								 SG_wc_tx * pWcTx,
								 const SG_stringarray * psaInputs,
								 const char * pszInput_DestDir,
								 SG_bool bNoAllowAfterTheFact);

void SG_wc_tx__move_rename(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   const char * pszInput_Src,
						   const char * pszInput_Dest,
						   SG_bool bNoAllowAfterTheFact);

void SG_wc_tx__move_rename2(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							const char * pszInput_Src,
							const char * pszInput_DestDir,
							const char * pszEntryname_Dest,
							SG_bool bNoAllowAfterTheFact);

void SG_wc_tx__move_rename3(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							const char * pszInput_Src,
							const char * pszInput_DestDir,
							const char * pszEntryname_Dest,
							SG_bool bNoAllowAfterTheFact,
							SG_wc_status_flags xu_mask);

void SG_wc_tx__rename(SG_context * pCtx,
					  SG_wc_tx * pWcTx,
					  const char * pszInput_Src,
					  const char * pszNewEntryname,
					  SG_bool bNoAllowAfterTheFact);

//////////////////////////////////////////////////////////////////

void SG_wc_tx__remove(SG_context * pCtx,
					  SG_wc_tx * pWcTx,
					  const char * pszInput,
					  SG_bool bNonRecursive,
					  SG_bool bForce,
					  SG_bool bNoBackups,
					  SG_bool bKeep);

void SG_wc_tx__remove2(SG_context * pCtx,
					   SG_wc_tx * pWcTx,
					   const char * pszInput,
					   SG_bool bNonRecursive,
					   SG_bool bForce,
					   SG_bool bNoBackups,
					   SG_bool bKeep,
					   SG_bool bFlattenAddSpecialPlusRemove);

void SG_wc_tx__remove__stringarray(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   const SG_stringarray * psaInputs,
								   SG_bool bNonRecursive,
								   SG_bool bForce,
								   SG_bool bNoBackups,
								   SG_bool bKeep);

//////////////////////////////////////////////////////////////////

void SG_wc_tx__status(SG_context * pCtx,
					  SG_wc_tx * pWcTx,
					  const char * pszInput,
					  SG_uint32 depth,
					  SG_bool bListUnchanged,
					  SG_bool bNoIgnores,
					  SG_bool bNoTSC,
					  SG_bool bListSparse,
					  SG_bool bListReserved,
					  SG_bool bNoSort,
					  SG_varray ** ppvaStatus,
					  SG_vhash ** ppvhLegend);

void SG_wc_tx__status__stringarray(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   const SG_stringarray * psaInputs,
								   SG_uint32 depth,
								   SG_bool bListUnchanged,
								   SG_bool bNoIgnores,
								   SG_bool bNoTSC,
								   SG_bool bListSparse,
								   SG_bool bListReserved,
								   SG_bool bNoSort,
								   SG_varray ** ppvaStatus,
								   SG_vhash ** ppvhLegend);

//////////////////////////////////////////////////////////////////

void SG_wc_tx__mstatus(SG_context * pCtx,
					   SG_wc_tx * pWcTx,
					   SG_bool bNoIgnores,
					   SG_bool bNoFallback,
					   SG_bool bNoSort,
					   SG_varray ** ppvaStatus,
					   SG_vhash ** ppvhLegend);

//////////////////////////////////////////////////////////////////

void SG_wc_tx__diff__setup(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   const SG_rev_spec * pRevSpec,
						   const char * pszInput,
						   SG_uint32 depth,
						   SG_bool bNoIgnores,
						   SG_bool bNoTSC,
						   SG_bool bNoSort,
						   SG_bool bInteractive,
						   const char * pszTool,
						   SG_varray ** ppvaDiffSteps);

void SG_wc_tx__diff__setup__stringarray(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										const SG_rev_spec * pRevSpec,
										const SG_stringarray * psaInputs,
										SG_uint32 depth,
										SG_bool bNoIgnores,
										SG_bool bNoTSC,
										SG_bool bNoSort,
										SG_bool bInteractive,
										const char * pszTool,
										SG_varray ** ppvaDiffSteps);

//////////////////////////////////////////////////////////////////

void SG_wc_tx__revert_all(SG_context * pCtx,
						  SG_wc_tx * pWcTx,
						  SG_bool bNoBackups,
						  SG_vhash ** ppvhStats);

void SG_wc_tx__revert_item(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							const char * pszInput,
							SG_uint32 depth,
							SG_wc_status_flags flagMask,
							SG_bool bNoBackups);

void SG_wc_tx__revert_items__stringarray(SG_context * pCtx,
										 SG_wc_tx * pWcTx,
										 const SG_stringarray * psaInputs,
										 SG_uint32 depth,
										 SG_wc_status_flags flagMask,
										 SG_bool bNoBackups);

//////////////////////////////////////////////////////////////////

void SG_wc_tx__flush_timestamp_cache(SG_context * pCtx, SG_wc_tx * pWcTx);

//////////////////////////////////////////////////////////////////

void SG_wc_tx__get_wc_csets__vhash(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   SG_vhash ** ppvhCSets);

void SG_wc_tx__get_wc_parents__varray(SG_context * pCtx,
									  SG_wc_tx * pWcTx,
									  SG_varray ** ppvaParents);

void SG_wc_tx__get_wc_parents__stringarray(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   SG_stringarray ** ppsaParents);

void SG_wc_tx__get_wc_baseline(SG_context * pCtx,
							   SG_wc_tx * pWcTx,
							   char ** ppszHidBaseline,
							   SG_bool * pbHasMerge);

void SG_wc_tx__get_wc_info(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   SG_vhash ** ppvhInfo);

//////////////////////////////////////////////////////////////////

void SG_wc_tx__overwrite_file_from_repo(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										const char * pszInput_Src,
										const char * pszHidBlob,
										SG_int64 attrbits,
										SG_bool bBackupFile,
										SG_wc_status_flags xu_mask);

void SG_wc_tx__overwrite_file_from_file(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										const char * pszInput_Src,
										const SG_pathname * pPathTemp,
										SG_int64 attrbits,
										SG_wc_status_flags xu_mask);

void SG_wc_tx__overwrite_file_from_file2(SG_context * pCtx,
										 SG_wc_tx * pWcTx,
										 const char * pszInput_Src,
										 const SG_pathname * pPathTemp,
										 SG_wc_status_flags xu_mask);

void SG_wc_tx__overwrite_symlink_from_repo(SG_context * pCtx,
										   SG_wc_tx * pWcTx,
										   const char * pszInput_Src,
										   const char * pszHid,
										   SG_int64 attrbits,
										   SG_wc_status_flags xu_mask);

void SG_wc_tx__overwrite_symlink_target(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										const char * pszInput_Src,
										const char * pszNewTarget,
										SG_wc_status_flags xu_mask);

void SG_wc_tx__set_attrbits(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							const char * pszInput_Src,
							SG_int64 attrbits,
							SG_wc_status_flags xu_mask);

void SG_wc_tx__add_special(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   const char * pszInput_Dir,
						   const char * pszEntryname,
						   const char * pszGid,
						   SG_treenode_entry_type tneType,
						   const char * pszHidBlob,
						   SG_int64 attrbits,
						   SG_wc_status_flags statusFlagsAddSpecialReason);

void SG_wc_tx__undo_delete(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   const char * pszInput_Src,
						   const char * pszInput_DestDir,
						   const char * pszEntryname_Dest,
						   const SG_wc_undo_delete_args * pArgs,
						   SG_wc_status_flags xu_mask);

void SG_wc_tx__undo_lost(SG_context * pCtx,
						 SG_wc_tx * pWcTx,
						 const char * pszInput_Src,
						 const char * pszInput_DestDir,
						 const char * pszEntryname_Dest,
						 const SG_wc_undo_delete_args * pArgs,
						 SG_wc_status_flags xu_mask);

void SG_wc_tx__get_item_repo_path(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  const char * pszInput,
								  SG_string ** ppStringRepoPathComputed);

void SG_wc_tx__get_item_gid(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							const char * pszInput,
							char ** ppszGid);

void SG_wc_tx__get_item_gid__stringarray(SG_context * pCtx,
										 SG_wc_tx * pWcTx,
										 const SG_stringarray * psaInput,
										 SG_stringarray ** ppsaGid);

void SG_wc_tx__get_item_gid_path(SG_context * pCtx,
								 SG_wc_tx * pWcTx,
								 const char * pszInput,
								 char ** ppszGidPath);

void SG_wc_tx__get_resolve_info(SG_context * pCtx,
								SG_wc_tx * pWcTx,
								SG_resolve ** ppResolve);

//////////////////////////////////////////////////////////////////

void SG_wc_tx__get_item_issues(SG_context * pCtx,
							   SG_wc_tx * pWcTx,
							   const char * pszInput,
							   SG_vhash ** ppvhItemIssues);

void SG_wc_tx__get_item_resolve_info(SG_context * pCtx,
									 SG_wc_tx * pWcTx,
									 const char * pszInput,
									 SG_vhash ** ppvhResolveInfo);

//////////////////////////////////////////////////////////////////

void SG_wc_tx__get_repo_and_wd_top(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   SG_repo ** ppRepo,
								   SG_pathname ** ppPathWorkingDirectoryTop);

void SG_wc_tx__get_wc_top(SG_context * pCtx,
						  SG_wc_tx * pWcTx,
						  SG_pathname ** ppPath);

//////////////////////////////////////////////////////////////////

void SG_wc_tx__branch__attach(SG_context * pCtx,
							  SG_wc_tx * pWcTx,
							  const char * pszBranchName);

void SG_wc_tx__branch__attach_new(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  const char * pszBranchName);

void SG_wc_tx__branch__detach(SG_context * pCtx,
							  SG_wc_tx * pWcTx);

void SG_wc_tx__branch__get(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   char ** ppszBranchName);

//////////////////////////////////////////////////////////////////

void SG_wc_tx__status1(SG_context * pCtx,
					   SG_wc_tx * pWcTx,
					   const SG_rev_spec * pRevSpec,
					   const char * pszInput,
					   SG_uint32 depth,
					   SG_bool bListUnchanged,
					   SG_bool bNoIgnores,
					   SG_bool bNoTSC,
					   SG_bool bListSparse,
					   SG_bool bListReserved,
					   SG_bool bNoSort,
					   SG_varray ** ppvaStatus);

void SG_wc_tx__status1__stringarray(SG_context * pCtx,
									SG_wc_tx * pWcTx,
									const SG_rev_spec * pRevSpec,
									const SG_stringarray * psaInputs,
									SG_uint32 depth,
									SG_bool bListUnchanged,
									SG_bool bNoIgnores,
									SG_bool bNoTSC,
									SG_bool bListSparse,
									SG_bool bListReserved,
									SG_bool bNoSort,
									SG_varray ** ppvaStatus);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC7TXAPI__PUBLIC_PROTOTYPES_H
