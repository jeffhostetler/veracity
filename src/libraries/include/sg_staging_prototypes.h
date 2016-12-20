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
 * @file sg_staging_prototypes.h
 */

#ifndef H_SG_STAGING_PROTOTYPES_H
#define H_SG_STAGING_PROTOTYPES_H

BEGIN_EXTERN_C

/*
 * A staging area is a directory which is used to accumulate the pieces of an
 * upcoming group of repo transactions.
 *
 * A staging area contains pending transactions for ONE repo.
 *
 * A staging area can have multiple transactions pending, one per dagnum.  For
 * each dagnum, there can be one dagfrag.
 *
 * A staging area can be used during either a push or pull operation. In either case,
 * the staging area is associated with the destination repository.
 *
 * A push is a network or local operation wherein the client sends fragballs to the
 * destination, which accumulates them into a staging area to be committed to the 
 * repo once everything is received.
 *
 * A pull is a network or local operation wherein the client requests fragballs from
 * the source repo and accumulates them into a staging area to be committed to the 
 * repo once everything is received.
 *
 * The layout and format of the contents of the staging area directory are
 * private to SG_staging.  Don't expect to read from this directory
 * directly.  And don't write anything to it.  Let SG_staging__* deal
 * with it.
 *
 * Except: The way to add a fragball to the staging area is to write
 * the fragball file into the staging area directory using a unique name
 * like a GID.  Then call slurp_fragball to tell the staging area to
 * take ownership of that fragball.
 */

/**
 * Create a new, empty staging area.
 */
void SG_staging__create(
	SG_context* pCtx,
	char** ppsz_tid_staging, // An ID that uniquely identifies the staging area
	SG_staging** ppStaging);

/**
 * Get a handle for an existing staging area.
 */
void SG_staging__open(SG_context* pCtx,
					  const char* psz_tid_staging,
					  SG_staging** ppStaging);

/**
 * Delete the staging area.
 */
void SG_staging__cleanup(SG_context* pCtx,
						 SG_staging** ppStaging);
/**
 * Delete the staging area identified by the provided pushid.
 */
void SG_staging__cleanup__by_id(SG_context* pCtx,
								const char* pszPushId);

/**
 * Free staging instance data.
 */
void SG_staging__free(SG_context* pCtx, SG_staging* pStaging);

/**
 * Get the staging area's disk path. Caller must free *ppPathname.
 */
void SG_staging__get_pathname(SG_context* pCtx, const char* psz_tid_staging, SG_pathname** ppPathname);

/**
 * Add the contents of a fragball into a staging area.
 */
void SG_staging__slurp_fragball(
	SG_context* pCtx,
	SG_staging* pStaging,
	const char* psz_filename
	);

/**
 * Return status information about a staging area. Generally speaking this status is intended
 * to inform the caller whether the staging area could be committed to the repository, and if not,
 * why?  There is additional status data provided not directly related to "commitability."
 * 
 * The status vhash follows this format:
 * 
 * {
 *		"dags" : // If bCheckConnectedness, this node will be present if there is at least one dag 
 *		         // that cannot be committed because it's dag is disconnected.
 *		{
 *			<dagnum> : // A disconnected DAG's dagnum.
 *			{
 *				<hid> : null, // The hid of a changeset missing from the staging area which would 
 *				              // prevent a commit from succeeding.
 *				...
 *			},
 *			...
 *		},
 *		"leaves" : // So that the other side of the push/pull can add additional generations to 
 *		           // achieve DAG connectivity, the leaves of disconnected DAGs (listed above in 
 *		           // "dags") and their generation are included in this node if bCheckConnectedness.
 *		{
 *			<dagnum> : // A disconnected DAG's dagnum.
 *			{
 *				<hid> : <generation>,
 *				...
 *			},
 *			...
 *		},
 *		"counts" : // If bGetCounts, information about the number of dagnodes and blobs is staging
 *		           // are given in this node. These lookups can be expensive, so this is intended
 *		           // primarily as debug, diagnostic info.
 *		{
 *			"dags" :
 *			{
 *				<dagnum> : <number of dagnodes in dagfrag, including fringe>,
 *				...
 *			}
 *			"blobs-referenced" : <the number of blobs we're aware of>
 *			"blobs-present" : <the number of blobs physically present in the staging area>
 *		}
 *		
 *		// TODO describe blobs status output
 *		// TODO describe new-nodes output, which is used by incoming/outgoing
 * }
 * 
 */
void SG_staging__check_status(
	SG_context* pCtx,
	SG_staging* pStaging,
	SG_repo* pRepo,
	SG_bool bCheckConnectedness,
	SG_bool bListNewNodes,
	SG_bool bCheckChangesetBlobs,
	SG_bool bCheckDataBlobs,
	SG_bool bGetCounts,			 /* Include the number of dagnodes and blobs in the returned status data. */
	SG_vhash** ppResult
	);

/**
 * This method takes everything in the staging
 * area and inserts it into the given repo, one
 * tx per dagnum.  The staging area itself is
 * untouched; nothing there is removed.
 */
void SG_staging__commit(
	SG_context* pCtx,
	SG_staging* pStaging,
	SG_repo* pRepo
	);

void SG_staging__fetch_blob_into_memory(SG_context* pCtx,
										SG_staging_blob_handle* pStagingBlobHandle,
										SG_byte ** ppBuf,
										SG_uint32 * pLenFull);

/**
 * Create a temporary staging area for a clone. 
 * This type of staging area is suitable only for clones and is therefore lighter weight:
 * it has no database for keeping track of dag fragments and blobs.
 * 
 * This routine will verify that the provided descriptor name is valid and unused.
 */
void SG_staging__clone__create(
	SG_context* pCtx,
	const char* psz_new_descriptor_name,
	const SG_vhash* pvhRepoInfo,
	const char** ppszCloneId);

void SG_staging__clone__commit(
	SG_context* pCtx,
	const char* pszCloneId,
	SG_pathname** ppPathFragball); // [in] Required. On success, this routine takes ownership of the pointer. It will free memory and delete the file.

/**
 * Commit a clone that needs user mapping.
 */
void SG_staging__clone__commit_maybe_usermap(
	SG_context* pCtx,
	const char* pszCloneId,
	const char* pszClosetAdminId,
	SG_pathname** ppPathFragball,	/*< [in] Required. On success, this routine takes ownership of the pointer. It will free memory and delete the file. */
	char** ppszDescriptorName,		/*< [out] Optional. Caller should free. */
	SG_bool* pbAvailable,			/*< [out] Optional. */
	SG_closet__repo_status* pStatus,/*< [out] Optional. */
	char** ppszStatus				/*< [out] Optional. Caller should free. */
	);

/**
 * Abort a clone. Cleans up the repo, descriptor, and staging area.
 * Intended to be called after SG_staging__clone__create but before on of the SG_staging__clone__commits.
 */
void SG_staging__clone__abort(
	SG_context* pCtx,
	const char* pszCloneId);

/**
 * Returns the repo info vhash that was provided in SG_staging__clone__create.
 */
void SG_staging__clone__get_info(
	SG_context* pCtx,
	const char* pszCloneId,
	SG_vhash** ppvhRepoInfo);

END_EXTERN_C

#endif
