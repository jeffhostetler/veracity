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
 * @file sg_vv2__cat.c
 *
 * @details Handle the 'vv cat' command.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

void _get_hid_for_gid(
	SG_context * pCtx,
	SG_repo * pRepo,
	const char * pszHidChangeset,
	const char * pszGid,
	char ** ppszHid
	)
{
	SG_treenode_entry * ptne = NULL;
	const char * pszHidBlob;			// we do not own this
	char * pszHidBlob_copy = NULL;

	SG_ERR_CHECK(  SG_repo__treendx__get_path_in_dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL,
														 pszGid, pszHidChangeset,
														 NULL, &ptne)  );
														 
	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, ptne, &pszHidBlob)  );
	SG_ERR_CHECK(  SG_strdup(pCtx, pszHidBlob, &pszHidBlob_copy)  );
	SG_RETURN_AND_NULL(pszHidBlob_copy, ppszHid);
fail:
	SG_NULLFREE(pCtx, pszHidBlob_copy);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne);
}

void _compare_two_versions(
	SG_context * pCtx, 
	SG_repo * pRepo, 
	const char * pszParentContentHid, 
	const char * pszChildContentHid, 
	SG_int32 nStartLine, 
	SG_int32 nLength, 
	SG_bool * pbWasChanged,
	SG_int32 * pnLineNumberInParent,
	SG_vhash ** ppvhResult)
{
	SG_tempfile * pParentTempFile = NULL;
	SG_tempfile * pChildTempFile = NULL;
	SG_vhash * pvhResultDetails = NULL;
	SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &pParentTempFile)  );
	SG_ERR_CHECK(  SG_repo__fetch_blob_into_file(pCtx, pRepo,pszParentContentHid,pParentTempFile->pFile,NULL)  );
	SG_ERR_CHECK(  SG_tempfile__close(pCtx, pParentTempFile)  );
	
	SG_ERR_CHECK(  SG_tempfile__open_new(pCtx, &pChildTempFile)  );
	SG_ERR_CHECK(  SG_repo__fetch_blob_into_file(pCtx, pRepo,pszChildContentHid,pChildTempFile->pFile,NULL)  );
	SG_ERR_CHECK(  SG_tempfile__close(pCtx, pChildTempFile)  );
	
	SG_ERR_CHECK(  SG_linediff(pCtx, pParentTempFile->pPathname, pChildTempFile->pPathname, nStartLine, nLength, pbWasChanged, pnLineNumberInParent, ppvhResult)  );
fail:
	SG_tempfile__delete(pCtx, &pChildTempFile);
	SG_tempfile__delete(pCtx, &pParentTempFile);
	SG_VHASH_NULLFREE(pCtx, pvhResultDetails);
}

void _process_history_results(SG_context * pCtx,
				 SG_repo * pRepo,
				 SG_history_result * pHistoryResult,
				 const char * pszGid,
				 SG_int32 nStartLine,
				 SG_int32 nLength,
				 SG_varray ** ppvaResults)
{
	SG_bool bOk = SG_TRUE;
	const char * pszChildChangesetHid = NULL;
	const char * pszParentChangesetHid = NULL;
	char * pszChildContentHid = NULL;
	char * pszParentContentHid = NULL;
	SG_ihash * pihParentLineNums = NULL;
	SG_bool bParentHasAlreadyBeenSeen = SG_FALSE;
	SG_bool bFirstTime = SG_TRUE;
	SG_varray * pva_results = NULL;
	SG_vhash * pvh_thisResult = NULL;

	SG_VARRAY__ALLOC(pCtx, &pva_results);
	SG_IHASH__ALLOC(pCtx, &pihParentLineNums);
	
	while (bOk)
	{
		SG_uint32 nPsuedoParentCount = 0;
		SG_uint32 index = 0;
		SG_int64 nLineNumInThisChangeset = -1;
		SG_uint32 nChildRevNo = 0;
		SG_uint32 nChangedParentsCount = 0;
		SG_ERR_CHECK(  SG_history_result__get_cshid(pCtx, pHistoryResult, &pszChildChangesetHid)  );

		if (bFirstTime == SG_FALSE)
		{
			SG_ERR_CHECK(  SG_ihash__check__int64(pCtx, pihParentLineNums, pszChildChangesetHid, &nLineNumInThisChangeset)  );
		}
		else
		{
			nLineNumInThisChangeset = nStartLine;
			bFirstTime = SG_FALSE;
		}

		//We need to skip this changeset.
		if (nLineNumInThisChangeset < 0)
		{
			SG_ERR_CHECK(  SG_history_result__next(pCtx, pHistoryResult, &bOk)  );
			continue;
		}

		SG_ERR_CHECK(  _get_hid_for_gid(pCtx, pRepo, pszChildChangesetHid, pszGid, &pszChildContentHid)  );

		SG_ERR_CHECK(  SG_history_result__get_revno(pCtx, pHistoryResult, &nChildRevNo)  );
		SG_ERR_CHECK(  SG_history_result__get_pseudo_parent__count(pCtx, pHistoryResult, &nPsuedoParentCount)  );
		if (nPsuedoParentCount == 0)
		{
			//If we've fallen off the end of the results without a hit, we need to add 
			//it to the results
			SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_thisResult)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_thisResult, "parent_csid", "")  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_thisResult, "child_csid", pszChildChangesetHid)  );
			SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_thisResult, "child_revno", nChildRevNo)  );
			SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_thisResult, "parent_revno", 0)  );
			SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_thisResult, "number_of_parents", 0)  );
			SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_thisResult, "parent_start_line", 1)  );
			SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_thisResult, "child_start_line", 1)  );
			SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_thisResult, "parent_length", 0)  );
			SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_thisResult, "child_length", SG_INT32_MAX)  );
			SG_ERR_CHECK(  SG_varray__append__vhash(pCtx, pva_results, &pvh_thisResult)  );
			pvh_thisResult = NULL;
		}
		else
		{
			for (index = 0; index < nPsuedoParentCount; index++)
			{
				SG_bool bWasChanged = SG_FALSE;
				SG_int32 nLineNumInParent = 0;
				SG_uint32 nParentRevNo = 0;
			
			
				SG_ERR_CHECK(  SG_history_result__get_pseudo_parent(pCtx, pHistoryResult, index, &pszParentChangesetHid, &nParentRevNo)  );
				SG_ERR_CHECK(  _get_hid_for_gid(pCtx, pRepo, pszParentChangesetHid, pszGid, &pszParentContentHid)  );
				SG_ERR_CHECK(  _compare_two_versions(pCtx, pRepo, pszParentContentHid, pszChildContentHid, (SG_int32)nLineNumInThisChangeset, nLength, &bWasChanged, &nLineNumInParent, &pvh_thisResult)  );
				if (bWasChanged == SG_FALSE)
				{
					//There was no change between the two changesets.
					//Make a note of the line number in the parent
					SG_ERR_CHECK(  SG_ihash__has(pCtx, pihParentLineNums, pszParentChangesetHid, &bParentHasAlreadyBeenSeen)  );
					if (bParentHasAlreadyBeenSeen == SG_FALSE)
						SG_ERR_CHECK(  SG_ihash__add__int64(pCtx, pihParentLineNums, pszParentChangesetHid, nLineNumInParent)  );
				}
				else
				{
					//Include the information about this result in the changeset.
					nChangedParentsCount++;
					SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_thisResult, "parent_csid", pszParentChangesetHid)  );
					SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_thisResult, "child_csid", pszChildChangesetHid)  );
					SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_thisResult, "child_revno", nChildRevNo)  );
					SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_thisResult, "parent_revno", nParentRevNo)  );
					SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_thisResult, "number_of_parents", nPsuedoParentCount)  );
					SG_ERR_CHECK(  SG_varray__append__vhash(pCtx, pva_results, &pvh_thisResult)  );
					pvh_thisResult = NULL;
				}
				SG_VHASH_NULLFREE(pCtx, pvh_thisResult);
				SG_NULLFREE(pCtx, pszParentContentHid);
			}
		}
		
		if (nChangedParentsCount != nPsuedoParentCount)
		{
			//This is not a "full stop" change, so it's probably ignorable.
			SG_uint32 nCountResults = 0;
			SG_uint32 index = 0;
			SG_vhash * pvh_currentResult = NULL;
			//Add a flag to each of the results that we just appended to the array.
			SG_ERR_CHECK(  SG_varray__count(pCtx, pva_results, &nCountResults)  );
			for (index = nCountResults - nChangedParentsCount; index < nCountResults; index++)
			{
				SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_results, index, &pvh_currentResult)  );
				SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvh_currentResult, "probably_ignorable", SG_TRUE)  );
			}
		}
		SG_NULLFREE(pCtx, pszChildContentHid);
		SG_ERR_CHECK(  SG_history_result__next(pCtx, pHistoryResult, &bOk)  );
	}
	if (ppvaResults)
		SG_RETURN_AND_NULL(pva_results, ppvaResults);
fail:
	SG_IHASH_NULLFREE(pCtx, pihParentLineNums);
	SG_VHASH_NULLFREE(pCtx, pvh_thisResult);
	SG_VARRAY_NULLFREE(pCtx, pva_results);
	SG_NULLFREE(pCtx, pszChildContentHid);
	SG_NULLFREE(pCtx, pszParentContentHid);
}

void SG_vv2__line_history(SG_context * pCtx,
				 const char * pszRepoName,
				 const SG_rev_spec * pRevSpec,
				 const char * pszInput,
				 SG_int32 nStartLine,
				 SG_int32 nLength,
				 SG_varray ** ppvaResults)
{
	SG_repo * pRepo = NULL;

	SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, &pRepo);

	SG_ERR_CHECK(  SG_vv2__line_history__repo(pCtx, pRepo, pRevSpec, pszInput, nStartLine, nLength, ppvaResults)   );
fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	return;
}

void SG_vv2__line_history__repo(SG_context * pCtx,
				 SG_repo * pRepo,
				 const SG_rev_spec * pRevSpec,
				 const char * pszInput,
				 SG_int32 nStartLine,
				 SG_int32 nLength,
				 SG_varray ** ppvaResults)
{
	char * pszChangesetHid = NULL;
	char * pszGid = NULL;
	SG_stringarray * pStringArrayGIDs = NULL;
	SG_stringarray * pStringArrayChangesets_starting = NULL;
	SG_bool bHasResult = SG_FALSE;
	SG_history_result * pHistoryResult = NULL;

	SG_NULLARGCHECK_RETURN( pszInput );	// we require one item
	SG_ARGCHECK_RETURN(nLength > 0, "length");

	if (nStartLine <= 0)
	{
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
			(pCtx, "The first line of a file is line number 1")  );
	}
	//Above this, start line is based at 1.  
	//textfilediff is based at 0.
	nStartLine -= 1;

	//Find the single revision given by the rev spec.

	SG_ERR_CHECK(  SG_rev_spec__get_one__repo(pCtx, pRepo, pRevSpec, SG_TRUE, &pszChangesetHid, NULL)  );
	SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &pStringArrayChangesets_starting, 1)  );
	SG_ERR_CHECK(  SG_stringarray__add(pCtx, pStringArrayChangesets_starting, pszChangesetHid)  );

	//Put the input GID into a stringarray to send down
	SG_ERR_CHECK(  sg_vv2__util__translate_input_to_gid(pCtx, pRepo, pszChangesetHid, pszInput, SG_TRUE, &pszGid)  );
	SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &pStringArrayGIDs, 1)  );
	SG_ERR_CHECK(  SG_stringarray__add(pCtx, pStringArrayGIDs, pszGid)  );

	SG_ERR_CHECK( SG_history__run(pCtx, pRepo, pStringArrayGIDs,
					pStringArrayChangesets_starting, NULL,
					NULL, NULL, SG_UINT32_MAX, SG_FALSE, SG_FALSE,
					0, SG_INT64_MAX, SG_FALSE /*Don't Recommend the dagwalk*/, SG_TRUE /*Reassemble the dag*/, &bHasResult, &pHistoryResult, NULL) );

	//Now we need to process the history results, to do the comparisons.
	if (bHasResult)
		SG_ERR_CHECK(  _process_history_results(pCtx, pRepo, pHistoryResult, pszGid, nStartLine, nLength, ppvaResults)  ); 
fail:
	SG_NULLFREE(pCtx, pszChangesetHid);
	SG_NULLFREE(pCtx, pszGid);
	SG_STRINGARRAY_NULLFREE(pCtx, pStringArrayChangesets_starting);
	SG_STRINGARRAY_NULLFREE(pCtx, pStringArrayGIDs);
	SG_HISTORY_RESULT_NULLFREE(pCtx, pHistoryResult);
}