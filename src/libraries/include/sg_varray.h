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

#ifndef H_SG_VARRAY_H
#define H_SG_VARRAY_H

BEGIN_EXTERN_C

/*
  SG_varray is an array for typed values.
*/

#define SG_VARRAY_STDERR(pva) { SG_string* pstr = NULL; SG_STRING__ALLOC(pCtx, &pstr); SG_varray__to_json__pretty_print_NOT_for_storage(pCtx, pva, pstr); SG_console__raw(pCtx, SG_CS_STDERR, SG_string__sz(pstr)); SG_STRING_NULLFREE(pCtx, pstr); }

#if DEBUG

#define SG_VARRAY_STDOUT(pva) { SG_string* pstr = NULL; SG_STRING__ALLOC(pCtx, &pstr); SG_varray__to_json__pretty_print_NOT_for_storage(pCtx, pva, pstr); SG_console__raw(pCtx, SG_CD_STDOUT, SG_string__sz(pstr)); SG_STRING_NULLFREE(pCtx, pstr); }

#define SG_STRINGARRAY_STDOUT(psa) { SG_varray* pva = NULL; SG_string* pstr = NULL; SG_varray__alloc__stringarray(pCtx, &pva, psa); SG_STRING__ALLOC(pCtx, &pstr); SG_varray__to_json__pretty_print_NOT_for_storage(pCtx, pva, pstr); SG_VARRAY_NULLFREE(pCtx, pva); printf("%s\n", SG_string__sz(pstr)); SG_STRING_NULLFREE(pCtx, pstr); }

#endif

//////////////////////////////////////////////////////////////////

void SG_varray__alloc__params(
        SG_context* pCtx,
        SG_varray** ppNew,
        SG_uint32 initial_size,
        SG_strpool* pPool,
        SG_varpool* pVarPool
        );

void SG_varray__alloc__shared(
        SG_context* pCtx,
        SG_varray** ppNew,
        SG_uint32 initial_size,
        SG_varray* pva_other
        );

void SG_varray__alloc__copy(
	SG_context*      pCtx,
	SG_varray**      ppNew,
	const SG_varray* pBase
	);

void SG_varray__alloc(SG_context* pCtx, SG_varray** ppNew);

void SG_varray__alloc__stringarray(SG_context* pCtx, SG_varray** ppNew, const SG_stringarray* pStringarray);

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
#define __SG_VARRAY__ALLOC__(pp,expr)		SG_STATEMENT(	SG_varray * _p = NULL;										\
															expr;														\
															_sg_mem__set_caller_data(_p,__FILE__,__LINE__,"SG_varray");	\
															*(pp) = _p;													)

#define SG_VARRAY__ALLOC__PARAMS(pCtx,ppNew,initial_size,pStrPool,pVarPool)		__SG_VARRAY__ALLOC__(ppNew, SG_varray__alloc__params   (pCtx,&_p,initial_size,pStrPool,pVarPool) )
#define SG_VARRAY__ALLOC__COPY(pCtx,ppNew,pBase)								__SG_VARRAY__ALLOC__(ppNew, SG_varray__alloc__copy     (pCtx,&_p,pBase) )
#define SG_VARRAY__ALLOC(pCtx,ppNew)											__SG_VARRAY__ALLOC__(ppNew, SG_varray__alloc           (pCtx,&_p) )

#define SG_VARRAY__ALLOC__FROM_JSON__SZ(pCtx,ppNew,pszJson)          __SG_VARRAY__ALLOC__(ppNew, SG_varray__alloc__from_json__sz(pCtx,&_p,pszJson) )
#define SG_VARRAY__ALLOC__FROM_JSON__BUFLEN(pCtx,ppNew,pszJson,len)  __SG_VARRAY__ALLOC__(ppNew, SG_varray__alloc__from_json__buflen(pCtx,&_p,pszJson,len) )
#define SG_VARRAY__ALLOC__FROM_JSON__STRING(pCtx,ppNew,pJson)        __SG_VARRAY__ALLOC__(ppNew, SG_varray__alloc__from_json__string(pCtx,&_p,pJson) )

#else

#define SG_VARRAY__ALLOC__PARAMS(pCtx,ppNew,initial_size,pStrPool,pVarPool)		SG_varray__alloc__params   (pCtx,ppNew,initial_size,pStrPool,pVarPool)
#define SG_VARRAY__ALLOC__COPY(pCtx,ppNew,pBase)								SG_varray__alloc__copy     (pCtx,ppNew,pBase)
#define SG_VARRAY__ALLOC(pCtx,ppNew)											SG_varray__alloc           (pCtx,ppNew)
#define SG_VARRAY__ALLOC__FROM_JSON__SZ(pCtx,ppNew,pszJson)          SG_varray__alloc__from_json__sz(pCtx,ppNew,pszJson)
#define SG_VARRAY__ALLOC__FROM_JSON__BUFLEN(pCtx,ppNew,pszJson,len)  SG_varray__alloc__from_json__buflen(pCtx,ppNew,pszJson,len)
#define SG_VARRAY__ALLOC__FROM_JSON__STRING(pCtx,ppNew,pJson)        SG_varray__alloc__from_json__string(pCtx,ppNew,pJson)

#endif

//////////////////////////////////////////////////////////////////

void        SG_varray__free(SG_context * pCtx, SG_varray* pva);

void        SG_varray__count(SG_context*, const SG_varray* pva, SG_uint32* piResult);

void        SG_varray__write_json(SG_context*, const SG_varray* pva, SG_jsonwriter* pjson);
void        SG_varray__to_json(SG_context*, const SG_varray* pva, SG_string* pStr);
void SG_varray__to_json__pretty_print_NOT_for_storage(SG_context* pCtx, const SG_varray* pva, SG_string* pStr);
void		SG_varray__alloc__from_json(SG_context* pCtx, const char* pszJson, SG_varray** pResult);

void        SG_varray__append__string__sz(SG_context*, SG_varray* pva, const char* putf8Value);
void        SG_varray__append__string__buflen(SG_context*, SG_varray* pva, const char* putf8Value, SG_uint32 len);

#ifndef SG_IOS
void        SG_varray__append__jsstring(SG_context* pCtx, SG_varray* pva, JSContext *cx, JSString *str);
#endif

void        SG_varray__append__int64(SG_context*, SG_varray* pva, SG_int64 intValue);
void        SG_varray__append__double(SG_context*, SG_varray* pva, double fv);
void        SG_varray__append__vhash(SG_context*, SG_varray* pva, SG_vhash** ppvh);
void		SG_varray__appendnew__vhash(SG_context* pCtx, SG_varray* pva, SG_vhash** ppvh);
void		SG_varray__appendnew__varray(SG_context* pCtx, SG_varray* pva, SG_varray** ppva);
void        SG_varray__append__varray(SG_context*, SG_varray* pva, SG_varray** ppva_val);
void        SG_varray__append__bool(SG_context*, SG_varray* pva, SG_bool b);
void        SG_varray__append__null(SG_context*, SG_varray* pva);
void        SG_varray__appendcopy__varray(SG_context* pCtx, SG_varray* pThis, const SG_varray* ppValue, SG_varray** pp);
void        SG_varray__appendcopy__vhash(SG_context* pCtx, SG_varray* pThis, const SG_vhash* ppValue, SG_vhash** pp);
void        SG_varray__appendcopy__variant(SG_context* pCtx, SG_varray* pThis, const SG_variant* ppValue);

// TODO insert

void        SG_varray__remove(SG_context*, SG_varray* pva, SG_uint32 ndx);

void        SG_varray__get__variant(SG_context*, const SG_varray* pva, SG_uint32 ndx, const SG_variant** ppResult);

void        SG_varray__get__sz(SG_context*, const SG_varray* pva, SG_uint32 ndx, const char** pResult);
void        SG_varray__get__int64(SG_context*, const SG_varray* pva, SG_uint32 ndx, SG_int64* pResult);
void        SG_varray__get__double(SG_context*, const SG_varray* pva, SG_uint32 ndx, double* pResult);
void        SG_varray__get__bool(SG_context*, const SG_varray* pva, SG_uint32 ndx, SG_bool* pResult);
void        SG_varray__get__vhash(SG_context*, const SG_varray* pva, SG_uint32 ndx, SG_vhash** pResult);  // TODO should the result be const?
void        SG_varray__get__varray(SG_context*, const SG_varray* pva, SG_uint32 ndx, SG_varray** pResult);  // TODO should the result be const?

typedef void (SG_varray_foreach_callback)(SG_context* pCtx, void* pVoidData, const SG_varray* pva, SG_uint32 ndx, const SG_variant* pv);

void        SG_varray__foreach(SG_context*, const SG_varray* pva, SG_varray_foreach_callback cb, void* ctx);

void        SG_varray__equal(SG_context*, const SG_varray* pv1, const SG_varray* pv2, SG_bool* pbResult);

void        SG_varray__typeof(SG_context*, const SG_varray* pva, SG_uint32 ndx, SG_uint16* pResult);

/**
 * Sort the varray.  Elements added after sorting will be appended to
 * the end (they are not inserted in order).
 *
 */
void SG_varray__sort(SG_context*, SG_varray * pva, SG_qsort_compare_function pfnCompare, void* p);

void SG_varray__to_rbtree(SG_context*, const SG_varray * pva, SG_rbtree ** pprbtree);

void SG_varray__to_stringarray(SG_context*, const SG_varray * pva, SG_stringarray ** ppsa);
void SG_varray__to_vhash_with_indexes(SG_context*, const SG_varray * pva, SG_vhash ** ppvh);
void SG_varray__to_vhash_with_indexes__update(SG_context*, const SG_varray * pva, SG_vhash ** ppvh);

void SG_varray__alloc__from_json__buflen(
        SG_context* pCtx, 
        SG_varray** pResult, 
        const char* pszJson,
        SG_uint32 len
        );

void SG_varray__alloc__from_json__sz(
        SG_context* pCtx, 
        SG_varray** pResult, 
        const char* pszJson
        );

void SG_varray__alloc__from_json__string(
        SG_context* pCtx, 
        SG_varray** pResult, 
        SG_string* pJson
        );

void SG_varray__reverse(SG_context*, SG_varray * pva);

void SG_varray__steal_the_pools(
    SG_context* pCtx,
    SG_varray* pva
	);

void SG_varray__copy_items(
    SG_context* pCtx,
    const SG_varray* pva_from,
    SG_varray* pva_to
	);

void SG_varray__copy_slice(
    SG_context* pCtx,
    const SG_varray* pva_from,
    SG_varray* pva_to,
	SG_uint32 startIndex,
	SG_uint32 copyCount
	);

void SG_vaofvh__flatten(
        SG_context* pCtx, 
        SG_varray* pva, 
        const char* psz_field,
        SG_varray** ppva_result
        );

void SG_vaofvh__get__sz(
        SG_context* pCtx, 
        SG_varray* pva, 
        SG_uint32 ndx,
        const char* psz_field,
        const char** ppsz
        );

void SG_vaofvh__dedup(SG_context * pCtx,
					  SG_varray * pva,
					  const char * pszKey);

/**
 * Checks if a varray contains a given value.
 */
void SG_varray__find(
	SG_context*       pCtx,      //< [in] [out] Error and context info.
	const SG_varray*  pThis,     //< [in] The array to check for a value.
	const SG_variant* pValue,    //< [in] The value to check for in the array.
	SG_bool*          pContains, //< [out] Set to SG_TRUE if the array contains the value, or SG_FALSE if it doesn't.
	                             //<       Ignored if NULL.
	SG_uint32*        pIndex     //< [out] Set to the index where the value was found.
	                             //<       Set to the size of the varray, if the value was not found.
	                             //<       Ignored if NULL.
	);

/**
 * Checks if a varray contains a NULL value.
 */
void SG_varray__find__null(
	SG_context*      pCtx,      //< [in] [out] Error and context info.
	const SG_varray* pThis,     //< [in] The array to check for a value.
	SG_bool*         pContains, //< [out] Set to SG_TRUE if the array contains the value, or SG_FALSE if it doesn't.
	                            //<       Ignored if NULL.
	SG_uint32*       pIndex     //< [out] Set to the index where the value was found.
	                            //<       Set to the size of the varray, if the value was not found.
	                            //<       Ignored if NULL.
	);

/**
 * Checks if a varray contains a given boolean value.
 */
void SG_varray__find__bool(
	SG_context*      pCtx,      //< [in] [out] Error and context info.
	const SG_varray* pThis,     //< [in] The array to check for a value.
	SG_bool          bValue,    //< [in] The value to check for in the array.
	SG_bool*         pContains, //< [out] Set to SG_TRUE if the array contains the value, or SG_FALSE if it doesn't.
	                            //<       Ignored if NULL.
	SG_uint32*       pIndex     //< [out] Set to the index where the value was found.
	                            //<       Set to the size of the varray, if the value was not found.
	                            //<       Ignored if NULL.
	);

/**
 * Checks if a varray contains a given integer value.
 */
void SG_varray__find__int64(
	SG_context*      pCtx,      //< [in] [out] Error and context info.
	const SG_varray* pThis,     //< [in] The array to check for a value.
	SG_int64         iValue,    //< [in] The value to check for in the array.
	SG_bool*         pContains, //< [out] Set to SG_TRUE if the array contains the value, or SG_FALSE if it doesn't.
	                            //<       Ignored if NULL.
	SG_uint32*       pIndex     //< [out] Set to the index where the value was found.
	                            //<       Set to the size of the varray, if the value was not found.
	                            //<       Ignored if NULL.
	);

/**
 * Checks if a varray contains a given double value.
 */
void SG_varray__find__double(
	SG_context*      pCtx,      //< [in] [out] Error and context info.
	const SG_varray* pThis,     //< [in] The array to check for a value.
	double           dValue,    //< [in] The value to check for in the array.
	SG_bool*         pContains, //< [out] Set to SG_TRUE if the array contains the value, or SG_FALSE if it doesn't.
	                            //<       Ignored if NULL.
	SG_uint32*       pIndex     //< [out] Set to the index where the value was found.
	                            //<       Set to the size of the varray, if the value was not found.
	                            //<       Ignored if NULL.
	);

/**
 * Checks if a varray contains a given string value.
 */
void SG_varray__find__sz(
	SG_context*      pCtx,      //< [in] [out] Error and context info.
	const SG_varray* pThis,     //< [in] The array to check for a value.
	const char*      szValue,   //< [in] The value to check for in the array.
	SG_bool*         pContains, //< [out] Set to SG_TRUE if the array contains the value, or SG_FALSE if it doesn't.
	                            //<       Ignored if NULL.
	SG_uint32*       pIndex     //< [out] Set to the index where the value was found.
	                            //<       Set to the size of the varray, if the value was not found.
	                            //<       Ignored if NULL.
	);

/**
 * Checks if a varray contains a given varray value.
 */
void SG_varray__find__varray(
	SG_context*      pCtx,      //< [in] [out] Error and context info.
	const SG_varray* pThis,     //< [in] The array to check for a value.
	const SG_varray* pValue,    //< [in] The value to check for in the array.
	SG_bool*         pContains, //< [out] Set to SG_TRUE if the array contains the value, or SG_FALSE if it doesn't.
	                            //<       Ignored if NULL.
	SG_uint32*       pIndex     //< [out] Set to the index where the value was found.
	                            //<       Set to the size of the varray, if the value was not found.
	                            //<       Ignored if NULL.
	);

/**
 * Checks if a varray contains a given vhash value.
 */
void SG_varray__find__vhash(
	SG_context*      pCtx,      //< [in] [out] Error and context info.
	const SG_varray* pThis,     //< [in] The array to check for a value.
	const SG_vhash*  pValue,    //< [in] The value to check for in the array.
	SG_bool*         pContains, //< [out] Set to SG_TRUE if the array contains the value, or SG_FALSE if it doesn't.
	                            //<       Ignored if NULL.
	SG_uint32*       pIndex     //< [out] Set to the index where the value was found.
	                            //<       Set to the size of the varray, if the value was not found.
	                            //<       Ignored if NULL.
	);

void SG_varray_debug__dump_varray_of_vhashes_to_console(SG_context * pCtx, const SG_varray * pva, const char * pszLabel);

END_EXTERN_C

#endif
