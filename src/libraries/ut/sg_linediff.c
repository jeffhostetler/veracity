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
 * @file sg_linediff.c
 *
 * @details A routine to diff two files on disk, and report if a certain range of lines was changed.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

/**
 * Compare two versions of a file to see if a certain area was changed between the them.
 */
void SG_linediff(SG_context * pCtx,
				   SG_pathname * pPathLocalFileParent,
				   SG_pathname * pPathLocalFileChild,
				   SG_int32 nStartLine,
				   SG_int32 nNumLinesToSearch,
				   SG_bool * pbWasChanged,
				   SG_int32 * pnLineNumInParent,
				   SG_vhash ** ppvh_ResultDetails)
{
	SG_textfilediff_t * pDiff = NULL;
	SG_bool bOk = SG_FALSE;
	SG_textfilediff_iterator * pit = NULL;
	SG_diff_type type = SG_DIFF_TYPE__COMMON;
	SG_int32 startParent = 0;
	SG_int32 startChild = 0;
	SG_int32 lenParent = 0;
	SG_int32 lenChild = 0;
	SG_int32 nStartLineInParent = 0;
	SG_int32 nAOI_firstLine = 0;
	SG_int32 nAOI_lastLine = 0;
	SG_int32 nLineDelta = 0;
	SG_int32 nThisDiff__child__lastLine = 0;
	SG_vhash * pvh_result = NULL;

	SG_ASSERT(pCtx!=NULL);
	SG_ASSERT(nNumLinesToSearch >= 0);
	SG_NULLARGCHECK_RETURN(pbWasChanged);
	SG_NULLARGCHECK_RETURN(pnLineNumInParent);
	//SG_NULLARGCHECK_RETURN(ppvh_ResultDetails);

	*pbWasChanged = SG_FALSE;
	
	if (nNumLinesToSearch == 0)
	{
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
			(pCtx, "Please provide a nonzero length")  );
	}

	SG_ERR_CHECK(  SG_textfilediff(pCtx, pPathLocalFileParent, pPathLocalFileChild, SG_TEXTFILEDIFF_OPTION__NATIVE_EOL, &pDiff)  );

	SG_ERR_CHECK(  SG_textfilediff__iterator__first(pCtx, pDiff, &pit, &bOk)  );

	nStartLineInParent = nStartLine;
	//AOI stands for Area Of Interest.
	nAOI_firstLine = nStartLine;
	nAOI_lastLine = nStartLine + (nNumLinesToSearch - 1);
	while (bOk)
	{
		SG_ERR_CHECK(  SG_textfilediff__iterator__details(pCtx, pit, &type, &startParent, &startChild, NULL, &lenParent, &lenChild, NULL)  );

		nThisDiff__child__lastLine = startChild + (lenChild - 1);

		if (startChild > nAOI_lastLine)
		{
			//This diff is entirely after the Area of Interest.
			//We don't need to look at any more diffs.
			break;
		}

		//We don't need to examine regions of the files that
		//are unchanged.
		if (type != SG_DIFF_TYPE__COMMON)
		{
			//There are four possible cases for the diff and the AOI to connect.
			// * OVERLAP 1: diff from 5-15, AOI from 10-20
			//   diff range:  |----*---------*-----|
			//    AOI range:  |---------*---------*|
			// * OVERLAP 2: diff from 10-20, AOI from 5-15
			//   diff range:  |---------*---------*|
			//    AOI range:  |----*---------*-----|
			// * CONTAIN 1: diff from 5-20, AOI from 10-15
			//   diff range:  |----*--------------*|
			//    AOI range:  |---------*----*-----|
			// * CONTAIN 2: diff from 10-15, AOI from 5-20
			//   diff range:  |---------*----*-----|
			//    AOI range:  |----*--------------*|
			// In each of these cases, either the start of the diff is in the AOI range, or 
			// the start of the AOI is in the diff range. Note that single-line ranges are
			// covered by the CONTAIN cases.
#define ISBETWEEN(num, rangeStart, rangeEnd) (rangeStart <= num && num <= rangeEnd)

			//A zero-length diff won't have a range to check.
			//zero-length diff in the child means that some lines in the parent were deleted in 
			//the child.
			if (   ISBETWEEN(startChild, nAOI_firstLine, nAOI_lastLine)
				|| (lenChild > 0 && ISBETWEEN(nAOI_firstLine, startChild, nThisDiff__child__lastLine)) )
			{
				//Some part of the Area of Interest is inside this diff.
				*pbWasChanged = SG_TRUE;
				SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_result)  );
				//Offset the parent and child start lines by 1
				SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_result, "parent_start_line", startParent + 1)  );
				SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_result, "child_start_line", startChild + 1)  );
				SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_result, "parent_length", lenParent)  );
				SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_result, "child_length", lenChild)  );
				SG_RETURN_AND_NULL(pvh_result, ppvh_ResultDetails);
				break;
			}
		}
		//This line delta may be positive or negative. Positive line deltas mean that the AOI 
		//has moved down in the file (it has a larger line number).  Negative line deltas mean that
		//the AOI has moved up in the file (it has a smaller line number)
		nLineDelta = lenParent - lenChild;
		nStartLineInParent += nLineDelta;
		//Move to the next difference between the files.
		SG_ERR_CHECK(  SG_textfilediff__iterator__next(pCtx, pit, &bOk)  );
	}
	//We fell off the end of the diff. Check to see if they gave us a line that is past the end of the file.
	if (bOk == SG_FALSE && nThisDiff__child__lastLine < nAOI_firstLine)
	{
		//They gave us a start line that was past the end of the file.  Throw!
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
			(pCtx, "The start line %d is past the end of the file. The last line is %d", nAOI_firstLine, nThisDiff__child__lastLine)  );
	}

	if (pnLineNumInParent)
		*pnLineNumInParent = nStartLineInParent;

fail:
	SG_VHASH_NULLFREE(pCtx, pvh_result);
	SG_TEXTFILEDIFF_ITERATOR_NULLFREE(pCtx, pit);
	SG_TEXTFILEDIFF_NULLFREE(pCtx, pDiff);
}
