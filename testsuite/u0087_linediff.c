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

#include <sg.h>
#include "unittests.h"

void _u0087__write_file(SG_context *pCtx, const char * sz, SG_pathname * pFilePath)
{
	SG_file * pFile = NULL;

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pFilePath, SG_FILE_OPEN_OR_CREATE|SG_FILE_WRONLY, 0666, &pFile)  );
	SG_ERR_CHECK(  SG_file__write__sz(pCtx, pFile, sz)  );
	SG_ERR_CHECK(  SG_file__truncate(pCtx, pFile)  );
	SG_FILE_NULLCLOSE(pCtx, pFile);

	return;
fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
}

void _u0087__check_range_in_diff(SG_context * pCtx, SG_pathname * pDiffFromFilePath, SG_pathname * pDiffToFilePath, SG_int32 rangeStart, SG_int32 rangeEnd, SG_bool bExpectChanged, SG_int32 nExpectedOffsetInParent)
{
	SG_bool bWasChanged = SG_FALSE;
	SG_int32 nLineNumInParent = 0;
	SG_vhash * pvhResults = NULL;
	VERIFY_ERR_CHECK(  SG_linediff(pCtx, pDiffFromFilePath, pDiffToFilePath, rangeStart, 1, &bWasChanged, &nLineNumInParent, &pvhResults)  );
	SG_VHASH_NULLFREE(pCtx, pvhResults);
	VERIFY_COND_FAIL("Linediff for start of range", bWasChanged == bExpectChanged); 
	if (bExpectChanged == SG_FALSE)
		VERIFY_COND_FAIL("Check line num in parent", nLineNumInParent == rangeStart + nExpectedOffsetInParent);
	//If we're looking for a change, then we can overlap the previous change to make sure that we get the same result
	if (bExpectChanged == SG_TRUE && rangeStart > 0)
	{
		VERIFY_ERR_CHECK(  SG_linediff(pCtx, pDiffFromFilePath, pDiffToFilePath, rangeStart - 1, 2, &bWasChanged, &nLineNumInParent, &pvhResults)  );
		SG_VHASH_NULLFREE(pCtx, pvhResults);
		VERIFY_COND_FAIL("Linediff for rangeStart - 1", bWasChanged == bExpectChanged);
		
		//For example, if the range is 5-10, the next linediff checks lines 4-11
		VERIFY_ERR_CHECK(  SG_linediff(pCtx, pDiffFromFilePath, pDiffToFilePath, rangeStart - 1, (rangeEnd - rangeStart) + 4, &bWasChanged, &nLineNumInParent, &pvhResults)  );
		SG_VHASH_NULLFREE(pCtx, pvhResults);
		VERIFY_COND_FAIL("Linediff for (rangeStart - 1) AND (rangeEnd + 1)", bWasChanged == bExpectChanged);
	}

	if ((rangeEnd - rangeStart) >= 3)
	{
		//Consider the range 5-10, the next linediff checks lines 6-9
		VERIFY_ERR_CHECK(  SG_linediff(pCtx, pDiffFromFilePath, pDiffToFilePath, rangeStart + 1, (rangeEnd - rangeStart) - 1, &bWasChanged, &nLineNumInParent, &pvhResults)  );
		SG_VHASH_NULLFREE(pCtx, pvhResults);
		VERIFY_COND_FAIL("Linediff for (rangeStart + 1) AND (rangeEnd - 1)", bWasChanged == bExpectChanged);
	}

	if (rangeEnd != rangeStart)
	{
		//Check the single line at the end of the range
		VERIFY_ERR_CHECK(  SG_linediff(pCtx, pDiffFromFilePath, pDiffToFilePath, rangeEnd, 1, &bWasChanged, &nLineNumInParent, &pvhResults)  );
		SG_VHASH_NULLFREE(pCtx, pvhResults);
		VERIFY_COND_FAIL("Linediff for rangeEnd", bWasChanged == bExpectChanged);
		if (bExpectChanged == SG_FALSE)
			VERIFY_COND_FAIL("Check line num in parent", nLineNumInParent == rangeEnd + nExpectedOffsetInParent);
		if (bExpectChanged == SG_TRUE)
		{
			//Extend the range past the end to make sure that it still reports changed
			VERIFY_ERR_CHECK(  SG_linediff(pCtx, pDiffFromFilePath, pDiffToFilePath, rangeEnd, 2, &bWasChanged, &nLineNumInParent, &pvhResults)  );
			SG_VHASH_NULLFREE(pCtx, pvhResults);
			VERIFY_COND_FAIL("Linediff for rangeEnd + 1", bWasChanged == bExpectChanged);
		}
	}
	
fail:
	SG_VHASH_NULLFREE(pCtx, pvhResults);
}

void _u0087__linediff(SG_context *pCtx)
{
	// Silly example text from wikipedia.

	const char * const DIFF_FROM =
		"This part of the" SG_PLATFORM_NATIVE_EOL_STR
		"document has stayed the" SG_PLATFORM_NATIVE_EOL_STR
		"same from version to" SG_PLATFORM_NATIVE_EOL_STR
		"version.  It shouldn't" SG_PLATFORM_NATIVE_EOL_STR
		"be shown if it doesn't" SG_PLATFORM_NATIVE_EOL_STR
		"change.  Otherwise, that" SG_PLATFORM_NATIVE_EOL_STR
		"would not be helping to" SG_PLATFORM_NATIVE_EOL_STR
		"compress the size of the" SG_PLATFORM_NATIVE_EOL_STR
		"changes." SG_PLATFORM_NATIVE_EOL_STR
		"" SG_PLATFORM_NATIVE_EOL_STR
		"This paragraph contains" SG_PLATFORM_NATIVE_EOL_STR
		"text that is outdated." SG_PLATFORM_NATIVE_EOL_STR
		"It will be deleted in the" SG_PLATFORM_NATIVE_EOL_STR
		"near future." SG_PLATFORM_NATIVE_EOL_STR
		"" SG_PLATFORM_NATIVE_EOL_STR
		"It is important to spell" SG_PLATFORM_NATIVE_EOL_STR
		"check this dokument. On" SG_PLATFORM_NATIVE_EOL_STR
		"the other hand, a" SG_PLATFORM_NATIVE_EOL_STR
		"misspelled word isn't" SG_PLATFORM_NATIVE_EOL_STR
		"the end of the world." SG_PLATFORM_NATIVE_EOL_STR
		"Nothing in the rest of" SG_PLATFORM_NATIVE_EOL_STR
		"this paragraph needs to" SG_PLATFORM_NATIVE_EOL_STR
		"be changed. Things can" SG_PLATFORM_NATIVE_EOL_STR
		"be added after it." SG_PLATFORM_NATIVE_EOL_STR;

	const char * const DIFF_TO =
		"This is an important" SG_PLATFORM_NATIVE_EOL_STR
		"notice! It should" SG_PLATFORM_NATIVE_EOL_STR
		"therefore be located at" SG_PLATFORM_NATIVE_EOL_STR
		"the beginning of this" SG_PLATFORM_NATIVE_EOL_STR
		"document!" SG_PLATFORM_NATIVE_EOL_STR
		"" SG_PLATFORM_NATIVE_EOL_STR
		"This part of the" SG_PLATFORM_NATIVE_EOL_STR
		"document has stayed the" SG_PLATFORM_NATIVE_EOL_STR
		"same from version to" SG_PLATFORM_NATIVE_EOL_STR
		"version.  It shouldn't" SG_PLATFORM_NATIVE_EOL_STR
		"be shown if it doesn't" SG_PLATFORM_NATIVE_EOL_STR
		"change.  Otherwise, that" SG_PLATFORM_NATIVE_EOL_STR
		"would not be helping to" SG_PLATFORM_NATIVE_EOL_STR
		"compress anything." SG_PLATFORM_NATIVE_EOL_STR
		"" SG_PLATFORM_NATIVE_EOL_STR
		"It is important to spell" SG_PLATFORM_NATIVE_EOL_STR
		"check this document. On" SG_PLATFORM_NATIVE_EOL_STR
		"the other hand, a" SG_PLATFORM_NATIVE_EOL_STR
		"misspelled word isn't" SG_PLATFORM_NATIVE_EOL_STR
		"the end of the world." SG_PLATFORM_NATIVE_EOL_STR
		"Nothing in the rest of" SG_PLATFORM_NATIVE_EOL_STR
		"this paragraph needs to" SG_PLATFORM_NATIVE_EOL_STR
		"be changed. Things can" SG_PLATFORM_NATIVE_EOL_STR
		"be added after it." SG_PLATFORM_NATIVE_EOL_STR
		"" SG_PLATFORM_NATIVE_EOL_STR
		"This paragraph contains" SG_PLATFORM_NATIVE_EOL_STR
		"important new additions" SG_PLATFORM_NATIVE_EOL_STR
		"to this document." SG_PLATFORM_NATIVE_EOL_STR;

/*	const char * const DIFF_EXPECTED_RESULT =
		"--- diff-from.txt" SG_PLATFORM_NATIVE_EOL_STR
		"+++ diff-to.txt" SG_PLATFORM_NATIVE_EOL_STR
		"@@ -1,3 +1,9 @@" SG_PLATFORM_NATIVE_EOL_STR
		"+This is an important" SG_PLATFORM_NATIVE_EOL_STR				//LINE 0
		"+notice! It should" SG_PLATFORM_NATIVE_EOL_STR
		"+therefore be located at" SG_PLATFORM_NATIVE_EOL_STR
		"+the beginning of this" SG_PLATFORM_NATIVE_EOL_STR
		"+document!" SG_PLATFORM_NATIVE_EOL_STR
		"+" SG_PLATFORM_NATIVE_EOL_STR
		" This part of the" SG_PLATFORM_NATIVE_EOL_STR					//LINE 6 was LINE 0
		" document has stayed the" SG_PLATFORM_NATIVE_EOL_STR
		" same from version to" SG_PLATFORM_NATIVE_EOL_STR
		" version.  It shouldn't" SG_PLATFORM_NATIVE_EOL_STR
		"@@ -5,16 +11,10 @@" SG_PLATFORM_NATIVE_EOL_STR
		" be shown if it doesn't" SG_PLATFORM_NATIVE_EOL_STR
		" change.  Otherwise, that" SG_PLATFORM_NATIVE_EOL_STR
		" would not be helping to" SG_PLATFORM_NATIVE_EOL_STR			//LINE 12 was LINE 6
		"-compress the size of the" SG_PLATFORM_NATIVE_EOL_STR
		"-changes." SG_PLATFORM_NATIVE_EOL_STR
		"-" SG_PLATFORM_NATIVE_EOL_STR
		"-This paragraph contains" SG_PLATFORM_NATIVE_EOL_STR
		"-text that is outdated." SG_PLATFORM_NATIVE_EOL_STR
		"-It will be deleted in the" SG_PLATFORM_NATIVE_EOL_STR
		"-near future." SG_PLATFORM_NATIVE_EOL_STR
		"+compress anything." SG_PLATFORM_NATIVE_EOL_STR				//LINE 13
		" " SG_PLATFORM_NATIVE_EOL_STR									//LINE 14
		" It is important to spell" SG_PLATFORM_NATIVE_EOL_STR
		"-check this dokument. On" SG_PLATFORM_NATIVE_EOL_STR
		"+check this document. On" SG_PLATFORM_NATIVE_EOL_STR			//LINE 16
		" the other hand, a" SG_PLATFORM_NATIVE_EOL_STR					//LINE 17
		" misspelled word isn't" SG_PLATFORM_NATIVE_EOL_STR
		" the end of the world." SG_PLATFORM_NATIVE_EOL_STR
		"@@ -22,3 +22,7 @@" SG_PLATFORM_NATIVE_EOL_STR
		" Nothing in the rest of" SG_PLATFORM_NATIVE_EOL_STR
		" this paragraph needs to" SG_PLATFORM_NATIVE_EOL_STR
		" be changed. Things can" SG_PLATFORM_NATIVE_EOL_STR
		" be added after it." SG_PLATFORM_NATIVE_EOL_STR
		"+" SG_PLATFORM_NATIVE_EOL_STR									//LINE 24
		"+This paragraph contains" SG_PLATFORM_NATIVE_EOL_STR
		"+important new additions" SG_PLATFORM_NATIVE_EOL_STR
		"+to this document." SG_PLATFORM_NATIVE_EOL_STR;				//LINE 27
		*/
	SG_pathname * pDiffFromFilePath = NULL;
	SG_pathname * pDiffToFilePath = NULL;

	SG_bool bWasChanged = SG_FALSE;
	SG_int32 nLineNumInParent = 0;
	SG_vhash * pvhResults = NULL;
	SG_ASSERT(pCtx!=NULL);

	//
	// Set up files.
	//

	VERIFY_ERR_CHECK(  SG_pathname__alloc__sz(pCtx, &pDiffFromFilePath, "u0078-diff-from.txt")  );
	VERIFY_ERR_CHECK(  SG_pathname__alloc__sz(pCtx, &pDiffToFilePath, "u0078-diff-to.txt")  );

	VERIFY_ERR_CHECK(  _u0087__write_file(pCtx, DIFF_FROM, pDiffFromFilePath)  );
	VERIFY_ERR_CHECK(  _u0087__write_file(pCtx, DIFF_TO, pDiffToFilePath)  );

	/////// Lines 0-5 should be modified
	_u0087__check_range_in_diff(pCtx, pDiffFromFilePath, pDiffToFilePath, 0, 5, SG_TRUE, 0);

	/////// Lines 6-12 should NOT be modified
	_u0087__check_range_in_diff(pCtx, pDiffFromFilePath, pDiffToFilePath, 6, 12, SG_FALSE, -6);
	
	////// Line 13 should be modified
	_u0087__check_range_in_diff(pCtx, pDiffFromFilePath, pDiffToFilePath, 13, 13, SG_TRUE, 0);

	////// Line 14 and 15 should NOT be modified
	_u0087__check_range_in_diff(pCtx, pDiffFromFilePath, pDiffToFilePath, 14, 15, SG_FALSE, 0); //offset actually back to zero, since lines were deleted from parent to child.

	////// Line 16 should be modified
	_u0087__check_range_in_diff(pCtx, pDiffFromFilePath, pDiffToFilePath, 16, 16, SG_TRUE, 0);
	
	////// Line 17-22 should NOT be modified
	_u0087__check_range_in_diff(pCtx, pDiffFromFilePath, pDiffToFilePath, 17, 22, SG_FALSE, 0);
	
	////// Line 24-27 should be modified
	_u0087__check_range_in_diff(pCtx, pDiffFromFilePath, pDiffToFilePath, 24, 27, SG_TRUE, 0);
	
	//Try some line numbers of the end of the file.  Expect an error.
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_linediff(pCtx, pDiffFromFilePath, pDiffToFilePath, 28, 1, &bWasChanged, &nLineNumInParent, &pvhResults), SG_ERR_INVALIDARG);
	SG_VHASH_NULLFREE(pCtx, pvhResults);
	VERIFY_ERR_CHECK_ERR_EQUALS_DISCARD(  SG_linediff(pCtx, pDiffFromFilePath, pDiffToFilePath, 200, 1, &bWasChanged, &nLineNumInParent, &pvhResults), SG_ERR_INVALIDARG);
	SG_VHASH_NULLFREE(pCtx, pvhResults);

	SG_PATHNAME_NULLFREE(pCtx, pDiffToFilePath);
	SG_PATHNAME_NULLFREE(pCtx, pDiffFromFilePath);
	return;
fail:
	SG_PATHNAME_NULLFREE(pCtx, pDiffToFilePath);
	SG_PATHNAME_NULLFREE(pCtx, pDiffFromFilePath);
}


TEST_MAIN(u0087_linediff)
{
	TEMPLATE_MAIN_START;

	BEGIN_TEST(  _u0087__linediff(pCtx)  );

	TEMPLATE_MAIN_END;
}
