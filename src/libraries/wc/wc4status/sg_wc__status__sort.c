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
 * @file sg_wc__status__sort.c
 *
 * @details Sort the given pvaStatus in-place.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * We assume we are called on a VARRAY of VHASH
 * where each VHASH contains a top-level "path"
 * field.
 *
 */
static SG_qsort_compare_function _compare_path;

static int _compare_path(SG_context * pCtx,
						 const void * pVoid_ppv1, // const SG_variant ** ppv1
						 const void * pVoid_ppv2, // const SG_variant ** ppv2
						 void * pVoidData)
{
	const SG_variant** ppv1 = (const SG_variant **)pVoid_ppv1;
	const SG_variant** ppv2 = (const SG_variant **)pVoid_ppv2;
	SG_vhash * pvh1;
	SG_vhash * pvh2;
	const char * psz1;
	const char * psz2;
	int result = 0;

	SG_UNUSED( pVoidData );

	if (*ppv1 == NULL && *ppv2 == NULL)
		return 0;
	if (*ppv1 == NULL)
		return -1;
	if (*ppv2 == NULL)
		return 1;

	SG_variant__get__vhash(pCtx, *ppv1, &pvh1);
	SG_variant__get__vhash(pCtx, *ppv2, &pvh2);

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh1, "path", &psz1)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh2, "path", &psz2)  );

	SG_ERR_CHECK(  SG_repopath__compare(pCtx, psz1, psz2, &result)  );

fail:
	return result;
}

void SG_wc__status__sort_by_repopath(SG_context * pCtx,
									 SG_varray * pvaStatus)
{
	SG_ERR_CHECK_RETURN(  SG_varray__sort(pCtx, pvaStatus, _compare_path, NULL)  );
}
