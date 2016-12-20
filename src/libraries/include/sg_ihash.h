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
 * @file sg_ihash.h
 *
 * @details SG_ihash is a hash table for variants (typed values).  It
 * stores key-value pairs.  Each key is a utf-8 string.  Values are
 * variants, which means they can be a variety of things.  This data
 * structure was designed to serve as a representation of a JSON
 * object.  Think of it as an assoc table or a dictionary.  Think of
 * it as something that can be easily serialized to/from JSON.
 *
 */

#ifndef H_SG_IHASH_H
#define H_SG_IHASH_H

BEGIN_EXTERN_C;

void SG_ihash__alloc(SG_context* pCtx, SG_ihash** ppNew);

/**
 * Free the ihash table and all memory associated with it.
 *
 * @note If this hash table owns its own string pool, it will be freed
 * as well.  On the other hand, if the pool was provided externally,
 * the ihash table will regard itself as a guest and will not free it.
 */
void SG_ihash__free(SG_context * pCtx, SG_ihash* pHash);

/**
 * @result The number of key-value pairs
 */
void SG_ihash__count(
    SG_context* pCtx,
	const SG_ihash* pHash,
	SG_uint32* piResult
	);

/**
 * @result A varray containing the keys
 */
void SG_ihash__get_keys(
	SG_context* pCtx,
	const SG_ihash* pHash,
	SG_varray** ppNew
	);

void SG_ihash__get_keys_as_rbtree(
	SG_context* pCtx,
	const SG_ihash* pvh,
    SG_rbtree** pprb
	);

void SG_ihash__add__int64(
    SG_context* pCtx,
	SG_ihash* pHash,
	const char* putf8Key,
	SG_int64 intValue
	);

void SG_ihash__update__int64(
    SG_context* pCtx,
	SG_ihash* pHash,
	const char* putf8Key,
	SG_int64 intValue
	);

void SG_ihash__remove(
    SG_context* pCtx,
	SG_ihash* pHash,
	const char* putf8Key
	);

void SG_ihash__remove_if_present(
    SG_context* pCtx,
	SG_ihash* pHash,
	const char* putf8Key,
    SG_bool* pb_was_removed
	);

void SG_ihash__copy_items(
    SG_context* pCtx,
    const SG_ihash* pih_from,
    SG_ihash* pih_to,
	SG_bool bOverwriteDuplicates
	);

void SG_ihash__has(
    SG_context* pCtx,
	const SG_ihash* pHash,
	const char* putf8Key,
	SG_bool* pbResult
	);

void SG_ihash__indexof(
        SG_context* pCtx, 
        SG_ihash* pvh, 
        const char* psz_key,
        SG_int32* pi
        );

void SG_ihash__key(
        SG_context* pCtx,
        const SG_ihash* pvh,
        const char* putf8Key,
        const char** pp
        );

void SG_ihash__check__int64(
    SG_context* pCtx, 
    const SG_ihash* pvh, 
    const char* psz_key, 
    SG_int64* pResult
    );

void        SG_ihash__get__int64(
    SG_context* pCtx,
	const SG_ihash* pvh,
	const char* putf8Key,
	SG_int64* pResult
	);

void SG_ihash__get_nth_pair(
	SG_context* pCtx,
	const SG_ihash* pHash,
	SG_uint32 n,
	const char** putf8Key,
    SG_int64* pi
	);


#if defined(DEBUG)
#define __SG_IHASH__ALLOC__(pp,expr)		SG_STATEMENT(	SG_ihash * _p = NULL;										\
															expr;														\
															_sg_mem__set_caller_data(_p,__FILE__,__LINE__,"SG_ihash");	\
															*(pp) = _p;													)

#define SG_IHASH__ALLOC__PARAMS(pCtx,ppNew,guess,pStrPool,pVarPool)		__SG_IHASH__ALLOC__(ppNew, SG_ihash__alloc__params   (pCtx,&_p,guess,pStrPool,pVarPool) )
#define SG_IHASH__ALLOC__SHARED(pCtx,ppNew,guess,pvhShared)				__SG_IHASH__ALLOC__(ppNew, SG_ihash__alloc__shared   (pCtx,&_p,guess,pvhShared) )
#define SG_IHASH__ALLOC__COPY(pCtx,ppNew,pvhOther)						__SG_IHASH__ALLOC__(ppNew, SG_ihash__alloc__copy     (pCtx,&_p,pvhOther) )
#define SG_IHASH__ALLOC(pCtx,ppNew)										__SG_IHASH__ALLOC__(ppNew, SG_ihash__alloc           (pCtx,&_p) )

#define SG_IHASH__ALLOC__FROM_JSON__SZ(pCtx,ppNew,pszJson)          __SG_IHASH__ALLOC__(ppNew, SG_ihash__alloc__from_json__sz(pCtx,&_p,pszJson) )
#define SG_IHASH__ALLOC__FROM_JSON__BUFLEN(pCtx,ppNew,pszJson,len)  __SG_IHASH__ALLOC__(ppNew, SG_ihash__alloc__from_json__buflen(pCtx,&_p,pszJson,len) )
#define SG_IHASH__ALLOC__FROM_JSON__STRING(pCtx,ppNew,pJson)        __SG_IHASH__ALLOC__(ppNew, SG_ihash__alloc__from_json__string(pCtx,&_p,pJson) )

#else

#define SG_IHASH__ALLOC__PARAMS(pCtx,ppNew,guess,pStrPool,pVarPool)		SG_ihash__alloc__params   (pCtx,ppNew,guess,pStrPool,pVarPool)
#define SG_IHASH__ALLOC__SHARED(pCtx,ppNew,guess,pvhShared)				SG_ihash__alloc__shared   (pCtx,ppNew,guess,pvhShared)
#define SG_IHASH__ALLOC__COPY(pCtx,ppNew,pvhOther)						SG_ihash__alloc__copy     (pCtx,ppNew,pvhOther)
#define SG_IHASH__ALLOC(pCtx,ppNew)										SG_ihash__alloc           (pCtx,ppNew)
#define SG_IHASH__ALLOC__FROM_JSON__SZ(pCtx,ppNew,pszJson)          SG_ihash__alloc__from_json__sz(pCtx,ppNew,pszJson)
#define SG_IHASH__ALLOC__FROM_JSON__BUFLEN(pCtx,ppNew,pszJson,len)  SG_ihash__alloc__from_json__buflen(pCtx,ppNew,pszJson,len)
#define SG_IHASH__ALLOC__FROM_JSON__STRING(pCtx,ppNew,pJson)        SG_ihash__alloc__from_json__string(pCtx,ppNew,pJson)

#endif


END_EXTERN_C;

#endif

