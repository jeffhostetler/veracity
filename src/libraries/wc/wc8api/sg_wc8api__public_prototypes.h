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

#ifndef H_SG_WC8API__PUBLIC_PROTOTYPES_H
#define H_SG_WC8API__PUBLIC_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Public, convenience versions of the WC verbs.  Each of them
 * begin a TX, do a single operation, and commit/rollback.
 *
 * They automatically find the (pRepo, pPathWorkingDirectoryTop)
 * using the CWD.
 *
 * Each of the "pszInput" variables can be either:
 * [] relative path,
 * [] absolute path,
 * [] repo-path
 *
 * If you need to do more than 1 operation and/or need to
 * specify a pPathWorkingDirectoryTop (different from the
 * one that we would find using the CWD), then use the
 * interfaces in wc7txapi rather than the ones here.
 *
 */

//////////////////////////////////////////////////////////////////

void SG_wc__checkout(SG_context * pCtx,
					 const char * pszRepoName,
					 const char * pszPath,
					 const SG_rev_spec * pRevSpec,
					 const char * pszAttach,
					 const char * pszAttachNew,
					 const SG_varray * pvaSparse);

void SG_wc__initialize(SG_context * pCtx,
					   const char * pszRepoName,
					   const char * pszPath,
					   const char * pszHidCSet,
					   const char * pszTargetBranchName);

//////////////////////////////////////////////////////////////////

void SG_wc__add(SG_context * pCtx,
				const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
				const char * pszInput,
				SG_uint32 depth,
				SG_bool bNoIgnores,
				SG_bool bTest,
				SG_varray ** ppvaJournal);

void SG_wc__add__stringarray(SG_context * pCtx,
							 const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
							 const SG_stringarray * psaInputs,
							 SG_uint32 depth,
							 SG_bool bNoIgnores,
							 SG_bool bTest,
							 SG_varray ** ppvaJournal);

//////////////////////////////////////////////////////////////////

void SG_wc__addremove(SG_context * pCtx,
					  const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
					  const char * pszInput,
					  SG_uint32 depth,
					  SG_bool bNoIgnores,
					  SG_bool bTest,
					  SG_varray ** ppvaJournal);

void SG_wc__addremove__stringarray(SG_context * pCtx,
								   const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
								   const SG_stringarray * psaInputs,
								   SG_uint32 depth,
								   SG_bool bNoIgnores,
								   SG_bool bTest,
								   SG_varray ** ppvaJournal);

//////////////////////////////////////////////////////////////////

void SG_wc__move(SG_context * pCtx,
				 const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
				 const char * pszInput_Src,
				 const char * pszInput_DestDir,
				 SG_bool bNoAllowAfterTheFact,
				 SG_bool bTest,
				 SG_varray ** ppvaJournal);

void SG_wc__move__stringarray(SG_context * pCtx,
							  const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
							  const SG_stringarray * psaInputs,
							  const char * pszInput_DestDir,
							  SG_bool bNoAllowAfterTheFact,
							  SG_bool bTest,
							  SG_varray ** ppvaJournal);

void SG_wc__move_rename(SG_context * pCtx,
						const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
						const char * pszInput_Src,
						const char * pszInput_Dest,
						SG_bool bNoAllowAfterTheFact,
						SG_bool bTest,
						SG_varray ** ppvaJournal);

void SG_wc__rename(SG_context * pCtx,
				   const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
				   const char * pszInput,
				   const char * pszNewEntryname,
				   SG_bool bNoAllowAfterTheFact,
				   SG_bool bTest,
				   SG_varray ** ppvaJournal);

//////////////////////////////////////////////////////////////////

void SG_wc__remove(SG_context * pCtx,
				   const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
				   const char * pszInput,
				   SG_bool bNonRecursive,
				   SG_bool bForce,
				   SG_bool bNoBackups,
				   SG_bool bKeep,
				   SG_bool bTest,
				   SG_varray ** ppvaJournal);

void SG_wc__remove__stringarray(SG_context * pCtx,
								const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
								const SG_stringarray * psaInputs,
								SG_bool bNonRecursive,
								SG_bool bForce,
								SG_bool bNoBackups,
								SG_bool bKeep,
								SG_bool bTest,
								SG_varray ** ppvaJournal);

//////////////////////////////////////////////////////////////////

void SG_wc__get_item_status_flags(SG_context * pCtx,
								  const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
								  const char * pszInput,
								  SG_bool bNoIgnores,
								  SG_bool bNoTSC,
								  SG_wc_status_flags * pStatusFlags,
								  SG_vhash ** ppvhProperties);

void SG_wc__get_item_dirstatus_flags(SG_context * pCtx,
									 const SG_pathname* pPathWc,
									 const char * pszInput,
									 SG_wc_status_flags * pStatusFlagsDir,
									 SG_vhash ** ppvhProperties,
									 SG_bool * pbDirContainsChanges);

//////////////////////////////////////////////////////////////////

void SG_wc__status(SG_context * pCtx,
				   const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
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

void SG_wc__status__stringarray(SG_context * pCtx,
								const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
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

void SG_wc__mstatus(SG_context * pCtx,
					const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
					SG_bool bNoIgnores,
					SG_bool bNoFallback,
					SG_bool bNoSort,
					SG_varray ** ppvaStatus,
					SG_vhash ** ppvhLegend);

//////////////////////////////////////////////////////////////////

void SG_wc__diff__run(SG_context * pCtx,
					  const SG_pathname * pPathWc,
					  const SG_vhash * pvhDiffItem,
					  SG_vhash ** ppvhResultCodes);

void SG_wc__diff__throw(SG_context * pCtx,
						const SG_pathname * pPathWc,
						const SG_rev_spec * pRevSpec,
						const SG_stringarray * psa_args,
						SG_uint32 nDepth,
						SG_bool bNoIgnores,
						SG_bool bNoTSC,
						SG_bool bInteractive,
						const char * psz_tool);

//////////////////////////////////////////////////////////////////

void SG_wc__flush_timestamp_cache(SG_context * pCtx,
								  const SG_pathname* pPathWc); // a disk path inside the working copy or NULL to use cwd

//////////////////////////////////////////////////////////////////

void SG_wc__get_wc_parents__varray(SG_context * pCtx,
								   const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
								   SG_varray ** ppvaParents);

void SG_wc__get_wc_parents__stringarray(SG_context * pCtx,
										const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
										SG_stringarray ** ppvaParents);

void SG_wc__get_wc_baseline(SG_context * pCtx,
							const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
							char ** ppszHidBaseline,
							SG_bool * pbHasMerge);

void SG_wc__get_wc_info(SG_context * pCtx,
						const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
						SG_vhash ** ppvhInfo);

void SG_wc__get_wc_top(SG_context * pCtx,
					   const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
					   SG_pathname ** ppPath);

//////////////////////////////////////////////////////////////////

void SG_wc__commit(SG_context * pCtx,
				   const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
				   const SG_wc_commit_args * pCommitArgs,
				   SG_bool bTest,
				   SG_varray ** ppvaJournal,
				   char ** ppszHidNewCSet);

//////////////////////////////////////////////////////////////////

void SG_wc__update(SG_context * pCtx,
				   const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
				   const SG_wc_update_args * pUpdateArgs,
				   SG_bool bTest,	// --test is a layer-8-only flag so don't put it in SG_wc_update_args.
				   SG_varray ** ppvaJournal,	// optional
				   SG_vhash ** ppvhStats,		// optional
				   SG_varray ** ppvaStatus,		// optional
				   char ** ppszHidChosen);

void SG_wc__merge(SG_context * pCtx,
				  const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
				  const SG_wc_merge_args * pMergeArgs,
				  SG_bool bTest,		// --test is a layer-8-only flag.
				  SG_varray ** ppvaJournal,
				  SG_vhash ** ppvhStats,
				  char ** ppszHidTarget,
				  SG_varray ** ppvaStatus);

void SG_wc__merge__compute_preview_target(SG_context * pCtx,
										  const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
										  const SG_wc_merge_args * pMergeArgs,
										  char ** ppszHidTarget);

void SG_wc__revert_all(SG_context * pCtx,
					   const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
					   SG_bool bNoBackups,
					   SG_bool bTest,
					   SG_varray ** ppvaJournal,
					   SG_vhash ** ppvhStats);

void SG_wc__revert_item(SG_context * pCtx,
						 const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
						 const char * pszInput,
						 SG_uint32 depth,
						 SG_wc_status_flags flagMask,
						 SG_bool bNoBackups,
						 SG_bool bTest,
						 SG_varray ** ppvaJournal);

void SG_wc__revert_items__stringarray(SG_context * pCtx,
									  const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
									  const SG_stringarray * psaInputs,
									  SG_uint32 depth,
									  SG_wc_status_flags flagMask,
									  SG_bool bNoBackups,
									  SG_bool bTest,
									  SG_varray ** ppvaJournal);

//////////////////////////////////////////////////////////////////

void SG_wc__lock(SG_context * pCtx,
				 const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
				 const SG_stringarray * psaArgs,
				 const char* psz_username,
				 const char* psz_password,
				 const char* psz_server);

void SG_wc__unlock(SG_context * pCtx,
				   const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
				   const SG_stringarray * psaInputs,
				   SG_bool bForce,
				   const char* psz_username,
				   const char* psz_password,
				   const char* psz_server);

//////////////////////////////////////////////////////////////////

void SG_wc__get_item_gid(SG_context * pCtx,
						 const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
						 const char * pszInput,
						 char ** ppszGid);

void SG_wc__get_item_gid__stringarray(SG_context * pCtx,
									  const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
									  const SG_stringarray * psaInput,
									  SG_stringarray ** ppsaGid);

void SG_wc__get_item_gid_path(SG_context * pCtx,
							  const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
							  const char * pszInput,
							  char ** ppszGidPath);

void SG_wc__get_item_issues(SG_context * pCtx,
							const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
							const char * pszInput,
							SG_vhash ** ppvhItemIssues);

void SG_wc__get_item_resolve_info(SG_context * pCtx,
								  const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
								  const char * pszInput,
								  SG_vhash ** ppvhResolveInfo);

//////////////////////////////////////////////////////////////////

void SG_wc__branch__attach(SG_context * pCtx,
						   const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
						   const char * pszBranchName);

void SG_wc__branch__attach_new(SG_context * pCtx,
							   const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
							   const char * pszBranchName);

void SG_wc__branch__detach(SG_context * pCtx,
						   const SG_pathname* pPathWc); // a disk path inside the working copy or NULL to use cwd

void SG_wc__branch__get(SG_context * pCtx,
						const SG_pathname* pPathWc, // a disk path inside the working copy or NULL to use cwd
						char ** ppszBranchName);

//////////////////////////////////////////////////////////////////

void SG_wc__line_history(SG_context * pCtx,
				const SG_pathname* pPathWc,
				const char * pszInput,
				SG_int32 start_line,
				SG_int32 length,
				SG_varray ** ppvaJournal);

//////////////////////////////////////////////////////////////////

void SG_wc__status1(SG_context * pCtx,
					const SG_pathname* pPathWc,
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

void SG_wc__status1__stringarray(SG_context * pCtx,
								 const SG_pathname* pPathWc,
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

#endif//H_SG_WC8API__PUBLIC_PROTOTYPES_H
