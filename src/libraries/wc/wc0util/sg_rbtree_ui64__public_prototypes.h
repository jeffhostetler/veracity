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

#ifndef H_SG_RBTREE_UI64__PUBLIC_PROTOTYPES_H
#define H_SG_RBTREE_UI64__PUBLIC_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_rbtree_ui64__alloc(
	SG_context* pCtx,
	SG_rbtree_ui64** ppNew
	);

#if defined(DEBUG)
#define SG_RBTREE_UI64__ALLOC(pCtx,ppNew)	SG_STATEMENT(	SG_rbtree_ui64 * _prbNew = NULL; \
															SG_rbtree_ui64__alloc(pCtx,&_prbNew); \
															_sg_mem__set_caller_data(_prbNew,__FILE__,__LINE__,"SG_rbtree_ui64"); \
															*(ppNew) = _prbNew;													)
#else
#define SG_RBTREE_UI64__ALLOC(pCtx,ppNew)	SG_rbtree_ui64__alloc(pCtx,ppNew)
#endif

void SG_rbtree_ui64__free__with_assoc(
	SG_context * pCtx,
	SG_rbtree_ui64 * ptree,
	SG_free_callback* cb
	);

#define SG_RBTREE_UI64_NULLFREE_WITH_ASSOC(pCtx,p,cb)  SG_STATEMENT(SG_context__push_level(pCtx);    SG_rbtree_ui64__free__with_assoc(pCtx, p,cb);SG_ASSERT(!SG_CONTEXT__HAS_ERR(pCtx));SG_context__pop_level(pCtx);p=NULL;)

void SG_rbtree_ui64__free(
	SG_context * pCtx,
	SG_rbtree_ui64 * ptree
	);

#define SG_RBTREE_UI64_NULLFREE(pCtx,p)                _sg_generic_nullfree(pCtx,p,SG_rbtree_ui64__free)

//////////////////////////////////////////////////////////////////

void SG_rbtree_ui64__add__with_assoc(
	SG_context* pCtx,
	SG_rbtree_ui64* prb,
	SG_uint64 ui64_key,
	void* assoc
	);

void SG_rbtree_ui64__update__with_assoc(
	SG_context* pCtx,
	SG_rbtree_ui64* prb,
	SG_uint64 ui64_key,
	void* assoc,
	void** ppOldAssoc
	);

void SG_rbtree_ui64__remove__with_assoc(
	SG_context * pCtx,
	SG_rbtree_ui64 * prb,
	SG_uint64 ui64_key,
	void ** ppAssoc
	);

//////////////////////////////////////////////////////////////////

void SG_rbtree_ui64__find(
	SG_context* pCtx,
	const SG_rbtree_ui64* prb,
	SG_uint64 ui64_key,
    SG_bool* pbFound,
	void** ppAssocData
	);

//////////////////////////////////////////////////////////////////

void SG_rbtree_ui64__iterator__first(
	SG_context* pCtx,
	SG_rbtree_ui64_iterator **ppIterator,
	const SG_rbtree_ui64 *tree,
	SG_bool* pbOK,
	SG_uint64 * pui64_key,
	void** ppAssocData
	);

void SG_rbtree_ui64__iterator__next(
	SG_context* pCtx,
	SG_rbtree_ui64_iterator * pIterator,
	SG_bool* pbOK,
	SG_uint64 * pui64_key,
	void** ppAssocData
	);

void SG_rbtree_ui64__iterator__free(SG_context * pCtx, SG_rbtree_ui64_iterator * pIterator);

#define SG_RBTREE_UI64_ITERATOR_NULLFREE(pCtx,p)       _sg_generic_nullfree(pCtx,p,SG_rbtree_ui64__iterator__free)

//////////////////////////////////////////////////////////////////

void SG_rbtree_ui64__foreach(
	SG_context* pCtx,
	const SG_rbtree_ui64* prb,
	SG_rbtree_ui64_foreach_callback* cb,
	void* ctx
	);

//////////////////////////////////////////////////////////////////

void SG_rbtree_ui64__count(
	SG_context * pCtx,
	const SG_rbtree_ui64 * prb,
	SG_uint32 * pCount);

END_EXTERN_C;

#endif//H_SG_RBTREE_UI64__PUBLIC_PROTOTYPES_H
