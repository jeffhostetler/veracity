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

#ifndef H_SG_NULLFREE_H
#define H_SG_NULLFREE_H

BEGIN_EXTERN_C

#define _sg_generic_nullfree(pCtx,p,fn) SG_STATEMENT(SG_context__push_level(pCtx);fn(pCtx, p);SG_ASSERT(!SG_CONTEXT__HAS_ERR(pCtx));SG_context__pop_level(pCtx);p=NULL;)

#define SG_FSOBJ_STAT_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_fsobj_stat__free)
#define SG_IHASH_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_ihash__free)
#define SG_BLOBSET_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_blobset__free)
#define SG_CHANGESET_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_changeset__free)
#define SG_DAGFRAG_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_dagfrag__free)
#define SG_DAGLCA_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_daglca__free)
#define SG_DAGLCA_ITERATOR_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_daglca__iterator__free)
#define SG_DAGNODE_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_dagnode__free)
#define SG_DBRECORD_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_dbrecord__free)
#define SG_FILEDIFF_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_filediff__free)
#define SG_EXEC_ARGVEC_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_exec_argvec__free)
#define SG_FILE_SPEC_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_file_spec__free)
#define SG_FRAGBALL_WRITER_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_fragball_writer__free)
#define SG_HDB_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_hdb__close_free)
#define SG_TNCACHE_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_tncache__free)
#define SG_HISTORY_RESULT_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_history_result__free)
#define SG_HISTORY_TOKEN_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_history_token__free)
#define SG_JSONPARSER_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_jsonparser__free)
#define SG_JSONWRITER_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_jsonwriter__free)
#define SG_JSONDB_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_jsondb__close_free)
#define SG_PATHNAME_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_pathname__free)
#define SG_PENDINGDB_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_pendingdb__free)
#define SG_RBTREE_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_rbtree__free)
#define SG_RBTREE_ITERATOR_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_rbtree__iterator__free)
#define SG_RBTREEDB_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_rbtreedb__close_free)
#define SG_REPO_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_repo__free)
#define SG_STAGING_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_staging__free)
#define SG_STRING_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_string__free)
#define SG_STRINGARRAY_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_stringarray__free)
#define SG_STRPOOL_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_strpool__free)
#define SG_SYNC_CLIENT_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_sync_client__close_free)
#define SG_TEXTFILEDIFF_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_textfilediff__free)
#define SG_TEXTFILEDIFF_ITERATOR_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_textfilediff__iterator__free)
#define SG_TIMESTAMP_CACHE_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_timestamp_cache__free)
#define SG_TREENODE_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_treenode__free)
#define SG_TREENODE_ENTRY_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_treenode_entry__free)
#define SG_UTF8_CONVERTER_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_utf8_converter__free)
#define SG_VARIANT_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_variant__free)
#define SG_VARPOOL_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_varpool__free)
#define SG_VARRAY_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_varray__free)
#define SG_VECTOR_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_vector__free)
#define SG_VECTOR_I64_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_vector_i64__free)
#define SG_BITVECTOR_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_bitvector__free)
#define SG_VHASH_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_vhash__free)
#define SG_ZINGFIELDATTRIBUTES_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_zingfieldattributes__free)
#define SG_ZINGRECTYPEINFO_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_zingrectypeinfo__free)
#define SG_ZINGRECORD_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_zingrecord__free)
#define SG_ZINGTEMPLATE_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_zingtemplate__free)

#define SG_NULLFREE(pCtx,p) _sg_generic_nullfree(pCtx,p,SG_free)

#define SG_CONTEXT_NULLFREE(p)                    SG_STATEMENT(                                            SG_context__free(p);                                                                           p=NULL;)

#define SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx,p,cb)  SG_STATEMENT(SG_context__push_level(pCtx);    SG_rbtree__free__with_assoc(pCtx, p,cb);SG_ASSERT(!SG_CONTEXT__HAS_ERR(pCtx));SG_context__pop_level(pCtx);p=NULL;)
#define SG_VECTOR_NULLFREE_WITH_ASSOC(pCtx,p,cb)  SG_STATEMENT(SG_context__push_level(pCtx);    SG_vector__free__with_assoc(pCtx, p,cb);SG_ASSERT(!SG_CONTEXT__HAS_ERR(pCtx));SG_context__pop_level(pCtx);p=NULL;)

END_EXTERN_C

#endif//H_SG_NULLFREE_H
