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
 * @file sg_wc_tx__resolve__utils__inline1.h
 *
 * @details Various utility routines that I found in the PendingTree
 * version of sg_resolve.c.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RESOLVE__UTILS__INLINE1_H
#define H_SG_WC_TX__RESOLVE__UTILS__INLINE1_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Generates an indexed label using the next available index.
 */
static void _generate_indexed_label(
	SG_context*         pCtx,    //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice, //< [in] The choice that will own the value we're generating a label for.
	const char*         szBase,  //< [in] The base label name to add the next available index to.
	SG_string**         ppLabel  //< [out] The generated label, using the next available index.
	)
{
	SG_string*         sLabel     = NULL;
	SG_resolve__value* pTempValue = NULL;
	SG_uint32          uIndex     = 1u;
	SG_bool            bUsed      = SG_TRUE;

	// keep looking up values with the given base label and an ever increasing index
	// until we find an index that's not yet in use on the given choice
	while (bUsed == SG_TRUE)
	{
		// safe to cast away the choice's const here, because:
		// 1) we know that this function won't alter it
		// 2) it's only const to begin with to explicitly indicate an input instead of an output
		SG_ERR_CHECK(  SG_resolve__choice__check_value__label(pCtx, pChoice, szBase, uIndex, &pTempValue)  );
		if (pTempValue == NULL)
		{
			bUsed = SG_FALSE;
		}
		else
		{
			uIndex += 1u;
		}
	}

	// create a label from the given base and next available index we found
	if (uIndex <= 1u)
	{
		// labels with index 1 should actually not have a numeric suffix
		// This makes for a friendlier user experience in the 99% of cases
		// where there is only one value with any given base name.
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sLabel, szBase)  );
	}
	else
	{
		// labels with a higher index need to have the index appended as a suffix
		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &sLabel)  );
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, sLabel, gszIndexedLabelFormat, szBase, uIndex)  );
	}

	// return the generated label
	*ppLabel = sLabel;
	sLabel = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, sLabel);
	return;
}

/**
 * Generates a temporary filename for a mergeable value.
 * It is assumed that this file will be in its choice's temporary path.
 */
static void _generate_temp_filename(
	SG_context*         pCtx,     //< [in] [out] Error and context info.
	SG_resolve__choice* pChoice,  //< [in] The choice that owns the value we're generating a filename for.
	const char*         szFormat, //< [in] Format string to use for the filename.
	                              //<      Should contain a %s (the label) and a %u (a counter), in that order.
	                              //<      See gszFilenameFormat_*
	const char*         szLabel,  //< [in] The label for the value we're generating a filename for.
	SG_pathname**       ppPath    //< [out] Full pathname including generated temporary filename.
	)
{
	SG_string*   pFilename = NULL;
	SG_pathname* pPath     = NULL;
	SG_uint32    uCount    = 0u;
	SG_bool      bUsed     = SG_TRUE;

	SG_NULLARGCHECK(pChoice);
	SG_NULLARGCHECK(szLabel);

	// keep generating filenames with an ever increasing counter
	// until we find one that doesn't exist yet
	//
	// Note: We *CAN* use SG_pathname__ and SG_fsobj__ routines
	// Note: because we are always referring to a TEMP file and
	// Note: don't need to ask WC to manage it.

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pFilename)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC(pCtx, &pPath)  );
	while (bUsed == SG_TRUE)
	{
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pFilename, szFormat, szLabel, uCount)  );
		SG_ERR_CHECK(  SG_pathname__set__from_pathname(pCtx, pPath, pChoice->pTempPath)  );
		SG_ERR_CHECK(  SG_pathname__append__from_string(pCtx, pPath, pFilename)  );
		SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bUsed, NULL, NULL)  );
		uCount += 1u;
	}

	if (ppPath != NULL)
	{
		*ppPath = pPath;
		pPath = NULL;
	}

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
	SG_STRING_NULLFREE(pCtx, pFilename);
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Create an absolute pathname from pValue->pVariantFile.
 * Per [2] pVariantFile should contain a repo-path.
 * 
 * The resulting pathname name MUST ONLY BE USED TO READ/STAT
 * the file (or passed to diff/merge tools that will read/stat it).
 *
 * Generally, this is a TEMP file that MERGE or RESOVLE created
 * and (should be a) read-only file.  In the event that this
 * refers to a 'working' value, we need to be careful because
 * the implied LVI could have a QUEUEd OVERWRITE pending; so
 * the WC layer may lie to us and give us a temp handle to the
 * source of the overwrite.
 *
 * See [4] and W4679.
 * 
 */
static void _wc__variantFile_to_absolute(SG_context * pCtx,
										 const SG_resolve__value * pValue,
										 SG_pathname ** ppPath)
{
	const char * psz;

	// TODO 2012/04/50 See [4] and W4679.

	SG_NULLARGCHECK_RETURN( pValue );
	SG_NULLARGCHECK_RETURN( pValue->pVariantFile );
	SG_NULLARGCHECK_RETURN( ppPath );

	psz = SG_string__sz(pValue->pVariantFile);
	if ((psz[0] == '@') && (psz[1] == SG_WC__REPO_PATH_DOMAIN__G))
	{
		SG_bool bIsTmp;

		SG_ERR_CHECK(  sg_wc_liveview_item__get_proxy_file_path(pCtx,
																pValue->pChoice->pItem->pLVI,
																pValue->pChoice->pItem->pResolve->pWcTx,
																ppPath, &bIsTmp)  );
		// TODO 2012/12/14 think about propagating bIsTmp to caller.
	}
	else if (strncmp(psz, "@/.sgdrawer/", strlen("@/.sgdrawer/")) == 0)
	{
		// pVariantFile points to a TEMP file from MERGE/RESOLVE.

		SG_ERR_CHECK(  sg_wc_db__path__repopath_to_absolute(pCtx,
															pValue->pChoice->pItem->pResolve->pWcTx->pDb,
															pValue->pVariantFile,
															ppPath)  );
	}
	else
	{
		// TODO 2012/04/06 Is this case possible?

		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx, "variantFile_to_absolute: possible '%s'", psz)  );
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * SG_mergetool__select() and SG_difftool__select() want to help
 * us select the proper mergetool or difftool to use on this item.
 * They take a repo-path and a pathname. The repo-path is *only*
 * used for pattern matching and the pathname is *only* used for
 * magic-number-type sniffing.
 * 
 *
 * The repo-path is *ONLY* used to pattern matching against the
 * set of fileclasses and/or config settings.  Normally, we think
 * of the fileclasses as containing a series of "*.c", "*.gif", ...,
 * but since we use the filespec engine, they could be more than
 * that.
 *
 * We are asked to return a repo-path for the item, but don't have
 * an immediate answer:
 * [] The historical repo-paths in issue.input.* are going to go away.
 *    They were created when thinking only about MERGE and the assumption
 *    that they would be constant. But with UPDATE that isn't true.
 * [] Even if present, it is an arbitrary choice of which to use.
 *
 * I'm going to use the pItem->pLVI info to compute the live/current
 * repo-path for the item.  I think this better captures the intent
 * of any pattern they may have in the fileclasses.  That is, at the
 * time they invoke merge, use the path that it currently has in the
 * pattern match.
 *
 * However, if MERGE gave it a temp name "<entryname>~<gid7>" (and they
 * haven't yet dealt with whatever other conflicts that caused the name
 * mangling), then we want to unmangle it *just* for use in the pattern
 * match.  (We trust SG_mergetool__select() to not try to actually use
 * this repo-path to do anything, since the file won't necessarily be
 * there.)
 *
 * This *should* be more than sufficient to get a good match most of
 * the time, but it can still be defeated:  Suppose in the ancestor you
 * have "foo.txt" and in one branch it was renamed "foo.c" and in the
 * other branch it was renamed "foo.gif" -- and the content was edited
 * in both. Which tool should we choose?
 *
 */
static void _resolve__util__compute_hypothetical_repo_path_for_tool_selection(
	SG_context * pCtx,
	SG_resolve__item * pItem,
	SG_string ** ppStringRepoPath)
{
	SG_string * pStringRepoPath = NULL;
	SG_string * pStringEntryname = NULL;

	SG_ERR_CHECK(  sg_wc_tx__liveview__compute_live_repo_path(pCtx, pItem->pResolve->pWcTx, pItem->pLVI,
															  &pStringRepoPath)  );
#if TRACE_WC_RESOLVE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "HypotheticalRepoPath: input  %s\n",
							   SG_string__sz(pStringRepoPath))  );
#endif

	if (pItem->szReferenceOriginalName)
	{
		// MERGE mangled it.  (Without regard for the reasons why
		// and/or whether RESOLVE needs to RESTORE/unmangle it
		// during ACCEPT and/or whether there is a NAME-CONFLICT
		// that has been/needs to be addressed.)

		SG_ERR_CHECK(  SG_repopath__get_last(pCtx, pStringRepoPath, &pStringEntryname)  );
		if (strcmp(SG_string__sz(pStringEntryname), pItem->szMergedName) == 0)
		{
			// The file still has the mangled name that MERGE gave it.

			SG_ERR_CHECK(  SG_repopath__remove_last(pCtx, pStringRepoPath)  );
			SG_ERR_CHECK(  SG_repopath__append_entryname(pCtx, pStringRepoPath, pItem->szReferenceOriginalName, SG_FALSE)  );
		}
		else
		{
			// They have manually renamed it and/or already dealt with
			// any NAME-CONFLICT, so use it as is.
		}
	}
	else
	{
		// MERGE did not mangle it.  So we don't know anything about
		// the entryname, so use it as is.
	}

#if TRACE_WC_RESOLVE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "HypotheticalRepoPath: output %s\n",
							   SG_string__sz(pStringRepoPath))  );
#endif

	*ppStringRepoPath = pStringRepoPath;
	pStringRepoPath = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
	SG_STRING_NULLFREE(pCtx, pStringEntryname);
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__UTILS__INLINE1_H
