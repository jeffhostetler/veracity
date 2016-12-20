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

#ifndef H_SG_WC_READDIR__PRIVATE_PROTOTYPES_H
#define H_SG_WC_READDIR__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void sg_wc_readdir__alloc__from_scan(SG_context * pCtx,
									 const SG_pathname * pPathDir,
									 SG_dir_foreach_flags flagsReaddir,
									 SG_rbtree ** pprb_readdir);

//////////////////////////////////////////////////////////////////

void sg_wc_readdir__row__free(SG_context * pCtx, sg_wc_readdir__row * pRow);

#define SG_WC_READDIR__ROW__NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,sg_wc_readdir__row__free)

void sg_wc_readdir__row__alloc(SG_context * pCtx,
							   sg_wc_readdir__row ** ppRow,
							   const SG_pathname * pPathDir,
							   const SG_string * pStringEntryname,
							   const SG_fsobj_stat * pfsStat);

void sg_wc_readdir__row__alloc__row_root(SG_context * pCtx,
										 sg_wc_readdir__row ** ppRow,
										 const SG_pathname * pPathWorkingDirectoryTop);

void sg_wc_readdir__row__alloc__copy(SG_context * pCtx,
									 sg_wc_readdir__row ** ppRow,
									 const sg_wc_readdir__row * pRow_Input);

#if defined(DEBUG)

#define SG_WC_READDIR__ROW__ALLOC(pCtx,ppNew,pPathDir,pString,pfsStat)	\
	SG_STATEMENT(														\
		sg_wc_readdir__row * _pNew = NULL;								\
		sg_wc_readdir__row__alloc(pCtx,&_pNew,pPathDir,pString,pfsStat); \
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_readdir__row"); \
		*(ppNew) = _pNew;												\
		)

#define SG_WC_READDIR__ROW__ALLOC__ROW_ROOT(pCtx,ppNew,pPathDir)		\
	SG_STATEMENT(														\
		sg_wc_readdir__row * _pNew = NULL;								\
		sg_wc_readdir__row__alloc__row_root(pCtx,&_pNew,pPathDir);		\
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_readdir__row"); \
		*(ppNew) = _pNew;												\
		)

#define SG_WC_READDIR__ROW__ALLOC__COPY(pCtx,ppNew,pRow_Input)			\
	SG_STATEMENT(														\
		sg_wc_readdir__row * _pNew = NULL;								\
		sg_wc_readdir__row__alloc__copy(pCtx,&_pNew,pRow_Input);		\
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_readdir__row"); \
		*(ppNew) = _pNew;												\
		)

#else

#define SG_WC_READDIR__ROW__ALLOC(pCtx,ppNew,pPathDir,pString,pfsStat) \
	sg_wc_readdir__row__alloc(pCtx,ppNew,pPathDir,pString,pfsStat)

#define SG_WC_READDIR__ROW__ALLOC__ROW_ROOT(pCtx,ppNew,pPathDir)		\
	sg_wc_readdir__row__alloc__row_root(pCtx,ppNew,pPathDir)

#define SG_WC_READDIR__ROW__ALLOC__COPY(pCtx,ppNew,pRow_Input)			\
	sg_wc_readdir__row__alloc__copy(pCtx,ppNew,pRow_Input)

#endif

//////////////////////////////////////////////////////////////////

void sg_wc_readdir__row__get_attrbits(SG_context * pCtx,
									  SG_wc_tx * pWcTx,
									  sg_wc_readdir__row * pRow);

//////////////////////////////////////////////////////////////////

void sg_wc_readdir__row__get_content_hid(SG_context * pCtx,
										 SG_wc_tx * pWcTx,
										 sg_wc_readdir__row * pRow,
										 SG_bool bNoTSC,
										 const char ** ppszHid,
										 SG_uint64 * pSize);

void sg_wc_readdir__row__get_content_hid__owned(SG_context * pCtx,
												SG_wc_tx * pWcTx,
												sg_wc_readdir__row * pRow,
												SG_bool bNoTSC,
												char ** ppszHid,
												SG_uint64 * pSize);

void sg_wc_readdir__row__get_content_symlink_target(SG_context * pCtx,
													SG_wc_tx * pWcTx,
													sg_wc_readdir__row * pRow,
													const SG_string ** ppString);

//////////////////////////////////////////////////////////////////

void sg_wc_compute_file_hid(SG_context * pCtx,
							SG_wc_tx * pWcTx,
							const SG_pathname * pPath,
							char ** ppszHid,
							SG_uint64 * pui64Size);

void sg_wc_compute_file_hid__sz(SG_context * pCtx,
								SG_wc_tx * pWcTx,
								const char * pszFile,
								char ** ppszHid,
								SG_uint64 * pui64Size);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_READDIR__PRIVATE_PROTOTYPES_H
