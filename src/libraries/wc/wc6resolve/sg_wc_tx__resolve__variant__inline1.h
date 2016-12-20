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
 * @file sg_wc_tx__resolve__variant__inline1.h
 *
 * @details Variant-related helper functions that I pulled from the
 * PendingTree version of sg_resolve.c
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_WC_TX__RESOLVE__VARIANT__INLINE1_H
#define H_SG_WC_TX__RESOLVE__VARIANT__INLINE1_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Allocates a variant and sets its value to a bool.
 */
static void _variant__alloc__bool(
	SG_context*  pCtx,      //< [in] [out] Error and context info.
	SG_variant** ppVariant, //< [out] The allocated variant.
	SG_uint64*   pSize,     //< [out] Set to the sizeof(bValue), if specified.
	SG_bool      bValue     //< [in] The value to set to the variant.
	)
{
	SG_variant* pVariant = NULL;

	SG_NULLARGCHECK(ppVariant);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pVariant)  );

	pVariant->type = SG_VARIANT_TYPE_BOOL;
	pVariant->v.val_bool = bValue;

	*ppVariant = pVariant;
	pVariant = NULL;

	if (pSize != NULL)
	{
		*pSize = sizeof(bValue);
	}

fail:
	SG_VARIANT_NULLFREE(pCtx, pVariant);
	return;
}

/**
 * Allocates a variant and sets its value to an int.
 */
static void _variant__alloc__int64(
	SG_context*  pCtx,      //< [in] [out] Error and context info.
	SG_variant** ppVariant, //< [out] The allocated variant.
	SG_uint64*   pSize,     //< [out] Set to the sizeof(iValue), if specified.
	SG_int64     iValue     //< [in] The value to set to the variant.
	)
{
	SG_variant* pVariant = NULL;

	SG_NULLARGCHECK(ppVariant);

	SG_ERR_CHECK(  SG_alloc1(pCtx, pVariant)  );

	pVariant->type = SG_VARIANT_TYPE_INT64;
	pVariant->v.val_int64 = iValue;

	*ppVariant = pVariant;
	pVariant = NULL;

	if (pSize != NULL)
	{
		*pSize = sizeof(iValue);
	}

fail:
	SG_VARIANT_NULLFREE(pCtx, pVariant);
	return;
}

/**
 * Allocates a variant and sets its value to a copy of a string.
 */
static void _variant__alloc__sz(
	SG_context*  pCtx,      //< [in] [out] Error and context info.
	SG_variant** ppVariant, //< [out] The allocated variant.
	SG_uint64*   pSize,     //< [out] Set to strlen(szValue), if specified.
	const char*  szValue    //< [in] The string to copy and set to the variant.
	)
{
	SG_variant* pVariant  = NULL;
	char*       szMyValue = NULL;

	SG_NULLARGCHECK(ppVariant);
	SG_NULLARGCHECK(szValue);

	SG_ERR_CHECK(  SG_strdup(pCtx, szValue, &szMyValue)  );
	SG_ERR_CHECK(  SG_alloc1(pCtx, pVariant)  );

	pVariant->type = SG_VARIANT_TYPE_SZ;
	pVariant->v.val_sz = szMyValue;
	szMyValue = NULL;

	*ppVariant = pVariant;
	pVariant = NULL;

	if (pSize != NULL)
	{
		*pSize = strlen(szValue);
	}

fail:
	SG_VARIANT_NULLFREE(pCtx, pVariant);
	SG_NULLFREE(pCtx, szMyValue);
	return;
}

/**
 * Allocates a variant and sets its value to the contents of a blob.
 */
static void _variant__alloc__blob(
	SG_context*  pCtx,      //< [in] [out] Error and context info.
	SG_wc_tx * pWcTx,
	SG_variant** ppVariant, //< [out] The allocated variant.
	SG_uint64*   pSize,     //< [out] Set to the size of the blob, if specified.
	const char*  szHid      //< [in] The HID of the blob to look up.
	)
{
	SG_byte*    pBuffer   = NULL;
	SG_uint64   uSize     = 0u;
	SG_variant* pVariant  = NULL;
	SG_string*  sContents = NULL;

	SG_NULLARGCHECK(pWcTx);
	SG_NULLARGCHECK(ppVariant);
	SG_NULLARGCHECK(szHid);

	// retrieve the HID contents from the repo
	// 
	// TODO 2012/03/21 This feels a little weird. We
	// TODO            [] fetch the blob into a raw buffer (buf,len)-style
	// TODO            [] allocate a string and copy the buffer into it
	// TODO               (i'm assuming that this is to get a null terminator).
	// TODO            [] steal the buffer from the string and give it to
	// TODO               the variant.
	// TODO
	// TODO            () we are making an extra copy of the buffer just
	// TODO               to get the null
	// TODO            () we are assuming that the blob does not contain
	// TODO               binary data (containing an actual null)
	// TODO            () we are throwing away the length info
	// TODO
	// TODO            I think is only used to get the symlink target, so
	// TODO            we should be OK.

	SG_ERR_CHECK(  SG_repo__fetch_blob_into_memory(pCtx, pWcTx->pDb->pRepo, szHid, &pBuffer, &uSize)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__BUF_LEN(pCtx, &sContents, pBuffer, (SG_uint32)uSize)  );

	// allocate the variant
	SG_ERR_CHECK(  SG_alloc1(pCtx, pVariant)  );
	pVariant->type = SG_VARIANT_TYPE_SZ;
	SG_ERR_CHECK(  SG_string__sizzle(pCtx, &sContents, (SG_byte**)&pVariant->v.val_sz, NULL)  );

	// return the variant
	*ppVariant = pVariant;
	pVariant = NULL;

	// return the size, if they want it
	if (pSize != NULL)
	{
		*pSize = uSize;
	}

fail:
	SG_NULLFREE(pCtx, pBuffer);
	SG_VARIANT_NULLFREE(pCtx, pVariant);
	SG_STRING_NULLFREE(pCtx, sContents);
	return;
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__VARIANT__INLINE1_H
