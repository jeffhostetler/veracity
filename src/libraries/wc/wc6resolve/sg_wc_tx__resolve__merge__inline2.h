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
 * @file sg_wc_tx__resolve__merge__inline2.h
 *
 * @details SG_resolve__merge related routines that I pulled from the
 * PendingTree version of sg_resolve.c.  These were considered
 * PUBLIC in sglib, but I want to make them PRIVATE.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RESOLVE__MERGE__INLINE2_H
#define H_SG_WC_TX__RESOLVE__MERGE__INLINE2_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_resolve__merge_values(
	SG_context*         pCtx,
	SG_resolve__value*  pBaseline,
	SG_resolve__value*  pOther,
	const char*         szTool,
	const char*         szLabel,
	SG_bool             bAccept,
	SG_int32*           pStatus,
	SG_resolve__value** ppMergeableResult,
	char**              ppTool
	)
{
	SG_resolve__choice* pChoice         = pBaseline->pChoice;
	SG_bool             bMergeable      = SG_FALSE;
	SG_resolve__value*  pAncestor       = NULL;
	SG_bool             bWorking        = SG_FALSE;
	SG_bool             bLive           = SG_FALSE;
	SG_string*          sResultLabel    = NULL;
	SG_pathname*        pResultTempFile = NULL;
	SG_int32            iStatus         = SG_FILETOOL__RESULT__COUNT;
	SG_bool             bResultExists   = SG_FALSE;
	char*               szToolName      = NULL;
	SG_resolve__value*  pMergeableResult = NULL;
	SG_bool             bOwnResult      = SG_FALSE;

	SG_NULLARGCHECK(pBaseline);
	SG_NULLARGCHECK(pOther);
	SG_ARGCHECK(pBaseline->pChoice == pOther->pChoice, pBaseline+pOther);

	// make sure this is a mergeable choice
	SG_ERR_CHECK(  SG_resolve__choice__get_mergeable(pCtx, pChoice, &bMergeable)  );
	SG_ARGCHECK(bMergeable == SG_TRUE, pBaseline|pOther);

	// if they supplied a label, make sure it will work
	if (szLabel != NULL)
	{
		SG_resolve__value* pTempValue = NULL;

		SG_ERR_CHECK(  SG_resolve__choice__check_value__label(pCtx, pChoice, szLabel, 0u, &pTempValue)  );
		if (pTempValue != NULL)
		{
			SG_bool bOverwritable = SG_FALSE;

			SG_ERR_CHECK(  SG_resolve__value__has_flag(pCtx, pTempValue, SG_RESOLVE__VALUE__OVERWRITABLE, &bOverwritable)  );
			if (bOverwritable == SG_FALSE)
			{
				SG_ERR_THROW2(SG_ERR_RESOLVE_INVALID_VALUE, (pCtx, "The specified value already exists and cannot be overwritten."));
			}
		}
	}

	// find our common ancestor value
	SG_ERR_CHECK(  SG_resolve__choice__get_value__label(pCtx, pChoice, SG_RESOLVE__LABEL__ANCESTOR, 0u, &pAncestor)  );

	// if either parent is a live working value, save it
	// that way the new merge value's parent will always have the same data it does right now
	// (we already know for sure that the choice is mergeable)
	SG_ERR_CHECK(  _value__mergeable_working_live(pCtx, pBaseline, NULL, &bWorking, &bLive)  );
	if (bWorking == SG_TRUE && bLive == SG_TRUE)
	{
		SG_ERR_CHECK(  _value__working_mergeable__save(pCtx, pBaseline)  );
	}
	SG_ERR_CHECK(  _value__mergeable_working_live(pCtx, pOther, NULL, &bWorking, &bLive)  );
	if (bWorking == SG_TRUE && bLive == SG_TRUE)
	{
		SG_ERR_CHECK(  _value__working_mergeable__save(pCtx, pOther)  );
	}

	// generate a label for the merge result
	if (szLabel == NULL)
	{
		SG_ERR_CHECK(  _generate_indexed_label(pCtx, pChoice, SG_RESOLVE__LABEL__MERGE, &sResultLabel)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &sResultLabel, szLabel)  );
	}

	// generate a temp filename for the merge result
	SG_ERR_CHECK(  _generate_temp_filename(pCtx,
										   pChoice,
										   gszFilenameFormat_Merge,
										   SG_string__sz(sResultLabel),
										   &pResultTempFile)  );

	// run a mergetool to merge the values into a result
	SG_ERR_CHECK(  _merge__run_tool(pCtx,
									pAncestor, pBaseline, pOther,
									SG_string__sz(sResultLabel),
									pResultTempFile,
									szTool,
									&iStatus, &bResultExists, &szToolName)  );

	// check the tool's output
	if (bResultExists == SG_TRUE)
	{
		if (iStatus == SG_MERGETOOL__RESULT__CANCEL)
		{
			// merge tool generated output, but was canceled
			// delete the output, we won't be using it
			//
			// Since this is a TEMP file, we can use SG_pathname__
			// and SG_fsobj__ routines.
			SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pResultTempFile)  );
		}
		else
		{
			SG_bool     bMerged        = SG_FALSE;
			const char* szTempToolName = NULL;

			// create a new value from the results
			SG_ERR_CHECK(  _value__alloc__merge(pCtx,
												pChoice,
												&sResultLabel,
												pResultTempFile,
												pBaseline,
												pOther,
												SG_FALSE,
												&szToolName,
												iStatus,
												&pMergeableResult)  );
			bOwnResult = SG_TRUE;

			// get the tool name back from the value so we can return it
			SG_ERR_CHECK(  SG_resolve__value__get_merge(pCtx, pMergeableResult, &bMerged, NULL, NULL, NULL, &szTempToolName, NULL, NULL)  );
			SG_ERR_CHECK(  SG_strdup(pCtx, szTempToolName, &szToolName)  );

			// TODO 2012/04/19 Question: if the above tool was not successful,
			// TODO            why would we want to call _merge__update_values()
			// TODO            to add this new value to the choice's set?
			// TODO            Seems like we should just discard it.

			// add/update the value to/in the choice's list of values
			SG_ERR_CHECK(  _merge__update_values(pCtx, pMergeableResult)  );
			bOwnResult = SG_FALSE;

			// TODO 2012/04/19 Not sure if I want to do this or not.
			// TODO            When the list of values in the choice
			// TODO            changes, we need to QUEUE a request to
			// TODO            tell the DB about it -- WHETHER OR NOT
			// TODO            WE ACTUALLY AUTO-ACCEPT THE NEW VALUE.
#if TRACE_WC_RESOLVE
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "MergeValues: Added new merge value to QUEUE: [%s][%s] %s\n",
									   pMergeableResult->szLabel,
									   ((pMergeableResult->pVariantFile) ? SG_string__sz(pMergeableResult->pVariantFile) : "(NULL)"),
									   SG_string__sz(pChoice->pItem->pStringGidDomainRepoPath))  );
#endif
			SG_ERR_CHECK(  _item__save_mergeable_data(pCtx, pChoice->pItem)  );


			// if the merge was successful, do a few post steps
			if (iStatus == SG_FILETOOL__RESULT__SUCCESS)
			{
				// update the list of leaf values
				SG_ERR_CHECK(  _choice__update_leaves(pCtx, pChoice, pMergeableResult, SG_TRUE)  );

				// if the choice is unresolved and the caller wants to
				// accept the successful merge result, do that
				if (bAccept == SG_TRUE && pChoice->pMergeableResult == NULL)
				{
					SG_ERR_CHECK(  SG_resolve__accept_value(pCtx, pMergeableResult)  );
				}
			}
		}
	}

	// return the results
	if (pStatus != NULL)
	{
		*pStatus = iStatus;
	}
	if (ppMergeableResult != NULL)
	{
		*ppMergeableResult = pMergeableResult;
		pMergeableResult = NULL;
	}
	if (ppTool != NULL)
	{
		*ppTool = szToolName;
		szToolName = NULL;
	}

fail:
	SG_STRING_NULLFREE(pCtx, sResultLabel);
	SG_PATHNAME_NULLFREE(pCtx, pResultTempFile);
	SG_NULLFREE(pCtx, szToolName);
	if (bOwnResult == SG_TRUE)
	{
		_VALUE__NULLFREE(pCtx, pMergeableResult);
	}
	return;
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__MERGE__INLINE2_H
