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
 * @file sg_vv2__export.c
 *
 * @details The new "vv2" version of "export".  This version has been
 * split out of the original sg_vv_verbs.c header file and it is
 * aware of the WC re-write of PendingTree.
 * 
 * Handle EXPORT command to create a non-controlled view of a VC tree.
 *
 * WE DO NOT ASSUME THAT A WD IS PRESENT.
 * 
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

// We ONLY need _WC_ for the portability routines.
#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

struct _export_dive_data
{
	const SG_pathname * pPathRootDir;
	SG_repo * pRepo;
	SG_file_spec * pFilespec;
	SG_wc_attrbits_data * pAttrbitsData;
	SG_wc_port_flags portMask;
};

typedef struct _export_dive_data export_dive_data;

//////////////////////////////////////////////////////////////////

static void _populate__cb(SG_context * pCtx,
						  export_dive_data * pData,
						  const SG_pathname * pPathDir,
						  SG_wc_port * pPortDir,
						  const char * pszGid_k,
						  const SG_treenode_entry * pTne_k);

static void _dive_into_dir(SG_context * pCtx,
						  export_dive_data * pData,
						  const SG_pathname * pPathDir,
						  const char * pszHidContentDir)
{
	SG_uint32 k, count;
	SG_treenode * pTnDir = NULL;
	SG_wc_port * pPortDir = NULL;

	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pData->pRepo, pszHidContentDir, &pTnDir)  );
	SG_ERR_CHECK(  SG_treenode__count(pCtx, pTnDir, &count)  );

	if (count > 0)
		SG_ERR_CHECK(  SG_wc_port__alloc(pCtx, pData->pRepo, NULL, pData->portMask, &pPortDir)  );

	for (k=0; k<count; k++)
	{
		const char * pszGid_k;
		const SG_treenode_entry * pTne_k;

		SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, pTnDir, k, &pszGid_k, &pTne_k)  );
		SG_ERR_CHECK(  _populate__cb(pCtx, pData, pPathDir, pPortDir, pszGid_k, pTne_k)  );
	}

fail:
	SG_TREENODE_NULLFREE(pCtx, pTnDir);
	SG_WC_PORT_NULLFREE(pCtx, pPortDir);
}

static void _populate_dir(SG_context * pCtx,
						  export_dive_data * pData,
						  const SG_pathname * pPath,
						  const SG_treenode_entry * pTne)
{
	const char * pszHidContent;

	SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPath)  );

	// TODO 2011/10/19 I'm going to assume that there are no
	// TODO            ATTRBITS defined on a directory.
	// TODO            Currently, we only define __FILE_X bit.

	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pTne, &pszHidContent)  );
	SG_ERR_CHECK(  _dive_into_dir(pCtx, pData, pPath, pszHidContent)  );

fail:
	return;
}

static void _populate_file(SG_context * pCtx,
						   export_dive_data * pData,
						   const SG_pathname * pPath,
						   const SG_treenode_entry * pTne)
{
	const char * pszHidContent;
	SG_int64 attrbits = 0;
	SG_file * pFile = NULL;
	SG_fsobj_perms perms = 0;

	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pTne, &pszHidContent)  );
    SG_ERR_CHECK(  SG_treenode_entry__get_attribute_bits(pCtx, pTne, &attrbits)  );

	// TODO 2011/10/19 I'm going to assume for now that all of the defined
	// TODO            ATTRBITS can be used during a SG_file__open__pathname()
	// TODO            and don't require further fix-up after the file is created.
	// TODO            Currently, we only define the __FILE_X (u+x) bit.

	SG_ERR_CHECK(  SG_wc_attrbits__compute_perms_for_new_file_from_attrbits(pCtx,
																			pData->pAttrbitsData,
																			attrbits,
																			&perms)  );

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_RDWR | SG_FILE_CREATE_NEW, perms, &pFile)  );
	SG_ERR_CHECK(  SG_repo__fetch_blob_into_file(pCtx,
												 pData->pRepo,
												 pszHidContent,
												 pFile,
												 NULL)  );
	SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

	return;

fail:
	SG_ERR_IGNORE(  SG_file__close(pCtx, &pFile)  );
}

#if !defined(WINDOWS)
static void _populate_symlink(SG_context * pCtx, 
							  export_dive_data * pData,
							  const SG_pathname * pPath,
							  const SG_treenode_entry * pTne)
{
	const char * pszHidContent;
	SG_string * pStringLink = NULL;
	SG_byte * pBytes = NULL;
	SG_uint64 iLenBytes = 0;

	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pTne, &pszHidContent)  );
	SG_ERR_CHECK(  SG_repo__fetch_blob_into_memory(pCtx,
												   pData->pRepo,
												   pszHidContent,
												   &pBytes,
												   &iLenBytes)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__BUF_LEN(pCtx, &pStringLink, pBytes, (SG_uint32)iLenBytes)  );
	SG_ERR_CHECK(  SG_fsobj__symlink(pCtx, pStringLink, pPath)  );

	// TODO 2011/10/19 I'm going to assume that there are no
	// TODO            ATTRBITS defined on a symlink.
	// TODO            Currently, we only define __FILE_X bit.

fail:
	SG_STRING_NULLFREE(pCtx, pStringLink);
	SG_NULLFREE(pCtx, pBytes);
}
#endif

	
static void _populate__cb(SG_context * pCtx,
						  export_dive_data * pData,
						  const SG_pathname * pPathDir,
						  SG_wc_port * pPortDir,
						  const char * pszGid_k,
						  const SG_treenode_entry * pTne_k)
{
	const char* pszName_k;
	SG_pathname * pPath_k = NULL;
	SG_treenode_entry_type tneType_k;
	SG_string * pStringRepoPath = NULL;
	const SG_string * pStringItemLog = NULL;	// we do not own this
	SG_wc_port_flags portFlags;	
	SG_bool bIsDuplicate;

	SG_UNUSED( pszGid_k );

	SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pTne_k, &pszName_k)  );
	SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pTne_k, &tneType_k)  );

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_k, pPathDir, pszName_k)  );

#if TRACE_VV2_EXPORT
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "Export: looking at: [tneType %d] %s\n",
							   tneType_k,
							   SG_pathname__sz(pPath_k))  );
#endif

	if (pData->pFilespec)
	{
		// See if this item is to be omitted as part of a sparse export.

		SG_file_spec__match_result eval;
		SG_bool bOmit;

		SG_ERR_CHECK(  SG_workingdir__wdpath_to_repopath(pCtx, pData->pPathRootDir, pPath_k, SG_FALSE,
														 &pStringRepoPath)  );
		SG_ERR_CHECK(  SG_file_spec__should_include(pCtx, pData->pFilespec,
													SG_string__sz(pStringRepoPath),
													SG_FILE_SPEC__NO_INCLUDES
													|SG_FILE_SPEC__NO_IGNORES
													|SG_FILE_SPEC__NO_RESERVED,
													&eval)  );
		bOmit = (eval == SG_FILE_SPEC__RESULT__EXCLUDED);
		if (bOmit)
		{
#if TRACE_VV2_EXPORT
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "Export: Omitting subtree starting at: %s\n",
									   SG_string__sz(pStringRepoPath))  );
#endif
			goto done;
		}
	}

	// see if we would have a portability problem creating this item

	SG_ERR_CHECK(  SG_wc_port__add_item(pCtx, pPortDir, NULL, pszName_k, tneType_k, &bIsDuplicate)  );
	if (bIsDuplicate)
	{
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "Duplicate entryname '%s' during export.  Should not happen. %s",
						 pszName_k, SG_pathname__sz(pPath_k))  );
	}
	SG_ERR_CHECK(  SG_wc_port__get_item_result_flags(pCtx, pPortDir, pszName_k,
													 &portFlags, &pStringItemLog)  );
	if (portFlags)
	{
#if TRACE_VV2_EXPORT
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   ("Export: problematic entryname: '%s'\n"
									"        Omitting subtree starting at: '%s'\n"
									"%s"),
								   pszName_k,
								   SG_pathname__sz(pPath_k),
								   SG_string__sz(pStringItemLog))  );
#endif
		goto done;
	}

	switch (tneType_k)
	{
	case SG_TREENODEENTRY_TYPE_DIRECTORY:
		SG_ERR_CHECK(  _populate_dir(pCtx, pData, pPath_k, pTne_k)  );
		break;

	case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
		SG_ERR_CHECK(  _populate_file(pCtx, pData, pPath_k, pTne_k)  );
		break;

	case SG_TREENODEENTRY_TYPE_SYMLINK:
#if defined(WINDOWS)
#if TRACE_VV2_EXPORT
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "Export: Omitting symlink: '%s'\n",
								   SG_pathname__sz(pPath_k))  );
#endif
#else
		SG_ERR_CHECK(  _populate_symlink(pCtx, pData, pPath_k, pTne_k)  );
#endif
		break;

	case SG_TREENODEENTRY_TYPE_SUBMODULE:
	default:
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "Unknown item type '%d' for '%s'.",
						 tneType_k,
						 SG_pathname__sz(pPath_k))  );
		break;
	}

done:
	;
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath_k);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

static void _populate_from_root(SG_context * pCtx, 
								export_dive_data * pData,
								const SG_pathname * pPathRootDir,
								const SG_treenode_entry * pTneRoot)
{
	SG_bool bExists;
	const char * pszHidContentRoot;
	
	SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathRootDir, &bExists, NULL, NULL)  );
	if (!bExists)
		SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathRootDir)  );

	// TODO 2011/10/21 I'm going to assume that there are no ATTRBITS
	// TODO            defined on a directory.
	// TODO            Currently, we only define __FILE_X bit.

	// populate the contents of the directory.

	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pTneRoot, &pszHidContentRoot)  );
	SG_ERR_CHECK(  _dive_into_dir(pCtx, pData, pPathRootDir, pszHidContentRoot)  );

fail:
	return;
}

/**
 * Export the contents of this repo as of this cset
 * into the given pathname.
 * 
 */
static void _do_export_proper(SG_context * pCtx,
							  const char * pszRepoName,
							  const SG_pathname * pPathRootDir,
							  const char * pszHidCSet,
							  const SG_varray * pvaSparse)
{
	char * pszHidSuperRoot = NULL;
	SG_treenode * pTnSuperRoot = NULL;
	const SG_treenode_entry * pTneRoot;
	export_dive_data data;

#if TRACE_VV2_EXPORT
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "Export: [cset %s][path %s]\n",
								   pszHidCSet, SG_pathname__sz(pPathRootDir))  );
#endif

	memset(&data, 0, sizeof(data));
	data.pPathRootDir = pPathRootDir;

	// get the HID of the content of the "@/" root directory.

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, &data.pRepo)  );
	SG_ERR_CHECK(  SG_wc_port__get_mask_from_config(pCtx, data.pRepo, NULL, &data.portMask)  );
	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, data.pRepo, pszHidCSet, &pszHidSuperRoot)  );
	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, data.pRepo, pszHidSuperRoot, &pTnSuperRoot)  );
	SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, pTnSuperRoot, 0, NULL, &pTneRoot)  );

	SG_ERR_CHECK(  SG_wc_attrbits__get_masks_from_config(pCtx, data.pRepo, NULL, &data.pAttrbitsData)  );

	if (pvaSparse)
		SG_ERR_CHECK(  SG_wc_sparse__build_pattern_list(pCtx, data.pRepo, pszHidCSet, pvaSparse, &data.pFilespec)  );

	// Rather than calling _populate_dir() on the root directory,
	// we special case its construction because we need to ignore
	// the "@/" and use the given pathname as the location of the
	// root directory.

	SG_ERR_CHECK(  _populate_from_root(pCtx, &data, pPathRootDir, pTneRoot)  );

fail:
	SG_REPO_NULLFREE(pCtx, data.pRepo);
	SG_NULLFREE(pCtx, pszHidSuperRoot);
	SG_TREENODE_NULLFREE(pCtx, pTnSuperRoot);
	SG_WC_ATTRBITS_DATA__NULLFREE(pCtx, data.pAttrbitsData);
	SG_FILE_SPEC_NULLFREE(pCtx, data.pFilespec);
}

//////////////////////////////////////////////////////////////////

void SG_vv2__export(SG_context * pCtx,
					const char * pszRepoName,
					const char * pszFolder,
					const SG_rev_spec * pRevSpec,
					const SG_varray * pvaSparse)
{
	SG_pathname * pPathAllocated = NULL;
	char * pszHidCSet = NULL;

	// pszRepoName is optional (defaults to WD if present).
	// pszFolder is optional and defaults to CWD.
	SG_NULLARGCHECK_RETURN( pRevSpec );
	// pvaSparse is optional

	// Either pszRepoName or pszFolder must be given.
	if ((!pszRepoName || !*pszRepoName) && (!pszFolder || !*pszFolder))
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Either 'repo' or 'folder' must be specified.")  );

	// convert given folder to a pathname; assume CWD if null.
	SG_ERR_CHECK(  SG_vv_utils__folder_arg_to_path(pCtx, pszFolder, &pPathAllocated)  );

	// when exporting an arbitrary version we require either a new or
	// empty target directory so that we are not trashing something.
	SG_ERR_CHECK(  SG_vv_utils__verify_dir_empty(pCtx, pPathAllocated)  );

	// Open the given repo (by name) in the closet and
	// interpret contents of pRevSpec and see if we can
	// uniquely identify a changeset.

	SG_ERR_CHECK(  SG_vv2__find_cset(pCtx, pszRepoName, pRevSpec, &pszHidCSet, NULL)  );

	SG_ERR_CHECK(  _do_export_proper(pCtx, pszRepoName, pPathAllocated, pszHidCSet, pvaSparse)  );

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathAllocated);
	SG_NULLFREE(pCtx, pszHidCSet);
}
