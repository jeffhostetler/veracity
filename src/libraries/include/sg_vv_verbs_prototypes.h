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
 * @file sg_vv_verbs_prototypes.h
 *
 * @details Routines to (eventually) perform most major vv commands.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_VV_VERBS_PROTOTYPES_H
#define H_SG_VV_VERBS_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////


/**
 * Wrapper for pull.  This does not include any option 
 * to trigger an update after the pull.  You must manually
 * call update yourself.  
 */
void SG_vv_verbs__pull(SG_context * pCtx,
					   const char* pszSrcRepoSpec,				// [in] The repo to pull from.  This may be a local repo name, or a URL of the remote server (http://SERVER/repos/REPONAME).
					   const char* pszUsername,                 // [in] (Optional) Username to use to authorize the pull.
					   const char* pszPassword,                 // [in] (Optional) Password to use to authorize the pull.
					   const char * pszDestRepoDescriptorName,  // [in] The local repo descriptor name.
					   SG_vhash * pvh_area_names,				// [in] The list of dag areas to pull.  It is an error to specify multiple entries in pva_area_names and any criteria in ppRevSpec.
					   SG_rev_spec** ppRevSpec,				    // [in] The nodes to pull. On succes, ownership is taken.
																//		The remote repo will look them up: the state of these branches in the destination repo is irrelevant.
					   SG_vhash** ppvhAccountInfo,			    // [out] (Optional) Contains account info when pulling from a hosted account.
					   SG_vhash** ppvhStats);					// [out] Optional stats about the work done by pull.
/**
 * Wrapper for push.
 */
void SG_vv_verbs__push(SG_context * pCtx,
						   const char * pszDestRepoSpec,			// [in]  The repo to push to.  This may be a local repo name, or a URL of the remote server (http://SERVER/repos/REPONAME).
						   const char* pszUsername,                 // [in] (Optional) Username to use to authorize the push.
						   const char* pszPassword,                 // [in] (Optional) Password to use to authorize the push.
						   const char* pszSrcRepoDescriptorName,	// [in]  The local repo descriptor name.
						   SG_vhash * pvh_area_names,				// [in]  The list of dags to push.  It is an error to specify multiple areas and either of prbTags or prbDagnodePrefixes.
						   SG_rev_spec* pRevSpec,
						   SG_bool bForce,             				// [in]  If true, do not perform the branch/lock pull-and-check before doing the push
						   SG_vhash** ppvhAccountInfo,			    // [out] (Optional) Contains account info when pushing to a hosted account.
						   SG_vhash** ppvhStats);					// [out] Optional stats about the work done by push.
/**
 * Wrapper for incoming.
 */
void SG_vv_verbs__incoming(SG_context * pCtx,
						   const char* pszSrcRepoSpec,					// [in]  The repo to pull from.  This may be a local repo name, or a URL of the remote server (http://SERVER/repos/REPONAME).
						   const char* pszUsername,						// [in] (Optional) Username to use to authorize the sync url requests.
						   const char* pszPassword,						// [in] (Optional) Password to use to authorize the sync url requests.
						   const char * pszDestRepoDescriptorName,		// [in]  The local repo descriptor name.
						   SG_history_result** pp_incomingChangesets,	// [out] The changesets that need to be transferred from src to dest.
						   SG_vhash** ppvhStats);						// [out] Optional stats about the work done by incoming.

void SG_vv_verbs__outgoing(SG_context* pCtx,
							const char* pszDestRepoSpec,				// [in]  The repo to push to.  This may be a local repo name, or a URL of the remote server (http://SERVER/repos/REPONAME).
							const char* pszUsername,					// [in] (Optional) Username to use to authorize the sync url requests.
							const char* pszPassword,					// [in] (Optional) Password to use to authorize the sync url requests.
							const char* pszSrcRepoDescriptorName,		// [in]  The local repo descriptor name.
							SG_history_result** pp_OutgoingChangesets,  // [out] The changesets that need to be transferred from src to dest.
							SG_vhash** ppvhStats);						// [out] Optional stats about the work done by outgoing.

/**
 *
 *
 */
void SG_vv_verbs__revert(SG_context * pCtx,
						 const char * pszFolder,
						 SG_bool bRecursive,
						 SG_bool bNoSubmodules,
						 SG_bool bVerbose,
						 SG_bool bTest,
						 SG_bool bNoBackups,
						 const SG_vector * pvec_use_submodules,
						 const SG_file_spec * pFilespec,
						 SG_uint32 count_items, const char ** paszItems,
						 SG_vhash ** ppvhResult);


void SG_vv_verbs__heads__repo(SG_context * pCtx, 
        SG_repo * pRepo,
		const SG_stringarray* psaRequestedBranchNames,
		SG_bool bAll,
		SG_rbtree ** pprbLeafHidsToOutput,
		SG_rbtree ** pprbNonLeafHidsToOutput);

void SG_vv_verbs__heads(SG_context * pCtx, 
        const char* psz_repo,
		const SG_stringarray* psaRequestedBranchNames,
		SG_bool bAll,
		SG_rbtree ** prbLeafHidsToOutput,
		SG_rbtree ** prbNonLeafHidsToOutput);


void SG_vv_verbs__upgrade_repos(SG_context* pCtx,
        SG_vhash* pvh_old_repos);

void SG_vv_verbs__upgrade__find_old_repos( SG_context* pCtx,
        SG_vhash** ppvh);


void SG_vv_verbs__work_items__text_search(SG_context * pCtx, 
		const char * pszRepoDescriptor, 
		const char * pszSearchString, 
		SG_varray ** ppva_results);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VV_VERBS_PROTOTYPES_H
