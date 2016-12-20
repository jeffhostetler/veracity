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

#ifndef H_SG_WC_PCTNE__PRIVATE_PROTOTYPES_H
#define H_SG_WC_PCTNE__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void sg_wc_pctne__row__free(SG_context * pCtx, sg_wc_pctne__row * pPT);

#define SG_WC_PCTNE__ROW__NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,sg_wc_pctne__row__free)

void sg_wc_pctne__row__alloc(SG_context * pCtx, sg_wc_pctne__row ** ppPT);

#if defined(DEBUG)

#define SG_WC_PCTNE__ROW__ALLOC(pCtx,ppNew)								\
	SG_STATEMENT(														\
		sg_wc_pctne__row * _pNew = NULL;								\
		sg_wc_pctne__row__alloc(pCtx,&_pNew);							\
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_pctne__row"); \
		*(ppNew) = _pNew;												\
		)

#else

#define SG_WC_PCTNE__ROW__ALLOC(pCtx,ppNew)		\
	sg_wc_pctne__row__alloc(pCtx,ppNew)

#endif

//////////////////////////////////////////////////////////////////

#if TRACE_WC_DB
void sg_wc_pctne__row__debug_print(SG_context * pCtx, const sg_wc_pctne__row * pPT);
#endif

void sg_wc_pctne__row__is_delete_pended(SG_context * pCtx,
										const sg_wc_pctne__row * pPT,
										SG_bool * pbResult);

void sg_wc_pctne__row__get_alias(SG_context * pCtx,
								 const sg_wc_pctne__row * pPT,
								 SG_uint64 * puiAliasGid);

void sg_wc_pctne__row__get_current_parent_alias(SG_context * pCtx,
												const sg_wc_pctne__row * pPT,
												SG_uint64 * puiAliasGidParent);

void sg_wc_pctne__row__get_original_parent_alias(SG_context * pCtx,
												 const sg_wc_pctne__row * pPT,
												 SG_uint64 * puiAliasGidParent);

void sg_wc_pctne__row__get_current_entryname(SG_context * pCtx,
											 const sg_wc_pctne__row * pPT,
											 const char ** ppszEntryname);

void sg_wc_pctne__row__get_original_entryname(SG_context * pCtx,
											  const sg_wc_pctne__row * pPT,
											  const char ** ppszEntryname);

void sg_wc_pctne__row__get_tne_type(SG_context * pCtx,
									const sg_wc_pctne__row * pPT,
									SG_treenode_entry_type * pTneType);

//////////////////////////////////////////////////////////////////

void sg_wc_pctne__get_row_by_alias(SG_context * pCtx,
								   sg_wc_db * pDb,
								   const sg_wc_db__cset_row * pCSetRow,
								   SG_uint64 uiAliasGid,
								   SG_bool * pbFound,
								   sg_wc_pctne__row ** ppPT);

void sg_wc_pctne__foreach_in_dir_by_parent_alias(SG_context * pCtx,
												 sg_wc_db * pDb,
												 const sg_wc_db__cset_row * pCSetRow,
												 SG_uint64 uiAliasGidParent,
												 sg_wc_pctne__foreach_cb * pfn_cb_level2,
												 void * pVoidLevel2Data);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_PCTNE__PRIVATE_PROTOTYPES_H
