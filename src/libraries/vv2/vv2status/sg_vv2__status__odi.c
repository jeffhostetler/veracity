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

#include <sg_wc__public_typedefs.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

void sg_vv2__status__odi__free(SG_context * pCtx, sg_vv2status_odi * pInst)
{
	if (!pInst)
		return;

	SG_NULLFREE(pCtx, pInst);
}

void sg_vv2__status__odi__alloc(SG_context * pCtx, sg_vv2status_odi ** ppInst)
{
	sg_vv2status_odi * pInst = NULL;

	SG_ERR_CHECK(  SG_alloc(pCtx, 1,sizeof(sg_vv2status_odi),&pInst)  );
	pInst->depthInTree = -1;

	// we let caller fill in the other fields.

	*ppInst = pInst;
	return;

fail:
	SG_ERR_IGNORE(  sg_vv2__status__odi__free(pCtx, pInst)  );
}
