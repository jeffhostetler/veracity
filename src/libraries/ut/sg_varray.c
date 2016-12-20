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

#include <sg.h>

static const SG_uint32 guDefaultSize = 16u;

struct _SG_varray
{
	SG_strpool *pStrPool;
	SG_bool strpool_is_mine;
	SG_varpool *pVarPool;
	SG_bool varpool_is_mine;
	SG_variant** aSlots;
	SG_uint32 space;
	SG_uint32 count;
};

void SG_varray__count(SG_context* pCtx, const SG_varray* pva, SG_uint32* piResult)
{
	SG_NULLARGCHECK_RETURN(pva);
	SG_NULLARGCHECK_RETURN(piResult);
	*piResult = pva->count;
}

void sg_varray__grow(SG_context* pCtx, SG_varray* pa)
{
	SG_uint32 new_space = pa->space * 2;
	SG_variant** new_aSlots = NULL;

	SG_ERR_CHECK_RETURN(  SG_alloc(pCtx, new_space, sizeof(SG_variant*), &new_aSlots)  );

	memcpy(new_aSlots, pa->aSlots, pa->count * sizeof(SG_variant*));
	SG_NULLFREE(pCtx, pa->aSlots);
	pa->aSlots = new_aSlots;
	pa->space = new_space;
}

void SG_varray__alloc__params(SG_context* pCtx, SG_varray** ppResult, SG_uint32 initial_space, SG_strpool* pStrPool, SG_varpool* pVarPool)
{
	SG_varray * pThis;

	SG_NULLARGCHECK_RETURN(ppResult);

	if (initial_space == 0u)
	{
		initial_space = guDefaultSize;
	}

	SG_ERR_CHECK(  SG_alloc1(pCtx, pThis)  );

	if (pStrPool)
	{
		pThis->strpool_is_mine = SG_FALSE;
		pThis->pStrPool = pStrPool;
	}
	else
	{
		pThis->strpool_is_mine = SG_TRUE;
		SG_ERR_CHECK(  SG_STRPOOL__ALLOC(pCtx, &pThis->pStrPool, initial_space * 10)  );
	}

	if (pVarPool)
	{
		pThis->varpool_is_mine = SG_FALSE;
		pThis->pVarPool = pVarPool;
	}
	else
	{
		pThis->varpool_is_mine = SG_TRUE;
		SG_ERR_CHECK(  SG_VARPOOL__ALLOC(pCtx, &pThis->pVarPool, initial_space)  );
	}

	pThis->space = initial_space;

	SG_ERR_CHECK(  SG_alloc(pCtx, pThis->space, sizeof(SG_variant *), &pThis->aSlots)  );

	*ppResult = pThis;

	return;
fail:
	if (pThis)
	{
		if (pThis->strpool_is_mine && pThis->pStrPool)
		{
			SG_STRPOOL_NULLFREE(pCtx, pThis->pStrPool);
		}

		if (pThis->varpool_is_mine && pThis->pVarPool)
		{
			SG_VARPOOL_NULLFREE(pCtx, pThis->pVarPool);
		}

		if (pThis->aSlots)
		{
			SG_NULLFREE(pCtx, pThis->aSlots);
		}

		SG_NULLFREE(pCtx, pThis);
	}
}

void SG_varray__alloc__shared(
        SG_context* pCtx,
        SG_varray** ppResult,
        SG_uint32 initial_size,
        SG_varray* pva_other
        )
{
	SG_ERR_CHECK_RETURN(  SG_varray__alloc__params(pCtx, ppResult, initial_size, pva_other->pStrPool, pva_other->pVarPool)  );		// do not use SG_VARRAY__ALLOC__PARAMS
}

void SG_varray__alloc__copy(
	SG_context*      pCtx,
	SG_varray**      ppNew,
	const SG_varray* pBase
	)
{
	SG_varray* pResult = NULL;

	SG_NULLARGCHECK(ppNew);
	SG_NULLARGCHECK(pBase);

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pResult)  );
	SG_ERR_CHECK(  SG_varray__copy_items(pCtx, pBase, pResult)  );

	*ppNew = pResult;
	pResult = NULL;

fail:
	SG_VARRAY_NULLFREE(pCtx, pResult);
	return;
}

void SG_varray__alloc(SG_context* pCtx, SG_varray** ppResult)
{
	SG_ERR_CHECK_RETURN(  SG_varray__alloc__params(pCtx, ppResult, guDefaultSize, NULL, NULL)  );		// do not use SG_VARRAY__ALLOC__PARAMS
}

void SG_varray__alloc__stringarray(SG_context* pCtx, SG_varray** ppNew, const SG_stringarray* pStringarray)
{
    SG_varray * pNew = NULL;
    SG_uint32 count = 0;
    SG_uint32 i;

    SG_ASSERT(pCtx!=NULL);
    SG_NULLARGCHECK_RETURN(ppNew);
    SG_NULLARGCHECK_RETURN(pStringarray);

    SG_ERR_CHECK(  SG_stringarray__count(pCtx, pStringarray, &count )  );
    SG_ERR_CHECK(  SG_varray__alloc__params(pCtx, &pNew, count, NULL, NULL)  );
    for(i=0;i<count;++i)
    {
        const char * sz = NULL;
        SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pStringarray, i, &sz)  );
        SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pNew, sz)  );
    }

    *ppNew = pNew;

    return;
fail:
    SG_VARRAY_NULLFREE(pCtx, pNew);
}

void SG_varray__free(SG_context * pCtx, SG_varray* pThis)
{
	SG_uint32 i;

	if (!pThis)
	{
		return;
	}

	for (i=0; i<pThis->count; i++)
	{
		switch (pThis->aSlots[i]->type)
		{
		case SG_VARIANT_TYPE_VARRAY:
			SG_VARRAY_NULLFREE(pCtx, pThis->aSlots[i]->v.val_varray);
			break;
		case SG_VARIANT_TYPE_VHASH:
			SG_VHASH_NULLFREE(pCtx, pThis->aSlots[i]->v.val_vhash);
			break;
		}
	}

	if (pThis->strpool_is_mine && pThis->pStrPool)
	{
		SG_STRPOOL_NULLFREE(pCtx, pThis->pStrPool);
	}

	if (pThis->varpool_is_mine && pThis->pVarPool)
	{
		SG_VARPOOL_NULLFREE(pCtx, pThis->pVarPool);
	}

	if (pThis->aSlots)
	{
		SG_NULLFREE(pCtx, pThis->aSlots);
	}

	SG_NULLFREE(pCtx, pThis);
}

void sg_varray__append(SG_context* pCtx, SG_varray* pva, SG_variant** ppv)
{
	if ((pva->count + 1) > pva->space)
	{
		SG_ERR_CHECK_RETURN(  sg_varray__grow(pCtx, pva)  );
	}

	SG_ERR_CHECK_RETURN(  SG_varpool__add(pCtx, pva->pVarPool, &(pva->aSlots[pva->count]))  );

	*ppv = pva->aSlots[pva->count++];
}

void SG_varray__remove(SG_context *pCtx, SG_varray *pThis, SG_uint32 idx)
{
	SG_uint32 i;

	SG_NULLARGCHECK_RETURN(pThis);
	if (idx >= pThis->count)
	{
		SG_ERR_THROW_RETURN(SG_ERR_VARRAY_INDEX_OUT_OF_RANGE);
	}

	switch (pThis->aSlots[idx]->type)
	{
		case SG_VARIANT_TYPE_VARRAY:
			SG_VARRAY_NULLFREE(pCtx, pThis->aSlots[idx]->v.val_varray);
			break;
		case SG_VARIANT_TYPE_VHASH:
			SG_VHASH_NULLFREE(pCtx, pThis->aSlots[idx]->v.val_vhash);
			break;
	}	

	for ( i = idx; i < pThis->count - 1; ++i )
	{
		pThis->aSlots[i]->type = pThis->aSlots[i + 1]->type;
		pThis->aSlots[i]->v = pThis->aSlots[i + 1]->v;
	}

	pThis->count--;
}

void SG_varray__append__string__sz(SG_context* pCtx, SG_varray* pva, const char* putf8Value)
{
	SG_ERR_CHECK_RETURN(  SG_varray__append__string__buflen(pCtx, pva, putf8Value, 0) );
}

void SG_varray__append__string__buflen(SG_context* pCtx, SG_varray* pva, const char* putf8Value, SG_uint32 len)
{
	SG_variant* pv = NULL;

	SG_NULLARGCHECK_RETURN(pva);
	SG_NULLARGCHECK_RETURN(putf8Value);

    SG_ERR_CHECK_RETURN(  sg_varray__append(pCtx, pva, &pv)  );

    pv->type = SG_VARIANT_TYPE_SZ;

    SG_ERR_CHECK_RETURN(  SG_strpool__add__buflen__sz(pCtx, pva->pStrPool, putf8Value, len, &(pv->v.val_sz))  );
}

#ifndef SG_IOS
void SG_varray__append__jsstring(SG_context* pCtx, SG_varray* pva, JSContext *cx, JSString *str)
{
	SG_variant* pv = NULL;

	SG_NULLARGCHECK_RETURN(pva);
	SG_NULLARGCHECK_RETURN(cx);
	SG_NULLARGCHECK_RETURN(str);

    SG_ERR_CHECK_RETURN(  sg_varray__append(pCtx, pva, &pv)  );

    pv->type = SG_VARIANT_TYPE_SZ;

    SG_ERR_CHECK_RETURN(  SG_strpool__add__jsstring(pCtx, pva->pStrPool, cx, str, &(pv->v.val_sz))  );
}
#endif

void SG_varray__append__int64(SG_context* pCtx, SG_varray* pva, SG_int64 ival)
{
	SG_variant* pv = NULL;

	SG_NULLARGCHECK_RETURN(pva);
	
	SG_ERR_CHECK_RETURN(  sg_varray__append(pCtx, pva, &pv)  );

	pv->type = SG_VARIANT_TYPE_INT64;
	pv->v.val_int64 = ival;
}

void SG_varray__append__double(SG_context* pCtx, SG_varray* pva, double fv)
{
	SG_variant* pv = NULL;

	SG_NULLARGCHECK_RETURN(pva);

	SG_ERR_CHECK_RETURN(  sg_varray__append(pCtx, pva, &pv)  );

	pv->type = SG_VARIANT_TYPE_DOUBLE;
	pv->v.val_double = fv;
}

void SG_varray__appendnew__vhash(SG_context* pCtx, SG_varray* pva, SG_vhash** ppvh)
{
    SG_vhash* pvh_sub = NULL;
    SG_vhash* pvh_result = NULL;

	SG_NULLARGCHECK_RETURN(pva);

    SG_ERR_CHECK(  SG_vhash__alloc__params(pCtx, &pvh_sub, 0, pva->pStrPool, pva->pVarPool)  );
    pvh_result = pvh_sub;
    SG_ERR_CHECK(  SG_varray__append__vhash(pCtx, pva, &pvh_sub)  );

    *ppvh = pvh_result;
    pvh_result = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_sub);
}

void SG_varray__append__vhash(SG_context* pCtx, SG_varray* pva, SG_vhash** ppvh)
{
	SG_variant* pv = NULL;

	SG_NULLARGCHECK_RETURN(pva);
	SG_NULLARGCHECK_RETURN(ppvh);
	SG_NULLARGCHECK_RETURN(*ppvh);

    SG_ERR_CHECK_RETURN(  sg_varray__append(pCtx, pva, &pv)  );

    pv->type = SG_VARIANT_TYPE_VHASH;
    pv->v.val_vhash = *ppvh;
    *ppvh = NULL;
}

void SG_varray__appendnew__varray(SG_context* pCtx, SG_varray* pva, SG_varray** ppva)
{
    SG_varray* pva_sub = NULL;
    SG_varray* pva_result = NULL;

	SG_NULLARGCHECK_RETURN(pva);

	SG_ERR_CHECK(  SG_varray__alloc__params(pCtx, &pva_sub, 4, pva->pStrPool, pva->pVarPool)  );
    pva_result = pva_sub;
    SG_ERR_CHECK(  SG_varray__append__varray(pCtx, pva, &pva_sub)  );

    *ppva = pva_result;
    pva_result = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_sub);
    ;
}

void SG_varray__append__varray(SG_context* pCtx, SG_varray* pva, SG_varray** ppva)
{
	SG_variant* pv = NULL;

	SG_NULLARGCHECK_RETURN(pva);
	SG_NULLARGCHECK_RETURN(ppva);
	SG_NULLARGCHECK_RETURN(*ppva);

    SG_ERR_CHECK_RETURN(  sg_varray__append(pCtx, pva, &pv)  );

    pv->type = SG_VARIANT_TYPE_VARRAY;
    pv->v.val_varray = *ppva;
    *ppva = NULL;
}

void SG_varray__append__bool(SG_context* pCtx, SG_varray* pva, SG_bool b)
{
	SG_variant* pv = NULL;

	SG_NULLARGCHECK_RETURN(pva);

	SG_ERR_CHECK_RETURN(  sg_varray__append(pCtx, pva, &pv)  );

	pv->type = SG_VARIANT_TYPE_BOOL;
	pv->v.val_bool = b;
}

void SG_varray__append__null(SG_context* pCtx, SG_varray* pva)
{
	SG_variant* pv = NULL;

	SG_NULLARGCHECK_RETURN(pva);

	SG_ERR_CHECK_RETURN(  sg_varray__append(pCtx, pva, &pv)  );

	pv->type = SG_VARIANT_TYPE_NULL;
}

void SG_varray__appendcopy__varray(
	SG_context*      pCtx,
	SG_varray*       pThis,
	const SG_varray* pValue,
    SG_varray** pp
	)
{
	SG_varray* pArray = NULL;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pValue);

	SG_ERR_CHECK(  SG_varray__appendnew__varray(pCtx, pThis, &pArray)  );
	SG_ERR_CHECK(  SG_varray__copy_items(pCtx, pValue, pArray)  );

    if (pp)
    {
        *pp = pArray;
    }

fail:
	return;
}

void SG_varray__appendcopy__vhash(
	SG_context*     pCtx,
	SG_varray*      pThis,
	const SG_vhash* pValue,
    SG_vhash** pp
	)
{
	SG_vhash* pHash = NULL;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pValue);

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pThis, &pHash)  );
	SG_ERR_CHECK(  SG_vhash__copy_items(pCtx, pValue, pHash)  );

    if (pp)
    {
        *pp = pHash;
    }

fail:
	return;
}

void SG_varray__appendcopy__variant(
	SG_context*       pCtx,
	SG_varray*        pThis,
	const SG_variant* pValue
	)
{
	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pValue);

	switch (pValue->type)
	{
	case SG_VARIANT_TYPE_DOUBLE:
		SG_ERR_CHECK(  SG_varray__append__double(pCtx, pThis, pValue->v.val_double)  );
		break;

	case SG_VARIANT_TYPE_INT64:
		SG_ERR_CHECK(  SG_varray__append__int64(pCtx, pThis, pValue->v.val_int64)  );
		break;

	case SG_VARIANT_TYPE_BOOL:
		SG_ERR_CHECK(  SG_varray__append__bool(pCtx, pThis, pValue->v.val_bool)  );
		break;

	case SG_VARIANT_TYPE_NULL:
		SG_ERR_CHECK(  SG_varray__append__null(pCtx, pThis)  );
		break;

	case SG_VARIANT_TYPE_SZ:
		SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pThis, pValue->v.val_sz)  );
		break;

	case SG_VARIANT_TYPE_VARRAY:
		SG_ERR_CHECK(  SG_varray__appendcopy__varray(pCtx, pThis, pValue->v.val_varray, NULL)  );
		break;
	
	case SG_VARIANT_TYPE_VHASH:
		SG_ERR_CHECK(  SG_varray__appendcopy__vhash(pCtx, pThis, pValue->v.val_vhash, NULL)  );
		break;

	default:
		SG_ERR_THROW2(SG_ERR_INVALIDARG, (pCtx, "Variant has unknown type: %d", pValue->type));
		break;
	}

fail:
	return;
}

void sg_varray__write_json_callback(SG_context* pCtx, void* ctx, const SG_varray* pva, SG_uint32 ndx, const SG_variant* pv)
{
	SG_jsonwriter* pjson = (SG_jsonwriter*) ctx;

	SG_UNUSED(pva);
	SG_UNUSED(ndx);

	SG_ERR_CHECK_RETURN(  SG_jsonwriter__write_element__variant(pCtx, pjson, pv)  );
}

void SG_varray__foreach(SG_context* pCtx, const SG_varray* pva, SG_varray_foreach_callback cb, void* ctx)
{
	SG_uint32 i;

	SG_NULLARGCHECK_RETURN(pva);

	for (i=0; i<pva->count; i++)
	{
		SG_ERR_CHECK_RETURN(  cb(pCtx, ctx, pva, i, pva->aSlots[i])  );
	}
}

void SG_varray__write_json(SG_context* pCtx, const SG_varray* pva, SG_jsonwriter* pjson)
{
	SG_ERR_CHECK_RETURN(  SG_jsonwriter__write_start_array(pCtx, pjson)  );

	SG_ERR_CHECK_RETURN(  SG_varray__foreach(pCtx, pva, sg_varray__write_json_callback, pjson)  );

	SG_ERR_CHECK_RETURN(  SG_jsonwriter__write_end_array(pCtx, pjson)  );
}

void SG_varray__to_json__pretty_print_NOT_for_storage(SG_context* pCtx, const SG_varray* pva, SG_string* pStr)
{
	SG_jsonwriter* pjson = NULL;

	SG_NULLARGCHECK_RETURN(pva);
	SG_NULLARGCHECK_RETURN(pStr);

	SG_ERR_CHECK(  SG_jsonwriter__alloc__pretty_print_NOT_for_storage(pCtx, &pjson, pStr)  );

	SG_ERR_CHECK(  SG_varray__write_json(pCtx, pva, pjson)  );

fail:
    SG_JSONWRITER_NULLFREE(pCtx, pjson);
}

void SG_varray__to_json(SG_context* pCtx, const SG_varray* pva, SG_string* pStr)
{
	SG_jsonwriter* pjson = NULL;

	SG_NULLARGCHECK_RETURN(pva);
	SG_NULLARGCHECK_RETURN(pStr);

	SG_ERR_CHECK(  SG_jsonwriter__alloc(pCtx, &pjson, pStr)  );

	SG_ERR_CHECK(  SG_varray__write_json(pCtx, pva, pjson)  );

fail:
    SG_JSONWRITER_NULLFREE(pCtx, pjson);
}

void SG_varray__equal(SG_context* pCtx, const SG_varray* pv1, const SG_varray* pv2, SG_bool* pbResult)
{
	SG_uint32 i;

	if (pv1->count != pv2->count)
	{
		*pbResult = SG_FALSE;
		return;
	}

	for (i=0; i<pv1->count; i++)
	{
		SG_bool b = SG_FALSE;

		SG_ERR_CHECK_RETURN(  SG_variant__equal(pCtx, pv1->aSlots[i], pv2->aSlots[i], &b)  );

		if (!b)
		{
			*pbResult = SG_FALSE;
			return;
		}
	}

	*pbResult = SG_TRUE;
}

void SG_varray__get__variant(SG_context* pCtx, const SG_varray* pva, SG_uint32 ndx, const SG_variant** pResult)
{
	if (ndx >= pva->count)
	{
		SG_ERR_THROW_RETURN(SG_ERR_VARRAY_INDEX_OUT_OF_RANGE);
	}

	*pResult = pva->aSlots[ndx];
}

void SG_varray__get__sz(SG_context* pCtx, const SG_varray* pva, SG_uint32 ndx, const char** pResult)
{
	if (ndx >= pva->count)
	{
		SG_ERR_THROW_RETURN(SG_ERR_VARRAY_INDEX_OUT_OF_RANGE);
	}

	SG_ERR_CHECK_RETURN(  SG_variant__get__sz(pCtx, pva->aSlots[ndx], pResult)  );
}

void SG_varray__get__int64(SG_context* pCtx, const SG_varray* pva, SG_uint32 ndx, SG_int64* pResult)
{
	if (ndx >= pva->count)
	{
		SG_ERR_THROW_RETURN(SG_ERR_VARRAY_INDEX_OUT_OF_RANGE);
	}

	SG_ERR_CHECK_RETURN(  SG_variant__get__int64(pCtx, pva->aSlots[ndx], pResult)  );
}

void SG_varray__get__double(SG_context* pCtx, const SG_varray* pva, SG_uint32 ndx, double* pResult)
{
	SG_NULLARGCHECK_RETURN(pva);

	if (ndx >= pva->count)
	{
		SG_ERR_THROW_RETURN(SG_ERR_VARRAY_INDEX_OUT_OF_RANGE);
	}

	SG_ERR_CHECK_RETURN(  SG_variant__get__double(pCtx, pva->aSlots[ndx], pResult)  );
}

void SG_varray__get__bool(SG_context* pCtx, const SG_varray* pva, SG_uint32 ndx, SG_bool* pResult)
{
	SG_NULLARGCHECK_RETURN(pva);

	if (ndx >= pva->count)
	{
		SG_ERR_THROW_RETURN(SG_ERR_VARRAY_INDEX_OUT_OF_RANGE);
	}

	SG_ERR_CHECK_RETURN(  SG_variant__get__bool(pCtx, pva->aSlots[ndx], pResult)  );
}

void SG_varray__get__varray(SG_context* pCtx, const SG_varray* pva, SG_uint32 ndx, SG_varray** pResult)  // TODO should the result be const?
{
	SG_NULLARGCHECK_RETURN(pva);

	if (ndx >= pva->count)
	{
		SG_ERR_THROW_RETURN(SG_ERR_VARRAY_INDEX_OUT_OF_RANGE);
	}

	SG_ERR_CHECK_RETURN(  SG_variant__get__varray(pCtx, pva->aSlots[ndx], pResult)  );
}

void SG_varray__get__vhash(SG_context* pCtx, const SG_varray* pva, SG_uint32 ndx, SG_vhash** pResult)  // TODO should the result be const?
{
	SG_NULLARGCHECK_RETURN(pva);

	if (ndx >= pva->count)
	{
		SG_ERR_THROW_RETURN(SG_ERR_VARRAY_INDEX_OUT_OF_RANGE);
	}

	SG_ERR_CHECK_RETURN(  SG_variant__get__vhash(pCtx, pva->aSlots[ndx], pResult)  );
}

void SG_varray__typeof(SG_context* pCtx, const SG_varray* pva, SG_uint32 ndx, SG_uint16* pResult)
{
	if (ndx >= pva->count)
	{
		SG_ERR_THROW_RETURN(SG_ERR_VARRAY_INDEX_OUT_OF_RANGE);
	}

	*pResult = pva->aSlots[ndx]->type;
}

void SG_varray__reverse(SG_context* pCtx, SG_varray * pva)
{
	SG_NULLARGCHECK_RETURN(pva);

    if (pva->count > 0)
    {
        SG_uint32 i = 0;

        for (i=0; i<=(pva->count-1)/2; i++)
        {
            SG_uint32 j = pva->count - 1 - i;
            SG_variant* t = NULL;

            t = pva->aSlots[i];
            pva->aSlots[i] = pva->aSlots[j];
            pva->aSlots[j] = t;
        }
    }
}

void SG_varray__sort(SG_context* pCtx, SG_varray * pva, SG_qsort_compare_function pfnCompare, void* p)
{
	SG_NULLARGCHECK_RETURN(pva);
	SG_ARGCHECK_RETURN( (pfnCompare!=NULL), pfnCompare );

	if (pva->count > 1)
	{
		/* We used to check every item for sortability, but since we can have a custom callback
		 * to sort vhashes by a specific key, it's better to let the comparer check for sortability
		 * (which they do).
		 */
		/* Now the sort should be completely safe, so just do it */

		SG_ERR_CHECK_RETURN(  SG_qsort(pCtx,
									   pva->aSlots,pva->count,sizeof(SG_variant *),
									   pfnCompare,
									   p)  );
	}
}

//////////////////////////////////////////////////////////////////

static void _sg_varray__to_idset_callback(
	SG_context* pCtx,
	void * ctx,
	SG_UNUSED_PARAM(const SG_varray * pva),
	SG_UNUSED_PARAM(SG_uint32 ndx),
	const SG_variant * pVariant
	)
{
	SG_rbtree * pidset = (SG_rbtree *)ctx;
	const char * szTemp;

	SG_UNUSED(pva);
	SG_UNUSED(ndx);

	SG_variant__get__sz(pCtx,pVariant,&szTemp);
	// might be SG_ERR_VARIANT_INVALIDTYPE
	SG_ERR_CHECK_RETURN_CURRENT;
	if (!szTemp)					// happens when SG_VARIANT_TYPE_NULL
		return;						// silently eat it.

	/* TODO should we silently eat the SG_VARIANT_TYPE_NULL case in
	 * this context?  In what situation are we converting a varray to
	 * an idset where we want to allow NULLs? */

	SG_ERR_CHECK_RETURN(  SG_rbtree__add(pCtx, pidset,szTemp)  );
}

void SG_varray__to_rbtree(SG_context* pCtx, const SG_varray * pva, SG_rbtree ** ppidset)
{
	SG_rbtree * pidset = NULL;

	SG_NULLARGCHECK_RETURN(pva);
	SG_NULLARGCHECK_RETURN(ppidset);

	SG_ERR_CHECK_RETURN(  SG_RBTREE__ALLOC__PARAMS(pCtx, &pidset, pva->count, NULL)  );

	SG_ERR_CHECK(  SG_varray__foreach(pCtx, pva,_sg_varray__to_idset_callback,pidset)  );

	*ppidset = pidset;
    pidset = NULL;

fail:
	SG_RBTREE_NULLFREE(pCtx, pidset);
}

static void _sg_varray__to_stringarray_callback(
	SG_context* pCtx,
	void * ctx,
	SG_UNUSED_PARAM(const SG_varray * pva),
	SG_UNUSED_PARAM(SG_uint32 ndx),
	const SG_variant * pVariant
	)
{
	SG_stringarray * psa = (SG_stringarray *)ctx;
	const char * szTemp;

	SG_UNUSED(pva);
	SG_UNUSED(ndx);

	SG_variant__get__sz(pCtx,pVariant,&szTemp);
	// might be SG_ERR_VARIANT_INVALIDTYPE
	SG_ERR_CHECK_RETURN_CURRENT;
	if (!szTemp)					// happens when SG_VARIANT_TYPE_NULL
		return;						// silently eat it.

	/* TODO should we silently eat the SG_VARIANT_TYPE_NULL case in
	 * this context?  In what situation are we converting a varray to
	 * an idset where we want to allow NULLs? */

	SG_ERR_CHECK_RETURN(  SG_stringarray__add(pCtx, psa, szTemp)  );
}

void SG_varray__to_stringarray(SG_context* pCtx, const SG_varray * pva, SG_stringarray ** ppsa)
{
	SG_stringarray * psa = NULL;

	SG_NULLARGCHECK_RETURN(pva);
	SG_NULLARGCHECK_RETURN(ppsa);

	SG_ERR_CHECK_RETURN(  SG_STRINGARRAY__ALLOC(pCtx, &psa, 1 + pva->count)  );

	SG_ERR_CHECK(  SG_varray__foreach(pCtx, pva,_sg_varray__to_stringarray_callback,psa)  );

	*ppsa = psa;
    psa = NULL;

	return;
fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psa);
}

void SG_varray__to_vhash_with_indexes(SG_context* pCtx, const SG_varray * pva, SG_vhash ** ppvh)
{
    SG_uint32 i = 0;
    SG_uint32 count = 0;
    SG_vhash* pvh = NULL;

	SG_NULLARGCHECK_RETURN(pva);
	SG_NULLARGCHECK_RETURN(ppvh);

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
    SG_ERR_CHECK(  SG_vhash__alloc__params(pCtx, &pvh, count, NULL, NULL)  );
    for (i=0; i<count; i++)
    {
        const char* psz = NULL;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva, i, &psz)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, psz, (SG_int64) i)  );
    }

	*ppvh = pvh;
    pvh = NULL;

	return;
fail:
	SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_varray__to_vhash_with_indexes__update(SG_context* pCtx, const SG_varray * pva, SG_vhash ** ppvh)
{
    SG_uint32 i = 0;
    SG_uint32 count = 0;
    SG_vhash* pvh = NULL;

	SG_NULLARGCHECK_RETURN(pva);
	SG_NULLARGCHECK_RETURN(ppvh);

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
    SG_ERR_CHECK(  SG_vhash__alloc__params(pCtx, &pvh, count, NULL, NULL)  );
    for (i=0; i<count; i++)
    {
        const char* psz = NULL;

        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva, i, &psz)  );
        SG_ERR_CHECK(  SG_vhash__update__int64(pCtx, pvh, psz, (SG_int64) i)  );
    }

	*ppvh = pvh;
    pvh = NULL;

fail:
	SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_varray_debug__dump_varray_of_vhashes_to_console(SG_context * pCtx, const SG_varray * pva, const char * pszLabel)
{
	SG_vhash * pvhItem = NULL;
	SG_string * pString = NULL;
	SG_uint32 k = 0, count = 0;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pString)  );

	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, ("VARRAY-of-VHASH Dump: %s\n"), ((pszLabel) ? pszLabel : ""))  );

	SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
	for (k=0; k<count; k++)
	{
		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, k, &pvhItem)  );

		SG_ERR_CHECK(  SG_string__clear(pCtx, pString)  );
		SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvhItem, pString)  );
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR ,"%s\n", SG_string__sz(pString))  );
	}

fail:
    SG_STRING_NULLFREE(pCtx, pString);
}


void SG_varray__copy_items(
    SG_context* pCtx,
    const SG_varray* pva_from,
    SG_varray* pva_to
	)
{
	SG_NULLARGCHECK_RETURN(pva_from);
	SG_NULLARGCHECK_RETURN(pva_to);

	SG_ERR_CHECK_RETURN(  SG_varray__copy_slice(pCtx, pva_from, pva_to, 0, pva_from->count)  );
}

void SG_varray__copy_slice(
	SG_context* pCtx,
	const SG_varray* pva_from,
	SG_varray* pva_to,
	SG_uint32 startIndex,
	SG_uint32 copyCount
	)
{
	SG_uint32 i = 0;
	SG_uint32 end = startIndex + copyCount;

	if ((copyCount > 0) && (end > pva_from->count))
		SG_ERR_THROW(SG_ERR_INDEXOUTOFRANGE);

	for (i=startIndex; i<end; i++)
	{
		const SG_variant* pv = NULL;

		SG_ERR_CHECK(  SG_varray__get__variant(pCtx, pva_from, i, &pv)  );
		SG_ERR_CHECK(  SG_varray__appendcopy__variant(pCtx, pva_to, pv)  );
	}

fail:
	;
}

void SG_varray__find(
	SG_context*       pCtx,
	const SG_varray*  pThis,
	const SG_variant* pValue,
	SG_bool*          pContains,
	SG_uint32*        pIndex
	)
{
	SG_bool   bContains = SG_FALSE;
	SG_uint32 uIndex    = 0u;

	SG_NULLARGCHECK(pThis);
	SG_NULLARGCHECK(pValue);
	SG_ARGCHECK(pContains != NULL || pIndex != NULL, pContains | pIndex);

	for (uIndex = 0u; uIndex < pThis->count; ++uIndex)
	{
		SG_ERR_CHECK(  SG_variant__equal(pCtx, pValue, pThis->aSlots[uIndex], &bContains)  );

		if (bContains == SG_TRUE)
		{
			break;
		}
	}

	if (pContains != NULL)
	{
		*pContains = bContains;
	}

	if (pIndex != NULL)
	{
		*pIndex = uIndex;
	}

fail:
	return;
}

void SG_varray__find__null(
	SG_context*      pCtx,
	const SG_varray* pThis,
	SG_bool*         pContains,
	SG_uint32*       pIndex
	)
{
	SG_variant cValue;
	cValue.type = SG_VARIANT_TYPE_NULL;
	SG_ERR_CHECK_RETURN(  SG_varray__find(pCtx, pThis, &cValue, pContains, pIndex)  );
}

void SG_varray__find__bool(
	SG_context*      pCtx,
	const SG_varray* pThis,
	SG_bool          bValue,
	SG_bool*         pContains,
	SG_uint32*       pIndex
	)
{
	SG_variant cValue;
	cValue.type       = SG_VARIANT_TYPE_BOOL;
	cValue.v.val_bool = bValue;
	SG_ERR_CHECK_RETURN(  SG_varray__find(pCtx, pThis, &cValue, pContains, pIndex)  );
}

void SG_varray__find__int64(
	SG_context*      pCtx,
	const SG_varray* pThis,
	SG_int64         iValue,
	SG_bool*         pContains,
	SG_uint32*       pIndex
	)
{
	SG_variant cValue;
	cValue.type        = SG_VARIANT_TYPE_INT64;
	cValue.v.val_int64 = iValue;
	SG_ERR_CHECK_RETURN(  SG_varray__find(pCtx, pThis, &cValue, pContains, pIndex)  );
}

void SG_varray__find__double(
	SG_context*      pCtx,
	const SG_varray* pThis,
	double           dValue,
	SG_bool*         pContains,
	SG_uint32*       pIndex
	)
{
	SG_variant cValue;
	cValue.type         = SG_VARIANT_TYPE_DOUBLE;
	cValue.v.val_double = dValue;
	SG_ERR_CHECK_RETURN(  SG_varray__find(pCtx, pThis, &cValue, pContains, pIndex)  );
}

void SG_varray__find__sz(
	SG_context*      pCtx,
	const SG_varray* pThis,
	const char*      szValue,
	SG_bool*         pContains,
	SG_uint32*       pIndex
	)
{
	SG_variant cValue;
	cValue.type     = SG_VARIANT_TYPE_SZ;
	cValue.v.val_sz = szValue;
	SG_ERR_CHECK_RETURN(  SG_varray__find(pCtx, pThis, &cValue, pContains, pIndex)  );
}

void SG_varray__find__varray(
	SG_context*      pCtx,
	const SG_varray* pThis,
	const SG_varray* pValue,
	SG_bool*         pContains,
	SG_uint32*       pIndex
	)
{
	SG_variant cValue;
	cValue.type         = SG_VARIANT_TYPE_VARRAY;
	cValue.v.val_varray = (SG_varray*)pValue; // casting away const okay, because cValue is passed to SG_varray__find as const
	SG_ERR_CHECK_RETURN(  SG_varray__find(pCtx, pThis, &cValue, pContains, pIndex)  );
}

void SG_varray__find__vhash(
	SG_context*      pCtx,
	const SG_varray* pThis,
	const SG_vhash*  pValue,
	SG_bool*         pContains,
	SG_uint32*       pIndex
	)
{
	SG_variant cValue;
	cValue.type        = SG_VARIANT_TYPE_VHASH;
	cValue.v.val_vhash = (SG_vhash*)pValue; // casting away const okay, because cValue is passed to SG_varray__find as const
	SG_ERR_CHECK_RETURN(  SG_varray__find(pCtx, pThis, &cValue, pContains, pIndex)  );
}

void SG_varray__alloc__from_json__sz(
        SG_context* pCtx, 
        SG_varray** pResult, 
        const char* pszJson
        )
{
    SG_ERR_CHECK_RETURN(  SG_varray__alloc__from_json__buflen(pCtx, pResult, pszJson, SG_STRLEN(pszJson))  );
}

void SG_varray__alloc__from_json__string(
        SG_context* pCtx, 
        SG_varray** pResult, 
        SG_string* pJson
        )
{
    SG_NULLARGCHECK_RETURN(pJson);
	SG_ERR_CHECK_RETURN(  SG_varray__alloc__from_json__buflen(pCtx, pResult, SG_string__sz(pJson), SG_string__length_in_bytes(pJson))  );
}

void SG_varray__steal_the_pools(
    SG_context* pCtx,
    SG_varray* pva
	)
{
    SG_NULLARGCHECK_RETURN(pva);

    pva->strpool_is_mine = SG_TRUE;
    pva->varpool_is_mine = SG_TRUE;
}

void SG_varray__alloc__from_json__buflen(
        SG_context* pCtx, 
        SG_varray** pResult, 
        const char* pszJson,
        SG_uint32 len
        )
{
    SG_vhash* pvh = NULL;
    SG_varray* pva = NULL;

    SG_ERR_CHECK(  SG_veither__parse_json__buflen(pCtx, pszJson, len, &pvh, &pva)  );
    if (pvh)
    {
        SG_ERR_THROW(  SG_ERR_JSON_WRONG_TOP_TYPE  );
    }

	*pResult = pva;
    pva = NULL;

fail:
	SG_VARRAY_NULLFREE(pCtx, pva);
	SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_vaofvh__flatten(
        SG_context* pCtx, 
        SG_varray* pva, 
        const char* psz_field,
        SG_varray** ppva_result
        )
{
    SG_uint32 count = 0;
    SG_uint32 i = 0;
    SG_varray* pva_flat = NULL;

    SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );
    SG_ERR_CHECK(  SG_varray__alloc__params(pCtx, &pva_flat, count, NULL, NULL)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh = NULL;
        const SG_variant* pv = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva, i, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__get__variant(pCtx, pvh, psz_field, &pv)  );
		SG_ERR_CHECK(  SG_varray__appendcopy__variant(pCtx, pva_flat, pv)  );
    }

    *ppva_result = pva_flat;
    pva_flat = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_flat);
}

void SG_vaofvh__get__sz(
        SG_context* pCtx, 
        SG_varray* pva, 
        SG_uint32 ndx,
        const char* psz_field,
        const char** ppsz
        )
{
    SG_vhash* pvh = NULL;

    SG_ERR_CHECK_RETURN(  SG_varray__get__vhash(pCtx, pva, ndx, &pvh)  );
    if (psz_field)
    {
        SG_ERR_CHECK_RETURN(  SG_vhash__get__sz(pCtx, pvh, psz_field, ppsz)  );
    }
    else
    {
        const SG_variant* pv = NULL;
        SG_uint32 count = 0;

        SG_ERR_CHECK_RETURN(  SG_vhash__count(pCtx, pvh, &count)  );
        if (1 != count)
        {
            SG_ERR_THROW_RETURN(SG_ERR_INVALIDARG);
        }
        SG_ERR_CHECK_RETURN(  SG_vhash__get_nth_pair(pCtx, pvh, 0, NULL, &pv)  );
        SG_ERR_CHECK_RETURN(  SG_variant__get__sz(pCtx, pv, ppsz)  );
    }
}


/**
 * Remove the duplicate rows in a VARRAY of VHASH
 * using the value associated with the given key.
 * We DO NOT look at any of the other fields in
 * each VHASH.
 *
 * We do this in-place and alter the given VARRAY.
 *
 * [ { <key> : <value1>, .... },
 *   { <key> : <value2>, .... },
 *   { <key> : <value1>, .... },
 *   ... ]
 *
 * Should remove either the 1st or 3rd row.
 *
 */
void SG_vaofvh__dedup(SG_context * pCtx,
					  SG_varray * pva,
					  const char * pszKey)
{
	SG_rbtree * prbSeen = NULL;
	SG_uint32 count = 0;
	SG_uint32 k;

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prbSeen)  );
	SG_ERR_CHECK(  SG_varray__count(pCtx, pva, &count)  );

	k = 0;
	while (k < count)
	{
		const char * pszValue_k;
		SG_bool bFound_k;

		SG_ERR_CHECK(  SG_vaofvh__get__sz(pCtx, pva, k, pszKey, &pszValue_k)  );
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, prbSeen, pszValue_k, &bFound_k, NULL)  );
		if (bFound_k)
		{
#if 0 && defined(DEBUG)
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "VarrayDedup: [%d][%s]\n",
									   k, pszValue_k)  );
#endif
			SG_ERR_CHECK(  SG_varray__remove(pCtx, pva, k)  );
			count--;
		}
		else
		{
			SG_ERR_CHECK(  SG_rbtree__add(pCtx, prbSeen, pszValue_k)  );
			k++;
		}
	}

fail:
	SG_RBTREE_NULLFREE(pCtx, prbSeen);
}
