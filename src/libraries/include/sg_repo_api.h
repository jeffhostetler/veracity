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

#ifndef H_SG_REPO_API_H
#define H_SG_REPO_API_H

#if defined(WINDOWS)

/* TODO: The C++ code that links with sglib wants constructors and assignment operators for struct _sg_repo__vtable.
         What's the right way to fix this? */
#pragma warning( disable : 4510 )
#pragma warning( disable : 4512 )
#pragma warning( disable : 4610 )

#endif

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////
// Function prototypes for the routines in the REPO VTABLE.
//
// TODO The documentation describing the REPO API VTABLE routines
// TODO should be moved and placed on the public SG_repo_ routines.
// TODO That is, FN__sg_repo__open_repo_instance() is the VTABLE
// TODO prototype, but users call SG_repo__open_repo_instance()
// TODO and we privately handle the vtable-switch.  So in theory,
// TODO this header should be private and not seen by sglib callers.

/**
 * Use this to open an existing repo instance.  The
 * repo's descriptor must already be set (which should
 * be obvious, since the descriptor was used to select
 * the vtable).
 */
typedef void FN__sg_repo__open_repo_instance(
		SG_context* pCtx, 
        SG_repo * pRepo
        );

/**
 * Use this to create a new repo instance. 
 */
typedef void FN__sg_repo__create_repo_instance(
		SG_context* pCtx,
        SG_repo * pRepo, 
        SG_bool b_indexes,
        const char* psz_hash_method,	/**< TODO is this optional and if so
										 * what is the default value and/or
										 * is it repo-specific? */
        const char* psz_repo_id,    /**< Required.  GID.  If you want to create
                                      an instance of an entirely new repo,
                                      create a new GID and pass it in.
                                      Otherwise, you are creating a clone, an
                                      instance of an existing repo, so just get
                                      the ID from that repo and pass that in
                                      here. */

        const char* psz_admin_id   /**< Required.  This is the admin GID of the repo. */
        );

/**
 * Use this to remove a repo instance from disk.
 */
typedef void FN__sg_repo__delete_repo_instance(
    SG_context * pCtx,
    SG_repo * pRepo
    );

typedef void FN__sg_repo__verify__dbndx_states(
    SG_context* pCtx,
    SG_repo * pRepo, 
    SG_uint64 dagnum,
    SG_vhash** pp_vhash
    );
             
typedef void FN__sg_repo__verify__dag_consistency(
    SG_context* pCtx,
    SG_repo * pRepo, 
    SG_uint64 dagnum,
    SG_vhash** pp_vhash
    );
             
/**
 * This method is used to ask questions about the capabilities
 * and limitations of a particular implementation of the repo
 * API.  The intent is that the implementation will respond
 * with answers which are hard-coded.  This is not to be used
 * for fetching data from the repo or about the repo's data.
 *
 * Note that it DOES NOT take a pRepo so it CANNOT answer
 * questions about a specific REPO on disk; only questions
 * specific to the implementation.
 */
typedef void FN__sg_repo__query_implementation(
    SG_context* pCtx,
    SG_uint16 question,
    SG_bool* p_bool,
    SG_int64* p_int,
    char* p_buf_string,
    SG_uint32 len_buf_string,
    SG_vhash** pp_vhash
    );
             
/**
 * This method removes all blobs that exist in the repo
 * from prbBlobHids.
 */
typedef void FN__sg_repo__query_blob_existence(
	SG_context* pCtx,
	SG_repo * pRepo,
	SG_stringarray* psaQueryBlobHids,
	SG_stringarray** ppsaNonexistentBlobs
	);

/**
 * This method will be called when the SG_repo object is freed.  It is your
 * opportunity to clean up anything.
 */
typedef void FN__sg_repo__close_repo_instance(
		SG_context* pCtx,
		SG_repo * pRepo
        );

#define SG_REPO_TX_FLAG__CLONING  1

typedef void FN__sg_repo__begin_tx(SG_context* pCtx,
								   SG_repo* pRepo,
                                   SG_uint32 flags,
								   SG_repo_tx_handle** ppTx);

typedef void FN__sg_repo__commit_tx(SG_context* pCtx,
									SG_repo* pRepo,
									SG_repo_tx_handle** ppTx);

typedef void FN__sg_repo__abort_tx(SG_context* pCtx,
								   SG_repo* pRepo,
								   SG_repo_tx_handle** ppTx);

typedef void FN__sg_repo__hash__begin(
	SG_context* pCtx,
    SG_repo * pRepo,
    SG_repo_hash_handle** ppHandle
    );

typedef void FN__sg_repo__hash__chunk(
    SG_context* pCtx,
	SG_repo * pRepo,
    SG_repo_hash_handle* pHandle,
    SG_uint32 len_chunk,
    const SG_byte* p_chunk
    );

typedef void FN__sg_repo__hash__end(
    SG_context* pCtx,
	SG_repo * pRepo,
    SG_repo_hash_handle** pHandle,
    char** ppsz_hid_returned
    );

typedef void FN__sg_repo__hash__abort(
    SG_context* pCtx,
	SG_repo * pRepo,
    SG_repo_hash_handle** pHandle
    );

typedef void FN__sg_repo__store_blob__changeset(
	SG_context* pCtx,
    SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    const char* psz_json,
    SG_uint32 len_full,
    const char* psz_hid_known,
    SG_vhash** ppvh,
    char** ppsz_hid_returned
    );

typedef void FN__sg_repo__store_blob__dbtemplate(
	SG_context* pCtx,
    SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    const char* psz_json,
    SG_uint32 len_full,
    SG_uint64 dagnum,
    const char* psz_hid_known,
    SG_vhash** ppvh,
    char** ppsz_hid_returned
    );

typedef void FN__sg_repo__store_blob__dbrecord(
	SG_context* pCtx,
    SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    const char* psz_json,
    SG_uint32 len_full,
    SG_uint64 dagnum,
    const char* psz_hid_known,
    SG_vhash** ppvh,
    char** ppsz_hid_returned
    );

typedef void FN__sg_repo__store_blob__begin(
	SG_context* pCtx,
    SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    SG_blob_encoding blob_encoding,
    const char* psz_hid_vcdiff_reference,
    SG_uint64 lenFull,
	SG_uint64 lenEncoded,
    const char* psz_hid_known,
    SG_repo_store_blob_handle** ppHandle
    );

typedef void FN__sg_repo__store_blob__chunk(
    SG_context* pCtx,
	SG_repo * pRepo,
    SG_repo_store_blob_handle* pHandle,
    SG_uint32 len_chunk,
    const SG_byte* p_chunk,
    SG_uint32* piWritten
    );

typedef void FN__sg_repo__store_blob__end(
    SG_context* pCtx,
	SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    SG_repo_store_blob_handle** pHandle,
    char** ppsz_hid_returned
    );

typedef void FN__sg_repo__store_blob__abort(
    SG_context* pCtx,
	SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    SG_repo_store_blob_handle** pHandle
    );

typedef void FN__sg_repo__get_blob(
    SG_context* pCtx,
	SG_repo * pRepo,
    const char* psz_hid_blob,
    SG_blob** ppHandle
    );
             
typedef void FN__sg_repo__release_blob(
    SG_context* pCtx,
	SG_repo * pRepo,
    SG_blob** ppBlob
    );
             
typedef void FN__sg_repo__fetch_blob__begin(
    SG_context* pCtx,
	SG_repo * pRepo,
    const char* psz_hid_blob,
    SG_bool b_convert_to_full,
    SG_blob_encoding* pBlobFormat,
    char** ppsz_hid_vcdiff_reference,
    SG_uint64* pLenRawData,
    SG_uint64* pLenFull,
    SG_repo_fetch_blob_handle** ppHandle
    );
             
typedef void FN__sg_repo__fetch_blob__chunk(
    SG_context* pCtx,
	SG_repo * pRepo,
    SG_repo_fetch_blob_handle* pHandle,
    SG_uint32 len_buf,
    SG_byte* p_buf,
    SG_uint32* p_len_got,
    SG_bool* pb_done
    );

typedef void FN__sg_repo__fetch_blob__end(
    SG_context* pCtx,
	SG_repo * pRepo,
    SG_repo_fetch_blob_handle** ppHandle
    );

typedef void FN__sg_repo__fetch_blob__abort(
    SG_context* pCtx,
	SG_repo * pRepo,
    SG_repo_fetch_blob_handle** ppHandle
    );

typedef void FN__sg_repo__fetch_repo__fragball(
	SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint32 version,
	const SG_pathname* pFragballDirPathname,
	char** ppszFragballName
	);

typedef void FN__sg_repo__fetch_dagnode(
        SG_context* pCtx,
		SG_repo * pRepo, 
		SG_uint64 iDagNum,
        const char* pszidHidChangeset, 
        SG_dagnode ** ppNewDagnode
        );

typedef void FN__sg_repo__find_dag_path(
	SG_context* pCtx,
	SG_repo * pRepo, 
	SG_uint64 iDagNum,
    const char* psz_id_min,
    const char* psz_id_max,
    SG_varray** ppva
	);

typedef void FN__sg_repo__fetch_dagnodes__begin(
	SG_context* pCtx,
	SG_repo * pRepo, 
	SG_uint64 iDagNum,
	SG_repo_fetch_dagnodes_handle ** ppHandle
	);

typedef void FN__sg_repo__fetch_dagnodes__one(
	SG_context* pCtx,
	SG_repo * pRepo,
	SG_repo_fetch_dagnodes_handle * pHandle,
	const char * pszidHidChangeset,
	SG_dagnode ** ppNewDagnode
	);

typedef void FN__sg_repo__fetch_dagnodes__end(
	SG_context* pCtx,
	SG_repo * pRepo, 
	SG_repo_fetch_dagnodes_handle ** ppHandle
	);


/**
 * This function returns the current collection of leaves for a given DAG.
 *
 * If you are writing an implementation of repo storage, you should
 * plan to keep the leaf list precalculated.  Yes, this will
 * require a concurrency lock, but it's almost certainly worth
 * the trouble.  Calculating the leaf list on the fly will
 * probably cost a fair amount.
 */
typedef void FN__sg_repo__fetch_dag_leaves(
        SG_context* pCtx,
		SG_repo * pRepo, 
        SG_uint64 iDagNum, 
        SG_rbtree ** ppIdsetReturned    /**< Caller must free */
        );


/**
 * This function returns the current collection of children for a given dagnode
 * in a given DAG.
 *
 * You should not call this a lot.  Any dag walking should be done from the leaves-up,
 * since that is much more efficient than calling this function a lot.
 */
typedef void FN__sg_repo__fetch_dagnode_children(
        SG_context* pCtx,
		SG_repo * pRepo,
        SG_uint64 iDagNum,
        const char * pszDagnodeID,
        SG_rbtree ** ppIdsetReturned    /**< Caller must free */
        );

typedef void FN__sg_repo__fetch_dagnode_ids(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint64 iDagNum,
    SG_int32 gen_min,
    SG_int32 gen_max,
    SG_ihash** ppih
    );

typedef void FN__sg_repo__find_new_dagnodes_since(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint64 iDagNum,
    SG_varray* pva_starting,
    SG_ihash** ppih
	);

typedef void FN__sg_repo__fetch_chrono_dagnode_list(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint64 iDagNum,
	SG_uint32 iStartRevNo,
	SG_uint32 iNumRevsRequested,
	SG_uint32* piNumRevsReturned,
	char*** ppaszHidsReturned); /**< Caller must free */

typedef void FN__sg_repo__check_dagfrag(
        SG_context* pCtx,
		SG_repo * pRepo,
        SG_dagfrag * pFrag,
		SG_bool * pbConnected,       /**< True if the provided dagfrag connects
									   to the repo's DAG, false if more nodes are 
									   needed. */
        SG_rbtree ** pprb_missing,   /**< If the insert would fail because the 
                                       DAG is disconnected (pbConnected is SG_FALSE), 
                                       this rbtree will contain a list of nodes
                                       which are missing.  You can use this as
                                       information to grow the fragment deeper
                                       for a retry.  Caller must free this. */

        SG_stringarray ** ppsaNew   /**< If the insert would succeed, this contains
                                       a list of all the IDs of dagnodes which
                                       would be actually inserted.  This is
                                       important because the fragment may have
                                       contained nodes that were already
                                       present in the dag.  Use this list to
                                       constrain the effort required in further
                                       updates.  Typically, after a
                                       store_dagfrag when syncing repo
                                       instances, the next step is to load each
                                       changeset and sync the blobs.  This list
                                       tells you which changesets you need to
                                       actually bother with.  Caller must free
                                       this. */
        );

typedef void FN__sg_repo__store_audit(
        SG_context* pCtx,
		SG_repo * pRepo,
		SG_repo_tx_handle* pTx,
        const char* psz_csid,
        SG_uint64 dagnum,
        const char* psz_userid,
        SG_int64 when
        );

/**
 * This function attempts to insert a dag fragment into
 * the dag.  If it fails, the error code will probably be
 * SG_ERR_CANNOT_CREATE_SPARSE_DAG, which indicates that
 * the fragment cannot be completely connected to nodes
 * already in the dag.  To avoid this, use check_dagfrag
 * BEFORE you begin_tx.  Inside the tx is not the place
 * to be fiddling with frags.
 */
typedef void FN__sg_repo__store_dagfrag(
        SG_context* pCtx,
		SG_repo * pRepo,
		SG_repo_tx_handle* pTx,
        SG_dagfrag * pFrag  /**< Caller no longer owns this */
        );

typedef void FN__sg_repo__done_with_dag(
        SG_context* pCtx,
		SG_repo * pRepo,
		SG_repo_tx_handle* pTx,
        SG_uint64 dagnum
        );

/**
 * This returns the ID of the repo.  This ID is the one which is shared by all
 * instances of that repo.  If you want to create a clone, a new instance of an
 * existing repo, then get this ID, and use it to create a new instance. */
typedef void FN__sg_repo__get_repo_id(
        SG_context* pCtx,
		SG_repo * pRepo, 
        char** ppsz_id  /**< Caller must free */
        );

typedef void FN__sg_repo__get_admin_id(
        SG_context* pCtx,
		SG_repo * pRepo, 
        char** ppsz_id  /**< Caller must free */
        );

/**
 * This function returns the hid of the dagnode exactly matching the provided
 * revision ID. 
 *
 * If no matching revision ID is found, SG_ERR_NOT_FOUND is thrown
 * and ppsz_hid is unaltered.
 */
typedef void FN__sg_repo__find_dagnode_by_rev_id(
        SG_context* pCtx,
		SG_repo * pRepo, 
        SG_uint64 iDagNum, 
        SG_uint32 iRevisionId,
        char** ppsz_hid        /**< Caller must free */
        );

/**
 * This function helps in situations where the user has typed
 * the beginning of an ID and you want to find all the IDs
 * that match.  There are two similar functions for this purpose.
 * This one matches only dagnodes in a given dagnum.  The other one, below,
 * will match any blob ID.
 */
typedef void FN__sg_repo__find_dagnodes_by_prefix(
        SG_context* pCtx,
		SG_repo * pRepo, 
        SG_uint64 iDagNum, 
        const char* psz_hid_prefix,
        SG_rbtree** pprb        /**< Caller must free */
        );

typedef void FN__sg_repo__find_blobs_by_prefix(
        SG_context* pCtx,
		SG_repo * pRepo, 
        const char* psz_hid_prefix, 
        SG_rbtree** pprb        /**< Caller must free */
        );

typedef void FN__sg_repo__list_blobs(
        SG_context* pCtx,
		SG_repo * pRepo, 
        SG_blob_encoding blob_encoding, /**< only return blobs of this encoding */
        SG_uint32 limit,                /**< only return this many entries */
        SG_uint32 offset,               /**< skip the first group of the sorted entries */
        SG_blobset** ppbs
        );

/**
 * Retrieve the lowest common ancestor of a set of dag nodes.
 */
typedef void FN__sg_repo__get_dag_lca(
        SG_context* pCtx,
		SG_repo* pRepo, 
        SG_uint64 iDagNum,
        const SG_rbtree* prbNodes, 
        SG_daglca ** ppDagLca
        );

/**
 * This function will return a list of all the dagnums that
 * actually exist in this repo instance.
 */
typedef void FN__sg_repo__list_dags(
        SG_context* pCtx,
		SG_repo* pRepo, 
        SG_uint32* piCount, 
        SG_uint64** paDagNums       /**< Caller must free */
        );

typedef void FN__sg_repo__rebuild_indexes(
	SG_context* pCtx,
    SG_repo* pRepo
    );

typedef void FN__sg_repo__query_audits(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        SG_int64 min_timestamp,
        SG_int64 max_timestamp,
        const char* psz_userid,
        SG_varray** ppva
        );

typedef void FN__sg_repo__list_audits_for_given_changesets(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        SG_varray* ppva_csid_list,
        SG_varray** ppva
        );

typedef void FN__sg_repo__lookup_audits(
	SG_context*,
    SG_repo* pRepo, 
    SG_uint64 iDagNum,
    const char* psz_csid,
    SG_varray** ppva
	);

#define SG_REPO__MAKE_DELTA_FLAG__ROWIDS  1

typedef void FN__sg_repo__dbndx__make_delta_from_path(
    SG_context* pCtx,
    SG_repo * pRepo,
    SG_uint64 dagnum,
    SG_varray* pva_path,
    SG_uint32 flags,
    SG_vhash* pvh_add,
    SG_vhash* pvh_remove
    );

typedef void FN__sg_repo__dbndx__query_record_history(
	SG_context*,
    SG_repo* pRepo, 
    SG_uint64 iDagNum,
    const char* psz_recid,  // this is NOT the hid of a record.  it is the zing recid
    const char* psz_rectype,
    SG_varray** ppva
	);

typedef void FN__sg_repo__dbndx__query_multiple_record_history(
	SG_context*,
    SG_repo* pRepo, 
    SG_uint64 iDagNum,
    const char* psz_rectype,
    SG_varray* pva_recids,
    const char* psz_field,
    SG_vhash** ppvh
	);

typedef void FN__sg_repo__dbndx__query__one(
	SG_context*,
    SG_repo* pRepo, 
    SG_uint64 iDagNum,
	const char* pidState,          /**< The ID of the changeset representing
	                                 the state of the db on which you want to
	                                 filter.  May not be NULL.  The
	                                 query will filter the results for all
	                                 records that are a member of that state.
	                                 */

    const char* psz_rectype,

    const char* psz_field_name,
    const char* psz_field_value,

    SG_varray* pva_slice_fields,
    SG_vhash** ppvh
    );

typedef void FN__sg_repo__dbndx__query__fts(
	SG_context* pCtx,
    SG_repo* pRepo, 
    SG_uint64 iDagNum,
	const char* pidState,
    const char* psz_rectype,
    const char* psz_field_name,
    const char* psz_keywords,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* pva_fields,
    SG_varray** ppva
	);

typedef void FN__sg_repo__dbndx__query__raw_history(
	SG_context* pCtx,
    SG_repo* pRepo, 
    SG_uint64 iDagNum,
    SG_int64 min_timestamp,
    SG_int64 max_timestamp,
    SG_vhash** ppvh
	);

typedef void FN__sg_repo__dbndx__query__recent(
	SG_context* pCtx,
    SG_repo* pRepo, 
    SG_uint64 iDagNum,
    const char* psz_rectype,
	const SG_varray* pcrit,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* pva_fields,
    SG_varray** ppva
	);

typedef void FN__sg_repo__dbndx__query__prep(
	SG_context*,
    SG_repo* pRepo, 
    SG_uint64 iDagNum,
	const char* pidState          /**< The ID of the changeset representing
	                                 the state of the db on which you want to
	                                 filter.  If you pass NULL here, the query
	                                 will return all records that match the
	                                 criteria, without filtering on any db
	                                 state.  If you pass a changeset id, the
	                                 query will filter the results for all
	                                 records that are a member of that state.
	                                 */
    );

typedef void FN__sg_repo__dbndx__query__count(
	SG_context*,
    SG_repo* pRepo, 
    SG_uint64 iDagNum,
	const char* pidState,          /**< The ID of the changeset representing
	                                 the state of the db on which you want to
	                                 filter.  If you pass NULL here, the query
	                                 will return all records that match the
	                                 criteria, without filtering on any db
	                                 state.  If you pass a changeset id, the
	                                 query will filter the results for all
	                                 records that are a member of that state.
	                                 */

    const char* psz_rectype,

	const SG_varray* pcrit,     /**< The criteria for the query.
									 * If you pass NULL here, the
									 * query will return all
									 * records. */
    SG_uint32* pi_count
    );

typedef void FN__sg_repo__dbndx__query(
	SG_context*,
    SG_repo* pRepo, 
    SG_uint64 iDagNum,
	const char* pidState,          /**< The ID of the changeset representing
	                                 the state of the db on which you want to
	                                 filter.  If you pass NULL here, the query
	                                 will return all records that match the
	                                 criteria, without filtering on any db
	                                 state.  If you pass a changeset id, the
	                                 query will filter the results for all
	                                 records that are a member of that state.
	                                 */

    const char* psz_rectype,

	const SG_varray* pcrit,     /**< The criteria for the query.
									 * If you pass NULL here, the
									 * query will return all
									 * records. */
	const SG_varray* pSort,         /**< The sort for the query. */
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* pva_slice_fields,
    SG_varray** ppva
    );

typedef void FN__sg_repo__treendx__get_paths(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        SG_uint64 iDagNum,
        SG_vhash* pvh_gids,
        SG_vhash** ppvh
        );

typedef void FN__sg_repo__treendx__search_changes(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        SG_uint64 iDagNum,
        SG_stringarray* psa_gids,
        SG_vhash** ppvh
        );

typedef void FN__sg_repo__get_hash_method(
        SG_context* pCtx, 
        SG_repo* pRepo, 
        char** ppsz_result  /* caller must free */
        );

//////////////////////////////////////////////////////////////////
// The REPO VTABLE

struct _sg_repo__vtable
{
	const char * const                                      pszStorage;

	FN__sg_repo__open_repo_instance		        * const		open_repo_instance;
	FN__sg_repo__create_repo_instance		    * const		create_repo_instance;
	FN__sg_repo__delete_repo_instance           * const     delete_repo_instance;
	FN__sg_repo__close_repo_instance		    * const		close_repo_instance;

	FN__sg_repo__begin_tx                       * const		begin_tx;
	FN__sg_repo__commit_tx                      * const		commit_tx;
	FN__sg_repo__abort_tx                       * const		abort_tx;

	FN__sg_repo__store_blob__changeset              * const		store_blob__changeset;
	FN__sg_repo__store_blob__dbrecord              * const		store_blob__dbrecord;
	FN__sg_repo__store_blob__dbtemplate              * const		store_blob__dbtemplate;
	FN__sg_repo__store_blob__begin              * const		store_blob__begin;
	FN__sg_repo__store_blob__chunk              * const		store_blob__chunk;
	FN__sg_repo__store_blob__end                * const		store_blob__end;
	FN__sg_repo__store_blob__abort              * const		store_blob__abort;

	FN__sg_repo__store_audit		            * const		store_audit;

	FN__sg_repo__store_dagfrag		            * const		store_dagfrag;
	FN__sg_repo__done_with_dag		            * const		done_with_dag;

	FN__sg_repo__get_blob                       * const		get_blob;
	FN__sg_repo__release_blob                   * const		release_blob;

	FN__sg_repo__fetch_blob__begin              * const		fetch_blob__begin;
	FN__sg_repo__fetch_blob__chunk              * const		fetch_blob__chunk;
	FN__sg_repo__fetch_blob__end                * const		fetch_blob__end;
	FN__sg_repo__fetch_blob__abort              * const		fetch_blob__abort;

	FN__sg_repo__fetch_repo__fragball           * const		fetch_repo__fragball;

	FN__sg_repo__check_dagfrag		            * const		check_dagfrag;

	FN__sg_repo__fetch_dagnode		            * const		fetch_dagnode;
	FN__sg_repo__find_dag_path          * const		find_dag_path;
	FN__sg_repo__fetch_dagnodes__begin          * const		fetch_dagnodes__begin;
	FN__sg_repo__fetch_dagnodes__one            * const		fetch_dagnodes__one;
	FN__sg_repo__fetch_dagnodes__end            * const		fetch_dagnodes__end;

	FN__sg_repo__find_dagnode_by_rev_id         * const     find_dagnode_by_rev_id;
    FN__sg_repo__find_dagnodes_by_prefix        * const     find_dagnodes_by_prefix;
    FN__sg_repo__find_blobs_by_prefix           * const     find_blobs_by_prefix;
    FN__sg_repo__list_blobs                     * const     list_blobs;

	FN__sg_repo__list_dags		                * const		list_dags;
	FN__sg_repo__fetch_dag_leaves		        * const		fetch_dag_leaves;
	FN__sg_repo__fetch_dagnode_children		    * const		fetch_dagnode_children;
	FN__sg_repo__fetch_chrono_dagnode_list		* const		fetch_chrono_dagnode_list;
	FN__sg_repo__fetch_dagnode_ids		* const		fetch_dagnode_ids;
	FN__sg_repo__find_new_dagnodes_since		* const		find_new_dagnodes_since;

    FN__sg_repo__get_dag_lca                    * const     get_dag_lca;
	FN__sg_repo__get_repo_id	                * const		get_repo_id;
	FN__sg_repo__get_admin_id	                * const		get_admin_id;

	FN__sg_repo__query_implementation           * const		query_implementation;
	FN__sg_repo__query_blob_existence			* const		query_blob_existence;
    
	FN__sg_repo__rebuild_indexes                 * const		rebuild_indexes;

	FN__sg_repo__dbndx__make_delta_from_path                  * const		dbndx__make_delta_from_path;
	FN__sg_repo__dbndx__query                  * const		dbndx__query;
	FN__sg_repo__dbndx__query__prep             * const		dbndx__query__prep;
	FN__sg_repo__dbndx__query__count             * const		dbndx__query__count;
	FN__sg_repo__dbndx__query__one             * const		dbndx__query__one;
	FN__sg_repo__dbndx__query__fts             * const		dbndx__query__fts;
	FN__sg_repo__dbndx__query__recent             * const		dbndx__query__recent;
	FN__sg_repo__dbndx__query__raw_history             * const		dbndx__query__raw_history;
	FN__sg_repo__dbndx__query_record_history    * const		dbndx__query_record_history;
	FN__sg_repo__dbndx__query_multiple_record_history    * const		dbndx__query_multiple_record_history;
	FN__sg_repo__lookup_audits           * const		lookup_audits;
	FN__sg_repo__query_audits           * const		query_audits;
	FN__sg_repo__list_audits_for_given_changesets           * const		list_audits_for_given_changesets;
    
	FN__sg_repo__treendx__get_paths   * const		treendx__get_paths;
	FN__sg_repo__treendx__search_changes         * const		treendx__search_changes;

	FN__sg_repo__get_hash_method                * const		get_hash_method;
	FN__sg_repo__hash__begin                    * const		hash__begin;
	FN__sg_repo__hash__chunk                    * const		hash__chunk;
	FN__sg_repo__hash__end                      * const		hash__end;
	FN__sg_repo__hash__abort                    * const		hash__abort;
	FN__sg_repo__verify__dbndx_states                * const		verify__dbndx_states;
	FN__sg_repo__verify__dag_consistency                * const		verify__dag_consistency;

};

typedef struct _sg_repo__vtable sg_repo__vtable;

//////////////////////////////////////////////////////////////////
// Convenience macro to declare a prototype for each function in the
// REPO VTABLE for a specific implementation.

#define DCL__REPO_VTABLE_PROTOTYPES(name)												            \
	FN__sg_repo__open_repo_instance			    sg_repo__##name##__open_repo_instance; 	        	\
	FN__sg_repo__create_repo_instance			sg_repo__##name##__create_repo_instance; 	        \
    FN__sg_repo__delete_repo_instance           sg_repo__##name##__delete_repo_instance;            \
	FN__sg_repo__close_repo_instance			sg_repo__##name##__close_repo_instance;		        \
	FN__sg_repo__begin_tx						sg_repo__##name##__begin_tx;                        \
	FN__sg_repo__commit_tx						sg_repo__##name##__commit_tx;                       \
	FN__sg_repo__abort_tx						sg_repo__##name##__abort_tx;                        \
	FN__sg_repo__store_blob__changeset              sg_repo__##name##__store_blob__changeset;               \
	FN__sg_repo__store_blob__dbrecord              sg_repo__##name##__store_blob__dbrecord;               \
	FN__sg_repo__store_blob__dbtemplate              sg_repo__##name##__store_blob__dbtemplate;               \
	FN__sg_repo__store_blob__begin              sg_repo__##name##__store_blob__begin;               \
	FN__sg_repo__store_blob__chunk              sg_repo__##name##__store_blob__chunk;               \
	FN__sg_repo__store_blob__end                sg_repo__##name##__store_blob__end;                 \
	FN__sg_repo__store_blob__abort              sg_repo__##name##__store_blob__abort;               \
	FN__sg_repo__store_audit			        sg_repo__##name##__store_audit;		            \
	FN__sg_repo__store_dagfrag			        sg_repo__##name##__store_dagfrag;		            \
	FN__sg_repo__done_with_dag			        sg_repo__##name##__done_with_dag;		            \
	FN__sg_repo__get_blob                       sg_repo__##name##__get_blob;                        \
	FN__sg_repo__release_blob                   sg_repo__##name##__release_blob;                    \
	FN__sg_repo__fetch_blob__begin              sg_repo__##name##__fetch_blob__begin;               \
	FN__sg_repo__fetch_blob__chunk              sg_repo__##name##__fetch_blob__chunk;               \
	FN__sg_repo__fetch_blob__end                sg_repo__##name##__fetch_blob__end;                 \
	FN__sg_repo__fetch_blob__abort              sg_repo__##name##__fetch_blob__abort;               \
	FN__sg_repo__fetch_repo__fragball           sg_repo__##name##__fetch_repo__fragball;            \
	FN__sg_repo__check_dagfrag			        sg_repo__##name##__check_dagfrag;		            \
	FN__sg_repo__fetch_dagnode			        sg_repo__##name##__fetch_dagnode;		            \
	FN__sg_repo__find_dag_path			sg_repo__##name##__find_dag_path;			\
	FN__sg_repo__fetch_dagnodes__begin			sg_repo__##name##__fetch_dagnodes__begin;			\
	FN__sg_repo__fetch_dagnodes__one			sg_repo__##name##__fetch_dagnodes__one;				\
	FN__sg_repo__fetch_dagnodes__end			sg_repo__##name##__fetch_dagnodes__end;				\
	FN__sg_repo__find_dagnode_by_rev_id		    sg_repo__##name##__find_dagnode_by_rev_id;	        \
	FN__sg_repo__find_dagnodes_by_prefix        sg_repo__##name##__find_dagnodes_by_prefix;	        \
	FN__sg_repo__find_blobs_by_prefix		    sg_repo__##name##__find_blobs_by_prefix;	        \
	FN__sg_repo__list_blobs                     sg_repo__##name##__list_blobs;	                    \
	FN__sg_repo__list_dags			            sg_repo__##name##__list_dags;		                \
	FN__sg_repo__fetch_dag_leaves			    sg_repo__##name##__fetch_dag_leaves;		        \
	FN__sg_repo__fetch_dagnode_children         sg_repo__##name##__fetch_dagnode_children;          \
	FN__sg_repo__fetch_chrono_dagnode_list		sg_repo__##name##__fetch_chrono_dagnode_list;		\
	FN__sg_repo__fetch_dagnode_ids		sg_repo__##name##__fetch_dagnode_ids;		\
	FN__sg_repo__find_new_dagnodes_since		sg_repo__##name##__find_new_dagnodes_since;		\
	FN__sg_repo__get_dag_lca			        sg_repo__##name##__get_dag_lca;	    	            \
	FN__sg_repo__get_repo_id	        	    sg_repo__##name##__get_repo_id;                     \
	FN__sg_repo__get_admin_id	   	            sg_repo__##name##__get_admin_id;                    \
	FN__sg_repo__query_implementation           sg_repo__##name##__query_implementation;            \
	FN__sg_repo__query_blob_existence           sg_repo__##name##__query_blob_existence;            \
	FN__sg_repo__rebuild_indexes                sg_repo__##name##__rebuild_indexes;					\
	FN__sg_repo__dbndx__make_delta_from_path					sg_repo__##name##__dbndx__make_delta_from_path;                    \
	FN__sg_repo__dbndx__query					sg_repo__##name##__dbndx__query;                    \
	FN__sg_repo__dbndx__query__prep				sg_repo__##name##__dbndx__query__prep;               \
	FN__sg_repo__dbndx__query__count				sg_repo__##name##__dbndx__query__count;               \
	FN__sg_repo__dbndx__query__one				sg_repo__##name##__dbndx__query__one;               \
	FN__sg_repo__dbndx__query__fts				sg_repo__##name##__dbndx__query__fts;               \
	FN__sg_repo__dbndx__query__recent			sg_repo__##name##__dbndx__query__recent;            \
	FN__sg_repo__dbndx__query__raw_history			sg_repo__##name##__dbndx__query__raw_history;            \
	FN__sg_repo__dbndx__query_record_history    sg_repo__##name##__dbndx__query_record_history;     \
	FN__sg_repo__dbndx__query_multiple_record_history    sg_repo__##name##__dbndx__query_multiple_record_history;     \
	FN__sg_repo__lookup_audits           sg_repo__##name##__lookup_audits;			\
	FN__sg_repo__query_audits           sg_repo__##name##__query_audits;			\
	FN__sg_repo__list_audits_for_given_changesets           sg_repo__##name##__list_audits_for_given_changesets;			\
	FN__sg_repo__treendx__get_paths   sg_repo__##name##__treendx__get_paths;    \
	FN__sg_repo__treendx__search_changes        sg_repo__##name##__treendx__search_changes;         \
	FN__sg_repo__get_hash_method                sg_repo__##name##__get_hash_method;                 \
	FN__sg_repo__hash__begin                    sg_repo__##name##__hash__begin;                     \
	FN__sg_repo__hash__chunk                    sg_repo__##name##__hash__chunk;                     \
	FN__sg_repo__hash__end                      sg_repo__##name##__hash__end;                       \
	FN__sg_repo__hash__abort                    sg_repo__##name##__hash__abort;                     \
	FN__sg_repo__verify__dbndx_states                sg_repo__##name##__verify__dbndx_states;       \
	FN__sg_repo__verify__dag_consistency                sg_repo__##name##__verify__dag_consistency;
	
// Convenience macro to declare a properly initialized static
// REPO VTABLE for a specific implementation.

#define DCL__REPO_VTABLE(storage,name)						\
	static sg_repo__vtable s_repo_vtable__##name =  		\
	{	storage,											\
		sg_repo__##name##__open_repo_instance,			    \
		sg_repo__##name##__create_repo_instance,			\
        sg_repo__##name##__delete_repo_instance,            \
		sg_repo__##name##__close_repo_instance,				\
		sg_repo__##name##__begin_tx,						\
		sg_repo__##name##__commit_tx,						\
		sg_repo__##name##__abort_tx,						\
		sg_repo__##name##__store_blob__changeset,               \
		sg_repo__##name##__store_blob__dbrecord,               \
		sg_repo__##name##__store_blob__dbtemplate,               \
		sg_repo__##name##__store_blob__begin,               \
		sg_repo__##name##__store_blob__chunk,               \
		sg_repo__##name##__store_blob__end,                 \
		sg_repo__##name##__store_blob__abort,               \
		sg_repo__##name##__store_audit, 					\
		sg_repo__##name##__store_dagfrag,					\
		sg_repo__##name##__done_with_dag,					\
		sg_repo__##name##__get_blob,						\
		sg_repo__##name##__release_blob,					\
		sg_repo__##name##__fetch_blob__begin,               \
		sg_repo__##name##__fetch_blob__chunk,               \
		sg_repo__##name##__fetch_blob__end,                 \
		sg_repo__##name##__fetch_blob__abort,               \
		sg_repo__##name##__fetch_repo__fragball,            \
		sg_repo__##name##__check_dagfrag,					\
		sg_repo__##name##__fetch_dagnode,				    \
		sg_repo__##name##__find_dag_path,		    \
		sg_repo__##name##__fetch_dagnodes__begin,		    \
		sg_repo__##name##__fetch_dagnodes__one,			    \
		sg_repo__##name##__fetch_dagnodes__end,			    \
		sg_repo__##name##__find_dagnode_by_rev_id,          \
		sg_repo__##name##__find_dagnodes_by_prefix,	        \
		sg_repo__##name##__find_blobs_by_prefix,   	        \
		sg_repo__##name##__list_blobs,          	        \
		sg_repo__##name##__list_dags,				        \
		sg_repo__##name##__fetch_dag_leaves,				\
		sg_repo__##name##__fetch_dagnode_children,			\
		sg_repo__##name##__fetch_chrono_dagnode_list,		\
		sg_repo__##name##__fetch_dagnode_ids,		\
		sg_repo__##name##__find_new_dagnodes_since,		\
		sg_repo__##name##__get_dag_lca,	    				\
		sg_repo__##name##__get_repo_id,     		        \
		sg_repo__##name##__get_admin_id,   			        \
        sg_repo__##name##__query_implementation,            \
        sg_repo__##name##__query_blob_existence,            \
        sg_repo__##name##__rebuild_indexes,                 \
        sg_repo__##name##__dbndx__make_delta_from_path,                   \
        sg_repo__##name##__dbndx__query,                   \
        sg_repo__##name##__dbndx__query__prep,              \
        sg_repo__##name##__dbndx__query__count,              \
        sg_repo__##name##__dbndx__query__one,              \
        sg_repo__##name##__dbndx__query__fts,              \
        sg_repo__##name##__dbndx__query__recent,              \
        sg_repo__##name##__dbndx__query__raw_history,              \
        sg_repo__##name##__dbndx__query_record_history,     \
        sg_repo__##name##__dbndx__query_multiple_record_history,     \
        sg_repo__##name##__lookup_audits,            \
        sg_repo__##name##__query_audits,            \
        sg_repo__##name##__list_audits_for_given_changesets,            \
        sg_repo__##name##__treendx__get_paths,    \
        sg_repo__##name##__treendx__search_changes,          \
        sg_repo__##name##__get_hash_method,                 \
        sg_repo__##name##__hash__begin,                     \
        sg_repo__##name##__hash__chunk,                     \
        sg_repo__##name##__hash__end,                       \
        sg_repo__##name##__hash__abort,                     \
        sg_repo__##name##__verify__dbndx_states,                 \
        sg_repo__##name##__verify__dag_consistency,                 \
	}

END_EXTERN_C;

#endif//H_SG_REPO_API_H

