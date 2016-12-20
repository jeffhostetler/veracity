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

static void _setup_cb1(SG_context * pCtx,
					   sg_wc6diff__setup_data * pData,
					   const SG_vhash * pvhItem)
{
	SG_vhash * pvhItemStatus;		// we do not own this
	SG_int64 i64;
	SG_wc_status_flags statusFlags;

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem, "status", &pvhItemStatus)  );
	SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhItemStatus, "flags", &i64)  );
	statusFlags = (SG_wc_status_flags)i64;

	switch (statusFlags & SG_WC_STATUS_FLAGS__T__MASK)
	{
	case SG_WC_STATUS_FLAGS__T__FILE:
		SG_ERR_CHECK(  sg_wc_tx__diff__setup__file(pCtx, pData, pvhItem, statusFlags)  );
		break;
			
	case SG_WC_STATUS_FLAGS__T__DIRECTORY:
		SG_ERR_CHECK(  sg_wc_tx__diff__setup__directory(pCtx, pData, pvhItem, statusFlags)  );
		break;

	case SG_WC_STATUS_FLAGS__T__SYMLINK:
		SG_ERR_CHECK(  sg_wc_tx__diff__setup__symlink(pCtx, pData, pvhItem, statusFlags)  );
		break;

	default:
		SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
	}
	
fail:
	return;
}

static SG_varray_foreach_callback _setup_cb;

static void _setup_cb(SG_context * pCtx,
					  void * pVoidData,
					  const SG_varray * pvaStatus,
					  SG_uint32 k,
					  const SG_variant * pv)
{
	sg_wc6diff__setup_data * pData = (sg_wc6diff__setup_data *)pVoidData;
	SG_vhash * pvhItem;				// we do not own this

	SG_UNUSED( pvaStatus );
	SG_UNUSED( k );

	SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvhItem)  );
	SG_ERR_CHECK(  _setup_cb1(pCtx, pData, pvhItem)  );

fail:
	return;
}

void sg_wc_tx__diff__setup(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   const SG_varray * pvaStatus,
						   SG_bool bInteractive,
						   const char * pszTool,
						   SG_varray ** ppvaDiffSteps)
{
	sg_wc6diff__setup_data data;

	memset(&data, 0, sizeof(data));
	data.pWcTx = pWcTx;
	data.pszTool = pszTool;
	data.bInteractive = bInteractive;
	if (bInteractive)
		data.pszDiffToolContext = SG_DIFFTOOL__CONTEXT__GUI;
	else
		data.pszDiffToolContext = SG_DIFFTOOL__CONTEXT__CONSOLE;
	data.pszSubsectionLeft = SG_WC__STATUS_SUBSECTION__B;
	data.pszSubsectionRight = SG_WC__STATUS_SUBSECTION__WC;

	SG_NULLARGCHECK_RETURN( pvaStatus );
	SG_NULLARGCHECK_RETURN( ppvaDiffSteps );

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &data.pvaDiffSteps)  );
	SG_ERR_CHECK(  SG_varray__foreach(pCtx, pvaStatus, _setup_cb, &data)  );

#if TRACE_WC_DIFF
	SG_ERR_IGNORE(  SG_varray_debug__dump_varray_of_vhashes_to_console(pCtx, data.pvaDiffSteps, "DiffSteps")  );
#endif

	SG_RETURN_AND_NULL(data.pvaDiffSteps, ppvaDiffSteps);

fail:
	SG_VARRAY_NULLFREE(pCtx, data.pvaDiffSteps);
}
