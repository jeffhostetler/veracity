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

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

void sg_vv2__diff__symlink(SG_context * pCtx,
						   sg_vv6diff__diff_to_stream_data * pData,
						   const SG_vhash * pvhItem,
						   SG_wc_status_flags statusFlags)
{
	SG_string * pStringHeader = NULL;
	SG_vhash * pvhSubsection = NULL;	// we do not own this
	const char * pszTarget = NULL;		// we do not own this

	SG_ERR_CHECK(  sg_vv2__diff__header(pCtx, pvhItem, &pStringHeader)  );

	if (statusFlags & (SG_WC_STATUS_FLAGS__S__ADDED
					   |SG_WC_STATUS_FLAGS__S__MERGE_CREATED
					   |SG_WC_STATUS_FLAGS__S__DELETED
					   |SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED))
	{
		// content added/deleted/changed.

		// TODO 2012/05/07 What format should we use to report symlink targets
		// TODO            and/or changes to the target?
		// TODO
		// TODO            For now, I'm just going to put more meta-data in
		// TODO            the header.

		SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvhItem, pData->pszSubsectionLeft, &pvhSubsection)  );
		if (pvhSubsection)
		{
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection, "target", &pszTarget)  );
			SG_ERR_CHECK(  SG_string__append__format(pCtx, pStringHeader, "=== was -> %s\n",
													 /*pData->pszSubsectionLeft,*/ pszTarget)  );
		}
		SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvhItem, pData->pszSubsectionRight, &pvhSubsection)  );
		if (pvhSubsection)
		{
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSubsection, "target", &pszTarget)  );
			SG_ERR_CHECK(  SG_string__append__format(pCtx, pStringHeader, "=== now -> %s\n",
													 /*pData->pszSubsectionRight,*/ pszTarget)  );
		}

		SG_ERR_CHECK(  sg_vv2__diff__print_header_on_console(pCtx, pData, pStringHeader)  );
	}
	else
	{
		// just print the header -- content did not change -- must be a simple structural change.
		SG_ERR_CHECK(  sg_vv2__diff__print_header_on_console(pCtx, pData, pStringHeader)  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringHeader);
}
