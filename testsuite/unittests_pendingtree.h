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
 * @file unittests_pendingtree.h
 *
 * @details _ut_pt__ debug routines inspect a pendingtree and convert it
 *          to pvh and prb so that we can look at the various flags and
 *          fields.
 *
 *          This was moved from u0043_pendingtree.c
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_UNITTESTS_PENDINGTREE_H
#define H_UNITTESTS_PENDINGTREE_H

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include <sg_vv2__public_typedefs.h>
#include <sg_vv2__public_prototypes.h>

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

// TODO These defines are currently private to sg_pendingtree.c
// TODO Should we make them public or just leave these here for testing.

#define sg_PTNODE_TEMP_FLAG_CHECKMYNAME        2
#define sg_PTNODE_TEMP_FLAG_COMMITTING         4
#define sg_PTNODE_SAVED_FLAG_DELETED           8
#define sg_PTNODE_SAVED_FLAG_MOVED_AWAY       16
#define sg_PTNODE_TEMP_FLAG_IMPLICIT_DELETE   32
#define sg_PTNODE_TEMP_FLAG_REVERTING         64
#define sg_PTNODE_TEMP_FLAG_EXISTS_LOCALLY   128
#define sg_PTNODE_TEMP_FLAG_IMPLICIT_ADD     256

#define _ut_pt__PTNODE_ZERO			0,0
#define _ut_pt__PTNODE_DELETED		sg_PTNODE_SAVED_FLAG_DELETED,0
#define _ut_pt__PTNODE_LOST			sg_PTNODE_SAVED_FLAG_DELETED,sg_PTNODE_TEMP_FLAG_IMPLICIT_DELETE
#define _ut_pt__PTNODE_FOUND		0,sg_PTNODE_TEMP_FLAG_IMPLICIT_ADD|sg_PTNODE_TEMP_FLAG_CHECKMYNAME

//////////////////////////////////////////////////////////////////

void _ut_pt__new_repo(SG_context * pCtx,
					  const char* pszRidescName,
					  const SG_pathname* pPathWorkingDir)
{
    SG_repo* pRepo = NULL;

	SG_ERR_CHECK_RETURN(  SG_vv2__init_new_repo(pCtx,
													 pszRidescName,
													 SG_pathname__sz(pPathWorkingDir),
													 NULL,
													 NULL,
													 SG_FALSE,
                                                     NULL,
													 SG_FALSE,
													 NULL,
													 NULL)  );
	SG_ERR_CHECK_RETURN(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRidescName, &pRepo)  );
    SG_ERR_CHECK_RETURN(  SG_user__create(pCtx, pRepo, "testing@sourcegear.com", NULL)  );
    SG_ERR_CHECK_RETURN(  SG_user__set_user__repo(pCtx, pRepo, "testing@sourcegear.com")  );
    SG_REPO_NULLFREE(pCtx, pRepo);
}

void _ut_pt__new_repo2(SG_context * pCtx,
					   const char* pszRidescName,
					   const SG_pathname* pPathWorkingDir,
					   char ** ppszidFirstChangeset)
{
    SG_repo* pRepo = NULL;

	SG_ERR_CHECK_RETURN(  SG_vv2__init_new_repo(pCtx,
													 pszRidescName,
													 SG_pathname__sz(pPathWorkingDir),
													 NULL,
													 NULL,
													 SG_FALSE,
                                                     NULL,
													 SG_FALSE,
													 NULL,
													 ppszidFirstChangeset)  );
	SG_ERR_CHECK_RETURN(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRidescName, &pRepo)  );
    SG_ERR_CHECK_RETURN(  SG_user__create(pCtx, pRepo, "testing@sourcegear.com", NULL)  );
    SG_ERR_CHECK_RETURN(  SG_user__set_user__repo(pCtx, pRepo, "testing@sourcegear.com")  );
    SG_REPO_NULLFREE(pCtx, pRepo);
}

void _ut_pt__new_repo3(SG_context * pCtx,
					   const char* pszRidescName,
					   const SG_pathname* pPathWorkingDir,
					   SG_bool bIgnoreBackups)
{
	SG_ERR_CHECK(  _ut_pt__new_repo(pCtx, pszRidescName, pPathWorkingDir)  );
	
	VERIFY_ERR_CHECK(  unittests__set__ignore_backup_files(pCtx, pPathWorkingDir, bIgnoreBackups)  );
	INFOP("new_repo3", ("Repo created in [%s][bIgnoreBackups %d]",
						SG_pathname__sz(pPathWorkingDir),
						bIgnoreBackups));

fail:
	return;
}

void _ut_pt__create_file__numbers(SG_context * pCtx,
	SG_pathname* pPath,
	const char* pszName,
	const SG_uint32 countLines
	)
{
	SG_pathname* pPathFile = NULL;
	SG_uint32 i;
	SG_file* pFile = NULL;

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathFile, pPath, pszName)  );
	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathFile, SG_FILE_CREATE_NEW | SG_FILE_RDWR, 0600, &pFile)  );
	for (i=0; i<countLines; i++)
	{
		char buf[64];

		VERIFY_ERR_CHECK(  SG_sprintf(pCtx, buf, sizeof(buf), "%d\n", i)  );
		VERIFY_ERR_CHECK(  SG_file__write(pCtx, pFile, (SG_uint32) strlen(buf), (SG_byte*) buf, NULL)  );
	}
	VERIFY_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

	SG_PATHNAME_NULLFREE(pCtx, pPathFile);

	return;

fail:
	return;
}

void _ut_pt__append_to_file__numbers(SG_context * pCtx,
	SG_pathname* pPath,
	const char* pszName,
	const SG_uint32 countLines
	)
{
	SG_pathname* pPathFile = NULL;
	SG_uint32 i;
	SG_file* pFile = NULL;
	SG_uint64 len = 0;

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathFile, pPath, pszName)  );
	VERIFY_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPathFile, &len, NULL)  );
	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathFile, SG_FILE_OPEN_EXISTING | SG_FILE_RDWR, 0600, &pFile)  );
	VERIFY_ERR_CHECK(  SG_file__seek(pCtx, pFile, len)  );
	for (i=0; i<countLines; i++)
	{
		char buf[64];

		VERIFY_ERR_CHECK(  SG_sprintf(pCtx, buf, sizeof(buf), "%d\n", i)  );
		VERIFY_ERR_CHECK(  SG_file__write(pCtx, pFile, (SG_uint32) strlen(buf), (SG_byte*) buf, NULL)  );
	}
	VERIFY_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

	SG_PATHNAME_NULLFREE(pCtx, pPathFile);

	return;

fail:
	return;
}

void _ut_pt__delete_file(SG_context * pCtx,
	SG_pathname* pPath,
	const char* pszName
	)
{
	SG_pathname* pPathFile = NULL;

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathFile, pPath, pszName)  );
	VERIFY_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPathFile)  );

	SG_PATHNAME_NULLFREE(pCtx, pPathFile);

	return;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void _ut_pt__addremove_param(
	SG_context* pCtx,
	const SG_pathname* pPathWorkingDir)
{
	SG_ERR_CHECK(  SG_wc__addremove(pCtx,
									pPathWorkingDir,
									"@/",
									WC__GET_DEPTH(SG_TRUE),	// bRecursive
									SG_FALSE, // bNoIgnores
									SG_FALSE, // bTest
									NULL      // journal
					   )  );
	
fail:
	;
}

void _ut_pt__addremove(SG_context * pCtx,
					   const SG_pathname* pPathWorkingDir
	)
{
	SG_ERR_CHECK_RETURN(  _ut_pt__addremove_param(pCtx, pPathWorkingDir)  );
}

//////////////////////////////////////////////////////////////////

#if 0
struct _p2r_data
{
	SG_rbtree *		prb;
	SG_string *		pStringRepoPathParent;
};

SG_vhash_foreach_callback _ut_pt__convert_pendingtree_pvh_2_repo_path_rbtree_cb;

/**
 * callback to process one entry within the pendingtree-expressed-in-a-vhash (like the dot-json vfile)
 * and add row to the rbtree using the full repo-path as the key.
 *
 * We borrow the given vhash (packed in a variant) for the assoc data in the rbtree.
 */
void _ut_pt__convert_pendingtree_pvh_2_repo_path_rbtree_cb(SG_context * pCtx, void * pVoidData,
														   SG_UNUSED_PARAM( const SG_vhash * pvhParentEntries ),
														   const char * pszKey,
														   const SG_variant * pVariantValue)
{
	struct _p2r_data * pDataParent = (struct _p2r_data *)pVoidData;
	SG_vhash * pvh;
	SG_string * pStringMyRepoPath = NULL;
	SG_string * pStringMyRepoPathKey = NULL;
	SG_vhash * pvhEntry;
	SG_vhash * pvhEntries;
	const char * psz;
	const char * pszName;
	SG_uint32 ui32;
	SG_uint32 flags;
	SG_treenode_entry_type tneType;

	SG_UNUSED( pvhParentEntries );

	// the key is the gid of this entry.
	//
	// we expect the value to be a vhash describing this entry -- AS IT IS CURRENTLY
	// DEFINED IN THE PENDINGTREE.

	VERIFY_ERR_CHECK(  SG_variant__get__vhash(pCtx,pVariantValue,&pvh)  );

	// for directories, the value of "entries" will be a sub-vhash of the entries.
	//
	// the value of "gid" should match the key we are given.
	//
	// the value of "entry" is the original/baseline treenode entry.
	//
	// if there is a value in "name", the entry has been renamed and this name
	// overrides the baseline value in "entry.name".  Likewise for some of the
	// other fields.
	//
	// "c09a64d707464f83aea4113bab3533cc7148c388e8ea11dea6d4002500da2b78" :
	// {
	//     "flags" : 0,
	//     "temp_flags" : 0,
	//     "entry" :
	//     {
	//         "bits" : 0,
	//          "hidblob" : "8de344d3c8412123e2b7431af65e1ea567b4bc55d0b465ebe2ad731f30dee905",
	//          "name" : "@",
	//          "tne_version" : 1,
	//          "type" : 2
	//      },
	//     "type" : 2,
	//     "name" : null,
	//     "hid" : null,
	//     "attrbits" : 0,
	//     "gid" : "c09a64d707464f83aea4113bab3533cc7148c388e8ea11dea6d4002500da2b78",
	//     "moved_from" : null,
	//     "entries" :
	//     { ... }
	// }

	VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx,pvh,"flags",&flags)  );

	VERIFY_ERR_CHECK(  SG_vhash__get__sz(pCtx,pvh,"gid",&psz)  );
	VERIFY_COND("gid",(strcmp(psz,pszKey)==0));

	VERIFY_ERR_CHECK(  SG_vhash__check__vhash(pCtx,pvh,"entry",&pvhEntry)  );		// reflects the baseline TNE

	VERIFY_ERR_CHECK(  SG_vhash__check__sz(pCtx,pvh,"name",&pszName)  );
	if (!pszName || !*pszName)
		VERIFY_ERR_CHECK(  SG_vhash__get__sz(pCtx,pvhEntry,"name",&pszName)  );

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx,&pStringMyRepoPath)  );
	if (flags & sg_PTNODE_SAVED_FLAG_MOVED_AWAY)
		VERIFY_ERR_CHECK(  SG_string__append__sz(pCtx,pStringMyRepoPath,"<phantom>")  );

	if (pDataParent->pStringRepoPathParent)
	{
		VERIFY_ERR_CHECK(  SG_string__append__string(pCtx,pStringMyRepoPath,pDataParent->pStringRepoPathParent)  );
		VERIFY_ERR_CHECK(  SG_string__append__sz(pCtx,pStringMyRepoPath,"/")  );
	}
	VERIFY_ERR_CHECK(  SG_string__append__sz(pCtx,pStringMyRepoPath,pszName)  );

	VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx,pvh,"type",&ui32)  );
	tneType = (SG_treenode_entry_type)ui32;

	VERIFY_ERR_CHECK(  SG_STRING__ALLOC(pCtx,&pStringMyRepoPathKey)  );

	if (flags & sg_PTNODE_SAVED_FLAG_DELETED)
	{
		switch (tneType)
		{
		case SG_TREENODEENTRY_TYPE_DIRECTORY:
			VERIFY_ERR_CHECK(  SG_string__append__sz(pCtx,pStringMyRepoPathKey,"<d>")  );
			break;

		case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
			VERIFY_ERR_CHECK(  SG_string__append__sz(pCtx,pStringMyRepoPathKey,"<f>")  );
			break;

		case SG_TREENODEENTRY_TYPE_SYMLINK:
			VERIFY_ERR_CHECK(  SG_string__append__sz(pCtx,pStringMyRepoPathKey,"<l>")  );
			break;

		default:
			break;
		}
	}
	else
	{
		switch (tneType)
		{
		case SG_TREENODEENTRY_TYPE_DIRECTORY:
			VERIFY_ERR_CHECK(  SG_string__append__sz(pCtx,pStringMyRepoPathKey,"<D>")  );
			break;

		case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
			VERIFY_ERR_CHECK(  SG_string__append__sz(pCtx,pStringMyRepoPathKey,"<F>")  );
			break;

		case SG_TREENODEENTRY_TYPE_SYMLINK:
			VERIFY_ERR_CHECK(  SG_string__append__sz(pCtx,pStringMyRepoPathKey,"<L>")  );
			break;

		default:
			break;
		}
	}

	VERIFY_ERR_CHECK(  SG_string__append__sz(pCtx,pStringMyRepoPathKey,SG_string__sz(pStringMyRepoPath))  );

	VERIFY_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx,pDataParent->prb,SG_string__sz(pStringMyRepoPathKey),pvh)  );

	if (tneType == SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		VERIFY_ERR_CHECK(  SG_vhash__check__vhash(pCtx,pvh,"entries",&pvhEntries)  );
		if (pvhEntries)
		{
			struct _p2r_data data;

			data.prb = pDataParent->prb;
			data.pStringRepoPathParent = pStringMyRepoPath;

			SG_ERR_CHECK(  SG_vhash__foreach(pCtx,
											 pvhEntries,
											 _ut_pt__convert_pendingtree_pvh_2_repo_path_rbtree_cb,
											 &data)  );
		}
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringMyRepoPath);
	SG_STRING_NULLFREE(pCtx, pStringMyRepoPathKey);
}

/**
 * Convert the PendingTree JSON VHash (a recursive structure, keyed by gid) into a
 * flat rbtree (keyed by repo-path).  This can be used to ask simple questions about
 * the current pending-tree, such as is "@/a/b/c" present and what flags does it have.
 *
 * Note we borrow the sub-vhashes within this vhash as the assoc data in the rbtree,
 * so don't delete the pvh until you're done with the prb.
 */
void _ut_pt__convert_pendingtree_pvh_2_repo_path_rbtree(SG_context * pCtx,
														const SG_vhash * pvhPending,
														SG_rbtree ** pprb)
{
	SG_rbtree * prb = NULL;
	const char * psz;
	SG_vhash * pvhEntry;
	SG_vhash * pvhEntries;
	SG_treenode_entry_type tneType;
	SG_uint32 ui32, flags, nrEntries;
	struct _p2r_data data;

#if 0	// less noise
	SG_ERR_IGNORE(  SG_vhash_debug__dump_to_console(pCtx, pvhPending)  );
#endif

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb)  );

	// we expect the main vhash to contain super-root/meta-data. for example:
	//
	// {
	//     "flags" : 0,
	//     "temp_flags" : 0,
	//     "entry" : null,
	//     "type" : 2,
	//     "name" : null,
	//     "hid" : "cad118179d8127ec0ecdc3297bf87a96ec9159ab82331b380c4bee8e987444eb",
	//     "attrbits" : 0,
	//     "gid" : null,
	//     "moved_from" : null,
	//     "entries" :
	//     { ... }
	// }
	//
	// The key "entries" contains a sub-vhash of for the real root.  this sub-vhash
	// is keyed by the actual root's gid.

	VERIFY_ERR_CHECK(  SG_vhash__check__sz(pCtx,pvhPending,"gid",&psz)  );
	VERIFY_COND("super-root:gid",(psz == NULL));
	VERIFY_ERR_CHECK(  SG_vhash__check__sz(pCtx,pvhPending,"name",&psz)  );
	VERIFY_COND("super-root:name",(psz == NULL));
	VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx,pvhPending,"type",&ui32)  );
	tneType = (SG_treenode_entry_type)ui32;
	VERIFY_COND("super-root:type",(tneType==SG_TREENODEENTRY_TYPE_DIRECTORY));
	VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx,pvhPending,"flags",&flags)  );
	VERIFY_COND("super-root:flags",(flags==0 /*sg_PTNODE_SAVED_FLAG__ZERO*/));
	VERIFY_ERR_CHECK(  SG_vhash__check__vhash(pCtx,pvhPending,"entry",&pvhEntry)  );
	VERIFY_COND("super-root:entry",(pvhEntry==NULL));

	VERIFY_ERR_CHECK(  SG_vhash__check__vhash(pCtx,pvhPending,"entries",&pvhEntries)  );	// if no changes, this is empty.
	if (pvhEntries)
	{
		VERIFY_ERR_CHECK(  SG_vhash__count(pCtx,pvhEntries,&nrEntries)  );
		VERIFY_COND("root:count",(nrEntries == 1));

		data.prb = prb;
		data.pStringRepoPathParent = NULL;

		SG_ERR_CHECK(  SG_vhash__foreach(pCtx,
										 pvhEntries,
										 _ut_pt__convert_pendingtree_pvh_2_repo_path_rbtree_cb,
										 &data)  );
	}

	*pprb = prb;
	return;

fail:
	SG_RBTREE_NULLFREE(pCtx, prb);
}

SG_rbtree_foreach_callback _ut_pt__pending_rbtree_dump_cb;

void _ut_pt__pending_rbtree_dump_cb(SG_context * pCtx,
									const char * pszKeyRepoPath, void * pVoidAssoc,
									SG_UNUSED_PARAM( void * pVoidData ))
{
	const SG_vhash * pvh = (const SG_vhash *)pVoidAssoc;
	const char * pszGid;
	SG_uint32 flags;
	SG_uint32 temp_flags;
	SG_bool bHasTempFlags;

	SG_UNUSED( pVoidData );

	VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx,pvh,"flags",&flags)  );
	VERIFY_ERR_CHECK(  SG_vhash__get__sz(pCtx,pvh,"gid",&pszGid)  );

	VERIFY_ERR_CHECK(  SG_vhash__has(pCtx,pvh,"temp_flags",&bHasTempFlags)  );		// was sglib built with -DDEBUG and was pvh exported rather than read from VFILE.
	if (bHasTempFlags)
	{
		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx,pvh,"temp_flags",&temp_flags)  );
		INFOP("pending_rbtree_row",("[s %3d][t %3d] %s %s",
									flags,temp_flags,pszGid,pszKeyRepoPath));
	}
	else
	{
		INFOP("pending_rbtree_row",("[%3d] %s %s",
									flags,pszGid,pszKeyRepoPath));
	}

fail:
	return;
}

void _ut_pt__pending_rbtree_dump(SG_context * pCtx, SG_rbtree * prbPending, const char * pszLabel)
{
	INFOP("pending_rbtree    ",("%s",pszLabel));
	VERIFY_ERR_CHECK(  SG_rbtree__foreach(pCtx,prbPending,_ut_pt__pending_rbtree_dump_cb,NULL)  );

fail:
	return;
}

void _ut_pt__pending_rbtree__verify_flags(SG_context * pCtx,
										  SG_rbtree * prbPending,
										  const char * pszRepoPath,
										  SG_uint32 flagsExpected,
										  SG_uint32 tempFlagsExpected)
{
	const SG_vhash * pvhAssoc;
	SG_uint32 flagsObserved;
	SG_bool bFound;
	SG_bool bHasTempFlags;
	SG_uint32 tempFlagsObserved;

	// lookup the given repo-path in the rbtree.
	//
	// if we don't find it, it may mean that the entry isn't dirty in the pending tree.
	// if we do find it, it MAY or MAY NOT be dirty in the pending tree (depending on
	// whether we've scrubbed the pending tree).
	//
	// if you pass in 0,0 for the flags, we may not be able to detect a real change
	// vs a missing item.

	VERIFY_ERR_CHECK(  SG_rbtree__find(pCtx,prbPending,pszRepoPath,&bFound,(void **)&pvhAssoc)  );
	if (!bFound && (flagsExpected == 0) && (tempFlagsExpected == 0))
		return;
	VERIFYP_COND_FAIL("pending_rbtree__verify_flags",(bFound),("Row [%s] not found",pszRepoPath));

	VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx,pvhAssoc,"flags",&flagsObserved)  );
	VERIFYP_COND("pending_rbtree__verify_flags",(flagsObserved == flagsExpected),
				 ("Row [%s] flags [obs %d][exp %d]",
				  pszRepoPath,flagsObserved,flagsExpected));

	VERIFY_ERR_CHECK(  SG_vhash__has(pCtx,pvhAssoc,"temp_flags",&bHasTempFlags)  );		// was sglib built with -DDEBUG and was pvh exported rather than read from VFILE.
	if (bHasTempFlags)
	{
		VERIFY_ERR_CHECK(  SG_vhash__get__uint32(pCtx,pvhAssoc,"temp_flags",&tempFlagsObserved)  );
		VERIFYP_COND("pending_rbtree__verify_temp_flags",(tempFlagsObserved == tempFlagsExpected),
					 ("Row [%s] temp_flags [obs %d][exp %d]",
					  pszRepoPath,tempFlagsObserved,tempFlagsExpected));
	}

fail:
	return;
}

void _ut_pt__pending_rbtree__verify_treediff_flags(SG_context * pCtx,
												   SG_rbtree * prbPending,
												   SG_treediff2 * pTreeDiff,
												   const char * pszRepoPath,
												   SG_diffstatus_flags dsFlagsExpected)
{
	const SG_vhash * pvhAssoc;
	const char * pszGid;
	const SG_treediff2_ObjectData * pOD;
	SG_diffstatus_flags dsFlagsObserved;
	SG_bool bFound;

	VERIFY_ERR_CHECK(  SG_rbtree__find(pCtx,prbPending,pszRepoPath,&bFound,(void **)&pvhAssoc)  );
	if (!bFound && (dsFlagsExpected == SG_DIFFSTATUS_FLAGS__ZERO))
		return;
	VERIFYP_COND_FAIL("pending_rbtree__verify_treediff_flags",(bFound),("Row [%s] not found",pszRepoPath));

	VERIFY_ERR_CHECK(  SG_vhash__get__sz(pCtx,pvhAssoc,"gid",&pszGid)  );
	VERIFY_ERR_CHECK(  SG_treediff2__get_ObjectData(pCtx,pTreeDiff,pszGid,&bFound,&pOD)  );
	if (!bFound && (dsFlagsExpected == SG_DIFFSTATUS_FLAGS__ZERO))
		return;
	VERIFYP_COND_FAIL("pending_rbtree__verify_treediff_flags",(bFound),("Row [%s][gid %s] not found in treediff",pszRepoPath,pszGid));

	VERIFY_ERR_CHECK(  SG_treediff2__ObjectData__get_dsFlags(pCtx,pOD,&dsFlagsObserved)  );
	VERIFYP_COND_FAIL("pending_rbtree__verify_treediff_flags",(dsFlagsObserved == dsFlagsExpected),("Row [%s][gid %s] flags [obs 0x%x][exp 0x%x]",
																									pszRepoPath,pszGid,
																									dsFlagsObserved,dsFlagsExpected));

fail:
	return;
}
#endif

//////////////////////////////////////////////////////////////////

/**
 * A wrapper around __get_wd_parents() for use when we
 * expect to only have 1 parent.
 *
 * This test wrapper was originally written against the PT routines
 * which did not assume the CWD was in the WD.  The WC layer does
 * so we juggle the CWD.
 */
void _ut_pt__get_baseline(SG_context * pCtx,
						  const SG_pathname * pPathWorkingDir,
						  char * pbuf_baseline, SG_uint32 len_buf_baseline)
{
	SG_varray * pva_wd_parents = NULL;
	const char * psz_hid_baseline;			// we do not own this
	SG_uint32 nrParents;

	SG_ERR_CHECK(  SG_wc__get_wc_parents__varray(pCtx, pPathWorkingDir, &pva_wd_parents)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pva_wd_parents, &nrParents)  );
	SG_ASSERT(  (nrParents >= 1)  );

	VERIFYP_COND("_ut_pt__get_baseline",
				 (nrParents == 1),
				 ("Expected only one parent; found %d; continuing test with parent[0]",nrParents));

	SG_ERR_CHECK_RETURN(  SG_varray__get__sz(pCtx, pva_wd_parents, 0, &psz_hid_baseline)  );
	SG_ERR_CHECK_RETURN(  SG_strcpy(pCtx, pbuf_baseline, len_buf_baseline, psz_hid_baseline)  );
	
fail:
	SG_VARRAY_NULLFREE(pCtx, pva_wd_parents);
}

/**
 * A wrapper to set (UPDATE) the WD to the given baseline.
 * This will throw if we have an uncommitted merge.
 */
void _ut_pt__set_baseline(SG_context * pCtx,
						  const SG_pathname * pPathWorkingDir,
						  const char * psz_hid_cs)
{
	SG_wc_update_args ua;
	SG_rev_spec * pRevSpec = NULL;

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, psz_hid_cs)  );

	memset(&ua, 0, sizeof(ua));
	ua.pRevSpec = pRevSpec;
	ua.pszAttach = UT__ATTACH_TO;
	ua.bDisallowWhenDirty = SG_FALSE;

	SG_ERR_CHECK(  SG_wc__update(pCtx, pPathWorkingDir, &ua,
								 SG_FALSE,	// bTest
								 NULL,		// pvaJournal (verbose)
								 NULL,		// pvhStats
								 NULL,		// ppvaStatus
								 NULL)  );	// ppszHidChosen

fail:
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
}

//////////////////////////////////////////////////////////////////

void _ut_pt__verify_vhash_count(SG_context * pCtx, 
												  const SG_vhash* pvh,
												  SG_uint32 count_should_be,
												  SG_uint32 lineNr)
{
	SG_uint32 count;
	
	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh, &count)  );
	VERIFYP_COND("_ut_pt__verify_vhash_count", (count == count_should_be),
				 ("%s:%d: [obs %d][exp %d]",__FILE__,lineNr,count,count_should_be));

	return;

fail:
	return;
}

void _ut_pt__verifytree__dir_has_symlink(SG_context * pCtx, 
	const SG_vhash* pvh,
	const char* pszName
	)
{
	SG_vhash* pvhItem = NULL;
	const char* pszType = NULL;
	
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh, pszName, &pvhItem)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "type", &pszType)  );
	VERIFY_COND("_ut_pt__verifytree__dir_has_symlink", (0 == strcmp(pszType, "symlink")));
	
	return;

fail:
	return;
}

void _ut_pt__verifytree__dir_has_file(SG_context * pCtx, 
	const SG_vhash* pvh,
	const char* pszName
	)
{
	SG_vhash* pvhItem = NULL;
	const char* pszType = NULL;
	
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh, pszName, &pvhItem)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "type", &pszType)  );
	VERIFY_COND("_ut_pt__verifytree__dir_has_file", (0 == strcmp(pszType, "file")));
	
	return;

fail:
	return;
}

void _ut_pt__verifytree__get_gid(SG_context * pCtx, 
	const SG_vhash* pvh,
	const char* pszName,
	char* pszgid,
	SG_uint32 bufLen
	)
{
	SG_vhash* pvhItem = NULL;
	const char* pszg = NULL;
	
	VERIFY_COND("_ut_pt__verifytree__get_gid", (bufLen >= SG_GID_BUFFER_LENGTH));

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh, pszName, &pvhItem)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszg)  );
	SG_ERR_CHECK(  SG_strcpy(pCtx, pszgid, bufLen, pszg)  );
	
	return;

fail:
	return;
}

void _ut_pt__verifytree__dir_get_dir(SG_context * pCtx, 
	const SG_vhash* pvh,
	const char* pszName,
	SG_vhash** ppResult
	)
{
	SG_vhash* pvhItem = NULL;
	const char* pszType = NULL;
	
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh, pszName, &pvhItem)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "type", &pszType)  );
	VERIFY_COND("_ut_pt__verifytree__dir_get_dir", (0 == strcmp(pszType, "directory")));
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem, "dir", ppResult)  );
	
	return;

fail:
	return;
}

void _ut_pt__verifytree__dir_has_dir(SG_context * pCtx, 
	const SG_vhash* pvh,
	const char* pszName
	)
{
	SG_vhash* pvhItem = NULL;
	const char* pszType = NULL;
	
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh, pszName, &pvhItem)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "type", &pszType)  );
	VERIFY_COND("_ut_pt__verifytree__dir_has_dir", (0 == strcmp(pszType, "directory")));
	
	return;

fail:
	return;
}

void _ut_pt__do_get_dir(SG_context * pCtx, SG_repo* pRepo, const char* pszidHidTreeNode, SG_vhash** ppResult)
{
	SG_uint32 count;
	SG_uint32 i;
	SG_treenode* pTreenode = NULL;
	SG_vhash* pvhDir = NULL;
	SG_vhash* pvhSub = NULL;
	SG_vhash* pvhSubDir = NULL;
	
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhDir)  );
	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pszidHidTreeNode, &pTreenode)  );
	SG_ERR_CHECK(  SG_treenode__count(pCtx, pTreenode, &count)  );
	for (i=0; i<count; i++)
	{
		const SG_treenode_entry* pEntry = NULL;
		SG_treenode_entry_type type;
		const char* pszName = NULL;
		const char* pszidHid = NULL;
		const char* pszgid = NULL;
		
		SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, pTreenode, i, &pszgid, &pEntry)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pEntry, &type)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pEntry, &pszName)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pEntry, &pszidHid)  );

		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhSub)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub, "hid", pszidHid)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub, "gid", pszgid)  );
		
		if (SG_TREENODEENTRY_TYPE_DIRECTORY == type)
		{
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub, "type", "directory")  );
			
			SG_ERR_CHECK(  _ut_pt__do_get_dir(pCtx, pRepo, pszidHid, &pvhSubDir)  );
			SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhSub, "dir", &pvhSubDir)  );
		}
		else if (SG_TREENODEENTRY_TYPE_REGULAR_FILE == type)
		{
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub, "type", "file")  );
			
		}
		else if (SG_TREENODEENTRY_TYPE_SYMLINK == type)
		{
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub, "type", "symlink")  );
			
		}
		else
		{
			SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
		}
		
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhDir, pszName, &pvhSub)  );
	}
	
	SG_TREENODE_NULLFREE(pCtx, pTreenode);
	pTreenode = NULL;

	*ppResult = pvhDir;

	return;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhDir);
	SG_VHASH_NULLFREE(pCtx, pvhSub);
}

void _ut_pt__get_entire_repo_manifest(SG_context * pCtx, 
	const char* pszRidescName,
	SG_vhash** ppResult
	)
{
	SG_vhash* pvh = NULL;
	SG_repo* pRepo = NULL;
	SG_rbtree* pIdsetLeaves = NULL;
	SG_uint32 count_leaves = 0;
	char* pszidUserSuperRoot = NULL;
	SG_rbtree_iterator* pIterator = NULL;
	SG_bool b = SG_FALSE;
	const char* pszLeaf = NULL;
	SG_treenode* pTreenode = NULL;
	const SG_treenode_entry* pEntry = NULL;
	const char* pszidHidActualRoot = NULL;
	SG_uint32 count = 0;
	
	/*
	 * Fetch the descriptor by its given name and use it to connect to
	 * the repo.
	 */
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRidescName, &pRepo)  );

	/*
	 * This routine currently only works if there is exactly one leaf
	 * in the repo.  A smarter version of this (later) will allow the
	 * desired dagnode to be specified either by its hex id or by a
	 * named branch or a label.
	 */
	SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo,SG_DAGNUM__VERSION_CONTROL,&pIdsetLeaves)  );
	SG_ERR_CHECK(  SG_rbtree__count(pCtx, pIdsetLeaves, &count_leaves)  );
	if (count_leaves != 1)
	{
		SG_ERR_THROW(  SG_ERR_INVALIDARG  ); /* TODO better err code here */
	}

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, pIdsetLeaves, &b, &pszLeaf, NULL)  );
	SG_ERR_IGNORE(  SG_rbtree__iterator__free(pCtx, pIterator)  );
	pIterator = NULL;

	/*
	 * Load the desired changeset from the repo so we can look up the
	 * id of its user root directory
	 */
	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, pszLeaf, &pszidUserSuperRoot)  );

	/*
	 * What we really want here is not the super-root, but the actual root.
	 * So let's jump down one directory.
	 */
	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pszidUserSuperRoot, &pTreenode)  );
	SG_ERR_CHECK(  SG_treenode__count(pCtx, pTreenode, &count)  );
	VERIFY_COND("count should be 1", (count == 1));
	
	SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, pTreenode, 0, NULL, &pEntry)  );
	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pEntry, &pszidHidActualRoot)  );

	/*
	 * Retrieve everything.  A new directory will be created in cwd.
	 * Its name will be the name of the descriptor.
	 */
	SG_ERR_CHECK(  _ut_pt__do_get_dir(pCtx, pRepo, pszidHidActualRoot, &pvh)  );
	
	SG_TREENODE_NULLFREE(pCtx, pTreenode);
	SG_NULLFREE(pCtx, pszidUserSuperRoot);
	SG_RBTREE_NULLFREE(pCtx, pIdsetLeaves);
	SG_REPO_NULLFREE(pCtx, pRepo);

	*ppResult = pvh;
	
	return;

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * A wrapper to see if the pendingtree is clean.  This replaces
 * a bunch of places in u0043 and friends where we were stat'ing
 * pending.json and checking for length zero.
 *
 * This routine knows too much about "wd.json" and the guts of
 * pendingtree.  It bypasses the usual "load the wd.json in memory,
 * build the ptnode tree, scan the WD, and run the treediff" code.
 * We just want to know if the "pending" member is present in the
 * file.
 *
 * (Granted both methods should give us the same answer, but it's
 * the file that I'm worrying about here.)
 *
 * UPDATE 2011/12/22 With the convesion from PendingTree to WC,
 * this wrapper is even more broken.  So, lets convert it to the
 * new way until all of our callers can be changed
 */
void _ut_pt__get_wd_dot_json_stats(SG_context * pCtx,
								   const SG_pathname * pPathWorkingDir,
								   SG_bool * pbHavePending)
{
	SG_uint32 nrChanges;

	SG_ERR_CHECK_RETURN(  unittests__wc_status__all(pCtx, pPathWorkingDir, NULL, &nrChanges, NULL)  );

	*pbHavePending = (nrChanges > 0);
}

#define VERIFY_WD_JSON_PENDING_CLEAN(pCtx, pPathWorkingDir)				\
	SG_STATEMENT(														\
	SG_bool _bHavePending;												\
	VERIFY_ERR_CHECK(  _ut_pt__get_wd_dot_json_stats((pCtx), (pPathWorkingDir), &_bHavePending)  ); \
	VERIFY_COND( "wd.json:pending is clean", (_bHavePending == SG_FALSE) );		)

#define VERIFY_WD_JSON_PENDING_DIRTY(pCtx, pPathWorkingDir)				\
	SG_STATEMENT(														\
	SG_bool _bHavePending;												\
	VERIFY_ERR_CHECK(  _ut_pt__get_wd_dot_json_stats((pCtx), (pPathWorkingDir), &_bHavePending)  ); \
	VERIFY_COND( "wd.json:pending is dirty", (_bHavePending == SG_TRUE) );		)


//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_UNITTESTS_PENDINGTREE_H
