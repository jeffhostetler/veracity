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
 * @file sg_vv2__diff_to_stream.c
 *
 * @details Compute a HISTORICAL DIFF between 2 changesets.
 *
 * We build upon the HISTORICAL STATUS, so see it for caveats.
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

/**
 * Historical diff of 2 changesets to console.
 * 
 */
void SG_vv2__diff_to_stream(SG_context * pCtx,
							const char * pszRepoName,
							const SG_rev_spec * pRevSpec,
							const SG_stringarray * psaInputs,
							SG_uint32 depth,
							SG_bool bNoSort,
							SG_bool bInteractive,
							const char * pszTool,
							SG_vhash ** ppvhResultCodes)
{
	SG_varray * pvaStatus = NULL;

	// pszRepoName is optional (defaults to WD if present)
	SG_NULLARGCHECK_RETURN( pRevSpec );
	// psaInputs is optional
	// pszTool is optional
	// ppvhResultCodes is optional

	SG_ERR_CHECK(  SG_vv2__status(pCtx, pszRepoName, pRevSpec,
								  psaInputs, depth,
								  bNoSort,
								  &pvaStatus, NULL)  );
	if (pvaStatus)
		SG_ERR_CHECK(  sg_vv2__diff__diff_to_stream(pCtx, pszRepoName, pvaStatus, bInteractive, pszTool, ppvhResultCodes)  );

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
}

void SG_vv2__diff_to_stream__throw(SG_context * pCtx,
								   const char * pszRepoName,
								   const SG_rev_spec * pRevSpec,
								   const SG_stringarray * psaInputs,
								   SG_uint32 depth,
								   SG_bool bNoSort,
								   SG_bool bInteractive,
								   const char * pszTool)
{
	SG_ERR_CHECK_RETURN(  SG_vv2__diff_to_stream(pCtx, pszRepoName, pRevSpec,
												 psaInputs, depth,
												 bNoSort, bInteractive, pszTool,
												 NULL)  );
}
