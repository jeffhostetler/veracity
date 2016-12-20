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
 * @file sg_wc__status__find_status__single_item.c
 *
 * @details A helper function to search status results to find the 
 * status vhash for one item.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 *
 */
void sg_wc__status__find_status__single_item(SG_context * pCtx,
									  const SG_varray * pva_statuses,
									  const char * psz_repo_path, //This must be the repo path, exactly as it appears in the status results.
									  SG_vhash ** ppvh_status_result)
{
	SG_vhash * pvh_status = NULL;
	SG_uint32 nArraySize = 0;
	SG_uint32 index = 0;
	const char * psz_repo_path_to_check = NULL;
	SG_bool bFound = SG_FALSE;
	SG_NULLARGCHECK(pva_statuses);

	SG_ERR_CHECK(  SG_varray__count(pCtx, pva_statuses, &nArraySize)  );

	for (index = 0; index < nArraySize; index++)
	{
		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_statuses, index, &pvh_status)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_status, "path", &psz_repo_path_to_check)  );
		if (SG_strcmp__null(psz_repo_path, psz_repo_path_to_check) == 0)
		{
			bFound = SG_TRUE;
			break;
		}
	}
	if (bFound)
		SG_RETURN_AND_NULL(pvh_status, ppvh_status_result);
fail:
	return;
}

