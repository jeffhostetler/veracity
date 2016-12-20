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

void sg_wc_tx__diff__setup__symlink(SG_context * pCtx,
									sg_wc6diff__setup_data * pData,
									const SG_vhash * pvhItem,
									SG_wc_status_flags statusFlags)
{
	SG_string * pStringHeader = NULL;
	SG_vhash * pvhSubsection = NULL;	// we do not own this
	const char * pszTarget = NULL;		// we do not own this
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

	// the following if-else-if-else... DOES NOT imply that these statuses
	// are mutually exclusive (for example, one could have ADDED+LOST), but
	// rather just the cases when we actually want to print the content diffs.
	//
	// Since we get all of the details from the pvhItem, we don't need to lookup the pLVI.
	// 
	// Since we put all of the diff-details in the header meta-data, we don't actually have
	// actual difftool-like output to report.  (I say that because it seems that GIT reports
	// changes in a symlink target as a regular 1-line unified file diff (with no final EOL).)

	if (statusFlags & SG_WC_STATUS_FLAGS__A__SPARSE)
	{
		SG_ERR_CHECK(  sg_wc_tx__diff__setup__header(pCtx, pvhItem, &pStringHeader)  );
		SG_ERR_CHECK(  SG_string__append__format(pCtx, pStringHeader, "=== %s\n", "Omitting details for sparse item.")  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "header", SG_string__sz(pStringHeader))  );
	}
	else if (statusFlags & SG_WC_STATUS_FLAGS__U__LOST)
	{
		SG_ERR_CHECK(  sg_wc_tx__diff__setup__header(pCtx, pvhItem, &pStringHeader)  );
		SG_ERR_CHECK(  SG_string__append__format(pCtx, pStringHeader, "=== %s\n", "Omitting details for lost item.")  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "header", SG_string__sz(pStringHeader))  );
	}
	else if (statusFlags & SG_WC_STATUS_FLAGS__U__FOUND)
	{
		// just build the header -- we never include content details for an uncontrolled item.
		SG_ERR_CHECK(  sg_wc_tx__diff__setup__header(pCtx, pvhItem, &pStringHeader)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "header", SG_string__sz(pStringHeader))  );
	}
	else if ((statusFlags & SG_WC_STATUS_FLAGS__S__MERGE_CREATED)
			 && (statusFlags & SG_WC_STATUS_FLAGS__S__DELETED))
	{
		// just build the header -- no content to diff -- not present now and not present in baseline
		SG_ERR_CHECK(  sg_wc_tx__diff__setup__header(pCtx, pvhItem, &pStringHeader)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "header", SG_string__sz(pStringHeader))  );
	}
	else if ((statusFlags & SG_WC_STATUS_FLAGS__S__UPDATE_CREATED)
			 && (statusFlags & SG_WC_STATUS_FLAGS__S__DELETED))
	{
		// just build the header -- no content to diff -- not present now and not present in baseline
		SG_ERR_CHECK(  sg_wc_tx__diff__setup__header(pCtx, pvhItem, &pStringHeader)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "header", SG_string__sz(pStringHeader))  );
	}
	else if (statusFlags & (SG_WC_STATUS_FLAGS__S__ADDED
							|SG_WC_STATUS_FLAGS__S__MERGE_CREATED
							|SG_WC_STATUS_FLAGS__S__UPDATE_CREATED
							|SG_WC_STATUS_FLAGS__S__DELETED
							|SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED))
	{
		// content added/deleted/changed.

		SG_ERR_CHECK(  sg_wc_tx__diff__setup__header(pCtx, pvhItem, &pStringHeader)  );
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
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "header", SG_string__sz(pStringHeader))  );
	}
	else
	{
		// just build the header -- content did not change -- must be a simple structural change.
		SG_ERR_CHECK(  sg_wc_tx__diff__setup__header(pCtx, pvhItem, &pStringHeader)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhDiffItem, "header", SG_string__sz(pStringHeader))  );
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStringHeader);
}
