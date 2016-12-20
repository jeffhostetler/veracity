// TODO 2011/09/12 Move this to libraries/ut.
// TODO            There is nothing WC-specific in it.
//////////////////////////////////////////////////////////////////
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
 * @file sg_rbtree_i64.c
 *
 * @details A wrapper for SG_rbtree that uses SG_uint64 values for keys.
 * There are more efficient ways to do this.  For now, I just have a
 * fixed rule for converting the ui64 keys into strings and using the
 * normal rbtree code.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

#if defined(DEBUG)
#define TRACE_RBTREE_UI64 0
#else
#define TRACE_RBTREE_UI64 0
#endif

//////////////////////////////////////////////////////////////////

typedef char sg_buf_ui64[17];

//////////////////////////////////////////////////////////////////

void SG_rbtree_ui64__alloc(
	SG_context* pCtx,
	SG_rbtree_ui64** ppNew
	)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree__alloc(pCtx, (SG_rbtree **)ppNew)  );
}

void SG_rbtree_ui64__free__with_assoc(
	SG_context * pCtx,
	SG_rbtree_ui64 * ptree,
	SG_free_callback* cb
	)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree__free__with_assoc(pCtx,
													  (SG_rbtree *)ptree,
													  cb)  );
}

void SG_rbtree_ui64__free(
	SG_context * pCtx,
	SG_rbtree_ui64 * prb)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree__free(pCtx,
										  (SG_rbtree *)prb)  );
}

//////////////////////////////////////////////////////////////////

void SG_rbtree_ui64__add__with_assoc(
	SG_context* pCtx,
	SG_rbtree_ui64* prb,
	SG_uint64 ui64_key,
	void* assoc
	)
{
	sg_buf_ui64 bufUI64;
	(void)SG_hex__format_uint64(bufUI64, ui64_key);

	SG_ERR_CHECK_RETURN(  SG_rbtree__add__with_assoc(pCtx,
													 (SG_rbtree *)prb,
													 bufUI64,
													 assoc)  );
}

void SG_rbtree_ui64__update__with_assoc(
	SG_context* pCtx,
	SG_rbtree_ui64* prb,
	SG_uint64 ui64_key,
	void* assoc,
	void** pOldAssoc
	)
{
	sg_buf_ui64 bufUI64;
	(void)SG_hex__format_uint64(bufUI64, ui64_key);

	SG_ERR_CHECK_RETURN(  SG_rbtree__update__with_assoc(pCtx,
														(SG_rbtree *)prb,
														bufUI64,
														assoc,
														pOldAssoc)  );
}

void SG_rbtree_ui64__remove__with_assoc(
	SG_context * pCtx,
	SG_rbtree_ui64 * prb,
	SG_uint64 ui64_key,
	void ** ppAssoc
	)
{
	sg_buf_ui64 bufUI64;
	(void)SG_hex__format_uint64(bufUI64, ui64_key);

	SG_ERR_CHECK_RETURN(  SG_rbtree__remove__with_assoc(pCtx,
														(SG_rbtree *)prb,
														bufUI64,
														ppAssoc)  );
}

//////////////////////////////////////////////////////////////////

void SG_rbtree_ui64__find(
	SG_context* pCtx,
	const SG_rbtree_ui64* prb,
	SG_uint64 ui64_key,
    SG_bool* pbFound,
	void** pAssocData
	)
{
	sg_buf_ui64 bufUI64;
	(void)SG_hex__format_uint64(bufUI64, ui64_key);

	SG_ERR_CHECK_RETURN(  SG_rbtree__find(pCtx,
										  (const SG_rbtree *)prb,
										  bufUI64,
										  pbFound,
										  pAssocData)  );
}

//////////////////////////////////////////////////////////////////

void SG_rbtree_ui64__iterator__first(
	SG_context* pCtx,
	SG_rbtree_ui64_iterator **ppIterator,
	const SG_rbtree_ui64 *tree,
	SG_bool* pbOK,
	SG_uint64 * pui64_key,
	void** ppAssocData
	)
{
	SG_uint64 ui;
	const char * psz_key;
	void * pAssoc;
	SG_bool bOK;

	SG_ERR_CHECK_RETURN(  SG_rbtree__iterator__first(pCtx,
													 (SG_rbtree_iterator **)ppIterator,
													 (const SG_rbtree *)tree,
													 &bOK,
													 &psz_key,
													 &pAssoc)  );
	if (bOK)
	{
#if TRACE_RBTREE_UI64
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "Rbtree_ui64_first: %s\n", psz_key)  );
#endif
		SG_ERR_CHECK_RETURN(  SG_hex__parse_hex_uint64(pCtx, psz_key, SG_STRLEN(psz_key), &ui)  );

		*pbOK = SG_TRUE;
		*pui64_key = ui;
		*ppAssocData = pAssoc;
	}
	else
	{
		*pbOK = SG_FALSE;
		*pui64_key = 0ULL;
		*ppAssocData = NULL;
	}
}

void SG_rbtree_ui64__iterator__next(
	SG_context* pCtx,
	SG_rbtree_ui64_iterator *pIterator,
	SG_bool* pbOK,
	SG_uint64 * pui64_key,
	void** ppAssocData
	)
{
	SG_uint64 ui;
	const char * psz_key;
	void * pAssoc;
	SG_bool bOK;

	SG_ERR_CHECK_RETURN(  SG_rbtree__iterator__next(pCtx,
													(SG_rbtree_iterator *)pIterator,
													&bOK,
													&psz_key,
													&pAssoc)  );
	if (bOK)
	{
#if TRACE_RBTREE_UI64
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "Rbtree_ui64_next: %s\n", psz_key)  );
#endif
		SG_ERR_CHECK_RETURN(  SG_hex__parse_hex_uint64(pCtx, psz_key, SG_STRLEN(psz_key), &ui)  );

		*pbOK = SG_TRUE;
		*pui64_key = ui;
		*ppAssocData = pAssoc;
	}
	else
	{
		*pbOK = SG_FALSE;
		*pui64_key = 0ULL;
		*ppAssocData = NULL;
	}
}

void SG_rbtree_ui64__iterator__free(SG_context * pCtx, SG_rbtree_ui64_iterator * pIterator)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree__iterator__free(pCtx,
													(SG_rbtree_iterator *)pIterator)  );
}

//////////////////////////////////////////////////////////////////

struct _cb_interlude_data
{
	SG_rbtree_ui64_foreach_callback * pfn_cb_ui64;
	void * pVoidData_ui64;
};

static SG_rbtree_foreach_callback _foreach_cb_interlude;

static void _foreach_cb_interlude(SG_context * pCtx,
								  const char * pszKey,
								  void * pVoidAssoc,
								  void * pVoidData)
{
	struct _cb_interlude_data * pInterludeData = (struct _cb_interlude_data *)pVoidData;
	SG_uint64 ui64_key = 0ULL;

	SG_ERR_CHECK_RETURN(  SG_hex__parse_hex_uint64(pCtx, pszKey, SG_STRLEN(pszKey), &ui64_key)  );
	SG_ERR_CHECK_RETURN(  (*pInterludeData->pfn_cb_ui64)(pCtx,
														 ui64_key,
														 pVoidAssoc,
														 pInterludeData->pVoidData_ui64)  );
}

void SG_rbtree_ui64__foreach(
	SG_context* pCtx,
	const SG_rbtree_ui64* prb,
	SG_rbtree_ui64_foreach_callback* cb,
	void* ctx
	)
{
	struct _cb_interlude_data interludeData;

	interludeData.pfn_cb_ui64 = cb;
	interludeData.pVoidData_ui64 = ctx;

	SG_ERR_CHECK_RETURN(  SG_rbtree__foreach(pCtx,
											 (const SG_rbtree *)prb,
											 _foreach_cb_interlude,
											 &interludeData)  );
}


void SG_rbtree_ui64__count(
	SG_context * pCtx,
	const SG_rbtree_ui64 * prb,
	SG_uint32 * pCount)
{
	SG_ERR_CHECK_RETURN(  SG_rbtree__count(pCtx, (const SG_rbtree *)prb, pCount)  );
}
