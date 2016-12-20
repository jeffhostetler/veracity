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
 * @file sg_sync_prototypes.h
 * 
 * @details Common routines used by push, pull, and clone.
 */

#ifndef H_SG_SYNC_PROTOTYPES_H
#define H_SG_SYNC_PROTOTYPES_H

BEGIN_EXTERN_C

/**
 * A routine that should be used only for tests to compare repository DAGs.
 * It checks exhaustively, walking the DAG, so you can find out what the differences are in a debugger.
 * But that's not necessary to just determine DAG equality. So don't call this in production code. Tests only.
 */
void SG_sync_debug__compare_one_dag(
	SG_context* pCtx,
	SG_repo* pRepo1,
	SG_repo* pRepo2,
	SG_uint64 iDagNum,
	SG_bool* pbIdentical);

/**
 * A routine that should be used only for tests to compare repository DAGs.
 * It checks exhaustively, walking the DAG, so you can find out what the differences are in a debugger.
 * But that's not necessary to just determine DAG equality. So don't call this in production code. Tests only.
 */
void SG_sync_debug__compare_repo_dags(
	SG_context* pCtx,
	SG_repo* pRepo1,
	SG_repo* pRepo2,
	SG_bool* pbIdentical);

void SG_sync__compare_repo_blobs(SG_context* pCtx,
								 SG_repo* pRepo1,
								 SG_repo* pRepo2,
								 SG_bool* pbIdentical);

void SG_sync__build_best_guess_dagfrag(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint64 iDagNum,
	SG_rbtree* prbStartFromHids,
	SG_vhash* pvhConnectToHidsAndGens,
	SG_dagfrag** ppFrag);

void SG_sync__add_blobs_to_fragball(SG_context* pCtx, SG_fragball_writer* pfb, SG_vhash* pvh_missing_blobs);


/** Add a repo descriptor or URL to the localsettings array.  This is used by the verbs for both push and pull */
void SG_sync__remember_sync_target(SG_context* pCtx, const char * pszLocalRepoDescriptor, const char * pszSyncTarget);

/** Return the path to the temporary directory where push, pull, and clone should save temporary data. */
void SG_sync__make_temp_path(
	SG_context* pCtx, 
	const char* psz_name, 
	SG_pathname** ppPath); // Caller should free.

/**
 * Sync the user DAGs of all repos in the closet.
 * Intended to be used when the admin ids of all repos in the closet match, but if they don't this
 * will sync with all repos matching the admin id of pRepoSrc.
 */
void SG_sync__closet_user_dags(
	SG_context* pCtx, 
	SG_repo* pRepoSrc, /*				< [in]  Required. */
	const char* pszRefHidLeafUserDag, /*< [in]  Optional. The HID of the leaf of the user DAG. 
										        If known, provide it. Otherwise this routine looks 
										        it up in pRepoSrc. */
	SG_varray** ppvaSyncedUserList); /*	< [out] Optional. Caller should free. */

/* Get account info from a repo info vhash.
 * Mostly a utility routine for use by SG_push, SG_pull, and SG_clone.
 * If you're not one of those, you're probably looking for SG_sync__get_account_info__details. */
void SG_sync__get_account_info__from_repo_info(
	SG_context* pCtx, 
	const SG_vhash* pvhRepoInfo, // A repo info vhash acquired with SG_sync_client__get_repo_info.
	SG_vhash** ppvhAccountInfo);
	
/* Get account info details from an account info vhash.
 * This exists because every push/pull/clone client will need to examine this structure, and I'm 
 * (perhaps unnecessarily) paranoid about lots of varied code poking around inside a vhash. */
void SG_sync__get_account_info__details(
	SG_context* pCtx, 
	const SG_vhash* pvhAcctInfo, // An account info vhash acquired with a push/pull/clone routine.
	                             // Can be NULL, resulting in ppszMessage and ppszStatus being untouched.
	const char** ppszMessage, // returns pointer to the string inside pvhAcctInfo--valid only until that's freed
	const char** ppszStatus); // returns pointer to the string inside pvhAcctInfo--valid only until that's freed

END_EXTERN_C

#endif

