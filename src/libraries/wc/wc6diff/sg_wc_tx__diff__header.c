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

void sg_wc_tx__diff__setup__header(SG_context * pCtx,
								   const SG_vhash * pvhItem,
								   SG_string ** ppStringHeader)
{
	SG_string * pStringHeader = NULL;
	SG_string * pStringDetail = NULL;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringHeader)  );
	SG_ERR_CHECK(  SG_string__append__format(pCtx, pStringHeader, "=== %s\n",
											 "================================================================")  );

	SG_ERR_CHECK(  SG_wc__status__classic_format2__item(pCtx, pvhItem, "=== ", "\n", &pStringDetail)  );
	SG_ERR_CHECK(  SG_string__append__string(pCtx, pStringHeader, pStringDetail)  );

	*ppStringHeader = pStringHeader;
	pStringHeader = NULL;
	
fail:
	SG_STRING_NULLFREE(pCtx, pStringHeader);
	SG_STRING_NULLFREE(pCtx, pStringDetail);
}
