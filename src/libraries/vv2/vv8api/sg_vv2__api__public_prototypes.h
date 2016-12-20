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

#ifndef H_SG_VV2__API__PUBLIC_PROTOTYPES_H
#define H_SG_VV2__API__PUBLIC_PROTOTYPES_H
#include <vv8api/sg_vv2__api__public_typedefs.h>

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////


/**
 * Create a new REPO with the given name and
 * return the HID of the initial CSet.
 *
 * You own the returned HID.
 *
 * TODO 2010/10/21 Deal with admin_id.
 */
void SG_vv2__init_new_repo(SG_context * pCtx,
						   const char * pszRepoName,
						   const char * pszFolder,         //< [in] arbitrary path (abs or rel) for init WD (if desired); defaults to CWD if blank or null
						   const char * pszStorage,       //< [in] If null, use default config: new_repo/driver
						   const char * pszHashMethod,	  //< [in] If null, use default config: new_repo/hash_method
						   SG_bool bNoWD,                 //< [in] Request a new repo only (no WD)
						   const char * psz_shared_users, //< [in] The new repo should share users accounts with this one 
						   SG_bool bFromUserMaster,
						   char ** ppszGidRepoId,         //< [out] repo-id of the created repo (gid)
						   char ** ppszHidCSetFirst);     //< [out] HID of the initial CSET

/**
 * Export the contents of the version control tree
 * as the given CSET HID.
 */
void SG_vv2__export(SG_context * pCtx,
					const char * pszRepoName,
					const char * pszFolder,
					const SG_rev_spec * pRevSpec,
					const SG_varray * pvaSparse);

//////////////////////////////////////////////////////////////////

/**
 * Add a COMMENT to an existing CSET.
 *
 * This DOES NOT REQUIRE A WD.  If repo-name is omitted,
 * it will try to get it from the WD, if it exists.
 * 
 */
void SG_vv2__comment(SG_context * pCtx,
					 const char * pszRepoName,
					 SG_rev_spec * pRevSpec,
					 const char * pszUserName,
					 const char * pszWhen,
					 const char * pszMessage,
					 SG_ut__prompt_for_comment__callback * pfnPrompt);

//////////////////////////////////////////////////////////////////

/**
 * Do a FULL HISTORICAL STATUS between 2 CSETS.
 *
 * This DOES NOT REQUIRE A WD.  If repo-name is omitted,
 * it will try to get it from the WD, if it exists.
 *
 */
void SG_vv2__status(SG_context * pCtx,
					const char * pszRepoName,
					const SG_rev_spec * pRevSpec,
					const SG_stringarray * psaInputs,
					SG_uint32 depth,
					SG_bool bNoSort,
					SG_varray ** ppvaStatus,
					SG_vhash ** ppvhLegend);


void SG_vv2__status__repo(SG_context * pCtx,
						  SG_repo * pRepo,
						  SG_rbtree * prbTreenodeCache,
						  const char * pszHid_0,
						  const char * pszHid_1,
						  char chDomain_0,
						  char chDomain_1,
						  const char * pszLabel_0,
						  const char * pszLabel_1,
						  const char * pszWasLabel_0,
						  const char * pszWasLabel_1,
						  SG_bool bNoSort,
						  SG_varray ** ppvaStatus,
						  SG_vhash ** ppvhLegend);

//////////////////////////////////////////////////////////////////

void SG_vv2__mstatus(SG_context * pCtx,
					 const char * pszRepoName,
					 const SG_rev_spec * pRevSpec,
					 SG_bool bNoFallback,
					 SG_bool bNoSort,
					 SG_varray ** ppvaStatus,
					 SG_vhash ** ppvhLegend);

//////////////////////////////////////////////////////////////////

void SG_vv2__diff_to_stream(SG_context * pCtx,
							const char * pszRepoName,
							const SG_rev_spec * pRevSpec,
							const SG_stringarray * psaInputs,
							SG_uint32 depth,
							SG_bool bNoSort,
							SG_bool bInteractive,
							const char * pszTool,
							SG_vhash ** ppvhResultCodes);

void SG_vv2__diff_to_stream__throw(SG_context * pCtx,
								   const char * pszRepoName,
								   const SG_rev_spec * pRevSpec,
								   const SG_stringarray * psaInputs,
								   SG_uint32 depth,
								   SG_bool bNoSort,
								   SG_bool bInteractive,
								   const char * pszTool);

//////////////////////////////////////////////////////////////////

void SG_vv2__compute_file_hid(SG_context * pCtx,
							  const char * pszRepoName,
							  const SG_pathname * pPathFile,
							  char ** ppszHid);

void SG_vv2__compute_file_hid__repo(SG_context * pCtx,
									SG_repo * pRepo,
									const SG_pathname * pPathFile,
									char ** ppszHid);

//////////////////////////////////////////////////////////////////

void SG_vv2__stamp__add(SG_context * pCtx,
						const char * pszRepoName,
						const SG_rev_spec * pRevSpec,
						const char * pszStampName,
						SG_varray ** ppvaInfo);

void SG_vv2__stamp__remove(SG_context * pCtx,
						   const char * pszRepoName,
						   const SG_rev_spec * pRevSpec,
						   const char * pszStampName,
						   SG_bool bAll,
						   SG_varray ** ppvaInfo);

void SG_vv2__stamp__remove_stamp_from_cset(SG_context * pCtx,
										   const char * pszRepoName,
										   const SG_rev_spec * pRevSpec,
										   const char * pszStampName,
										   SG_varray ** ppvaInfo);

void SG_vv2__stamp__remove_stamp_from_all_csets(SG_context * pCtx,
												const char * pszRepoName,
												const char * pszStampName,
												SG_varray ** ppvaInfo);

void SG_vv2__stamp__remove_all_stamps_from_cset(SG_context * pCtx,
												const char * pszRepoName,
												const SG_rev_spec * pRevSpec,
												SG_varray ** ppvaInfo);

void SG_vv2__stamp__list(SG_context * pCtx,
						 const char * pszRepoName,
						 const char * pszStampName,
						 SG_stringarray ** ppsaHidChangesets);

void SG_vv2__stamps(SG_context * pCtx,
					const char * pszRepoName,
					SG_varray ** ppva_results);

//////////////////////////////////////////////////////////////////

void SG_vv2__tag__add(SG_context * pCtx,
					  const char * pszRepoName,
					  const SG_rev_spec * pRevSpec,
					  const char * pszTagName,
					  SG_bool bForceMove);

void SG_vv2__tag__move(SG_context * pCtx,
					   const char * pszRepoName,
					   const SG_rev_spec * pRevSpec,
					   const char * pszTagName);

void SG_vv2__tag__remove(SG_context * pCtx,
						 const char * pszRepoName,
						 const char * pszTagName);

void SG_vv2__tags(SG_context * pCtx,
				  const char * pszRepoName,
				  SG_bool bGetRevno,
				  SG_varray ** ppva_results);

//////////////////////////////////////////////////////////////////

/**
 * Set the whoami for the given repository.  If no repository is passed, one is inferred from the current working directory.
 * If the pszUsername is set, the whoami will be set to that username.  Use the bCreate flag to create a new user.  If bCreate
 * is supplied, pszUsername can not be NULL.
 *
 * The value of ppszNewestUsername will be set to the value of whoami for that repository.  If
 * pszUsername is supplied, and setting the whoami succeeded, the same contents will be returned 
 * in ppszNewestUsername.
 */
void SG_vv2__whoami(SG_context * pCtx,
				  const char * pszRepoName,
				  const char * pszUsername,
				  SG_bool bCreate,
				  char ** ppszNewestUsername);

//////////////////////////////////////////////////////////////////

void SG_vv2__leaves(SG_context * pCtx,
					const char * pszRepoName,
					SG_uint64 ui64DagNum,
					SG_varray ** ppvaHidLeaves);

//////////////////////////////////////////////////////////////////

void SG_vv2__history(SG_context * pCtx,
					 const SG_vv2_history_args * pHistoryArgs,
					 SG_bool* pbHasResult,
					 SG_vhash** ppvhBranchPile,
					 SG_history_result ** ppHistoryResult);

//////////////////////////////////////////////////////////////////

void SG_vv2__locks(SG_context * pCtx,
				   const char * pszRepoName,
				   const char * pszBranchName,
				   const char * pszServerName,
				   const char * pszUserName,
				   const char * pszPassword,
				   SG_bool bPull,
				   SG_bool bVerbose,
				   char ** ppsz_tied_branch_name,
				   SG_vhash** ppvh_locks_by_branch,
				   SG_vhash** ppvh_violated_locks,
				   SG_vhash** ppvh_duplicate_locks);

//////////////////////////////////////////////////////////////////

void SG_vv2__find_cset(SG_context * pCtx,
					   const char * pszRepoName,
					   const SG_rev_spec * pRevSpec,
					   char ** ppszHidCSet,
					   char ** ppsz_branch_name);

void SG_vv2__check_attach_name(SG_context * pCtx,
							   const char * pszRepoName,
							   const char * pszCandidateBranchName,
							   SG_bool bMustExist,
							   char ** ppszNormalizedBranchName);

//////////////////////////////////////////////////////////////////

void SG_vv2__cat(SG_context * pCtx,
				 const char * pszRepoName,
				 const SG_rev_spec * pRevSpec,
				 const SG_stringarray * psaInput,
				 SG_varray ** ppvaResults,
				 SG_repo ** ppRepo);

void SG_vv2__cat__to_console(SG_context * pCtx,
							 const char * pszRepoName,
							 const SG_rev_spec * pRevSpec,
							 const SG_stringarray * psaInput);

void SG_vv2__cat__to_pathname(SG_context * pCtx,
							  const char * pszRepoName,
							  const SG_rev_spec * pRevSpec,
							  const SG_stringarray * psaInput,
							  const SG_pathname * pPathOutput);

//////////////////////////////////////////////////////////////////

void SG_vv2__line_history(SG_context * pCtx,
				 const char * pszRepoName,
				 const SG_rev_spec * pRevSpec,
				 const char * pszInput,
				 SG_int32 nStartLine,
				 SG_int32 nLength,
				 SG_varray ** ppvaResults);

void SG_vv2__line_history__repo(SG_context * pCtx,
				 SG_repo * pRepo,
				 const SG_rev_spec * pRevSpec,
				 const char * pszInput,
				 SG_int32 nStartLine,
				 SG_int32 nLength,
				 SG_varray ** ppvaResults);
END_EXTERN_C;

#endif//H_SG_VV2__API__PUBLIC_PROTOTYPES_H
