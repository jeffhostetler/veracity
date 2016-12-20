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
 * @file sg_wc_tx__diff.c
 *
 * @details Handle 'vv diff' functionality for baseline-vs-WD
 * for a single item, a directory, or the whole tree.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Setup for a DIFF on a single item.
 *
 * We return a varray of "DiffSteps".
 * 
 */
void SG_wc_tx__diff__setup(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   const SG_rev_spec * pRevSpec,
						   const char * pszInput,
						   SG_uint32 depth,
						   SG_bool bNoIgnores,
						   SG_bool bNoTSC,
						   SG_bool bNoSort,
						   SG_bool bInteractive,
						   const char * pszTool,
						   SG_varray ** ppvaDiffSteps)
{
	SG_varray * pvaStatus = NULL;
	SG_varray * pvaDiffSteps = NULL;
	SG_uint32 nrRevSpecs = 0;

	SG_NULLARGCHECK_RETURN( pWcTx );
	// pszInput is optional
	// pszTool is optional
	SG_NULLARGCHECK_RETURN( ppvaDiffSteps );

	if (pRevSpec)
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &nrRevSpecs)  );

	if (nrRevSpecs > 0)
		SG_ERR_CHECK(  SG_wc_tx__status1(pCtx, pWcTx, pRevSpec,
										 pszInput, depth,
										 SG_FALSE,	// bListUnchanged
										 bNoIgnores,
										 bNoTSC,
										 SG_FALSE,	// bListSparse
										 SG_FALSE,	// bListReserved
										 bNoSort,
										 &pvaStatus)  );
	else
		SG_ERR_CHECK(  SG_wc_tx__status(pCtx, pWcTx,
										pszInput, depth,
										SG_FALSE,	// bListUnchanged
										bNoIgnores,
										bNoTSC,
										SG_FALSE,	// bListSparse
										SG_FALSE,	// bListReserved
										bNoSort,
										&pvaStatus, NULL)  );

	if (pvaStatus)
		SG_ERR_CHECK(  sg_wc_tx__diff__setup(pCtx, pWcTx, pvaStatus,
											 bInteractive, pszTool,
											 &pvaDiffSteps)  );

	SG_RETURN_AND_NULL(pvaDiffSteps, ppvaDiffSteps);

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
}

/**
 * Setup for a DIFF on the given set of items.
 *
 * This version allows the caller to specify a SG_stringarray of items.
 * If given, we use it as the basis of the STATUS.  If not, we assume
 * a diff on the whole tree.
 *
 */
void SG_wc_tx__diff__setup__stringarray(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										const SG_rev_spec * pRevSpec,
										const SG_stringarray * psaInputs,
										SG_uint32 depth,
										SG_bool bNoIgnores,
										SG_bool bNoTSC,
										SG_bool bNoSort,
										SG_bool bInteractive,
										const char * pszTool,
										SG_varray ** ppvaDiffSteps)
{
	SG_varray * pvaStatus = NULL;
	SG_varray * pvaDiffSteps = NULL;
	SG_uint32 nrRevSpecs = 0;

	SG_NULLARGCHECK_RETURN( pWcTx );
	// psaInput is optional
	// pszTool is optional
	SG_NULLARGCHECK_RETURN( ppvaDiffSteps );

	if (pRevSpec)
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &nrRevSpecs)  );

	if (nrRevSpecs > 0)
		SG_ERR_CHECK(  SG_wc_tx__status1__stringarray(pCtx, pWcTx, pRevSpec,
													  psaInputs, depth,
													  SG_FALSE,	// bListUnchanged
													  bNoIgnores,
													  bNoTSC,
													  SG_FALSE,	// bListSparse
													  SG_FALSE,	// bListReserved
													  bNoSort,
													  &pvaStatus)  );
	else
		SG_ERR_CHECK(  SG_wc_tx__status__stringarray(pCtx, pWcTx,
													 psaInputs, depth,
													 SG_FALSE,	// bListUnchanged
													 bNoIgnores,
													 bNoTSC,
													 SG_FALSE,	// bListSparse
													 SG_FALSE,	// bListReserved
													 bNoSort,
													 &pvaStatus, NULL)  );

	if (pvaStatus)
		SG_ERR_CHECK(  sg_wc_tx__diff__setup(pCtx, pWcTx, pvaStatus,
											 bInteractive, pszTool,
											 &pvaDiffSteps)  );

	SG_RETURN_AND_NULL(pvaDiffSteps, ppvaDiffSteps);

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
}
