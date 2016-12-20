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

// TODO This file is intended to have common functions that are *above* the
// TODO REPO API line.  For example, computing a hash on a buffer using the
// TODO current REPO will make multiple calls to the REPO API.
// TODO
// TODO We should probably formalize this (and make sure we don't have any
// TODO below-the-line stuff in here).

#include <sg.h>

#include "sg_repo__private.h"
#include "sg_zing__private.h"

void SG_repo__validate_repo_name(
	SG_context* pCtx,
	const char* psz_new_descriptor_name,
	char** ppszNormalizedDescriptorName
	)
{
	SG_NULLARGCHECK(ppszNormalizedDescriptorName);

	SG_ERR_CHECK(  SG_validate__ensure__trim(pCtx, psz_new_descriptor_name, 1u, 256u, SG_VALIDATE__BANNED_CHARS, SG_TRUE, SG_ERR_INVALID_REPO_NAME, "repository name", ppszNormalizedDescriptorName)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void SG_repo__fetch_vhash__utf8_fix(SG_context* pCtx,
						  SG_repo * pRepo,
						  const char* pszidHidBlob,
						  SG_vhash ** ppVhashReturned)
{
	// fetch contents of a blob and convert to a vhash object.

    SG_blob* pblob = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(pszidHidBlob);
	SG_NULLARGCHECK_RETURN(ppVhashReturned);

	*ppVhashReturned = NULL;

	SG_ERR_CHECK(  SG_repo__get_blob(pCtx,pRepo,pszidHidBlob,&pblob)  );
	SG_ERR_CHECK(  SG_vhash__alloc__from_json__buflen__utf8_fix(pCtx, ppVhashReturned, (const char *)pblob->data, (SG_uint32) pblob->length)  );
	SG_ERR_CHECK(  SG_repo__release_blob(pCtx, pRepo, &pblob)  );

	return;

fail:
	SG_ERR_IGNORE(  SG_repo__release_blob(pCtx, pRepo, &pblob)  );
}

void SG_repo__fetch_vhash(SG_context* pCtx,
						  SG_repo * pRepo,
						  const char* pszidHidBlob,
						  SG_vhash ** ppVhashReturned)
{
	// fetch contents of a blob and convert to a vhash object.

    SG_blob* pblob = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(pszidHidBlob);
	SG_NULLARGCHECK_RETURN(ppVhashReturned);

	*ppVhashReturned = NULL;

	SG_ERR_CHECK(  SG_repo__get_blob(pCtx,pRepo,pszidHidBlob,&pblob)  );
	SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__BUFLEN(pCtx, ppVhashReturned, (const char *)pblob->data, (SG_uint32) pblob->length)  );
	SG_ERR_CHECK(  SG_repo__release_blob(pCtx, pRepo, &pblob)  );

	return;

fail:
	SG_ERR_IGNORE(  SG_repo__release_blob(pCtx, pRepo, &pblob)  );
}

#ifndef SG_IOS
void SG_repo__create_user_root_directory(
	SG_context* pCtx,
	SG_repo* pRepo,
	const char* pszName,
	SG_changeset** ppChangeset,
	char** ppszidGidActualRoot
	)
{
	SG_committing* ptx = NULL;
	SG_changeset * pChangesetOriginal = NULL;
	SG_dagnode * pDagnodeOriginal = NULL;
	SG_treenode* pTreenodeActualRoot = NULL;
	SG_treenode* pTreenodeSuperRoot = NULL;
	char* pszidHidActualRoot = NULL;
	char* pszidHidSuperRoot = NULL;
	SG_treenode_entry* pEntry = NULL;
    const char* psz_hid_cs = NULL;
    SG_audit q;

	SG_ERR_CHECK(  SG_audit__init__maybe_nobody(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW, SG_AUDIT__WHO__FROM_SETTINGS)  );
	SG_ERR_CHECK(  SG_committing__alloc(pCtx, &ptx, pRepo, SG_DAGNUM__VERSION_CONTROL, &q, SG_CSET_VERSION__CURRENT)  );
    /* we don't add a parent here */

	/* First add an empty tree node to be the actual root directory */
	SG_ERR_CHECK(  SG_treenode__alloc(pCtx, &pTreenodeActualRoot)  );
	SG_ERR_CHECK(  SG_treenode__set_version(pCtx, pTreenodeActualRoot, SG_TN_VERSION__CURRENT)  );
	SG_ERR_CHECK(  SG_committing__tree__add_treenode(pCtx, ptx, &pTreenodeActualRoot, &pszidHidActualRoot)  );
	SG_TREENODE_NULLFREE(pCtx, pTreenodeActualRoot);
	pTreenodeActualRoot = NULL;

	/* Now create the super root with the actual root as its only entry */
	SG_ERR_CHECK(  SG_treenode__alloc(pCtx, &pTreenodeSuperRoot)  );
	SG_ERR_CHECK(  SG_treenode__set_version(pCtx, pTreenodeSuperRoot, SG_TN_VERSION__CURRENT)  );

	SG_ERR_CHECK(  SG_TREENODE_ENTRY__ALLOC(pCtx, &pEntry)  );
	SG_ERR_CHECK(  SG_treenode_entry__set_entry_name(pCtx, pEntry, pszName)  );
	SG_ERR_CHECK(  SG_treenode_entry__set_entry_type(pCtx, pEntry, SG_TREENODEENTRY_TYPE_DIRECTORY)  );
	SG_ERR_CHECK(  SG_treenode_entry__set_hid_blob(pCtx, pEntry, pszidHidActualRoot)  );
	SG_ERR_CHECK(  SG_treenode_entry__set_attribute_bits(pCtx, pEntry, 0)  );

	SG_ERR_CHECK(  SG_gid__alloc(pCtx, ppszidGidActualRoot)  );

	SG_ERR_CHECK(  SG_treenode__add_entry(pCtx, pTreenodeSuperRoot, *ppszidGidActualRoot, &pEntry)  );

	SG_ERR_CHECK(  SG_committing__tree__add_treenode(pCtx, ptx, &pTreenodeSuperRoot, &pszidHidSuperRoot)  );
	SG_TREENODE_NULLFREE(pCtx, pTreenodeSuperRoot);
	pTreenodeSuperRoot = NULL;

	SG_ERR_CHECK(  SG_committing__tree__set_root(pCtx, ptx, pszidHidSuperRoot)  );

	SG_ERR_CHECK(  SG_committing__end(pCtx, ptx, &pChangesetOriginal, &pDagnodeOriginal)  );
    ptx = NULL;

    SG_ERR_CHECK(  SG_dagnode__get_id_ref(pCtx, pDagnodeOriginal, &psz_hid_cs)  );

	SG_DAGNODE_NULLFREE(pCtx, pDagnodeOriginal);

	SG_NULLFREE(pCtx, pszidHidActualRoot);
	SG_NULLFREE(pCtx, pszidHidSuperRoot);

	*ppChangeset = pChangesetOriginal;

	return;

fail:
	/* TODO free other stuff */

	SG_ERR_IGNORE(  SG_committing__abort(pCtx, ptx)  );
    SG_TREENODE_ENTRY_NULLFREE(pCtx, pEntry);
}
#endif // !SG_IOS

static void _create_empty_repo__replace(SG_context* pCtx,
							   const char* psz_storage,			// use config: new_repo/driver if null
							   const char* psz_hash_method,		// use config: new_repo/hash_method if null
							   const char* psz_repo_id,
							   const char* psz_admin_id,
							   const char* psz_new_descriptor_name,
							   SG_closet__repo_status status,
							   SG_bool b_indexes,
                               SG_repo** ppRepo)
{
	SG_closet_descriptor_handle* ph = NULL;
	const char* pszRefValidatedName = NULL;
	SG_vhash* pvhDescriptorPartial = NULL;
	const SG_vhash* pvhRefFullDescriptor = NULL;
	SG_repo* pNewRepo = NULL;

	SG_ERR_CHECK_RETURN(  SG_gid__argcheck(pCtx, psz_repo_id)  );
	SG_ERR_CHECK_RETURN(  SG_gid__argcheck(pCtx, psz_admin_id)  );

	/* Now construct a new repo with the same ID. */
	SG_ERR_CHECK(  SG_closet__descriptors__replace_begin(pCtx, 
		psz_new_descriptor_name, 
		psz_storage, 
		psz_repo_id, 
		psz_admin_id, 
		&pszRefValidatedName, &pvhDescriptorPartial, &ph)  );
	SG_ERR_CHECK(  SG_repo__create_repo_instance(pCtx,
		pszRefValidatedName,
		pvhDescriptorPartial,
		b_indexes,
		psz_hash_method,
		psz_repo_id,
		psz_admin_id,
		&pNewRepo)  );

	/* And save the descriptor for that new repo */
	SG_ERR_CHECK(  SG_repo__get_descriptor(pCtx, pNewRepo, &pvhRefFullDescriptor)  );
	SG_ERR_CHECK(  SG_closet__descriptors__replace_commit(pCtx, &ph, pvhRefFullDescriptor, status)  );

    if (ppRepo)
    {
        *ppRepo = pNewRepo;
        pNewRepo = NULL;
    }

	/* fall through */
fail:
	SG_VHASH_NULLFREE(pCtx, pvhDescriptorPartial);
	SG_REPO_NULLFREE(pCtx, pNewRepo);
	SG_ERR_IGNORE(  SG_closet__descriptors__replace_abort(pCtx, &ph)  );
}

static void _create_empty_repo(SG_context* pCtx,
							   const char* psz_storage,			// use config: new_repo/driver if null
							   const char* psz_hash_method,		// use config: new_repo/hash_method if null
							   const char* psz_repo_id,
							   const char* psz_admin_id,
							   const char* psz_new_descriptor_name,
							   SG_closet__repo_status status,
							   SG_bool b_indexes,
                               SG_repo** ppRepo)
{
	SG_closet_descriptor_handle* ph = NULL;
	const char* pszRefValidatedName = NULL;
	SG_vhash* pvhDescriptorPartial = NULL;
	const SG_vhash* pvhRefFullDescriptor = NULL;
	SG_repo* pNewRepo = NULL;

	SG_ERR_CHECK_RETURN(  SG_gid__argcheck(pCtx, psz_repo_id)  );
	SG_ERR_CHECK_RETURN(  SG_gid__argcheck(pCtx, psz_admin_id)  );

	/* Now construct a new repo with the same ID. */
	SG_ERR_CHECK(  SG_closet__descriptors__add_begin(pCtx, 
		psz_new_descriptor_name, 
		psz_storage, 
		psz_repo_id, 
		psz_admin_id, 
		&pszRefValidatedName, &pvhDescriptorPartial, &ph)  );
	SG_ERR_CHECK(  SG_repo__create_repo_instance(pCtx,
		pszRefValidatedName,
		pvhDescriptorPartial,
		b_indexes,
		psz_hash_method,
		psz_repo_id,
		psz_admin_id,
		&pNewRepo)  );

	/* And save the descriptor for that new repo */
	SG_ERR_CHECK(  SG_repo__get_descriptor(pCtx, pNewRepo, &pvhRefFullDescriptor)  );
	SG_ERR_CHECK(  SG_closet__descriptors__add_commit(pCtx, &ph, pvhRefFullDescriptor, status)  );

    if (ppRepo)
    {
        *ppRepo = pNewRepo;
        pNewRepo = NULL;
    }

	/* fall through */
fail:
	SG_VHASH_NULLFREE(pCtx, pvhDescriptorPartial);
	SG_REPO_NULLFREE(pCtx, pNewRepo);
	SG_ERR_IGNORE(  SG_closet__descriptors__add_abort(pCtx, &ph)  );
}

void SG_repo__replace_empty(
	SG_context* pCtx,
    const char* psz_repo_id,
    const char* psz_admin_id,
    const char* psz_hash_method,
	const char* psz_new_descriptor_name,
	SG_closet__repo_status status,
    SG_repo** ppRepo
	)
{
	SG_ERR_CHECK(  _create_empty_repo__replace(pCtx, NULL, psz_hash_method, psz_repo_id, psz_admin_id, 
		psz_new_descriptor_name, status, SG_TRUE, ppRepo)  );

fail:
    ;
}

void SG_repo__create_empty(
	SG_context* pCtx,
    const char* psz_repo_id,
    const char* psz_admin_id,
    const char* psz_hash_method,
	const char* psz_new_descriptor_name,
	SG_closet__repo_status status,
    SG_repo** ppRepo
	)
{
	SG_ERR_CHECK(  _create_empty_repo(pCtx, NULL, psz_hash_method, psz_repo_id, psz_admin_id, 
		psz_new_descriptor_name, status, SG_TRUE, ppRepo)  );

fail:
    ;
}

void SG_repo__create__completely_new__empty__closet(
	SG_context* pCtx,
    const char* psz_admin_id,
	const char* pszStorage,			// use config: new_repo/driver if null
    const char* psz_hash_method,	// use config: new_repo/hash_method if null
    const char* pszRidescName
    )
{
    char buf_repo_id[SG_GID_BUFFER_LENGTH];
	SG_repo* pRepo = NULL;

    SG_ERR_CHECK(  SG_gid__generate(pCtx, buf_repo_id, sizeof(buf_repo_id))  );
	SG_ERR_CHECK(  _create_empty_repo(pCtx, pszStorage, psz_hash_method, buf_repo_id, psz_admin_id, 
		pszRidescName, SG_REPO_STATUS__NORMAL, SG_TRUE, &pRepo)  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
}

#ifndef SG_IOS
void SG_repo__setup_basic_stuff(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_changeset** ppcs,
    char** ppsz_gid_root
    )
{
	SG_changeset* pcsFirst = NULL;
	char* pszidGidActualRoot = NULL;
    SG_audit q;
    const char* psz_csid_first = NULL;

    // ask zing to install default templates for all our builtin databases
    SG_ERR_CHECK(  sg_zing__init_new_repo(pCtx, pRepo)  );

    // now create the tree
    SG_ERR_CHECK(  SG_repo__create_user_root_directory(pCtx, pRepo, "@", &pcsFirst, &pszidGidActualRoot)  );

	SG_ERR_CHECK(  SG_audit__init__maybe_nobody(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW, SG_AUDIT__WHO__FROM_SETTINGS)  );
    SG_ERR_CHECK(  SG_changeset__get_id_ref(pCtx, pcsFirst, &psz_csid_first)  );

    SG_ERR_CHECK(  SG_vc_branches__add_head(pCtx, pRepo, SG_VC_BRANCHES__DEFAULT, psz_csid_first, NULL, SG_FALSE, &q)  );

    // add areas
    SG_ERR_CHECK(  SG_area__add(pCtx, pRepo, SG_AREA_NAME__CORE, SG_DAGNUM__GET_AREA_ID(SG_DAGNUM__USERS), &q)  );
    SG_ERR_CHECK(  SG_area__add(pCtx, pRepo, SG_AREA_NAME__VERSION_CONTROL, SG_DAGNUM__GET_AREA_ID(SG_DAGNUM__VERSION_CONTROL), &q)  );

#if 0
	SG_ERR_CHECK(  SG_area__add(pCtx, pRepo, "scrum", SG_DAGNUM__GET_AREA_ID(SG_DAGNUM__WORK_ITEMS), &q)  );
#endif

    if (ppcs)
    {
        *ppcs = pcsFirst;
        pcsFirst = NULL;
    }
    if (ppsz_gid_root)
    {
        *ppsz_gid_root = pszidGidActualRoot;
        pszidGidActualRoot = NULL;
    }

    /* fall */

fail:
    SG_CHANGESET_NULLFREE(pCtx, pcsFirst);
    SG_NULLFREE(pCtx, pszidGidActualRoot);
}
#endif // !SG_IOS

void SG_repo__hidlookup__dagnode(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint64 iDagNum,
	const char* psz_hid_prefix,
	char** ppsz_hid
	)
{
    const char* psz_hid = NULL;
	char* psz_my_hid = NULL;
    SG_rbtree* prb = NULL;
	SG_uint32 iRevId = 0;
    SG_uint32 count = 0;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(psz_hid_prefix);
	SG_NULLARGCHECK_RETURN(ppsz_hid);

	SG_uint32__parse__strict(pCtx, &iRevId, psz_hid_prefix);
	SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_INT_PARSE);

	if (iRevId)
	{
		SG_repo__find_dagnode_by_rev_id(pCtx, pRepo, iDagNum, iRevId, &psz_my_hid);
		SG_ERR_CHECK_CURRENT_DISREGARD(SG_ERR_NOT_FOUND);
	}

	if (psz_my_hid)
	{
		SG_RETURN_AND_NULL(psz_my_hid, ppsz_hid);
	}
	else
	{
		SG_ERR_CHECK(  SG_repo__find_dagnodes_by_prefix(pCtx, pRepo, iDagNum, psz_hid_prefix, &prb)  );

		SG_ERR_CHECK(  SG_rbtree__count(pCtx, prb, &count)  );

		if (1 == count)
		{
			SG_ERR_CHECK(  SG_rbtree__get_only_entry(pCtx, prb, &psz_hid, NULL)  );
			SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_hid, ppsz_hid)  );
		}
		else if (0 == count)
		{
			SG_ERR_THROW2(  SG_ERR_NO_CHANGESET_WITH_PREFIX,
							(pCtx, "%s", psz_hid_prefix)  );
		}
		else
		{
			SG_ERR_THROW2(  SG_ERR_AMBIGUOUS_ID_PREFIX,
							(pCtx, "%s", psz_hid_prefix)  );
		}
	}

fail:
    SG_RBTREE_NULLFREE(pCtx, prb);
	SG_NULLFREE(pCtx, psz_my_hid);
}

void SG_repo__hidlookup__blob(
		SG_context* pCtx,
        SG_repo* pRepo,
        const char* psz_hid_prefix,
        char** ppsz_hid
        )
{
    const char* psz_hid = NULL;
    SG_rbtree* prb = NULL;
    SG_uint32 count = 0;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(psz_hid_prefix);
	SG_NULLARGCHECK_RETURN(ppsz_hid);

    SG_ERR_CHECK(  SG_repo__find_blobs_by_prefix(pCtx, pRepo, psz_hid_prefix, &prb)  );

    SG_ERR_CHECK(  SG_rbtree__count(pCtx, prb, &count)  );

    if (1 == count)
    {
        SG_ERR_CHECK(  SG_rbtree__get_only_entry(pCtx, prb, &psz_hid, NULL)  );
        SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_hid, ppsz_hid)  );
    }
    else
    {
        SG_ERR_THROW(  SG_ERR_AMBIGUOUS_ID_PREFIX  );
    }

    SG_RBTREE_NULLFREE(pCtx, prb);

    return;

fail:
    SG_RBTREE_NULLFREE(pCtx, prb);
}

void SG_repo__store_dagnode(SG_context* pCtx,
							SG_repo* pRepo,
							SG_repo_tx_handle* pTx,
							SG_uint64 iDagNum,
							SG_dagnode * pDagnode)
{
    SG_dagfrag* pFrag = NULL;
	SG_bool bIsFrozen;
    char* psz_repo_id = NULL;
    char* psz_admin_id = NULL;

	SG_NULLARGCHECK_RETURN(pTx);

	// If there's a dagnode provided, a dagnum must also be provided.
	// Null dagnode and SG_DAGNUM__NONE is allowed, but Ian doesn't remember why.
	//
	// TODO Review: revisit this argcheck.  if pDagnode is null, the following
	//              __is_frozen call is going to fail anyway.
	if ( (pDagnode == NULL) ^ (iDagNum == SG_DAGNUM__NONE) )
		SG_ERR_THROW_RETURN(SG_ERR_INVALIDARG);

    SG_ERR_CHECK_RETURN(  SG_dagnode__is_frozen(pCtx, pDagnode,&bIsFrozen) );
    if (!bIsFrozen)
        SG_ERR_THROW_RETURN(SG_ERR_INVALID_UNLESS_FROZEN);

    SG_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pRepo, &psz_repo_id)  );
    SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pRepo, &psz_admin_id)  );

    SG_ERR_CHECK(  SG_dagfrag__alloc(pCtx, &pFrag, psz_repo_id, psz_admin_id, iDagNum)  );
    SG_NULLFREE(pCtx, psz_repo_id);
    SG_NULLFREE(pCtx, psz_admin_id);

    SG_ERR_CHECK(  SG_dagfrag__add_dagnode(pCtx, pFrag, &pDagnode)  );
    SG_ERR_CHECK(  SG_repo__store_dagfrag(pCtx, pRepo, pTx, pFrag)  );

    return;

fail:
    SG_DAGFRAG_NULLFREE(pCtx, pFrag);
}

void SG_repo__alloc_compute_hash__from_bytes(SG_context * pCtx,
											 SG_repo * pRepo,
											 SG_uint32 lenBuf,
											 const SG_byte * pBuf,
											 char ** ppsz_hid_returned)
{
	SG_repo_hash_handle * pRHH = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(ppsz_hid_returned);
	if (lenBuf > 0)
		SG_NULLARGCHECK_RETURN(pBuf);

	SG_ERR_CHECK(  SG_repo__hash__begin(pCtx,pRepo,&pRHH)  );
	SG_ERR_CHECK(  SG_repo__hash__chunk(pCtx,pRepo,pRHH,lenBuf,pBuf)  );
	SG_ERR_CHECK(  SG_repo__hash__end(pCtx,pRepo,&pRHH,ppsz_hid_returned)  );
	return;

fail:
	SG_ERR_CHECK(  SG_repo__hash__abort(pCtx,pRepo,&pRHH)  );
}

void SG_repo__alloc_compute_hash__from_string(SG_context * pCtx,
											  SG_repo * pRepo,
											  const SG_string * pString,
											  char ** ppsz_hid_returned)
{
	SG_NULLARGCHECK_RETURN(pString);

	SG_ERR_CHECK_RETURN(  SG_repo__alloc_compute_hash__from_bytes(pCtx,pRepo,
																  SG_string__length_in_bytes(pString),
																  (SG_byte *)SG_string__sz(pString),
																  ppsz_hid_returned)  );
}

// TODO What buffer size should we use when hashing the contents of a file?

#define COMPUTE_HASH_FILE_BUFFER_SIZE					(64 * 1024)

void SG_repo__alloc_compute_hash__from_file(SG_context * pCtx,
											SG_repo * pRepo,
											SG_file * pFile,
											char ** ppsz_hid_returned)
{
	SG_repo_hash_handle * pRHH = NULL;
	SG_byte buf[COMPUTE_HASH_FILE_BUFFER_SIZE];
	SG_uint32 nbr;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(pFile);
	SG_NULLARGCHECK_RETURN(ppsz_hid_returned);

	SG_ERR_CHECK(  SG_repo__hash__begin(pCtx,pRepo,&pRHH)  );

	SG_ERR_CHECK(  SG_file__seek(pCtx,pFile,0)  );
	while (1)
	{
		SG_file__read(pCtx,pFile,sizeof(buf),buf,&nbr);
		if (SG_context__err_equals(pCtx, SG_ERR_EOF))
		{
			SG_context__err_reset(pCtx);
			break;
		}
		SG_ERR_CHECK_CURRENT;

		SG_ERR_CHECK(  SG_repo__hash__chunk(pCtx,pRepo,pRHH,nbr,buf)  );
	}
	SG_ERR_CHECK(  SG_repo__hash__end(pCtx,pRepo,&pRHH,ppsz_hid_returned)  );
	return;

fail:
	SG_ERR_CHECK(  SG_repo__hash__abort(pCtx,pRepo,&pRHH)  );
}

void SG_repo__get_descriptor__ref(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_vhash** ppvh
        )
{
	SG_NULLARGCHECK_RETURN(pRepo);

    *ppvh = pRepo->pvh_descriptor;
}

void SG_repo__get_instance_data(
        SG_context* pCtx,
        SG_repo* pRepo,
        void** pp
        )
{
	SG_NULLARGCHECK_RETURN(pRepo);

    *pp = pRepo->p_vtable_instance_data;
}

void SG_repo__set_instance_data(
        SG_context* pCtx,
        SG_repo* pRepo,
        void* p
        )
{
	SG_NULLARGCHECK_RETURN(pRepo);

    pRepo->p_vtable_instance_data = p;
}

void SG_repo__does_blob_exist(
	SG_context* pCtx,
	SG_repo* pRepo,
	const char* psz_hid,
	SG_bool* pbExists)
{
	SG_repo_fetch_blob_handle* pHandle = NULL;

	SG_repo__fetch_blob__begin(pCtx, pRepo, psz_hid, SG_FALSE, NULL, NULL, NULL, NULL, &pHandle);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		if (SG_context__err_equals(pCtx, SG_ERR_BLOB_NOT_FOUND))
		{
			SG_ERR_DISCARD;
			if (pbExists)
				*pbExists = SG_FALSE;
			return;
		}
		SG_ERR_RETHROW_RETURN;
	}

	if (pbExists)
		*pbExists = (pHandle != NULL);
	
	if (pHandle)
		SG_ERR_IGNORE(  SG_repo__fetch_blob__abort(pCtx, pRepo, &pHandle)  );
}

void SG_repo__list_dags__rbtree(SG_context* pCtx, SG_repo* pRepo, SG_rbtree** pprbDagnums)
{
	SG_rbtree* prb = NULL;
	SG_uint32 count = 0;
	SG_uint32 i;
	SG_uint64* dagnumArray = NULL;
	char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];

	SG_ERR_CHECK(  SG_repo__list_dags(pCtx, pRepo, &count, &dagnumArray)  );
	if (count)
	{
		SG_ERR_CHECK(  SG_RBTREE__ALLOC__PARAMS(pCtx, &prb, count, NULL)  );
		for (i = 0; i < count; i++)
		{
			SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, dagnumArray[i], buf_dagnum, sizeof(buf_dagnum))  );
			SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb, buf_dagnum)  );
		}
	}

	SG_RETURN_AND_NULL(prb, pprbDagnums);

	/* Common cleanup */
fail:
	SG_RBTREE_NULLFREE(pCtx, prb);
	SG_NULLFREE(pCtx, dagnumArray);
}

#define SG_CLOSET__PROPERTY_NAME__USER_MASTER "USER_MASTER_REPO_DESCRIPTOR"

void SG_repo__user_master__open(
	SG_context* pCtx,
	SG_repo** ppRepo)
{
	char* pszDescriptor = NULL;
	SG_vhash* pvhDescriptor = NULL;
	SG_repo* pRepo = NULL;

	SG_NULLARGCHECK_RETURN(ppRepo);

	SG_closet__property__get(pCtx, SG_CLOSET__PROPERTY_NAME__USER_MASTER, &pszDescriptor);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		SG_ERR_REPLACE2(SG_ERR_NO_SUCH_CLOSET_PROP, SG_ERR_NOTAREPOSITORY, (pCtx, "%s", "no user master repository is present"));
		SG_ERR_CHECK_CURRENT;
	}

	SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, &pvhDescriptor, pszDescriptor)  );

	SG_ERR_CHECK(  SG_repo__open_from_descriptor(pCtx, &pvhDescriptor, &pRepo)  );

	*ppRepo = pRepo;
	pRepo = NULL;

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pszDescriptor);
	SG_VHASH_NULLFREE(pCtx, pvhDescriptor);
}

void SG_repo__user_master__create(
	SG_context* pCtx,
	const char* psz_admin_id,
	const char* pszStorage,
	const char* psz_hash_method)
{
	char buf_repo_id[SG_GID_BUFFER_LENGTH];
	SG_closet_descriptor_handle* ph = NULL;
	SG_repo* pRepo = NULL;
	SG_vhash* pvhDescriptorPartial = NULL;
	SG_string* pstrDescriptorFull = NULL;
	SG_audit q;
	const SG_vhash* pvhRefDescriptorFull = NULL;

	SG_NONEMPTYCHECK_RETURN(psz_admin_id);

	SG_ERR_CHECK(  SG_gid__generate(pCtx, buf_repo_id, sizeof(buf_repo_id))  );

	SG_closet__descriptors__add_begin(pCtx, 
		SG_CLOSET__PROPERTY_NAME__USER_MASTER, 
		pszStorage, 
		buf_repo_id, 
		psz_admin_id, 
		NULL, &pvhDescriptorPartial, &ph);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		SG_ERR_REPLACE2(SG_ERR_CLOSET_PROP_ALREADY_EXISTS, SG_ERR_REPO_ALREADY_EXISTS, (pCtx, "%s", "user master"));
		SG_ERR_CHECK_CURRENT;
	}
	SG_ERR_CHECK(  SG_closet__descriptors__add_abort(pCtx, &ph)  );
	
	SG_ERR_CHECK(  SG_repo__create_repo_instance(pCtx,
		SG_CLOSET__PROPERTY_NAME__USER_MASTER,
		pvhDescriptorPartial,
		SG_TRUE,
		psz_hash_method,
		buf_repo_id,
		psz_admin_id,
		&pRepo)  );
	SG_ERR_CHECK(  sg_zing__init_new_repo(pCtx, pRepo)  );
	SG_ERR_CHECK(  SG_user__create_nobody(pCtx, pRepo)  );
	SG_ERR_CHECK(  SG_audit__init__maybe_nobody(pCtx, &q, pRepo, SG_AUDIT__WHEN__NOW, SG_AUDIT__WHO__FROM_SETTINGS)  );
	SG_ERR_CHECK(  SG_area__add(pCtx, pRepo, SG_AREA_NAME__CORE, SG_DAGNUM__GET_AREA_ID(SG_DAGNUM__USERS), &q)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstrDescriptorFull)  );
	SG_ERR_CHECK(  SG_repo__get_descriptor__ref(pCtx, pRepo, (SG_vhash**)&pvhRefDescriptorFull)  );
	SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvhRefDescriptorFull, pstrDescriptorFull)  );
	SG_ERR_CHECK(  SG_closet__property__add(pCtx, SG_CLOSET__PROPERTY_NAME__USER_MASTER, SG_string__sz(pstrDescriptorFull))  );

	/* fall through */
fail:
	SG_ERR_IGNORE(  SG_closet__descriptors__add_abort(pCtx, &ph)  );
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_VHASH_NULLFREE(pCtx, pvhDescriptorPartial);
	SG_STRING_NULLFREE(pCtx, pstrDescriptorFull);
}

void SG_repo__user_master__exists(SG_context* pCtx, SG_bool* pbExists)
{
	SG_ERR_CHECK_RETURN(  SG_closet__property__exists(pCtx, SG_CLOSET__PROPERTY_NAME__USER_MASTER, pbExists)  );
}

void SG_repo__user_master__get_adminid(SG_context* pCtx, char** ppszAdminId)
{
	char* pszVal = NULL;
	SG_vhash* pvh = NULL;
	const char* pszRefAdminId = NULL;

	SG_NULLARGCHECK_RETURN(ppszAdminId);

	SG_closet__property__get(pCtx, SG_CLOSET__PROPERTY_NAME__USER_MASTER, &pszVal);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		SG_ERR_REPLACE2(SG_ERR_NO_SUCH_CLOSET_PROP, SG_ERR_NOTAREPOSITORY, (pCtx, "%s", "no user master repository is present"));
		SG_ERR_CHECK_CURRENT;
	}
	SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__SZ(pCtx, &pvh, pszVal)  );
	
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, SG_RIDESC_KEY__ADMIN_ID, &pszRefAdminId)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszRefAdminId, ppszAdminId)  );

	/* fall through */
fail:
	SG_NULLFREE(pCtx, pszVal);
	SG_VHASH_NULLFREE(pCtx, pvh);
}

//////////////////////////////////////////////////////////////////////////
