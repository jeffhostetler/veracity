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

#ifndef H_SG_WC_PRESCAN__PRIVATE_PROTOTYPES_H
#define H_SG_WC_PRESCAN__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void sg_wc_prescan_dir__free(SG_context * pCtx, sg_wc_prescan_dir * pPrescanDir);

#define SG_WC_PRESCAN_DIR__NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,sg_wc_prescan_dir__free)

void sg_wc_prescan_dir__alloc(SG_context * pCtx, sg_wc_prescan_dir ** ppPrescanDir,
							  SG_uint64 uiAliasGidDir,
							  const SG_string * pStringRefRepoPath);

void sg_wc_prescan_dir__alloc__scan_and_match(SG_context * pCtx,
											  SG_wc_tx * pWcTx,
											  SG_uint64 uiAliasGidDir,
											  const SG_string * pStringRefRepoPath,
											  sg_wc_prescan_flags scan_flags,
											  sg_wc_prescan_dir ** ppPrescanDir);

#if defined(DEBUG)

#define SG_WC_PRESCAN_DIR__ALLOC(pCtx,ppNew,ui,pStr)					\
	SG_STATEMENT(														\
		sg_wc_prescan_dir * _pNew = NULL;								\
		sg_wc_prescan_dir__alloc(pCtx,&_pNew,ui,pStr);					\
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_prescan_dir"); \
		*(ppNew) = _pNew;												\
		)

#define SG_WC_PRESCAN_DIR__ALLOC__SCAN_AND_MATCH(pCtx,pWcTx,ui,pStr,sf,ppNew) \
	SG_STATEMENT(														\
		sg_wc_prescan_dir * _pNew = NULL;								\
		sg_wc_prescan_dir__alloc__scan_and_match(pCtx,pWcTx,ui,pStr,sf,&_pNew); \
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_prescan_dir"); \
		*(ppNew) = _pNew;												\
		)

#else

#define SG_WC_PRESCAN_DIR__ALLOC(pCtx,ppNew,ui,pStr)	\
	sg_wc_prescan_dir__alloc(pCtx,ppNew,ui,pStr)

#define SG_WC_PRESCAN_DIR__ALLOC__SCAN_AND_MATCH(pCtx,pWcTx,ui,pStr,sf,ppNew) \
	sg_wc_prescan_dir__alloc__scan_and_match(pCtx,pWcTx,ui,pStr,sf,ppNew)

#endif

//////////////////////////////////////////////////////////////////

void sg_wc_prescan_dir__foreach(SG_context * pCtx,
								const sg_wc_prescan_dir * pPrescanDir,
								SG_rbtree_ui64_foreach_callback * pfn_cb,
								void * pVoidData);

//////////////////////////////////////////////////////////////////

void sg_wc_prescan_dir__find_row_by_alias(SG_context * pCtx,
										  const sg_wc_prescan_dir * pPrescanDir,
										  SG_uint64 uiAliasGid,
										  SG_bool * pbFound,
										  sg_wc_prescan_row ** ppPrescanRow);

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
void sg_wc_prescan_dir_debug__dump_to_console(SG_context * pCtx,
											  const sg_wc_prescan_dir * pPrescanDir);
#endif

//////////////////////////////////////////////////////////////////

void sg_wc_prescan_row__free(SG_context * pCtx, sg_wc_prescan_row * pPrescanRow);

#define SG_WC_PRESCAN_ROW__NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,sg_wc_prescan_row__free)

void sg_wc_prescan_row__alloc__row_root(SG_context * pCtx,
										SG_wc_tx * pWcTx,
										sg_wc_prescan_row ** ppPrescanRow);

#if defined(DEBUG)

#define SG_WC_PRESCAN_ROW__ALLOC__ROW_ROOT(pCtx,pWcTx,ppNew)			\
	SG_STATEMENT(														\
		sg_wc_prescan_row * _pNew = NULL;								\
		sg_wc_prescan_row__alloc__row_root(pCtx,pWcTx,&_pNew);			\
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_prescan_row"); \
		*(ppNew) = _pNew;												\
		)

#else

#define SG_WC_PRESCAN_ROW__ALLOC__ROW_ROOT(pCtx,pWcTx,ppNew)	\
	sg_wc_prescan_row__alloc(pCtx,pWcTx,ppNew)

#endif

//////////////////////////////////////////////////////////////////

void sg_wc_prescan_row__synthesize_replacement_row(SG_context * pCtx,
												   sg_wc_prescan_row ** ppPrescanRowReplacement,
												   SG_wc_tx * pWctx,
												   const sg_wc_prescan_row * pPrescanRow_Input);

//////////////////////////////////////////////////////////////////

void sg_wc_prescan_row__get_tne_type(SG_context * pCtx,
									 const sg_wc_prescan_row * pPrescanRow,
									 SG_treenode_entry_type * pTneType);

void sg_wc_prescan_row__get_current_entryname(SG_context * pCtx,
											  const sg_wc_prescan_row * pPrescanRow,
											  const char ** ppszEntryname);

void sg_wc_prescan_row__get_current_parent_alias(SG_context * pCtx,
												 const sg_wc_prescan_row * pPrescanRow,
												 SG_uint64 * puiAliasGidParent);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_PRESCAN__PRIVATE_PROTOTYPES_H
