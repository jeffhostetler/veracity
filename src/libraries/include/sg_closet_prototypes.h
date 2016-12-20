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
 * @file sg_closet_prototypes.h
 *
 * @details The Closet is a private place where Veracity can store
 * information that is specific to a single user.
 *
 * In the initial design, the Closet is a directory within the user's
 * home directory, and any database-oriented stuff in the closet is
 * intended to be in sqlite db files within that directory.
 *
 * But the Closet API is designed to hide the details of how/where the
 * Closet is stored.  This helps the notion of the Closet stay more
 * cross-platform, as the Closet will appear in a different place on a
 * Windows system than it does on a Linux system.
 *
 * In the future, we may decide to implement the Closet differently,
 * such as in a PostgreSQL database.  Hopefully this API will hide the
 * details.
 *
 * The Closet is the place to store the following things:
 *
 * Repo Instance Descriptors
 *
 * REPO instances
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_CLOSET_PROTOTYPES_H
#define H_SG_CLOSET_PROTOTYPES_H

BEGIN_EXTERN_C;

/**
 * The closet can contain repo instances.  Use this function to reserve a 
 * name and obtain the partial repo instance descriptor necessary to create
 * a repo instance in the closet.
 * 
 * Note that this maintains a lock on the descriptors database until commit or abort
 * is called. You shouldn't do something that could take a while (e.g. clone) in between.
 */
void SG_closet__descriptors__add_begin(
	SG_context * pCtx,
	const char* pszName,
	const char * pszStorage,	//< [in] override default from "new_repo/driver"
	const char * pszRepoId,		//< [in] optional repo-id of the repo
	const char * pszAdminId,	//< [in] optional admin-id of the repo
	const char** ppszValidatedName,
	SG_vhash** ppvhPartialDescriptor,
	SG_closet_descriptor_handle** ppHandle
	);

void SG_closet__descriptors__add_commit(
	SG_context* pCtx,
	SG_closet_descriptor_handle** ppHandle, //< [in] Required. Routine will free and null caller's copy ON SUCCESS OR FAILURE.
	const SG_vhash* pvhFullDescriptor,
	SG_closet__repo_status status
	);

void SG_closet__descriptors__add_abort(
	SG_context* pCtx,
	SG_closet_descriptor_handle** ppHandle
	);

// like add_begin, except the repo must already exist, and will be replaced with an empty repo
void SG_closet__descriptors__replace_begin(
	SG_context * pCtx,
	const char* pszName,
	const char * pszStorage,	//< [in] override default from "new_repo/driver"
	const char * pszRepoId,		//< [in] optional repo-id of the repo
	const char * pszAdminId,	//< [in] optional admin-id of the repo
	const char** ppszValidatedName,
	SG_vhash** ppvhPartialDescriptor,
	SG_closet_descriptor_handle** ppHandle
	);

void SG_closet__descriptors__replace_commit(
	SG_context* pCtx,
	SG_closet_descriptor_handle** ppHandle, //< [in] Required. Routine will free and null caller's copy ON SUCCESS OR FAILURE.
	const SG_vhash* pvhFullDescriptor,
	SG_closet__repo_status status);

void SG_closet__descriptors__replace_abort(
	SG_context* pCtx,
	SG_closet_descriptor_handle** ppHandle
	);

/**
 * Change the named repo instance's status to new_status.
 */
void SG_closet__descriptors__set_status(
	SG_context * pCtx,
	const char* pszName,
	SG_closet__repo_status new_status
	);

/**
 * Get an existing repo instance descriptor by name.
 * Fetching a name that does not exist is an error.
 * Fetching a name with an unavailable status is an error.
 */
void SG_closet__descriptors__get(
	SG_context * pCtx,
	const char* pszName,
	char** ppszValidatedName,			/* < [out] Optional. Caller must free. */
	SG_vhash** ppvhDescriptor			/* < [out] Required. Caller must free. */
	);

/**
 * Get existing repo instance descriptor info by name, even if unavailable.
 * Fetching a name that does not exist is an error.
 */
void SG_closet__descriptors__get__unavailable(
	SG_context * pCtx, 
	const char* pszName,
	char** ppszValidatedName,			/* < [out] Optional. Caller must free. */
	SG_bool* pbAvailable,				/* < [out] Optional. */
	SG_closet__repo_status* pStatus,	/* < [out] Optional. */
	char** ppszStatus,					/* < [out] Optional. Caller must free. */
	SG_vhash** ppvhDescriptor			/* < [out] Required. Caller must free. */
	);

/**
 * Determine if a descriptor by the provided name exists.
 */
void SG_closet__descriptors__exists__status(SG_context* pCtx, const char* pszName, SG_bool* pbExists, SG_closet__repo_status* piStatus);
void SG_closet__descriptors__exists(SG_context* pCtx, const char* pszName, SG_bool* pbExists);

/**
 * Delete an existing repo instance descriptor by name.  If the given
 * name does not exist, this function will return SG_ERR_OK and
 * effectively do nothing.
 */
void SG_closet__descriptors__remove(
	SG_context * pCtx,
	const char* pszName
	);

void SG_closet__descriptors__update(
	SG_context* pCtx,
	const char* psz_existing_name,
	const SG_vhash* pvhNewFullDescriptor,
	SG_closet__repo_status status);

/**
 * Change the name of an existing descriptor.
 * pszOld must exist and pszNew must not.
 */
void SG_closet__descriptors__rename(SG_context* pCtx, const char* pszOld, const char* pszNew);

/**
 * Retrieve the names and descriptors of all repositories with status SG_REPO_STATUS__NORMAL.
 * The list is returned as a vhash.  
 * The keys of the vhash are the names.  
 * The value for each key is another vhash containing that repo instance descriptor.
 */
void SG_closet__descriptors__list(
	SG_context * pCtx,
	SG_vhash** ppvhDescriptors /**< Caller must free this. */
	);

/**
 * Retrieve the names and descriptors of all repositories with the provided status.
 * The list is returned as a vhash.  
 * The keys of the vhash are the names.  
 * The value for each key is another vhash containing that repo instance descriptor.
 */
void SG_closet__descriptors__list__status(
	SG_context * pCtx, 
	SG_closet__repo_status status, 
	SG_vhash** ppResult
	);

/**
 * List repo instance descriptors and their statuses. This returns ALL repositories, not just available ones.
 * The list is returned as a vhash.  
 * The keys of the vhash are the names.  
 * The value for each key is another vhash of this format:
 * {
 *    status: <SG_closet__repo_status>,
 *    descriptor : <repo descriptor vhash>
 * }
 * containing that repo instance descriptor.
 */
void SG_closet__descriptors__list__all(
	SG_context * pCtx,
	SG_vhash** ppvhResult /**< Caller must free this */
	);

void SG_closet__get_descriptor_rowid(SG_context* pCtx, const char* pszDescriptorName, SG_int64* pId);
	
/**
 * returns the path to the server-state folder inside the closet.
 */
void SG_closet__get_server_state_path(
	SG_context * pCtx,
	SG_pathname ** ppPath
    );

/**
 * returns the path to the usermap folder inside the closet.
 */
void SG_closet__get_usermap_path(
	SG_context * pCtx,
	SG_pathname ** ppPath
	);

void SG_closet__read_remote_leaves(
	SG_context* pCtx,
    const char* psz_remote_spec,
    SG_vhash** ppvh_since
    );

void SG_closet__write_remote_leaves(
	SG_context* pCtx,
    const char* psz_remote_spec,
    SG_vhash* pvh_since
    );

void SG_closet__get_localsettings(
	SG_context * pCtx,
	SG_jsondb** ppJsondb
    );

void SG_closet__get_private_key_path(
	SG_context * pCtx,
    const char* psz_userid,
    SG_pathname** ppResult
    );

/**
 * Compute a reverse map for using the given repo-id to find all of the
 * named repo descriptors that are aliases for a repo with this id. 
 * 
 * Returns only repos of status SG_REPO_STATUS__NORMAL.
 * 
 * The keys in the top-most vhash are aliases and the values are sub-vhashes
 * with the contents of /instance/<name>/paths/
 *
 * The pvec_use_submodules contains a list of "suggested" repo-names that the
 * user gave via one or more "--use-sub" arguments.  These are *hints* only.
 * We'll use these and try to find a match before we do the full linear search.
 * 
 * The returned vhash should have this general form:
 * 
 * { "name1" : { "default" : "http://..." },
 *   "name2" : { "default" : "<binding1>",
 *               "<foo>"   : "<binding2>" },
 *   "name3" : null }
 *
 * This is a very expensive call.
 */
void SG_closet__reverse_map_repo_id_to_descriptor_names(SG_context * pCtx,
														const char * pszRepoId,
														const SG_vector * pvec_use_submodules,
														SG_vhash ** ppvhList);


/**
 * Creates a new closet property, but fails with SG_ERR_CLOSET_PROP_ALREADY_EXISTS if it exists.
 */
void SG_closet__property__add(SG_context* pCtx, const char* pszName, const char* pszValue);

/**
 * Set the value for a closet property. Will create the property if it doesn't exist.
 */
void SG_closet__property__set(SG_context* pCtx, const char* pszName, const char* pszValue);

/**
 * Get the value for a closet property. Fails with SG_ERR_NO_SUCH_CLOSET_PROP if it doesn't exist.
 */
void SG_closet__property__get(SG_context* pCtx, const char* pszName, char** ppszValue);

void SG_closet__get_closet_path(SG_context * pCtx,
								SG_pathname ** ppPath);

void SG_closet__get_size(
        SG_context* pCtx,
        SG_uint64* pi
        );

void SG_closet__backup(
        SG_context* pCtx,
        SG_pathname** pp
        );
	
void SG_closet__restore(
        SG_context* pCtx,
        SG_pathname* pPath,
        SG_pathname** ppPath_new_backup
        );
	
void SG_closet__property__exists(SG_context* pCtx, const char* pszName, SG_bool* pbExists);

#ifdef SG_IOS
void SG_closet__set_prefix(SG_context *pCtx, const char *prefix);
void SG_closet__get_prefix(SG_context *pCtx, const char **prefix);
#endif
	
END_EXTERN_C;

#endif //H_SG_CLOSET_PROTOTYPES_H
