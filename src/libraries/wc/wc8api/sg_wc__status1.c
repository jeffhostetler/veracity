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
 * @file sg_wc__status1.c
 *
 * @details Wrapper for STATUS.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void SG_wc__status1(SG_context * pCtx,
					const SG_pathname* pPathWc,
					const SG_rev_spec * pRevSpec,
					const char * pszInput,
					SG_uint32 depth,
					SG_bool bListUnchanged,
					SG_bool bNoIgnores,
					SG_bool bNoTSC,
					SG_bool bListSparse,
					SG_bool bListReserved,
					SG_bool bNoSort,
					SG_varray ** ppvaStatus)
{
	SG_WC__WRAPPER_TEMPLATE__ro(
		pPathWc,
		SG_ERR_CHECK(  SG_wc_tx__status1(pCtx, pWcTx, pRevSpec,
										 pszInput,
										 depth,
										 bListUnchanged,
										 bNoIgnores, bNoTSC,
										 bListSparse,
										 bListReserved,
										 bNoSort,
										 ppvaStatus)  )  );
}

void SG_wc__status1__stringarray(SG_context * pCtx,
								 const SG_pathname* pPathWc,
								 const SG_rev_spec * pRevSpec,
								 const SG_stringarray * psaInputs,
								 SG_uint32 depth,
								 SG_bool bListUnchanged,
								 SG_bool bNoIgnores,
								 SG_bool bNoTSC,
								 SG_bool bListSparse,
								 SG_bool bListReserved,
								 SG_bool bNoSort,
								 SG_varray ** ppvaStatus)
{
	SG_WC__WRAPPER_TEMPLATE__ro(
		pPathWc,
		SG_ERR_CHECK(  SG_wc_tx__status1__stringarray(pCtx, pWcTx, pRevSpec,
													  psaInputs,
													  depth,
													  bListUnchanged,
													  bNoIgnores, bNoTSC,
													  bListSparse,
													  bListReserved,
													  bNoSort,
													  ppvaStatus)  )  );
}
