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

void sg_vv2__diff__directory(SG_context * pCtx,
							 sg_vv6diff__diff_to_stream_data * pData,
							 const SG_vhash * pvhItem,
							 SG_wc_status_flags statusFlags)
{
	SG_string * pStringHeader = NULL;

	SG_UNUSED( statusFlags );

	SG_ERR_CHECK(  sg_vv2__diff__header(pCtx, pvhItem, &pStringHeader)  );
	SG_ERR_CHECK(  sg_vv2__diff__print_header_on_console(pCtx, pData, pStringHeader)  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringHeader);
}
