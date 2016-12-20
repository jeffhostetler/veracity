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
 * @file sg_wc__checkout.c
 *
 * @details Deal with a CHECKOUT.  Creating a WD (in either a new
 * directory or an existing empty directory) and populating it with
 * the contents of a changeset.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include <sg_vv2__public_typedefs.h>
#include <sg_vv2__public_prototypes.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * The root directory ("@/") was created way back at the beginning
 * of the CHECKOUT (so that .sgdrawer and friends could be created),
 * but that code didn't have the contents/details of the TreeNodeEntry
 * for it so we didn't take time to completely populate it.
 *
 * Here we finish that.
 *
 * Also, return the alias of the root directory.
 *
 */
static void _finish_populate_of_root_directory(SG_context * pCtx,
											   SG_wc_tx * pWcTx,
											   SG_uint64 * puiAliasGid_Root)
{
	SG_uint64 uiAliasGid_Root;
	sg_wc_db__tne_row * pTneRow_Root = NULL;
	SG_bool bFound;

	SG_ERR_CHECK(  sg_wc_db__tne__get_alias_of_root(pCtx, pWcTx->pDb, pWcTx->pCSetRow_Baseline,
													&uiAliasGid_Root)  );
	SG_ERR_CHECK(  sg_wc_db__tne__get_row_by_alias(pCtx, pWcTx->pDb, pWcTx->pCSetRow_Baseline,
												   uiAliasGid_Root,
												   &bFound, &pTneRow_Root)  );
	SG_ASSERT( (bFound) );

	// TODO 2011/10/19 I'm going to assume that there are no
	// TODO            ATTRBITS defined on a directory.
	// TODO            Currently, we only define __FILE_X bit.

	*puiAliasGid_Root = uiAliasGid_Root;

fail:
	SG_WC_DB__TNE_ROW__NULLFREE(pCtx, pTneRow_Root);
}

//////////////////////////////////////////////////////////////////

static sg_wc_db__tne__foreach_cb _mark_sparse__cb;

/**
 * Mark the item as SPARSE rather than populate it.
 * This adds a row to the tbl_PC with the SPARSE bit
 * set.  Other than that bit, it should be exactly
 * the same as the baseline version.
 *
 * WARNING: This routine DOES NOT know about any of
 * the readdir/scandir/liveview cacheing.  Nor does
 * it know about the QUEUE/APPLY phases.
 *
 */
static void _mark_item_sparse(SG_context * pCtx, 
							  SG_wc_tx * pWcTx,
							  sg_wc_db__tne_row * pTneRow)
{
	sg_wc_db__pc_row * pPcRow = NULL;

#if TRACE_WC_CHECKOUT
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "Checkout: mark_item_sparse: %s\n",
							   pTneRow->p_s->pszEntryname)  );
#endif

	SG_ERR_CHECK(  SG_WC_DB__PC_ROW__ALLOC__FROM_TNE_ROW(pCtx, &pPcRow, pTneRow, SG_TRUE)  );

	SG_ASSERT( (pPcRow->flags_net == SG_WC_DB__PC_ROW__FLAGS_NET__SPARSE) );
	SG_ERR_CHECK(  sg_wc_db__pc__insert_self_immediate(pCtx, pWcTx->pDb, pWcTx->pCSetRow_Baseline, pPcRow)  );

	if ((pTneRow->p_s->tneType) == SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		SG_ERR_CHECK(  sg_wc_db__tne__foreach_in_dir_by_parent_alias(pCtx,
																	 pWcTx->pDb,
																	 pWcTx->pCSetRow_Baseline,
																	 pTneRow->p_s->uiAliasGid,
																	 _mark_sparse__cb,
																	 (void *)pWcTx)  );
	}

fail:
	SG_WC_DB__PC_ROW__NULLFREE(pCtx, pPcRow);
}

static void _mark_sparse__cb(SG_context * pCtx,
							 void * pVoidData,
							 sg_wc_db__tne_row ** ppTneRow)
{
	SG_wc_tx * pWcTx = (SG_wc_tx *)pVoidData;

	SG_ERR_CHECK(  _mark_item_sparse(pCtx, pWcTx, (*ppTneRow))  );


fail:
	return;
}

//////////////////////////////////////////////////////////////////

static sg_wc_db__tne__foreach_cb _populate__cb;

// TODO 2011/10/19 deal with Portability collider in each directory.

struct _populate_data
{
	SG_wc_tx * pWcTx;					// we do not own this (this is inherited/shared from the top)
	SG_file_spec * pFilespec;			// we do not own this (this is inherited/shared from the top)
	const SG_pathname * pPathDir;		// we do not own this
	SG_wc_port * pPort;
};

static void _dive_into_dir(SG_context * pCtx, 
						   SG_wc_tx * pWcTx,
						   SG_file_spec * pFilespec,
						   const SG_pathname * pPath,
						   SG_uint64 uiAliasGid)
{
	struct _populate_data dirData;
	
	dirData.pWcTx               = pWcTx;
	dirData.pFilespec           = pFilespec;
	dirData.pPathDir            = pPath;
	dirData.pPort               = NULL;

	// before we start looking at the contents of the directory
	// create a Portability Collider and accumulate entrynames
	// as we walk the directory.  If we encounter a name that
	// cannot be created on this platform, we can mark it sparse.
	SG_ERR_CHECK(  sg_wc_db__create_port(pCtx, pWcTx->pDb, &dirData.pPort)  );

	SG_ERR_CHECK(  sg_wc_db__tne__foreach_in_dir_by_parent_alias(pCtx,
																 pWcTx->pDb,
																 pWcTx->pCSetRow_Baseline,
																 uiAliasGid,
																 _populate__cb,
																 &dirData)  );

	// TODO 2011/10/20 Should we return the number of items
	// TODO            that had issues and/or had to be made
	// TODO            sparse?

fail:
	SG_WC_PORT_NULLFREE(pCtx, dirData.pPort);
}

static void _populate_dir(SG_context * pCtx, 
						  struct _populate_data * pDirData,
						  const SG_pathname * pPath,
						  sg_wc_db__tne_row * pTneRow)
{
	SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPath)  );

	// TODO 2011/10/19 I'm going to assume that there are no
	// TODO            ATTRBITS defined on a directory.
	// TODO            Currently, we only define __FILE_X bit.

	SG_ERR_CHECK(  _dive_into_dir(pCtx,
								  pDirData->pWcTx,
								  pDirData->pFilespec,
								  pPath,
								  pTneRow->p_s->uiAliasGid)  );

fail:
	return;
}

static void _populate_file(SG_context * pCtx, 
						   struct _populate_data * pDirData,
						   const SG_pathname * pPath,
						   sg_wc_db__tne_row * pTneRow)
{
	SG_file * pFile = NULL;
	SG_fsobj_perms perms = 0;

	// TODO 2011/10/19 I'm going to assume for now that all of the defined
	// TODO            ATTRBITS can be used during a SG_file__open__pathname()
	// TODO            and don't require further fix-up after the file is created.
	// TODO            Currently, we only define the __FILE_X (u+x) bit.

	SG_ERR_CHECK(  SG_wc_attrbits__compute_perms_for_new_file_from_attrbits(pCtx,
																			pDirData->pWcTx->pDb->pAttrbitsData,
																			pTneRow->p_d->attrbits,
																			&perms)  );

#if TRACE_WC_ATTRBITS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
				   "Checkout: populate_file (0x%x --> 0%o) %s\n",
				   ((SG_uint32)pTneRow->p_d->attrbits),
				   ((SG_uint32)perms),
				   SG_pathname__sz(pPath))  );
#endif

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath, SG_FILE_RDWR | SG_FILE_CREATE_NEW, perms, &pFile)  );
	SG_ERR_CHECK(  SG_repo__fetch_blob_into_file(pCtx,
												 pDirData->pWcTx->pDb->pRepo,
												 pTneRow->p_d->pszHid,
												 pFile,
												 NULL)  );
	SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

	// We DO NOT record anything in the Timestamp Cache
	// because of the clock blurr problem.

	return;

fail:
	SG_ERR_IGNORE(  SG_file__close(pCtx, &pFile)  );
}

#if !defined(WINDOWS)
static void _populate_symlink(SG_context * pCtx, 
							  struct _populate_data * pDirData,
							  const SG_pathname * pPath,
							  sg_wc_db__tne_row * pTneRow)
{
	SG_string * pStringLink = NULL;
	SG_byte * pBytes = NULL;
	SG_uint64 iLenBytes = 0;

	SG_ERR_CHECK(  SG_repo__fetch_blob_into_memory(pCtx,
												   pDirData->pWcTx->pDb->pRepo,
												   pTneRow->p_d->pszHid,
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

/**
 * We get called once for each tne-row in the baseline version
 * of the directory that we are iterating over.
 *
 * We have the option of stealing the pTneRow or letting the
 * foreach iterator free it.
 *
 */
static void _populate__cb(SG_context * pCtx,
						  void * pVoidData,
						  sg_wc_db__tne_row ** ppTneRow)
{
	struct _populate_data * pDirData = (struct _populate_data *)pVoidData;
	SG_pathname * pPath = NULL;
	SG_string * pStringRepoPath = NULL;
	const SG_string * pStringItemLog = NULL;	// we do not own this
	SG_wc_port_flags portFlags;	
	SG_bool bIsDuplicate;

#if defined(DEBUG)
	// Force a failure while we are populating the WD in
	// order to test the CHECKOUT cleanup/recovery code.
	// See st_wc_checkout_W7885.js
#	define DEBUG_CHECKOUT_FAILURE__E1 "gf2e335b610fa4318a6f5e139230f955c064847bed0fc11e19b53002500da2b78.test"
	if (strcmp( (*ppTneRow)->p_s->pszEntryname, DEBUG_CHECKOUT_FAILURE__E1) == 0)
		SG_ERR_THROW2(  SG_ERR_DEBUG_1, (pCtx, "Checkout: %s", DEBUG_CHECKOUT_FAILURE__E1)  );
#endif

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath,
												   pDirData->pPathDir,
												   (*ppTneRow)->p_s->pszEntryname)  );
#if TRACE_WC_CHECKOUT
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "Checkout: populate looking at: [tneType %d] %s\n",
							   (*ppTneRow)->p_s->tneType,
							   SG_pathname__sz(pPath))  );
#endif

	if (pDirData->pFilespec)
	{
		// See if this item is to be omitted as part of a sparse checkout.

		SG_file_spec__match_result eval;
		SG_bool bOmit;

		SG_ERR_CHECK(  sg_wc_db__path__absolute_to_repopath(pCtx, pDirData->pWcTx->pDb,
															pPath, &pStringRepoPath)  );
		SG_ERR_CHECK(  SG_file_spec__should_include(pCtx, pDirData->pFilespec,
													SG_string__sz(pStringRepoPath),
													SG_FILE_SPEC__NO_INCLUDES
													|SG_FILE_SPEC__NO_IGNORES
													|SG_FILE_SPEC__NO_RESERVED,
													&eval)  );
		bOmit = (eval == SG_FILE_SPEC__RESULT__EXCLUDED);
		if (bOmit)
		{
#if TRACE_WC_CHECKOUT
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "Checkout: exclude subtree starting at: %s\n",
									   SG_string__sz(pStringRepoPath))  );
#endif
			SG_ERR_CHECK(  _mark_item_sparse(pCtx, pDirData->pWcTx, (*ppTneRow))  );
			goto done;
		}
	}

	// See if this item's entryname has portability problems.
	// (In the normal case, this will be no.  But they could
	// have turned down the portability mask on a Linux machine
	// and created "foo" and "FOO" in the same directory and
	// did a COMMIT.  Then turn the portability mask back to
	// the default and do a CHECKOUT on a Windows machine.
	// Far fetched, but the alternative is to throw up.
	//
	// There are 3 cases:
	// [1] a hard collision -- a duplicate name -- this should
	//     never happen no matter how much they play with the
	//     portability masks.
	// [2] a problematic entryname -- such as 'COM1' -- mark
	//     it sparse.
	// [3] a potential collision -- such as 'foo' and 'FOO'
	//     -- make the second one sparse.
	//
	// TODO 2011/10/20 Should we use the LOG mechanism to
	// TODO            write a note for the things we make
	// TODO            sparse because we can't/shouldn't
	// TODO            create them on this platform?
	// TODO
	// TODO            Or maybe return the number of such issues?
	//
	// TODO 2012/07/13 See W2050 -- consider treating like a (merge)
	// TODO                         conflict, do the rename, and
	// TODO                         create an ISSUE for RESOLVE to
	// TODO                         deal with.

	SG_ERR_CHECK(  SG_wc_port__add_item(pCtx, pDirData->pPort, NULL,
										(*ppTneRow)->p_s->pszEntryname,
										(*ppTneRow)->p_s->tneType,
										&bIsDuplicate)  );
	if (bIsDuplicate)	// [1]
	{
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "Duplcate entryname '%s' during checkout. Should not happen. %s",
						 (*ppTneRow)->p_s->pszEntryname,
						 SG_pathname__sz(pPath))  );
	}
	SG_ERR_CHECK(  SG_wc_port__get_item_result_flags(pCtx, pDirData->pPort,
													 (*ppTneRow)->p_s->pszEntryname,
													 &portFlags, &pStringItemLog)  );
	if (portFlags)		// [2] and [3]
	{
#if TRACE_WC_CHECKOUT
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   ("Checkout: problematic entryname: '%s'\n"
									"          Marking sparse: '%s'\n"
									"%s"),
								   (*ppTneRow)->p_s->pszEntryname,
								   SG_pathname__sz(pPath),
								   SG_string__sz(pStringItemLog))  );
#endif
		SG_ERR_CHECK(  _mark_item_sparse(pCtx, pDirData->pWcTx, (*ppTneRow))  );
		goto done;
	}

	switch ((*ppTneRow)->p_s->tneType)
	{
	case SG_TREENODEENTRY_TYPE_DIRECTORY:
		SG_ERR_CHECK(  _populate_dir(pCtx, pDirData, pPath, (*ppTneRow))  );
		break;

	case SG_TREENODEENTRY_TYPE_REGULAR_FILE:
		SG_ERR_CHECK(  _populate_file(pCtx, pDirData, pPath, (*ppTneRow))  );
		break;

	case SG_TREENODEENTRY_TYPE_SYMLINK:
#if defined(WINDOWS)
#if TRACE_WC_CHECKOUT
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "Checkout: Marking symlink sparse: '%s'\n",
								   SG_pathname__sz(pPath))  );
#endif
		SG_ERR_CHECK(  _mark_item_sparse(pCtx, pDirData->pWcTx, (*ppTneRow))  );
#else
		SG_ERR_CHECK(  _populate_symlink(pCtx, pDirData, pPath, (*ppTneRow))  );
#endif
		break;

	case SG_TREENODEENTRY_TYPE_SUBMODULE:
	default:
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "Unknown item type '%d' for '%s'.",
						 (*ppTneRow)->p_s->tneType,
						 SG_pathname__sz(pPath))  );
		break;
	}

done:
	;
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

/**
 * During a full CHECKOUT we can cut some corners and just do
 * it directly:
 * 
 * [] we can simply walk the tne_L0 table (or we could use the
 *    treenodes again)
 * [] we don't have to mess with readdir/prescan/tbl_PC and
 *    LVD/LVI (since they don't exist yet)
 * [] and we don't have to worry about any already-pended (pre-TX)
 *    structural changes to the tree
 *
 * For portability reasons, we mark anything that we can't create
 * on this platform as SPARSE.  For example:
 * [] symlinks on Windows
 * [] files named 'com1' on Windows
 * [] items that lose a case collision on Windows and Mac
 * These are only possible if they alter the portability mask on
 * the repo.  (Suppose on a Linux system they turned off some of
 * the Windows constraint bits and ADDED and COMMITTED
 *     "@/foo"
 *     "@/FOO"
 *     "@/COM1"
 * then tried to do a CHECKOUT on a Windows system; rather than
 * aborting the checkout, we leave the things we can't create as
 * SPARSE.)
 *
 * TODO 2011/10/19 It could be argued that we should use the
 * TODO            rename-trick "@/foo~L0~g123456~" and etc
 * TODO            (like we do in MERGE) for things that have
 * TODO            a simple filename issue, rather leaving
 * TODO            them SPARSE.  I could go either way on it.
 * TODO
 * TODO            For now, I'm doing it this way to be consistent
 * TODO            with what we do for things like symlinks.
 *
 * TODO 2012/07/13 See W2050. Consider letting RESOLVE handle it.
 *
 * We are given an OPTIONAL pvaSparse.
 * [] If null, we create a full non-sparse tree.
 * [] If non-null, we consider it a list of things to exclude
 *    (mark sparse) (not populate).
 *
 */
static void _populate_from_root(SG_context * pCtx, SG_wc_tx * pWcTx,
								SG_file_spec * pFilespec)
{
	SG_uint64 uiAliasGid_Root = 0;	// init only to quiet compiler

#if TRACE_WC_CHECKOUT
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "Checkout: beginning populate_from_root: %s\n",
							   SG_pathname__sz(pWcTx->pDb->pPathWorkingDirectoryTop))  );
#endif

	SG_ERR_CHECK(  _finish_populate_of_root_directory(pCtx, pWcTx, &uiAliasGid_Root)  );
	
	// dive into the tree.  we do not have any depth limits here.

	SG_ERR_CHECK(  _dive_into_dir(pCtx, pWcTx, pFilespec,
								  pWcTx->pDb->pPathWorkingDirectoryTop,
								  uiAliasGid_Root)  );

#if TRACE_WC_CHECKOUT
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "Checkout: finished populate_from_root: %s\n",
							   SG_pathname__sz(pWcTx->pDb->pPathWorkingDirectoryTop))  );
#endif

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Checkout everything in this tree proper.
 *
 */
static void _sg_wc__checkout_proper(SG_context * pCtx,
									const char * pszRepoName,
									const char * pszPath,
									const char * pszHidCSet,
									const char * pszBranchNameChosen,
									const SG_varray * pvaSparse)
{
	SG_wc_tx * pWcTx = NULL;
	SG_file_spec * pFilespec = NULL;

	// Create the WD root, the drawer, and the SQL DB.
	// Load the given CSET into the tne_L0 table.
	// 
	// We DO NOT create a timestamp cache at this time
	// because of clock blurr problem.  (We can't trust
	// mtime until about 2 seconds after we create the
	// files.)
	
	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN__CREATE(pCtx, &pWcTx, pszRepoName, pszPath,
												  SG_TRUE, pszHidCSet)  );

	if (!pWcTx->pDb->pAttrbitsData)
		SG_ERR_CHECK(  SG_wc_attrbits__get_masks_from_config(pCtx,
															 pWcTx->pDb->pRepo,
															 pWcTx->pDb->pPathWorkingDirectoryTop,
															 &pWcTx->pDb->pAttrbitsData)  );

	if (pvaSparse)
		SG_ERR_CHECK(  SG_wc_sparse__build_pattern_list(pCtx,
														pWcTx->pDb->pRepo,
														pszHidCSet,
														pvaSparse,
														&pFilespec)  );

	SG_ERR_CHECK(  _populate_from_root(pCtx, pWcTx, pFilespec)  );

	if (pszBranchNameChosen)
	{
		// We have already normalized/validated/verified
		// the chosen name, so we don't need to do it again
		// so we use the db layer rather than the TX layer.
		//
		// I want to keep the above code doing the work
		// because I want the validation to happen *before*
		// we populate the tree.

		SG_ERR_CHECK(  sg_wc_db__branch__attach(pCtx, pWcTx->pDb, pszBranchNameChosen,
												SG_VC_BRANCHES__CHECK_ATTACH_NAME__DONT_CARE, SG_FALSE)  );
	}

	// commit the TX at this point.  we have everything
	// in this WD-proper that we want.

	SG_ERR_CHECK(  SG_wc_tx__apply(pCtx, pWcTx)  );
	SG_ERR_CHECK(  SG_wc_tx__free(pCtx, pWcTx)  );
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
	return;

fail:
	SG_FILE_SPEC_NULLFREE(pCtx, pFilespec);
	SG_ERR_IGNORE(  SG_wc_tx__abort_create_and_free(pCtx, &pWcTx)  );
}

//////////////////////////////////////////////////////////////////

/**
 * Do a CHECKOUT -- creating a new working copy.
 *
 * WARNING: This routine deviates from the model of the other SG_wc__
 * WARNING: routines because the WD does not yet exist (and we haven't
 * WARNING: yet created .sgdrawer).  The caller may have already created
 * WARNING: an empty directory for us, but that is it.
 *
 * WARNING: This routine also deviates in that we *ONLY* provide a wc8api
 * WARNING: version and *NOT* a wc7txapi version.  Likewise we only provide
 * WARNING: a sg.wc.checkout() version and not a sg_wc_tx.checkout() version.
 * WARNING: This is because I want it to be a complete self-contained
 * WARNING: operation (and I want to hide the number of SQL TXs that are
 * WARNING: required to get everything set up).
 *
 * Also, the given path (to the new WD root) can be an absolute or
 * relative path or NULL (we'll substitute the CWD).  Unlike the other
 * API routines, it cannot be a repo-path.
 *
 * This directory, if it exists, must be empty.  If it does not exist
 * it will be created.
 * 
 */
void SG_wc__checkout(SG_context * pCtx,
					 const char * pszRepoName,
					 const char * pszPath,
					 const SG_rev_spec * pRevSpec,
					 const char * pszAttach,
					 const char * pszAttachNew,
					 const SG_varray * pvaSparse)
{
	char * pszHidCSet = NULL;
	char * pszBranchNameObserved = NULL;
	char * pszNormalizedAttach = NULL;
	char * pszNormalizedAttachNew = NULL;
	const char * pszBranchNameChosen = NULL;	// we do not own this

	SG_NONEMPTYCHECK_RETURN( pszRepoName );		// repo-name is required
	// pszPath is optional; default to CWD.
	SG_NULLARGCHECK_RETURN( pRevSpec );
	// pszAttach is optional
	// pszAttachNew is optional
	// pvaSparse is optional

	// Open the given repo (by name) in the closet and
	// interpret contents of pRevSpec and see if we can
	// uniquely identify a changeset.  This also returns
	// the branch name if the HID is a head.

	SG_ERR_CHECK(  SG_vv2__find_cset(pCtx, pszRepoName, pRevSpec,
									 &pszHidCSet, &pszBranchNameObserved)  );

	if (pszAttach && pszAttachNew)
	{
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Cannot combine 'attach' and 'attach-new' options.")  );
	}
	else if (pszAttach)
	{
		// To avoid user confusion and/or accidents, we require
		// a --rev or --tag with --attach.

		SG_uint32 countBranches;

		SG_ERR_CHECK(  SG_rev_spec__count_branches(pCtx, pRevSpec, &countBranches)  );
		if (countBranches > 0)
			SG_ERR_THROW2(  SG_ERR_INVALIDARG,
							(pCtx, "When using the 'attach' option, you cannot also use 'branch'.")  );

		if (SG_rev_spec__is_empty(pCtx, pRevSpec))
			SG_ERR_THROW2(  SG_ERR_INVALIDARG,
							(pCtx, "When using the 'attach' option, you must also specify a 'revision' or 'tag'.")  );

		// Normalize the requested attach name and make sure
		// that the branch exists.
		SG_ERR_CHECK(  SG_vv2__check_attach_name(pCtx, pszRepoName, pszAttach,
												 SG_TRUE,  // named branch must exist for attach
												 &pszNormalizedAttach)  );
		pszBranchNameChosen = pszNormalizedAttach;
	}
	else if (pszAttachNew)
	{
		// I think we can allow "--branch foo --attach-new bar".

		// Normalize the requested attach-new name and make sure
		// that it doesn't already exist.
		SG_ERR_CHECK(  SG_vv2__check_attach_name(pCtx, pszRepoName, pszAttachNew,
												 SG_FALSE, // named branch must not exist yet
												 &pszNormalizedAttachNew)  );
		pszBranchNameChosen = pszNormalizedAttachNew;
	}
	else // default to branch associated with head, if set.
	{
		pszBranchNameChosen = pszBranchNameObserved;
	}
	
	SG_ERR_CHECK(  _sg_wc__checkout_proper(pCtx, pszRepoName, pszPath, 
										   pszHidCSet, pszBranchNameChosen,
										   pvaSparse)  );

fail:
	SG_NULLFREE(pCtx, pszHidCSet);
	SG_NULLFREE(pCtx, pszBranchNameObserved);
	SG_NULLFREE(pCtx, pszNormalizedAttach);
	SG_NULLFREE(pCtx, pszNormalizedAttachNew);
}
