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

static SG_varray_foreach_callback _diff_to_stream_cb;

static void _diff_to_stream_cb(SG_context * pCtx,
							   void * pVoidData,
							   const SG_varray * pvaStatus,
							   SG_uint32 k,
							   const SG_variant * pv)
{
	sg_vv6diff__diff_to_stream_data * pData = (sg_vv6diff__diff_to_stream_data *)pVoidData;
	SG_vhash * pvhItem;				// we do not own this
	SG_vhash * pvhItemStatus;		// we do not own this
	SG_int64 i64;
	SG_wc_status_flags statusFlags;

	SG_UNUSED( pvaStatus );
	SG_UNUSED( k );

	SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvhItem)  );
		
	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem, "status", &pvhItemStatus)  );
	SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhItemStatus, "flags", &i64)  );
	statusFlags = (SG_wc_status_flags)i64;

	switch (statusFlags & SG_WC_STATUS_FLAGS__T__MASK)
	{
	case SG_WC_STATUS_FLAGS__T__FILE:
		SG_ERR_CHECK(  sg_vv2__diff__file(pCtx, pData, pvhItem, statusFlags)  );
		break;
			
	case SG_WC_STATUS_FLAGS__T__DIRECTORY:
		SG_ERR_CHECK(  sg_vv2__diff__directory(pCtx, pData, pvhItem, statusFlags)  );
		break;

	case SG_WC_STATUS_FLAGS__T__SYMLINK:
		SG_ERR_CHECK(  sg_vv2__diff__symlink(pCtx, pData, pvhItem, statusFlags)  );
		break;

//	case SG_WC_STATUS_FLAGS__T__SUBREPO:
	default:
		SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
	}
	
fail:
	return;
}
							   
void sg_vv2__diff__diff_to_stream(SG_context * pCtx,
								  const char * pszRepoName,
								  const SG_varray * pvaStatus,
								  SG_bool bInteractive,
								  const char * pszTool,
								  SG_vhash ** ppvhResultCodes)
{
	sg_vv6diff__diff_to_stream_data data;
	SG_vhash * pvhWcInfo = NULL;
	SG_bool bGivenRepoName;

	// ppvhResultCodes is optional.

	memset(&data, 0, sizeof(data));

	bGivenRepoName = (pszRepoName && *pszRepoName);
	if (!bGivenRepoName)
	{
		// If they didn't give us a RepoName, try to get it from the WD.
		SG_wc__get_wc_info(pCtx, NULL, &pvhWcInfo);
		if (SG_CONTEXT__HAS_ERR(pCtx))
			SG_ERR_RETHROW2(  (pCtx, "Use 'repo' option or be in a working copy.")  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhWcInfo, "repo", &pszRepoName)  );
	}

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, &data.pRepo)  );
	data.pPathSessionTempDir = NULL;	// defer until needed
	data.pszTool = pszTool;
	data.bInteractive = bInteractive;
	if (bInteractive)
		data.pszDiffToolContext = SG_DIFFTOOL__CONTEXT__GUI;
	else
		data.pszDiffToolContext = SG_DIFFTOOL__CONTEXT__CONSOLE;
	data.pszSubsectionLeft  = SG_WC__STATUS_SUBSECTION__A;	// must match values in sg_vv2__status.c
	data.pszSubsectionRight = SG_WC__STATUS_SUBSECTION__B;

	if (ppvhResultCodes)
	{
		// The caller wants to know if we were able to lauch
		// a difftool.  However, we take a VARRAY of items to
		// compare -- rather than a single item.  So we must
		// build a container to return the individual tool
		// results on each file.

		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &data.pvhResultCodes)  );
	}

	SG_ERR_CHECK(  SG_varray__foreach(pCtx, pvaStatus, _diff_to_stream_cb, &data)  );

	if (ppvhResultCodes)
	{
		*ppvhResultCodes = data.pvhResultCodes;
		data.pvhResultCodes = NULL;
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pvhWcInfo);
	SG_REPO_NULLFREE(pCtx, data.pRepo);
	if (data.pPathSessionTempDir)
	{
		// we may or may not be able to delete the tmp dir (they may be visiting it in another window)
		// so we have to allow this to fail and not mess up the real context.

		SG_ERR_IGNORE(  SG_fsobj__rmdir_recursive__pathname(pCtx, data.pPathSessionTempDir)  );
		SG_PATHNAME_NULLFREE(pCtx, data.pPathSessionTempDir);
	}
	SG_VHASH_NULLFREE(pCtx, data.pvhResultCodes);
}

void sg_vv2__diff__print_header_on_console(SG_context * pCtx,
										   sg_vv6diff__diff_to_stream_data * pData,
										   const SG_string * pStringHeader)
{
	if (pData->bInteractive)
		return;

	SG_ERR_CHECK(  SG_console__raw(pCtx, SG_CS_STDOUT, SG_string__sz(pStringHeader))  );

fail:
	return;
}
