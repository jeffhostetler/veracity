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
 * @file sg_cbuffer.c
 *
 */

#include <sg.h>

///////////////////////////////////////////////////////////////////////////////


void SG_cbuffer__alloc__new(
	SG_context * pCtx,
	SG_cbuffer ** ppCbuffer,
	SG_uint32 len
	)
{
	SG_byte * pBuf = NULL;
	SG_cbuffer * pCbuffer = NULL;

	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(ppCbuffer);

	SG_ERR_CHECK_RETURN(  SG_alloc1(pCtx, pCbuffer)  );

	SG_ERR_CHECK(  SG_allocN(pCtx, SG_MAX(len,1), pBuf)  );
	pCbuffer->pBuf = pBuf;
	pCbuffer->len = len;

	*ppCbuffer = pCbuffer;

	return;
fail:
	SG_NULLFREE(pCtx, pCbuffer);
	SG_NULLFREE(pCtx, pBuf);
}

void SG_cbuffer__alloc__string(
	SG_context * pCtx,
	SG_cbuffer ** ppCbuffer,
	SG_string ** ppString
	)
{
	SG_cbuffer * pCbuffer = NULL;

	SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(ppCbuffer);
	SG_NULL_PP_CHECK_RETURN(ppString);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pCbuffer)  );
	SG_ERR_CHECK(  SG_string__sizzle(pCtx, ppString, &pCbuffer->pBuf, &pCbuffer->len)  );
	*ppCbuffer = pCbuffer;

	return;
fail:
	SG_NULLFREE(pCtx, pCbuffer);
}

void SG_cbuffer__nullfree(
	SG_cbuffer ** ppCbuffer
	)
{
	if(ppCbuffer!=NULL && *ppCbuffer!=NULL)
	{
		SG_free__no_ctx((*ppCbuffer)->pBuf);
		SG_free__no_ctx(*ppCbuffer);
		*ppCbuffer = NULL;
	}
}
