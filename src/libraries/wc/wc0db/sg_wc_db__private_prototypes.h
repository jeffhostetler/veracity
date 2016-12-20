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

#ifndef H_SG_WC_DB__PRIVATE_PROTOTYPES_H
#define H_SG_WC_DB__PRIVATE_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void sg_wc_db__free(SG_context * pCtx,
					sg_wc_db * pDb);

#define SG_WC_DB_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,sg_wc_db__free)

void sg_wc_db__tx__free_cached_statements(SG_context * pCtx, 
										  sg_wc_db * pDb);

void sg_wc_db__close_db(SG_context * pCtx,
						sg_wc_db * pDb,
						SG_bool bDelete_DB);

void sg_wc_db__open_db(SG_context * pCtx,
					   const SG_repo * pRepo,
					   const SG_pathname * pPathWorkingDirectoryTop,
					   SG_bool bWorkingDirPathFromCwd,
					   sg_wc_db ** ppDb);

void sg_wc_db__create_db(SG_context * pCtx,
						 const SG_repo * pRepo,
						 const SG_pathname * pPathWorkingDirectoryTop,
						 SG_bool bWorkingDirPathFromCwd,
						 sg_wc_db ** ppDb);

void sg_wc_db__load_named_cset(SG_context * pCtx,
							   sg_wc_db * pDb,
							   const char * pszTableName,
							   const char * pszHidCSet,
							   SG_bool bPopulateTne,
							   SG_bool bCreateTneIndex,
							   SG_bool bCreatePC);

//////////////////////////////////////////////////////////////////

void sg_wc_db__ignores__load(SG_context * pCtx, sg_wc_db * pDb);

void sg_wc_db__ignores__matches_ignorable_pattern(SG_context * pCtx,
												  sg_wc_db * pDb,
												  const SG_string * pStringLiveRepoPath,
												  SG_bool * pbIgnorable);

void sg_wc_db__ignores__get_varray(SG_context * pCtx,
								   sg_wc_db * pDb,
								   SG_varray ** ppva);

//////////////////////////////////////////////////////////////////

void sg_wc_db__info__create_table(SG_context * pCtx, sg_wc_db * pDb);

void sg_wc_db__info__write_version(SG_context * pCtx, sg_wc_db * pDb);

void sg_wc_db__info__read_version(SG_context * pCtx, sg_wc_db * pDb, SG_uint32 * pDbVersion);

//////////////////////////////////////////////////////////////////

void sg_wc_db__gid__create_table(SG_context * pCtx, sg_wc_db * pDb);

void sg_wc_db__gid__insert_known(SG_context * pCtx,
								 sg_wc_db * pDb,
								 SG_uint64 uiAliasGid,
								 const char * pszGid);

void sg_wc_db__gid__insert(SG_context * pCtx,
						   sg_wc_db * pDb,
						   const char * pszGid);

void sg_wc_db__gid__insert_new(SG_context * pCtx,
							   sg_wc_db * pDb,
							   SG_bool bIsTmp,
							   SG_uint64 * puiAliasGidNew);

void sg_wc_db__gid__prepare_toggle_tmp_stmt(SG_context * pCtx,
											sg_wc_db * pDb,
											SG_uint64 uiAliasGid,
											SG_bool bIsTmp,
											sqlite3_stmt ** ppStmt);

void sg_wc_db__gid__get_alias_from_gid(SG_context * pCtx,
									   sg_wc_db * pDb,
									   const char * pszGid,
									   SG_uint64 * puiAliasGid);

void sg_wc_db__gid__get_gid_from_alias(SG_context * pCtx,
									   sg_wc_db * pDb,
									   SG_uint64 uiAliasGid,
									   char ** ppszGid);

void sg_wc_db__gid__get_gid_from_alias2(SG_context * pCtx,
										sg_wc_db * pDb,
										SG_uint64 uiAliasGid,
										char ** ppszGid,
										SG_bool * pbIsTmp);

void sg_wc_db__gid__get_gid_from_alias3(SG_context * pCtx,
										sg_wc_db * pDb,
										SG_uint64 uiAliasGid,
										char ** ppszGid);

void sg_wc_db__gid__get_or_insert_alias_from_gid(SG_context * pCtx,
												 sg_wc_db * pDb,
												 const char * pszGid,
												 SG_uint64 * puiAliasGid);

void sg_wc_db__gid__delete_all_tmp(SG_context * pCtx,
								   sg_wc_db * pDb);

//////////////////////////////////////////////////////////////////

void sg_wc_db__csets__create_table(SG_context * pCtx, sg_wc_db * pDb);

void sg_wc_db__csets__insert(SG_context * pCtx,
							 sg_wc_db * pDb,
							 const char * pszLabel,
							 const char * pszHidCSet,
							 const char * pszHidSuperRoot);

void sg_wc_db__csets__update__replace_all(SG_context * pCtx,
										  sg_wc_db * pDb,
										  const char * pszLabel,
										  const char * pszHidCSet,
										  const char * psz_tne,
										  const char * psz_pc,
										  const char * pszHidSuperRoot);

void sg_wc_db__csets__update__cset_hid(SG_context * pCtx,
									   sg_wc_db * pDb,
									   const char * pszLabel,
									   const char * pszHidCSet,
									   const char * pszHidSuperRoot);

void sg_wc_db__csets__get_row_by_label(SG_context * pCtx,
									   sg_wc_db * pDb,
									   const char * pszLabel,
									   SG_bool * pbFound,
									   sg_wc_db__cset_row ** ppCSetRow);

void sg_wc_db__csets__get_row_by_hid_cset(SG_context * pCtx,
										  sg_wc_db * pDb,
										  const char * pszHidCSet,
										  sg_wc_db__cset_row ** ppCSetRow);

void sg_wc_db__csets__foreach_in_wc(SG_context * pCtx,
									sg_wc_db * pDb,
									sg_wc_db__csets__foreach_cb * pfn_cb,
									void * pVoidData);
#if 0 // not currently needed
void sg_wc_db__csets__delete_all(SG_context * pCtx,
								 sg_wc_db * pDb);
void sg_wc_db__csets__delete_all_except_one(SG_context * pCtx,
											sg_wc_db * pDb,
											const char * pszLabelOfOneToKeep);
#endif

void sg_wc_db__csets__delete_row(SG_context * pCtx,
								 sg_wc_db * pDb,
								 const char * pszLabel);

void sg_wc_db__csets__get_all(SG_context * pCtx,
							  sg_wc_db * pDb,
							  SG_vector ** ppvecList);

//////////////////////////////////////////////////////////////////

void sg_wc_db__cset_row__free(SG_context * pCtx, sg_wc_db__cset_row * pCSetRow);

#define SG_WC_DB__CSET_ROW__NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,sg_wc_db__cset_row__free)

//////////////////////////////////////////////////////////////////

void sg_wc_db__tne__create_named_table(SG_context * pCtx,
									   sg_wc_db * pDB,
									   const sg_wc_db__cset_row * pCSetRow);

void sg_wc_db__tne__create_index(SG_context * pCtx,
								 sg_wc_db * pDb,
								 const sg_wc_db__cset_row * pCSetRow);

void sg_wc_db__tne__drop_named_table(SG_context * pCtx,
									 sg_wc_db * pDB,
									 const sg_wc_db__cset_row * pCSetRow);

void sg_wc_db__tne__bind_insert(SG_context * pCtx,
								sqlite3_stmt * pStmt,
								SG_uint64 uiAliasGid,
								SG_uint64 uiAliasGidParent,
								const SG_treenode_entry * pTreenodeEntry);

void sg_wc_db__tne__bind_insert_and_step(SG_context * pCtx,
										 sqlite3_stmt * pStmt,
										 SG_uint64 uiAliasGid,
										 SG_uint64 uiAliasGidParent,
										 const SG_treenode_entry * pTreenodeEntry);

void sg_wc_db__tne__insert_recursive(SG_context * pCtx,
									 sg_wc_db * pDb,
									 const sg_wc_db__cset_row * pCSetRow,
									 sqlite3_stmt * pStmt,
									 SG_uint64 uiAliasGidParent,
									 SG_uint64 uiAliasGid,
									 const SG_treenode_entry * pTreenodeEntry);

void sg_wc_db__tne__process_super_root(SG_context * pCtx,
									   sg_wc_db * pDb,
									   const sg_wc_db__cset_row * pCSetRow,
									   sqlite3_stmt * pStmt,
									   const char * pszHidSuperRoot);

void sg_wc_db__tne__populate_named_table(SG_context * pCtx,
										 sg_wc_db * pDb,
										 const sg_wc_db__cset_row * pCSetRow,
										 const char * pszHidSuperRoot,
										 SG_bool bCreateIndex);

void sg_wc_db__tne__get_row_by_alias(SG_context * pCtx,
									 sg_wc_db * pDb,
									 const sg_wc_db__cset_row * pCSetRow,
									 SG_uint64 uiAliasGid,
									 SG_bool * pbFound,
									 sg_wc_db__tne_row ** ppTneRow);

void sg_wc_db__tne__get_row_by_gid(SG_context * pCtx,
								   sg_wc_db * pDb,
								   const sg_wc_db__cset_row * pCSetRow,
								   const char * pszGid,
								   SG_bool * pbFound,
								   sg_wc_db__tne_row ** ppTneRow);

void sg_wc_db__tne__foreach_in_dir_by_parent_alias(SG_context * pCtx,
												   sg_wc_db * pDb,
												   const sg_wc_db__cset_row * pCSetRow,
												   SG_uint64 uiAliasGidParent,
												   sg_wc_db__tne__foreach_cb * pfn_cb,
												   void * pVoidData);

void sg_wc_db__tne__get_alias_of_root(SG_context * pCtx,
									  sg_wc_db * pDb,
									  const sg_wc_db__cset_row * pCSetRow,
									  SG_uint64 * puiAliasGid_Root);

void sg_wc_db__tne__prepare_insert_stmt(SG_context * pCtx,
										sg_wc_db * pDb,
										sg_wc_db__cset_row * pCSetRow,
										SG_uint64 uiAliasGid,
										SG_uint64 uiAliasGid_Parent,
										const SG_treenode_entry * pTNE,
										sqlite3_stmt ** ppStmt);

void sg_wc_db__tne__prepare_delete_stmt(SG_context * pCtx,
										sg_wc_db * pDb,
										sg_wc_db__cset_row * pCSetRow,
										SG_uint64 uiAliasGid,
										sqlite3_stmt ** ppStmt);

void sg_wc_db__tne__get_row_by_parent_alias_and_entryname(SG_context * pCtx,
														  sg_wc_db * pDb,
														  const sg_wc_db__cset_row * pCSetRow,
														  SG_uint64 uiAliasGidParent,
														  const char * pszEntryname,
														  SG_bool * pbFound,
														  sg_wc_db__tne_row ** ppTneRow);

void sg_wc_db__tne__get_alias_from_extended_repo_path(SG_context * pCtx,
													  sg_wc_db * pDb,
													  const sg_wc_db__cset_row * pCSetRow,
													  const char * pszBaselineRepoPath,
													  SG_bool * pbFound,
													  SG_uint64 * puiAliasGid);

void sg_wc_db__tne__get_extended_repo_path_from_alias(SG_context * pCtx,
													  sg_wc_db * pDb,
													  const sg_wc_db__cset_row * pCSetRow,
													  SG_uint64 uiAliasGid,
													  char chDomain,
													  SG_string ** ppStringRepoPath);

//////////////////////////////////////////////////////////////////

void sg_wc_db__tne_row__free(SG_context * pCtx, sg_wc_db__tne_row * pTneRow);

#define SG_WC_DB__TNE_ROW__NULLFREE(pCtx,p)              _sg_generic_nullfree(pCtx,p,sg_wc_db__tne_row__free)

#if TRACE_WC_DB
void sg_wc_db__debug__tne_row__print(SG_context * pCtx, sg_wc_db__tne_row * pTneRow);
#endif

//////////////////////////////////////////////////////////////////

void sg_wc_db__path__compute_pathname_in_drawer(SG_context * pCtx, sg_wc_db * pDb);

void sg_wc_db__path__get_temp_dir(SG_context * pCtx,
								  const sg_wc_db * pDb,
								  SG_pathname ** ppPathTempDir);

void sg_wc_db__path__get_unique_temp_path(SG_context * pCtx,
										  const sg_wc_db * pDb,
										  SG_pathname ** ppPathTemp);

void sg_wc_db__path__generate_backup_path(SG_context * pCtx,
										  const sg_wc_db * pDb,
										  const char * pszGid,
										  const char * pszEntryname,
										  SG_pathname ** ppPathTemp);

void sg_wc_db__path__absolute_to_repopath(SG_context * pCtx,
										  const sg_wc_db * pDb,
										  const SG_pathname * pPathItem,
										  SG_string ** ppStringRepoPath);

void sg_wc_db__path__relative_to_absolute(SG_context * pCtx,
										  const sg_wc_db * pDb,
										  const SG_string * pStringRelativePath,
										  SG_pathname ** ppPathItem);

void sg_wc_db__path__sz_repopath_to_absolute(SG_context * pCtx,
											 const sg_wc_db * pDb,
											 const char * pszRepoPath,
											 SG_pathname ** ppPathItem);

void sg_wc_db__path__repopath_to_absolute(SG_context * pCtx,
										  const sg_wc_db * pDb,
										  const SG_string * pStringRepoPath,
										  SG_pathname ** ppPathItem);

void sg_wc_db__path__anything_to_repopath(SG_context * pCtx,
										  const sg_wc_db * pDb,
										  const char * pszInput,
										  sg_wc_db__path__import_flags flags,
										  SG_string ** ppStringRepoPath,
										  char * pcDomain);

void sg_wc_db__path__gid_to_gid_repopath(SG_context * pCtx,
										 const char * pszGid,
										 SG_string ** ppStringGidRepoPath);

void sg_wc_db__path__alias_to_gid_repopath(SG_context * pCtx,
										   sg_wc_db * pDb,
										   SG_uint64 uiAliasGid,
										   SG_string ** ppStringGidRepoPath);

void sg_wc_db__path__is_reserved_entryname(SG_context * pCtx,
										   sg_wc_db * pDb,
										   const char * pszEntryname,
										   SG_bool * pbIsReserved);

//////////////////////////////////////////////////////////////////

void sg_wc_db__tx__begin(SG_context * pCtx, sg_wc_db * pDb, SG_bool bExclusive);

void sg_wc_db__tx__commit(SG_context * pCtx, sg_wc_db * pDb);

void sg_wc_db__tx__rollback(SG_context * pCtx, sg_wc_db * pDb);

void sg_wc_db__tx__assert(SG_context * pCtx, sg_wc_db * pDb);

void sg_wc_db__tx__in_tx(SG_context * pCtx, sg_wc_db * pDb, SG_bool * pbInTx);

void sg_wc_db__tx__assert_not(SG_context * pCtx, sg_wc_db * pDb);

//////////////////////////////////////////////////////////////////

void sg_wc_db__pc_row__free(SG_context * pCtx, sg_wc_db__pc_row * pPcRow);

#define SG_WC_DB__PC_ROW__NULLFREE(pCtx,p)               _sg_generic_nullfree(pCtx,p,sg_wc_db__pc_row__free)

void sg_wc_db__pc_row__alloc(SG_context * pCtx, sg_wc_db__pc_row ** ppPcRow);

void sg_wc_db__pc_row__alloc__copy(SG_context * pCtx,
								   sg_wc_db__pc_row ** ppPcRow,
								   const sg_wc_db__pc_row * pPcRow_src);

void sg_wc_db__pc_row__alloc__from_tne_row(SG_context * pCtx,
										   sg_wc_db__pc_row ** ppPcRow,
										   const sg_wc_db__tne_row * pTneRow,
										   SG_bool bMarkItSparse);

void sg_wc_db__pc_row__alloc__from_readdir(SG_context * pCtx,
										   sg_wc_db__pc_row ** ppPcRow,
										   const sg_wc_readdir__row * pRD,
										   SG_uint64 uiAliasGid,
										   SG_uint64 uiAliasGidParent);

#if defined(DEBUG)

#define SG_WC_DB__PC_ROW__ALLOC(pCtx,ppNew)								\
	SG_STATEMENT(														\
		sg_wc_db__pc_row * _pNew = NULL;								\
		sg_wc_db__pc_row__alloc(pCtx,&_pNew);							\
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_db__pc_row"); \
		*(ppNew) = _pNew;												\
		)

#define SG_WC_DB__PC_ROW__ALLOC__COPY(pCtx,ppNew,pSrc)					\
	SG_STATEMENT(														\
		sg_wc_db__pc_row * _pNew = NULL;								\
		sg_wc_db__pc_row__alloc__copy(pCtx,&_pNew,pSrc);				\
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_db__pc_row"); \
		*(ppNew) = _pNew;												\
		)

#define SG_WC_DB__PC_ROW__ALLOC__FROM_TNE_ROW(pCtx,ppNew,pSrc,b)		\
	SG_STATEMENT(														\
		sg_wc_db__pc_row * _pNew = NULL;								\
		sg_wc_db__pc_row__alloc__from_tne_row(pCtx,&_pNew,pSrc,b);		\
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_db__pc_row"); \
		*(ppNew) = _pNew;												\
		)

#define SG_WC_DB__PC_ROW__ALLOC__FROM_READDIR(pCtx,ppNew,pRD,ui1,ui2)	\
	SG_STATEMENT(														\
		sg_wc_db__pc_row * _pNew = NULL;								\
		sg_wc_db__pc_row__alloc__from_readdir(pCtx,&_pNew,pRD,ui1,ui2);	\
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_db__pc_row"); \
		*(ppNew) = _pNew;												\
		)

#else

#define SG_WC_DB__PC_ROW__ALLOC(pCtx,ppNew)		\
	sg_wc_db__pc_row__alloc(pCtx,ppNew)

#define SG_WC_DB__PC_ROW__ALLOC__COPY(pCtx,ppNew,pSrc)	\
	sg_wc_db__pc_row__alloc__copy(pCtx,ppNew,pSrc)

#define SG_WC_DB__PC_ROW__ALLOC__FROM_TNE_ROW(pCtx,ppNew,pSrc,b)	\
	sg_wc_db__pc_row__alloc__from_tne_row(pCtx,ppNew,pSrc,b)

#define SG_WC_DB__PC_ROW__ALLOC__FROM_READDIR(pCtx,ppNew,pRD,ui1,ui2)	\
	sg_wc_db__pc_row__alloc__from_readdir(pCtx,ppNew,pRD,ui1,ui2)

#endif

#if defined(DEBUG)
void sg_wc_pc__debug__pc_row__print(SG_context * pCtx, const sg_wc_db__pc_row * pPcRow);
#endif

//////////////////////////////////////////////////////////////////

void sg_wc_db__pc__create_table(SG_context * pCtx,
								sg_wc_db * pDb,
								const sg_wc_db__cset_row * pCSetRow);

void sg_wc_db__pc__create_index(SG_context * pCtx,
								sg_wc_db * pDb,
								const sg_wc_db__cset_row * pCSetRow);

void sg_wc_db__pc__drop_named_table(SG_context * pCtx,
									sg_wc_db * pDb,
									const sg_wc_db__cset_row * pCSetRow);

void sg_wc_db__pc__insert_self_immediate(SG_context * pCtx,
										 sg_wc_db * pDb,
										 const sg_wc_db__cset_row * pCSetRow,
										 const sg_wc_db__pc_row * pPcRow);

void sg_wc_db__pc__prepare_insert_stmt(SG_context * pCtx,
									   sg_wc_db * pDb,
									   const sg_wc_db__cset_row * pCSetRow,
									   const sg_wc_db__pc_row * pPcRow,
									   sqlite3_stmt ** ppStmt);

void sg_wc_db__pc__prepare_delete_stmt(SG_context * pCtx,
									   sg_wc_db * pDb,
									   const sg_wc_db__cset_row * pCSetRow,
									   SG_uint64 uiAliasGid,
									   sqlite3_stmt ** ppStmt);

void sg_wc_db__pc__prepare_clean_but_leave_sparse_stmt(SG_context * pCtx,
													   sg_wc_db * pDb,
													   const sg_wc_db__cset_row * pCSetRow,
													   SG_uint64 uiAliasGid,
													   sqlite3_stmt ** ppStmt);

void sg_wc_db__pc__get_row_by_alias(SG_context * pCtx,
									sg_wc_db * pDb,
									const sg_wc_db__cset_row * pCSetRow,
									SG_uint64 uiAliasGid,
									SG_bool * pbFound,
									sg_wc_db__pc_row ** ppPcRow);

void sg_wc_db__pc__foreach_in_dir_by_parent_alias(SG_context * pCtx,
												  sg_wc_db * pDb,
												  const sg_wc_db__cset_row * pCSetRow,
												  SG_uint64 uiAliasGidParent,
												  sg_wc_db__pc__foreach_cb * pfn_cb,
												  void * pVoidData);

#if defined(DEBUG)
void sg_wc_db__debug__pc_row__print(SG_context * pCtx, const sg_wc_db__pc_row * pPcRow);
#endif

//////////////////////////////////////////////////////////////////

void sg_wc_db__state_structural__free(SG_context * pCtx, sg_wc_db__state_structural * p_s);

#define SG_WC_DB__STATE_STRUCTURAL__NULLFREE(pCtx,p)     _sg_generic_nullfree(pCtx,p,sg_wc_db__state_structural__free)

void sg_wc_db__state_structural__alloc(SG_context * pCtx, sg_wc_db__state_structural ** pp_s);

void sg_wc_db__state_structural__alloc__copy(SG_context * pCtx,
											 sg_wc_db__state_structural ** pp_s,
											 const sg_wc_db__state_structural * p_s_src);

void sg_wc_db__state_structural__alloc__fields(SG_context * pCtx,
											   sg_wc_db__state_structural ** pp_s,
											   SG_uint64 uiAliasGid,
											   SG_uint64 uiAliasGidParent,
											   const char * pszEntryname,
											   SG_treenode_entry_type tneType);

#if defined(DEBUG)

#define SG_WC_DB__STATE_STRUCTURAL__ALLOC(pCtx,ppNew)					\
	SG_STATEMENT(														\
		sg_wc_db__state_structural * _pNew = NULL;						\
		sg_wc_db__state_structural__alloc(pCtx,&_pNew);					\
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_db__state_structural"); \
		*(ppNew) = _pNew;												\
		)

#define SG_WC_DB__STATE_STRUCTURAL__ALLOC__COPY(pCtx,ppNew,pSrc)		\
	SG_STATEMENT(														\
		sg_wc_db__state_structural * _pNew = NULL;						\
		sg_wc_db__state_structural__alloc__copy(pCtx,&_pNew,pSrc);		\
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_db__state_structural"); \
		*(ppNew) = _pNew;												\
		)

#define SG_WC_DB__STATE_STRUCTURAL__ALLOC__FIELDS(pCtx,ppNew,ui1,ui2,psz,tne) \
	SG_STATEMENT(														\
		sg_wc_db__state_structural * _pNew = NULL;						\
		sg_wc_db__state_structural__alloc__fields(pCtx,&_pNew,ui1,ui2,psz,tne); \
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_db__state_structural"); \
		*(ppNew) = _pNew;												\
		)

#else

#define SG_WC_DB__STATE_STRUCTURAL__ALLOC(pCtx,ppNew)					\
	sg_wc_db__state_structural__alloc(pCtx,ppNew)

#define SG_WC_DB__STATE_STRUCTURAL__ALLOC__COPY(pCtx,ppNew,pSrc)		\
	sg_wc_db__state_structural__alloc__copy(pCtx,ppNew,pSrc)

#define SG_WC_DB__STATE_STRUCTURAL__ALLOC__FIELDS(pCtx,ppNew,ui1,ui2,psz,tne) \
	sg_wc_db__state_structural__alloc__fields(pCtx,ppNew,ui1,ui2,psz,tne)

#endif

//////////////////////////////////////////////////////////////////

void sg_wc_db__state_dynamic__free(SG_context * pCtx, sg_wc_db__state_dynamic * p_d);

#define SG_WC_DB__STATE_DYNAMIC__NULLFREE(pCtx,p)        _sg_generic_nullfree(pCtx,p,sg_wc_db__state_dynamic__free)

void sg_wc_db__state_dynamic__alloc(SG_context * pCtx, sg_wc_db__state_dynamic ** pp_d);

void sg_wc_db__state_dynamic__alloc__copy(SG_context * pCtx,
										  sg_wc_db__state_dynamic ** pp_d,
										  const sg_wc_db__state_dynamic * p_d_src);

#if defined(DEBUG)

#define SG_WC_DB__STATE_DYNAMIC__ALLOC(pCtx,ppNew)						\
	SG_STATEMENT(														\
		sg_wc_db__state_dynamic * _pNew = NULL;							\
		sg_wc_db__state_dynamic__alloc(pCtx,&_pNew);					\
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_db__state_dynamic"); \
		*(ppNew) = _pNew;												\
		)

#define SG_WC_DB__STATE_DYNAMIC__ALLOC__COPY(pCtx,ppNew,pSrc)			\
	SG_STATEMENT(														\
		sg_wc_db__state_dynamic * _pNew = NULL;							\
		sg_wc_db__state_dynamic__alloc__copy(pCtx,&_pNew,pSrc);			\
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"sg_wc_db__state_dynamic"); \
		*(ppNew) = _pNew;												\
		)

#else

#define SG_WC_DB__STATE_DYNAMIC__ALLOC(pCtx,ppNew)	\
	sg_wc_db__state_dynamic__alloc(pCtx,ppNew)

#define SG_WC_DB__STATE_DYNAMIC__ALLOC__COPY(pCtx,ppNew,pSrc)	\
	sg_wc_db__state_dynamic__alloc(pCtx,ppNew,pSrc)

#endif

//////////////////////////////////////////////////////////////////

void sg_wc_db__create_port(SG_context * pCtx,
						   const sg_wc_db * pDb,
						   SG_wc_port ** ppPort);

void sg_wc_db__create_port__no_indiv(SG_context * pCtx,
									 const sg_wc_db * pDb,
									 SG_wc_port ** ppPort);

//////////////////////////////////////////////////////////////////

void sg_wc_db__timestamp_cache__open(SG_context * pCtx, sg_wc_db * pDb);

void sg_wc_db__timestamp_cache__is_valid(SG_context * pCtx,
										 sg_wc_db * pDb,
										 const char * pszGid,
										 const SG_fsobj_stat * pfsStat,
										 SG_bool * pbValid,
										 const SG_timestamp_data ** ppTSData);

void sg_wc_db__timestamp_cache__set(SG_context * pCtx,
									sg_wc_db * pDb,
									const char * pszGid,
									const SG_fsobj_stat * pfsStat,
									const char * pszHid);

void sg_wc_db__timestamp_cache__remove(SG_context * pCtx,
									   sg_wc_db * pDb,
									   const char * pszGid);

void sg_wc_db__timestamp_cache__remove_all(SG_context * pCtx, sg_wc_db * pDb);

void sg_wc_db__timestamp_cache__save(SG_context * pCtx, sg_wc_db * pDb);

//////////////////////////////////////////////////////////////////

void sg_wc_db__issue__create_table(SG_context * pCtx,
								   sg_wc_db * pDb);

void sg_wc_db__issue__drop_table(SG_context * pCtx,
								 sg_wc_db * pDb);

void sg_wc_db__issue__prepare_insert(SG_context * pCtx,
									 sg_wc_db * pDb,
									 SG_uint64 uiAliasGid,
									 SG_wc_status_flags statusFlags_x_xr_xu,
									 const SG_string * pStringIssue,
									 const SG_string * pStringResolve,
									 sqlite3_stmt ** ppStmt);

void sg_wc_db__issue__prepare_update_status__s(SG_context * pCtx,
											   sg_wc_db * pDb,
											   SG_uint64 uiAliasGid,
											   SG_wc_status_flags statusFlags_x_xr_xu,
											   sqlite3_stmt ** ppStmt);

void sg_wc_db__issue__prepare_update_status__sr(SG_context * pCtx,
												sg_wc_db * pDb,
												SG_uint64 uiAliasGid,
												SG_wc_status_flags statusFlags_x_xr_xu,
												const SG_string * pStringResolve,
												sqlite3_stmt ** ppStmt);

void sg_wc_db__issue__prepare_delete(SG_context * pCtx,
									 sg_wc_db * pDb,
									 SG_uint64 uiAliasGid,
									 sqlite3_stmt ** ppStmt);

void sg_wc_db__issue__get_issue(SG_context * pCtx,
								sg_wc_db * pDb,
								SG_uint64 uiAliasGid,
								SG_bool * pbFound,
								SG_wc_status_flags * pStatusFlags_x_xr_xu,
								SG_vhash ** ppvhIssue,
								SG_vhash ** ppvhResolve);

#if 1
// TODO            See W8312.
void sg_wc_db__issue__foreach(SG_context * pCtx,
							  sg_wc_db * pDb,
							  sg_wc_db__issue__foreach_cb * pfn_cb,
							  void * pVoidData);
#endif

//////////////////////////////////////////////////////////////////

void sg_wc_db__branch__create_table(SG_context * pCtx,
									sg_wc_db * pDb);

void sg_wc_db__branch__get_branch(SG_context * pCtx,
								  sg_wc_db * pDb,
								  char ** ppszBranchName);

void sg_wc_db__branch__attach(SG_context * pCtx,
							  sg_wc_db * pDb,
							  const char * pszBranchName,
							  SG_vc_branches__check_attach_name__flags flags,
							  SG_bool bValidateName);

void sg_wc_db__branch__detach(SG_context * pCtx,
							  sg_wc_db * pDb);

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_DB__PRIVATE_PROTOTYPES_H
