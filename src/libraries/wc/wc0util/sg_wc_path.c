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
 * @file sg_wc_path.c
 *
 * @details Generic pathname-like routines (that don't need
 * to know about pDb or pPathWorkingDirectoryTop).
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Split a RELATIVE-PATH or REPO-PATH (no leading '/') into a VARRAY of
 * the individual parts.
 *
 * This may be a true relative pathname or a repo-path.
 * In the case of a repo-path cell[0] of the array will be the '@'.
 *
 * This is just pathname/string parsing.
 *
 */
void sg_wc_path__split_string_into_varray(SG_context * pCtx,
										  const SG_string * pString,
										  SG_varray ** ppvaResult)
{
	SG_varray * pva = NULL;
	const char * pszStart = SG_string__sz(pString);

	if ((*pszStart==0) || (*pszStart=='/'))
		SG_ERR_THROW2_RETURN(  SG_ERR_INVALIDARG,
							   (pCtx, "Empty or absolute path '%s'.", pszStart)  );
	
	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );

	while (*pszStart)
	{
		const char * pszEnd = pszStart;

		while (*pszEnd && (*pszEnd != '/'))
			pszEnd++;

		if (pszEnd > pszStart)
			SG_ERR_CHECK(  SG_varray__append__string__buflen(pCtx, pva, pszStart, (SG_uint32)(pszEnd-pszStart))  );

		// we want to eat double slashes (we shouldn't get any of these).
		// we also want to eat trailing slashes.
		while (*pszEnd && (*pszEnd == '/'))
			pszEnd++;
		
		pszStart = pszEnd;
	}

	*ppvaResult = pva;
	return;

fail:
	SG_VARRAY_NULLFREE(pCtx, pva);
}

//////////////////////////////////////////////////////////////////

