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

//////////////////////////////////////////////////////////////////

void sg_wc_tx__diff__setup__directory(SG_context * pCtx,
									  sg_wc6diff__setup_data * pData,
									  const SG_vhash * pvhItem,
									  SG_wc_status_flags statusFlags)
{
	SG_string * pStringHeader = NULL;
	SG_vhash * pvhDiffItem = NULL;		// we do not own this
	const char * pszGid;				// we do not own this
	const char * pszLiveRepoPath;		// we do not own this
	SG_vhash * pvhItemStatus;			// we do not own this

	if (statusFlags & SG_WC_STATUS_FLAGS__U__IGNORED)
	{
		// The code in wc4status won't create header info for ignored
		// items unless verbose is set and diff never sets it, so we
		// skip this item to avoid getting the divider row of equal signs.

		return;
	}

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pData->pvaDiffSteps, &pvhDiffItem)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "gid", &pszGid)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "gid", pszGid)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "path", &pszLiveRepoPath)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "path", pszLiveRepoPath)  );
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem, "status", &pvhItemStatus)  );
	SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pvhDiffItem, "status", pvhItemStatus)  );

	SG_ERR_CHECK(  sg_wc_tx__diff__setup__header(pCtx, pvhItem, &pStringHeader)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "header", SG_string__sz(pStringHeader))  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringHeader);
}
