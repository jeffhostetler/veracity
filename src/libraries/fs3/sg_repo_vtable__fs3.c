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

/*
 *
 * @file sg_repo_vtable__fs3.c
 *
 */

#include "sg_fs3__private.h"

#include <zlib.h>

// TODO temporary hack:
#ifdef WINDOWS
#pragma warning(disable:4706)
#endif

// TODO unify the _BUSY error codes.  too many.

// TODO
//
// put timestamps and/or pids in lock files.  find a good place
// to delete stale locks.
//
// fix mem leaks on abort of concurrency test.
//


#define MY_SLEEP_MS      100
#define MY_TIMEOUT_MS  16000

#define SG_RETRY_THINGIE(expr) \
SG_STATEMENT( \
  SG_uint32 slept = 0; \
  while(1) \
  { \
      SG_ASSERT(!pData->b_in_sqlite_transaction); \
      SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pData->psql, "BEGIN TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  ); \
      pData->b_in_sqlite_transaction = SG_TRUE; \
      expr; \
      if ( \
          SG_context__err_equals(pCtx, SG_ERR_DB_BUSY) \
          || SG_context__err_equals(pCtx, SG_ERR_SQLITE(SQLITE_BUSY)) \
          ) \
      { \
          SG_context__err_reset(pCtx); \
          SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pData->psql, "ROLLBACK TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  ); \
          pData->b_in_sqlite_transaction = SG_FALSE; \
          if (slept > MY_TIMEOUT_MS) \
          { \
              SG_ERR_THROW(  SG_ERR_DB_BUSY  ); \
          } \
          SG_sleep_ms(MY_SLEEP_MS); \
          slept += MY_SLEEP_MS; \
      } \
      else if (SG_CONTEXT__HAS_ERR(pCtx)) \
      { \
          SG_ERR_IGNORE(  sg_sqlite__exec__retry(pCtx, pData->psql, "ROLLBACK TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );\
          pData->b_in_sqlite_transaction = SG_FALSE; \
          SG_ERR_CHECK_CURRENT; \
      } \
      else \
      { \
          SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pData->psql, "COMMIT TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );\
          pData->b_in_sqlite_transaction = SG_FALSE; \
          break; \
      } \
  } \
)

//////////////////////////////////////////////////////////////////

typedef struct _sg_blob_fs3_handle_store sg_blob_fs3_handle_store;
typedef struct _sg_blob_fs3_handle_fetch sg_blob_fs3_handle_fetch;

struct _my_tx_data
{
    SG_uint32 flags;
    SG_rbtree* prb_append_locks;
    SG_rbtree* prb_file_handles;
    SG_rbtree* prb_frags;
    sqlite3* psql_new_audits;
    sqlite3_stmt* pStmt_audits;
    SG_blobset* pbs_new_blobs;
	sg_blob_fs3_handle_store* pBlobStoreHandle;

    struct
    {
        SG_rbtree* prb_dbndx_update;
        SG_vhash* pvh_templates;
        sqlite3_stmt* pStmt_insert;
    } cloning;
};
typedef struct _my_tx_data my_tx_data;

/**
 * we get one pointer in the SG_repo for our instance data.  in SG_repo
 * this is an opaque "sg_repo__vtable__instance_data *".  we cast it
 * to the following definition.
 */
struct _my_instance_data
{
    SG_repo*					pRepo;				// we DO NOT own this (but rather we are bound to it)
	SG_pathname *				pPathParentDir;
	SG_pathname *				pPathMyDir;

    char						buf_hash_method[SG_REPO_HASH_METHOD__MAX_BUFFER_LENGTH];
    char						buf_repo_id[SG_GID_BUFFER_LENGTH];
    char						buf_admin_id[SG_GID_BUFFER_LENGTH];

    sqlite3*					psql;

    SG_bool                     b_in_sqlite_transaction;

	SG_uint32					strlen_hashes;

    SG_rbtree*                  prb_paths;
    SG_rbtree*                  prb_sql;

    SG_bool b_new_audits;

    my_tx_data* ptx;
};
typedef struct _my_instance_data my_instance_data;

#define MY_CHUNK_SIZE			(16*1024)

struct _sg_blob_fs3_handle_fetch
{
	my_instance_data *			pData;

    char sz_hid_requested[SG_HID_MAX_BUFFER_LENGTH];
	SG_uint64					len_encoded_stored;		// we need to make sure whatever we read matches this
	SG_uint64					len_encoded_observed;	// value calculated as we read/uncompress/expand blob content into Raw Data
	SG_uint64					len_full_observed;	// value calculated as we read/uncompress/expand blob content into Raw Data
	SG_uint64					len_full_stored;
    SG_blob_encoding            blob_encoding_stored;
    SG_blob_encoding            blob_encoding_returning;
    char*                       psz_hid_vcdiff_reference_stored_freeme;
	SG_file *					m_pFileBlob;
	SG_repo_hash_handle *		pRHH_VerifyOnFetch;
    SG_uint32                   filenumber;
    SG_uint64                   offset;

    /* vcdiff stuff */
    SG_bool                     b_undeltifying;
    SG_vcdiff_undeltify_state* pst;
    sg_blob_fs3_handle_fetch* pbh_delta;
    SG_readstream* pstrm_delta;
    SG_seekreader* psr_reference;
    SG_pathname* pPath_tempfile_vcdiff_reference;
    SG_byte* p_buf;
    SG_uint32 count;
    SG_uint32 next;

    /* zlib stuff */
    SG_bool                     b_uncompressing;
	z_stream zStream;
	SG_byte bufCompressed[MY_CHUNK_SIZE];

	SG_bool						b_we_own_file; // When closing the handle, should we close the file?
};

struct _sg_blob_fs3_handle_store
{
    SG_uint32 filenumber;
    SG_uint64 offset;
    SG_file* pFileBlob;

    my_instance_data* pData;
    SG_blob_encoding blob_encoding_given;
    SG_blob_encoding blob_encoding_storing;
    const char* psz_hid_vcdiff_reference;
    SG_uint64 len_full_given;
    SG_uint64 len_full_observed;
    SG_uint64 len_encoded_observed;
    const char* psz_hid_known;
    const char* psz_hid_blob_final;
	SG_repo_hash_handle * pRHH_ComputeOnStore;

    /* zlib stuff */
    SG_bool b_compressing;
	z_stream zStream;
	SG_byte bufOut[MY_CHUNK_SIZE];
};

void sg_blob_fs3__fetch_blob__begin(
    SG_context * pCtx,
    my_instance_data* pData,
    const char* psz_hid_blob,
    SG_bool b_convert_to_full,
    SG_blob_encoding* pBlobFormat,
    char** ppsz_hid_vcdiff_reference,
    SG_uint64* pLenRawData,
    SG_uint64* pLenFull,
	SG_bool b_open_file,
    sg_blob_fs3_handle_fetch** ppHandle
    );

void sg_blob_fs3__fetch_blob__chunk(
    SG_context * pCtx,
    sg_blob_fs3_handle_fetch* ppHandle,
    SG_uint32 len_buf,
    SG_byte* p_buf,
    SG_uint32* p_len_got,
    SG_bool* pb_done
    );

static void fs3__get_sql(
	SG_context* pCtx,
    my_instance_data* pData,
    SG_pathname* pPath,
    SG_uint32 synchro,
    sqlite3** ppsql
    );

void sg_blob_fs3__fetch_blob__end(
    SG_context * pCtx,
    sg_blob_fs3_handle_fetch** ppbh
    );

void sg_blob_fs3__fetch_blob__abort(
    SG_context * pCtx,
    sg_blob_fs3_handle_fetch** ppbh
    );

void sg_fs3__begin_tx(
    SG_context * pCtx,
    my_instance_data* pData,
    SG_uint32 flags,
    my_tx_data** pptx
    );

static void sg_fs3__get_dbndx_path(
    SG_context * pCtx,
    my_instance_data* pData,
    SG_uint64 iDagNum,
    SG_pathname** ppResult
    )
{
    char buf[SG_DAGNUM__BUF_MAX__HEX + 10];
    SG_pathname* pPath = NULL;

    SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, iDagNum, buf, sizeof(buf))  );
    SG_ERR_CHECK(  SG_strcat(pCtx, buf, sizeof(buf), ".dbndx")  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pData->pPathMyDir, buf)  );

    *ppResult = pPath;

fail:
    ;
}

static void sg_fs3__get_treendx_path(
    SG_context * pCtx,
    my_instance_data* pData,
    SG_uint64 iDagNum,
    SG_pathname** ppResult
    )
{
    char buf[SG_DAGNUM__BUF_MAX__HEX + 10];
    SG_pathname* pPath = NULL;

    SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, iDagNum, buf, sizeof(buf))  );
    SG_ERR_CHECK(  SG_strcat(pCtx, buf, sizeof(buf), ".treendx")  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pData->pPathMyDir, buf)  );

    *ppResult = pPath;

fail:
    ;
}

void sg_fs3__commit_tx(
	SG_context * pCtx,
	my_instance_data* pData,
	my_tx_data** pptx
	);


//////////////////////////////////////////////////////////////////

// In FS3, the blob data is stored in a series of "Number Files".
// Each can independently grow to ~1 Gb.  Each is named using the
// following sprintf format spec: "%06d".
//
// Note: this could probably be 7 or 8, but I pulled this value from
// Note: various stack variables.

#define sg_FILENUMBER_BUFFER_LENGTH				32

// In FS3, various lock files are used on each of the "Number Files".
// For most locks, we just have one, such as "%06d_append".

#define sg_MAX_LOCKFILE_TYPE_LENGTH				10			// { "append" }
#define sg_MAX_LOCKFILE_LENGTH_RANDOM			32			// only needed for "read_<tid>".  this gives us one UUID's worth of random digits.
#define sg_MAX_LOCKFILE_SUFFIX_LENGTH			(sg_MAX_LOCKFILE_TYPE_LENGTH + sg_MAX_LOCKFILE_LENGTH_RANDOM)
#define sg_MAX_LOCKFILE_BUFFER_LENGTH			(sg_FILENUMBER_BUFFER_LENGTH + 1 + sg_MAX_LOCKFILE_SUFFIX_LENGTH)

static void sg_fs3__lock(
        SG_context * pCtx,
        my_instance_data* pData,
        const char* psz_filenumber,
        SG_pathname** ppLockFile
        );

static void my_fetch_info(
        SG_context * pCtx,
        sqlite3* psql,
        const char * szHidBlob,
        SG_bool* pb_found,
        SG_uint32* p_filenumber,
        SG_uint64* p_offset,
        SG_blob_encoding* p_blob_encoding,
        SG_uint64* p_len_encoded,
        SG_uint64* p_len_full,
        char** ppsz_hid_vcdiff_reference
        )
{
	sqlite3_stmt * pStmt = NULL;
    int rc;
    SG_blob_encoding blob_encoding = 0;
    SG_uint64 len_encoded = 0;
    SG_uint64 len_full = 0;
    const char * psz_hid_vcdiff_reference = NULL;
    SG_uint32 filenumber = 0;
    SG_uint64 offset = 0;

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt,
									  "SELECT \"encoding\", \"len_encoded\", \"len_full\", \"hid_vcdiff\", \"filename\", \"offset\" FROM \"blobs\" WHERE \"hid\" = ?")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt,1,szHidBlob)  );

    rc = sqlite3_step(pStmt);
    if (SQLITE_ROW == rc)
    {
        blob_encoding = (SG_blob_encoding) sqlite3_column_int(pStmt, 0);

        len_encoded = sqlite3_column_int64(pStmt, 1);
        len_full = sqlite3_column_int64(pStmt, 2);

        if (SG_IS_BLOBENCODING_FULL(blob_encoding))
        {
            SG_ASSERT(len_encoded == len_full);
        }

        psz_hid_vcdiff_reference = (const char *)sqlite3_column_text(pStmt,3);

        if (ppsz_hid_vcdiff_reference)
        {
            if (psz_hid_vcdiff_reference)
            {
                SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_hid_vcdiff_reference, ppsz_hid_vcdiff_reference)  );
            }
            else
            {
                *ppsz_hid_vcdiff_reference = NULL;
            }
        }

        filenumber = sqlite3_column_int(pStmt,4); // will convert the string to an int
        offset = sqlite3_column_int64(pStmt, 5);

        SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );

        SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

        if (p_blob_encoding)
        {
            *p_blob_encoding = blob_encoding;
        }
        if (p_len_encoded)
        {
            *p_len_encoded = len_encoded;
        }
        if (p_len_full)
        {
            *p_len_full = len_full;
        }
        if (p_filenumber)
        {
            *p_filenumber = filenumber;
        }
        if (p_offset)
        {
            *p_offset = offset;
        }

        *pb_found = SG_TRUE;
    }
    else if (SQLITE_DONE == rc)
    {
        *pb_found = SG_FALSE;
        SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
    }
    else
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

static void do_fetch_info(
        SG_context * pCtx,
        my_instance_data* pData,
        const char * szHidBlob,
        SG_uint32* p_filenumber,
        SG_uint64* p_offset,
        SG_blob_encoding* p_blob_encoding,
        SG_uint64* p_len_encoded,
        SG_uint64* p_len_full,
        char** ppsz_hid_vcdiff_reference
        )
{
    SG_bool b_found = SG_FALSE;
    char* psz_filename = NULL;

    if (pData->ptx && pData->ptx->pbs_new_blobs)
    {
        SG_bool b_inserting = SG_FALSE;

        SG_ERR_CHECK(  SG_blobset__inserting(pCtx, pData->ptx->pbs_new_blobs, &b_inserting)  );
        if (!b_inserting)
        {
            SG_ERR_CHECK(  SG_blobset__lookup(
                        pCtx, 
                        pData->ptx->pbs_new_blobs, 
                        szHidBlob, 
                        &b_found, 
                        &psz_filename, 
                        p_offset, 
                        p_blob_encoding, 
                        p_len_encoded, 
                        p_len_full, 
                        ppsz_hid_vcdiff_reference
                        )  );
            if (b_found)
            {
                *p_filenumber = (SG_uint32) atoi(psz_filename); // TODO atoi?
                SG_NULLFREE(pCtx, psz_filename);
            }
        }
    }

    if (!b_found)
    {
        if (!pData->b_in_sqlite_transaction)
        {
            SG_RETRY_THINGIE(
            SG_ERR_CHECK(  my_fetch_info(pCtx, pData->psql, szHidBlob, &b_found, p_filenumber, p_offset, p_blob_encoding, p_len_encoded, p_len_full, ppsz_hid_vcdiff_reference)  );
                );
        }
        else
        {
            SG_ERR_CHECK(  my_fetch_info(pCtx, pData->psql, szHidBlob, &b_found, p_filenumber, p_offset, p_blob_encoding, p_len_encoded, p_len_full, ppsz_hid_vcdiff_reference)  );
        }
    }

    if (!b_found)
    {
        SG_ERR_THROW2(SG_ERR_BLOB_NOT_FOUND, (pCtx, "%s", szHidBlob));
    }

fail:
    SG_NULLFREE(pCtx, psz_filename);
}

void sg_blob_fs3_handle_store__free(SG_context* pCtx, sg_blob_fs3_handle_store* pbh)
{
    if (!pbh)
    {
        return;
    }

	deflateEnd(&pbh->zStream);

    if (pbh->pRHH_ComputeOnStore)
	{
		// if this handle is being freed with an active repo_hash_handle, we destroy it too.
		// if everything went as expected, our caller should have finalized the hash and
		// gotten the computed HID and already freed the repo_hash_handle.
		SG_ERR_IGNORE(  sg_repo__fs3__hash__abort(pCtx, pbh->pData->pRepo, &pbh->pRHH_ComputeOnStore)  );
	}

#if 0 // write handles are now closed elsewhere
    SG_FILE_NULLCLOSE(pCtx, pbh->pFileBlob);
#endif
    SG_NULLFREE(pCtx, pbh->psz_hid_blob_final);
    SG_NULLFREE(pCtx, pbh);
}

#define sg_FS3_MAX_FILE_LENGTH (1024*1024*1024)

static void sg_fs3__get_filenumber_path__sz(SG_context * pCtx, my_instance_data* pData, const char* psz_filenumber, SG_pathname** ppPath)
{
    SG_pathname* pPath = NULL;
    SG_pathname* pPath_freeme = NULL;
    SG_bool b_already = SG_FALSE;

    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pData->prb_paths, psz_filenumber, &b_already, (void**) &pPath)  );
    if (!pPath)
    {
        SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_freeme, pData->pPathMyDir, psz_filenumber)  );
        SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pData->prb_paths, psz_filenumber, pPath_freeme)  );
        pPath = pPath_freeme;
        pPath_freeme = NULL;
    }

    *ppPath = pPath;
    pPath = NULL;

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath_freeme);
}

static void sg_fs3__filenumber_to_filename(SG_context * pCtx, char* buf, SG_uint32 bufsize, SG_uint32 filenumber)
{
	SG_ASSERT(  (bufsize >= sg_FILENUMBER_BUFFER_LENGTH)  );
    // TODO sprintf is too heavy for this

    SG_ERR_CHECK_RETURN(  SG_sprintf(pCtx, buf, bufsize, "%06d", filenumber)  );
}

static void sg_fs3__get_filenumber_path__uint32(SG_context * pCtx, my_instance_data* pData, SG_uint32 filenumber, SG_pathname** ppPath)
{
    char buf[sg_FILENUMBER_BUFFER_LENGTH];

    SG_ERR_CHECK_RETURN(  sg_fs3__filenumber_to_filename(pCtx, buf, sizeof(buf), filenumber)  );
    SG_ERR_CHECK_RETURN(  sg_fs3__get_filenumber_path__sz(pCtx, pData, buf, ppPath)  );
}

static void sg_fs3__find_a_place__already_locked(
    SG_context * pCtx,
    my_instance_data* pData,
    SG_uint64 space,
    SG_uint32* p_filenumber
    )
{
    SG_pathname* pPath_file = NULL;
    SG_uint64 len = 0;
    SG_uint32 result_filenumber = 0;
    const char* psz_filenumber = NULL;
    SG_bool b = SG_FALSE;
    SG_rbtree_iterator* pit = NULL;

    SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, pData->ptx->prb_append_locks, &b, &psz_filenumber, NULL)  );
    while (b)
    {
        SG_ERR_CHECK(  sg_fs3__get_filenumber_path__sz(pCtx, pData, psz_filenumber, &pPath_file)  );

        /* Now see if there is room to append this blob.  Note that
         * this check only really applies if the file exists and
         * has non-zero length.  Every blob has to go somewhere,
         * regardless of its length, so if a file is empty or
         * does not exist yet, we allow the blob to go in there. */

        SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath_file, &len, NULL)  );
        if (
                len
                && ((len + space) > sg_FS3_MAX_FILE_LENGTH)
           )
        {
            goto nope;
        }

        /* this file is okay. */
        result_filenumber = atoi(psz_filenumber); // TODO atoi?

        break;

nope:
        SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_filenumber, NULL)  );
    }

    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);

    *p_filenumber = result_filenumber;

    return;

fail:
    ;
}

static void sg_fs3__get_lock_pathname(
        SG_context * pCtx,
        my_instance_data* pData,
        const char* psz_filenumber,
        const char* psz_suffix,
        SG_pathname** ppPath
        )
{
    SG_pathname* pPath_lock = NULL;
    char buf_lock[sg_MAX_LOCKFILE_BUFFER_LENGTH];

	SG_NULLARGCHECK_RETURN(psz_filenumber);
	SG_NULLARGCHECK_RETURN(psz_suffix);
	SG_NULLARGCHECK_RETURN(ppPath);

    SG_ERR_CHECK(  SG_strcpy(pCtx, buf_lock, sizeof(buf_lock), psz_filenumber)  );
    SG_ERR_CHECK(  SG_strcat(pCtx, buf_lock, sizeof(buf_lock), "_")  );
    SG_ERR_CHECK(  SG_strcat(pCtx, buf_lock, sizeof(buf_lock), psz_suffix)  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_lock, pData->pPathMyDir, buf_lock)  );

    *ppPath = pPath_lock;
    pPath_lock = NULL;

    return;

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath_lock);
}

static void sg_fs3__create_lockish_file(
        SG_context * pCtx,
        SG_pathname* pPath_lock,
        SG_bool* pb
        )
{
    SG_file* pFile = NULL;

    SG_file__open__pathname(pCtx,pPath_lock,SG_FILE_WRONLY|SG_FILE_CREATE_NEW,0644,&pFile);
    SG_FILE_NULLCLOSE(pCtx, pFile);

    /* TODO should we check specifically for the error wherein the lock
     * file already exists?  Or can we just continue the loop on any error
     * at all?  If we're going to check for the error specifically, we
     * probably need SG_file to return a special error code for this
     * case, rather than returning something errno/win32 wrapped. */
    if (SG_CONTEXT__HAS_ERR(pCtx))
    {
        *pb = SG_FALSE;
		SG_context__err_reset(pCtx);
    }
    else
    {
        *pb = SG_TRUE;
    }
}

static void sg_fs3__lock(
        SG_context * pCtx,
        my_instance_data* pData,
        const char* psz_filenumber,
        SG_pathname** ppLockFile
        )
{
    SG_pathname* pPath_append = NULL;
    SG_pathname* pPath_lock_result = NULL;
    SG_bool b = SG_FALSE;

	SG_NULLARGCHECK_RETURN(psz_filenumber);

    SG_ERR_CHECK(  sg_fs3__get_lock_pathname(pCtx, pData, psz_filenumber, "append", &pPath_append)  );

    b = SG_FALSE;
    SG_ERR_CHECK(  sg_fs3__create_lockish_file(pCtx, pPath_append, &b)  );
    if (b)
    {
        pPath_lock_result = pPath_append;
        pPath_append = NULL;
    }

    if (ppLockFile)
    {
        *ppLockFile = pPath_lock_result;
        pPath_lock_result = NULL;
    }

    // fall thru

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath_lock_result);
    SG_PATHNAME_NULLFREE(pCtx, pPath_append);

    return;
}

static void sg_fs3__find_a_place__new_lock(
    SG_context * pCtx,
    my_instance_data* pData,
    SG_uint64 space,
    SG_uint32* p_filenumber
    )
{
    SG_pathname* pPath_file = NULL;
    char buf_file[sg_FILENUMBER_BUFFER_LENGTH];
    SG_file* pFile = NULL;
    SG_uint64 len = 0;
    SG_bool bExists;
    SG_pathname* pPath_lock = NULL;
    SG_uint32 filenumber = 0;

    /* Look through all the files and find one which has enough space for the proposed blob. */
    filenumber = 0;
    while (1)
    {
        SG_bool bAlreadyLocked = SG_FALSE;

        filenumber++;
        SG_ERR_CHECK(  sg_fs3__filenumber_to_filename(pCtx, buf_file, sizeof(buf_file), filenumber)  );

        /* skip anything where we already have the lock */
        SG_ERR_CHECK(  SG_rbtree__find(pCtx, pData->ptx->prb_append_locks, buf_file, &bAlreadyLocked, NULL)  );
        if (bAlreadyLocked)
        {
            continue;
        }

        SG_ERR_CHECK(  sg_fs3__get_filenumber_path__sz(pCtx, pData, buf_file, &pPath_file)  );

        bExists = SG_FALSE;
        SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_file, &bExists, NULL, NULL)  );
        if (bExists)
        {
            /* Now see if there is room to append this blob.  Note that
             * this check only really applies if the file exists and
             * has non-zero length.  Every blob has to go somewhere,
             * regardless of its length, so if a file is empty or
             * does not exist yet, we allow the blob to go in there. */

            SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath_file, &len, NULL)  );
            if (
                    len
                    && ((len + space) > sg_FS3_MAX_FILE_LENGTH)
               )
            {
                continue;
            }
        }

        /* OK, this is our file as long as we can get a lock on it */

        pPath_lock = NULL;
        SG_ERR_CHECK(  sg_fs3__lock(pCtx, pData, buf_file, &pPath_lock)  );

        if (!pPath_lock)
        {
            continue;
        }

        /* add the lock to our list */
        SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pData->ptx->prb_append_locks, buf_file, pPath_lock)  );
        pPath_lock = NULL;

        /* OK, we got the lock, so the file is ours.  Now we have to
         * deal with the case where the file doesn't exist yet.  Just
         * create it empty. */
        SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_file, &bExists, NULL, NULL)  );
        if (!bExists)
        {
            SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPath_file,SG_FILE_WRONLY|SG_FILE_CREATE_NEW,0644,&pFile)  );
            SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );
        }

        /* If we get here, then all is okay.  We've got our file
         * and the lock on it, so we can exit the loop. */
        break;
    }

    *p_filenumber = filenumber;

    return;
fail:
    SG_FILE_NULLCLOSE(pCtx, pFile);
}

static void sg_fs3__find_a_place(
    SG_context * pCtx,
    my_instance_data* pData,
    SG_uint64 space,
    SG_uint32* p_filenumber
    )
{
    SG_uint32 filenumber = 0;

    SG_ERR_CHECK(  sg_fs3__find_a_place__already_locked(pCtx, pData, space, &filenumber)  );

    if (!filenumber)
    {
        SG_ERR_CHECK(  sg_fs3__find_a_place__new_lock(pCtx, pData, space, &filenumber)  );
    }

    SG_ASSERT(filenumber);

    *p_filenumber = filenumber;

    return;
fail:
    return;
}

static void sg_fs3__open_file_for_writing(
    SG_context * pCtx,
    my_instance_data* pData,
    sg_blob_fs3_handle_store* pbh
    )
{
    SG_bool b_already = SG_FALSE;
    SG_file* pFile = NULL;
    char buf[sg_FILENUMBER_BUFFER_LENGTH];
    SG_pathname* pPathnameFile = NULL;

    SG_ERR_CHECK(  sg_fs3__filenumber_to_filename(pCtx, buf, sizeof(buf), pbh->filenumber)  );

    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pData->ptx->prb_file_handles, buf, &b_already, (void**) &pFile)  );
    if (pFile)
    {
        pbh->pFileBlob = pFile;
    }
    else
    {
        SG_ERR_CHECK(  sg_fs3__get_filenumber_path__sz(pCtx, pData, buf, &pPathnameFile)  );
        SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathnameFile ,SG_FILE_WRONLY|SG_FILE_OPEN_EXISTING,0644,&pbh->pFileBlob)  );
        SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pData->ptx->prb_file_handles, buf, pbh->pFileBlob)  );
    }

    SG_ERR_CHECK(  SG_file__seek_end(pCtx, pbh->pFileBlob, &pbh->offset)  );

fail:
    ;
}

static void my_sg_blob_fs3__store_blob__begin(
    SG_context * pCtx,
    my_instance_data* pData,
    SG_blob_encoding blob_encoding_given,
    const char* psz_hid_vcdiff_reference,
    SG_bool b_zlib,
    SG_uint64 len_encoded,
    SG_uint64 len_full,
    const char* psz_hid_known,
    sg_blob_fs3_handle_store** ppHandle
    )
{
    sg_blob_fs3_handle_store* pbh = NULL;
	int zError;
    SG_uint64 space_needed = 0;

	SG_NULLARGCHECK_RETURN(pData);

	if (!pData->ptx)
	{
		SG_ERR_THROW(  SG_ERR_NO_REPO_TX  );
	}

    /* TODO should we check here to see if the blob is already there?
     * the check is potentially expensive, since it requires us to
     * hit the sqlite db.  also, we can only check when psz_hid_known
     * is provided.  also, doing the check here doesn't spare us the
     * need to handle collisions during the tx commit, since if we
     * check here and the blob is not present, it might be in progress
     * on another thread/process. */

    SG_ERR_CHECK(  SG_alloc(pCtx, 1, sizeof(sg_blob_fs3_handle_store), &pbh)  );

    if (SG_IS_BLOBENCODING_FULL(blob_encoding_given))
    {
		SG_ERR_CHECK(  sg_repo__fs3__hash__begin(pCtx, pData->pRepo, &pbh->pRHH_ComputeOnStore)  );
    }
    else
    {
        SG_NULLARGCHECK(  psz_hid_known  );
    }

    if (SG_IS_BLOBENCODING_VCDIFF(blob_encoding_given))
    {
        SG_NULLARGCHECK(  psz_hid_vcdiff_reference  );
        SG_ARGCHECK(psz_hid_vcdiff_reference[0], psz_hid_vcdiff_reference);
    }
    else
    {
        SG_ARGCHECK(psz_hid_vcdiff_reference == NULL, psz_hid_vcdiff_reference);
    }

    pbh->pData = pData;
    pbh->blob_encoding_given = blob_encoding_given;
    pbh->psz_hid_vcdiff_reference = psz_hid_vcdiff_reference;
    pbh->len_full_given = len_full;
    pbh->psz_hid_known = psz_hid_known;

	memset(&pbh->zStream,0,sizeof(pbh->zStream));

    if (
            b_zlib
       )
    {
        zError = deflateInit(&pbh->zStream,Z_DEFAULT_COMPRESSION);
        if (zError != Z_OK)
        {
            SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
        }
        pbh->b_compressing = SG_TRUE;
        pbh->blob_encoding_storing = SG_BLOBENCODING__ZLIB;

        space_needed = len_full;
    }
    else
    {
        pbh->blob_encoding_storing = pbh->blob_encoding_given;
        pbh->b_compressing = SG_FALSE;

        if (len_encoded > 0)
        {
            space_needed = len_encoded;
        }
        else
        {
            space_needed = len_full;
        }
    }

    SG_ERR_CHECK(  sg_fs3__find_a_place(pCtx, pData, space_needed, &pbh->filenumber)  );

    SG_ERR_CHECK(  sg_fs3__open_file_for_writing(pCtx, pData, pbh)  );

    *ppHandle = pbh;
    pbh = NULL;

    return;
fail:
    SG_ERR_IGNORE(  sg_blob_fs3_handle_store__free(pCtx, pbh)  );
}

void sg_blob_fs3__store_blob__begin(
    SG_context * pCtx,
    my_instance_data* pData,
    SG_blob_encoding blob_encoding_given,
    const char* psz_hid_vcdiff_reference,
    SG_uint64 lenFull,
	SG_uint64 lenEncoded,
    const char* psz_hid_known,
    sg_blob_fs3_handle_store** ppHandle
    )
{
    SG_bool b_zlib = SG_FALSE;
    sg_blob_fs3_handle_store* pbh = NULL;

	SG_NULLARGCHECK_RETURN(pData);

	if (pData->ptx->pBlobStoreHandle)
    {
		SG_ERR_THROW_RETURN(SG_ERR_INCOMPLETE_BLOB_IN_REPO_TX);
    }

    if (SG_BLOBENCODING__FULL == blob_encoding_given)
    {
        // When an incoming blob is SG_BLOBENCODING__FULL, we are
        // free to compress it if we want.

        b_zlib = SG_TRUE;
    }
    else if (SG_BLOBENCODING__KEEPFULLFORNOW == blob_encoding_given)
    {
        // When an incoming blob is SG_BLOBENCODING__KEEPFULLFORNOW,
        // we store it FULL.

        blob_encoding_given = SG_BLOBENCODING__FULL;
    }

    SG_ERR_CHECK(  my_sg_blob_fs3__store_blob__begin(pCtx,
            pData,
            blob_encoding_given,
            psz_hid_vcdiff_reference,
            b_zlib,
            lenEncoded,
            lenFull,
            psz_hid_known,
            &pbh)  );

	pData->ptx->pBlobStoreHandle = pbh;
    *ppHandle = pbh;

    return;
fail:
    return;
}

void sg_blob_fs3__store_blob__chunk(
    SG_context * pCtx,
    sg_blob_fs3_handle_store* pbh,
    SG_uint32 len_chunk,
    const SG_byte* p_chunk,
    SG_uint32* piBytesWritten
    )
{

    if (pbh->pRHH_ComputeOnStore)
    {
		SG_ERR_CHECK(  sg_repo__fs3__hash__chunk(pCtx, pbh->pData->pRepo, pbh->pRHH_ComputeOnStore, len_chunk, p_chunk)  );
    }

    if (pbh->b_compressing)
    {
        // give this chunk to compressor (it will update next_in and avail_in as
        // it consumes the input).

        pbh->zStream.next_in = (SG_byte*) p_chunk;
        pbh->zStream.avail_in = len_chunk;
        pbh->len_full_observed += len_chunk;

        while (1)
        {
            int zError;

            // give the compressor the complete output buffer

            pbh->zStream.next_out = pbh->bufOut;
            pbh->zStream.avail_out = MY_CHUNK_SIZE;

            // let compressor compress what it can of our input.  it may or
            // may not take all of it.  this will update next_in, avail_in,
            // next_out, and avail_out.

            zError = deflate(&pbh->zStream,Z_NO_FLUSH);
            if (zError != Z_OK)
            {
                SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
            }

            // if there was enough input to generate a compression block,
            // we can write it to our output file.  the amount generated
            // is the delta of avail_out (or equally of next_out).

            if (pbh->zStream.avail_out < MY_CHUNK_SIZE)
            {
                SG_uint32 nOut = MY_CHUNK_SIZE - pbh->zStream.avail_out;
                SG_ASSERT ( (nOut == (SG_uint32)(pbh->zStream.next_out - pbh->bufOut)) );

                SG_ERR_CHECK(  SG_file__write(pCtx, pbh->pFileBlob,nOut,pbh->bufOut,NULL)  );
                pbh->len_encoded_observed += nOut;
            }

            // if compressor didn't take all of our input, let it
            // continue with the existing buffer (next_in was advanced).

            if (pbh->zStream.avail_in == 0)
                break;
        }
    }
    else
    {
        SG_ERR_CHECK(  SG_file__write(pCtx, pbh->pFileBlob,len_chunk,p_chunk,NULL)  );
        if (SG_IS_BLOBENCODING_FULL(pbh->blob_encoding_given))
        {
            pbh->len_full_observed += len_chunk;
        }
        pbh->len_encoded_observed += len_chunk;
    }

    if (piBytesWritten)
    {
        *piBytesWritten = len_chunk;
    }

    return;
fail:
    return;
}

void sg_blob_fs3__store_blob__end(
    SG_context * pCtx,
    sg_blob_fs3_handle_store** ppbh,
    char** ppsz_hid_returned
    )
{
    char* szHidBlob = NULL;
    sg_blob_fs3_handle_store* pbh = *ppbh;
    char buf_filenumber[sg_FILENUMBER_BUFFER_LENGTH];

    if (pbh->b_compressing)
    {
        int zError;

        // we reached end of the input file.  now we need to tell the
        // compressor that we have no more input and that it needs to
        // compress and flush any partial buffers that it may have.

        SG_ASSERT( (pbh->zStream.avail_in == 0) );
        while (1)
        {
            pbh->zStream.next_out = pbh->bufOut;
            pbh->zStream.avail_out = MY_CHUNK_SIZE;

            zError = deflate(&pbh->zStream,Z_FINISH);
            if ((zError != Z_OK) && (zError != Z_STREAM_END))
            {
                SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
            }

            if (pbh->zStream.avail_out < MY_CHUNK_SIZE)
            {
                SG_uint32 nOut = MY_CHUNK_SIZE - pbh->zStream.avail_out;
                SG_ASSERT ( (nOut == (SG_uint32)(pbh->zStream.next_out - pbh->bufOut)) );

                SG_ERR_CHECK(  SG_file__write(pCtx, pbh->pFileBlob,nOut,pbh->bufOut,NULL)  );

                pbh->len_encoded_observed += nOut;
            }

            if (zError == Z_STREAM_END)
                break;
        }
    }

    if (pbh->pRHH_ComputeOnStore)
    {
		// all of the data has been hashed now.  let the hash algorithm finalize stuff
		// and spit out an HID for us.  this allocates an HID and frees the repo_hash_handle.
		SG_ERR_CHECK(  sg_repo__fs3__hash__end(pCtx, pbh->pData->pRepo, &pbh->pRHH_ComputeOnStore, &szHidBlob)  );

        if (pbh->psz_hid_known)
        {
            if (0 != strcmp(szHidBlob, pbh->psz_hid_known))
            {
                SG_ERR_THROW(  SG_ERR_BLOB_NOT_VERIFIED_MISMATCH  );
            }
        }
    }
    else
    {
        /* we are trusting psz_hid_known */
        SG_ERR_CHECK(  SG_STRDUP(pCtx, pbh->psz_hid_known, &szHidBlob)  );
    }

    SG_ASSERT(szHidBlob);
    pbh->psz_hid_blob_final = szHidBlob;
    szHidBlob = NULL;

    if (SG_IS_BLOBENCODING_FULL(pbh->blob_encoding_given))
    {
        if (pbh->len_full_observed != pbh->len_full_given)
        {
            SG_ERR_THROW(  SG_ERR_BLOB_NOT_VERIFIED_INCOMPLETE  );
        }
    }

#if 0 // we no longer close the file here
    SG_ERR_CHECK(  SG_file__close(pCtx, &pbh->pFileBlob)  );
#endif

    if (SG_BLOBENCODING__VCDIFF == pbh->blob_encoding_storing)
    {
        SG_ASSERT(pbh->psz_hid_vcdiff_reference);
    }

    SG_ERR_CHECK(  sg_fs3__filenumber_to_filename(pCtx, buf_filenumber, sizeof(buf_filenumber), pbh->filenumber)  );

    if (pbh->pData->ptx->flags & SG_REPO_TX_FLAG__CLONING)
    {
        SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pbh->pData->ptx->cloning.pStmt_insert)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pbh->pData->ptx->cloning.pStmt_insert)  );

        SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pbh->pData->ptx->cloning.pStmt_insert,1,pbh->psz_hid_blob_final)  );
        SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pbh->pData->ptx->cloning.pStmt_insert,2,buf_filenumber)  );
        SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pbh->pData->ptx->cloning.pStmt_insert,3,pbh->offset)  );
        SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pbh->pData->ptx->cloning.pStmt_insert,4,pbh->blob_encoding_storing)  );
        SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pbh->pData->ptx->cloning.pStmt_insert,5,pbh->len_encoded_observed)  );
        SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pbh->pData->ptx->cloning.pStmt_insert,6,pbh->len_full_given)  );
        if (pbh->psz_hid_vcdiff_reference)
        {
            SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pbh->pData->ptx->cloning.pStmt_insert,7,pbh->psz_hid_vcdiff_reference)  );
        }
        else
        {
            SG_ERR_CHECK(  sg_sqlite__bind_null(pCtx, pbh->pData->ptx->cloning.pStmt_insert,7)  );
        }
        sg_sqlite__step(pCtx,pbh->pData->ptx->cloning.pStmt_insert,SQLITE_DONE);
    }
    else
    {
        SG_ERR_CHECK(  SG_blobset__insert(
                    pCtx,
                    pbh->pData->ptx->pbs_new_blobs,
                    pbh->psz_hid_blob_final,
                    buf_filenumber,
                    pbh->offset,
                    pbh->blob_encoding_storing,
                    pbh->len_encoded_observed,
                    pbh->len_full_given,
                    pbh->psz_hid_vcdiff_reference
                    )  );
    }

	if (ppsz_hid_returned)
    {
        SG_ERR_CHECK(  SG_STRDUP(pCtx, pbh->psz_hid_blob_final, ppsz_hid_returned)  );
    }

	// fall-thru to common cleanup

fail:
#if 0 // the file is closed elsewhere
	SG_FILE_NULLCLOSE(pCtx, pbh->pFileBlob);
#endif
	SG_NULLFREE(pCtx, szHidBlob);

    SG_ERR_IGNORE(  sg_blob_fs3_handle_store__free(pCtx, pbh)  );

    /* The blob handle gets freed whether this function succeeds or fails,
     * so we null the reference */
    *ppbh = NULL;
}

void sg_blob_fs3__store_blob__abort(
    SG_context * pCtx,
    sg_blob_fs3_handle_store** ppbh
    )
{
    SG_ERR_IGNORE(  sg_blob_fs3_handle_store__free(pCtx, *ppbh)  );
    *ppbh = NULL;
}

static void _blob_handle__init_digest(SG_context * pCtx, sg_blob_fs3_handle_fetch * pbh)
{
	// If they want us to verify the HID using the actual contents of the Raw
	// Data that we stored in the BLOBFILE (after we uncompress/undeltafy/etc it),
	// we start a digest now.

	SG_ERR_CHECK_RETURN(  sg_repo__fs3__hash__begin(pCtx, pbh->pData->pRepo, &pbh->pRHH_VerifyOnFetch)  );
}

static void _sg_blob_handle__check_digest(SG_context * pCtx, sg_blob_fs3_handle_fetch * pbh)
{
	// verify that the digest that we computed as we read the contents
	// of the blob matches the HID that we think it should have.

	if (pbh->len_full_stored != 0 && pbh->len_encoded_stored != pbh->len_encoded_observed)
		SG_ERR_THROW_RETURN(SG_ERR_BLOB_NOT_VERIFIED_INCOMPLETE);

	if (pbh->pRHH_VerifyOnFetch)
	{
		char * pszHid_Computed = NULL;
		SG_bool bNoMatch;

		SG_ERR_CHECK_RETURN(  sg_repo__fs3__hash__end(pCtx, pbh->pData->pRepo, &pbh->pRHH_VerifyOnFetch, &pszHid_Computed)  );
		bNoMatch = (0 != strcmp(pszHid_Computed, pbh->sz_hid_requested));
		SG_NULLFREE(pCtx, pszHid_Computed);

		if (bNoMatch)
        {
			SG_ERR_THROW_RETURN(SG_ERR_BLOB_NOT_VERIFIED_MISMATCH);
        }
	}
}

static void _sg_blob_handle__close(SG_context * pCtx, sg_blob_fs3_handle_fetch * pbh);

void sg_fs3__fetch_blob_into_tempfile(
	SG_context * pCtx,
    my_instance_data* pData,
	const char* psz_hid_blob,
	SG_pathname** ppResult
	)
{
    sg_blob_fs3_handle_fetch* pbh = NULL;
    SG_uint64 len = 0;
    SG_uint32 got = 0;
    SG_byte* p_buf = NULL;
    SG_uint64 left = 0;
	SG_pathname* pTempDirPath = NULL;
	char buf_filename[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pTempFilePath = NULL;
	SG_file* pFile = NULL;
    SG_bool b_done = SG_FALSE;

	SG_NULLARGCHECK_RETURN(psz_hid_blob);

	SG_ERR_CHECK( SG_PATHNAME__ALLOC__USER_TEMP_DIRECTORY(pCtx, &pTempDirPath)  );

	SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_filename, sizeof(buf_filename))  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pTempFilePath, pTempDirPath, buf_filename)  );
	SG_PATHNAME_NULLFREE(pCtx, pTempDirPath);

    SG_ERR_CHECK(  sg_blob_fs3__fetch_blob__begin(pCtx,
                pData,
                psz_hid_blob,
                SG_TRUE,
                NULL,
                NULL,
                NULL,
                &len,
				SG_TRUE,
                &pbh)  );
	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pTempFilePath, SG_FILE_RDWR | SG_FILE_CREATE_NEW, 0644, &pFile) );

    left = len;
    SG_ERR_CHECK(  SG_alloc(pCtx, SG_STREAMING_BUFFER_SIZE, 1, &p_buf)  );
    while (!b_done)
    {
        SG_uint32 want = SG_STREAMING_BUFFER_SIZE;
        if (want > left)
        {
            want = (SG_uint32) left;
        }
        SG_ERR_CHECK(  sg_blob_fs3__fetch_blob__chunk(pCtx, pbh, want, p_buf, &got, &b_done)  );
        SG_ERR_CHECK(  SG_file__write(pCtx, pFile, got, p_buf, NULL)  );

        left -= got;
    }
    SG_ERR_CHECK(  sg_blob_fs3__fetch_blob__end(pCtx, &pbh)  );
    SG_ASSERT(0 == left);
    SG_NULLFREE(pCtx, p_buf);
    SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

    *ppResult = pTempFilePath;

    return;
fail:
    SG_ERR_IGNORE(  sg_blob_fs3__fetch_blob__abort(pCtx, &pbh)  );
    SG_FILE_NULLCLOSE(pCtx, pFile);
    SG_NULLFREE(pCtx, p_buf);
}

static void _open_blobfile_for_reading( SG_context * pCtx, my_instance_data* pData, sg_blob_fs3_handle_fetch * pbh)
{
	SG_pathname* pPathnameFile = NULL;

	SG_ERR_CHECK(  sg_fs3__get_filenumber_path__uint32(pCtx, pData, pbh->filenumber, &pPathnameFile)  );

	SG_file__open__pathname(pCtx,pPathnameFile,SG_FILE_RDONLY|SG_FILE_OPEN_EXISTING,
		SG_FSOBJ_PERMS__UNUSED,&pbh->m_pFileBlob);
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		SG_error err = SG_ERR_UNSPECIFIED;
		SG_context__get_err(pCtx, &err);

		switch (err)
		{
		default:
			SG_ERR_RETHROW;

#if defined(MAC) || defined(LINUX)
		case SG_ERR_ERRNO(ENOENT):
#endif
#if defined(WINDOWS)
		case SG_ERR_GETLASTERROR(ERROR_FILE_NOT_FOUND):
		case SG_ERR_GETLASTERROR(ERROR_PATH_NOT_FOUND):
#endif
			SG_ERR_RESET_THROW(SG_ERR_BLOB_NOT_FOUND);
		}
	}

	if (pbh->offset)
	{
		SG_ERR_CHECK(  SG_file__seek(pCtx, pbh->m_pFileBlob, pbh->offset)  );
	}

	/* fall through */
fail:
	;
}

/* This call opens a blob handle for reading.  Its parameters
are the information from the directory.  This version of the
call is used while building a fragball, iterating over the
whole blob directory. */
static void sg_fs3__open_blob_for_reading__already_got_info(
	SG_context * pCtx,
    my_instance_data* pData,
	const char* szHidBlob, // HID of BLOBFILE that we want
	SG_bool b_convert_to_full,
	SG_bool b_open_file,
    SG_uint32 filenumber,
    SG_uint64 offset,
    SG_blob_encoding blob_encoding_stored,
    SG_uint64 len_encoded_stored,
    SG_uint64 len_full_stored,
    char** ppsz_hid_vcdiff_reference_stored,
	sg_blob_fs3_handle_fetch ** ppbh // returned handle for BLOB -- must call our close
	)
{
	sg_blob_fs3_handle_fetch * pbh = NULL;

	SG_NULLARGCHECK_RETURN(pData);
	SG_NULLARGCHECK_RETURN(szHidBlob);
	SG_NULLARGCHECK_RETURN(ppbh);
    // TODO more arg checks here?

    SG_ERR_CHECK(  SG_alloc1(pCtx, pbh)  );
    pbh->pData = pData;

    SG_ERR_CHECK(  SG_strcpy(pCtx, pbh->sz_hid_requested, sizeof(pbh->sz_hid_requested), szHidBlob)  );
    pbh->filenumber = filenumber;
    pbh->offset = offset;
    pbh->blob_encoding_stored = blob_encoding_stored;
    pbh->len_encoded_stored = len_encoded_stored;
    pbh->len_full_stored = len_full_stored;
    pbh->psz_hid_vcdiff_reference_stored_freeme = *ppsz_hid_vcdiff_reference_stored;
    *ppsz_hid_vcdiff_reference_stored = NULL;

    if (
            b_convert_to_full
            && SG_IS_BLOBENCODING_VCDIFF(pbh->blob_encoding_stored)
       )
    {
        SG_ERR_CHECK(  sg_fs3__fetch_blob_into_tempfile(pCtx, pData, pbh->psz_hid_vcdiff_reference_stored_freeme, &pbh->pPath_tempfile_vcdiff_reference)  );
    }

	pbh->b_we_own_file = b_open_file;
	if (b_open_file)
	{
		SG_ERR_CHECK(  _open_blobfile_for_reading(pCtx, pData, pbh)  );
	}

    if (
            SG_IS_BLOBENCODING_FULL(pbh->blob_encoding_stored)
            || b_convert_to_full
       )
    {
        SG_ERR_CHECK(  _blob_handle__init_digest(pCtx, pbh)  );
    }

    *ppbh = pbh;	// caller must call our close_handle routine to free this

	return;

fail:
	SG_ERR_IGNORE(  _sg_blob_handle__close(pCtx, pbh)  );
}

/* This call opens a blob handle for reading.  Its primary
paraemter is the HID of the blob.  It will look up the
info in the blob directory. */
static void sg_fs3__open_blob_for_reading(
	SG_context * pCtx,
    my_instance_data* pData,
	const char* szHidBlob, // HID of BLOBFILE that we want
	SG_bool b_convert_to_full,
	SG_bool b_open_file,
	sg_blob_fs3_handle_fetch ** ppbh // returned handle for BLOB -- must call our close
	)
{
	sg_blob_fs3_handle_fetch * pbh = NULL;
    SG_uint32 filenumber = 0;
    SG_uint64 offset = 0;
    SG_blob_encoding blob_encoding_stored = 0;
    SG_uint64 len_encoded_stored = 0;
    SG_uint64 len_full_stored = 0;
    char* psz_hid_vcdiff_reference_stored = NULL;

	SG_NULLARGCHECK_RETURN(pData);
	SG_NULLARGCHECK_RETURN(szHidBlob);
	SG_NULLARGCHECK_RETURN(ppbh);

    SG_ERR_CHECK(  do_fetch_info(pCtx, pData, szHidBlob, &filenumber, &offset, &blob_encoding_stored, &len_encoded_stored, &len_full_stored, &psz_hid_vcdiff_reference_stored)  );

    SG_ERR_CHECK(  sg_fs3__open_blob_for_reading__already_got_info(
                pCtx, 
                pData,
                szHidBlob,
                b_convert_to_full,
                b_open_file,
                filenumber,
                offset,
                blob_encoding_stored,
                len_encoded_stored,
                len_full_stored,
                &psz_hid_vcdiff_reference_stored,
                &pbh
                )  );

    *ppbh = pbh;	// caller must call our close_handle routine to free this
    pbh = NULL;

fail:
    SG_NULLFREE(pCtx, psz_hid_vcdiff_reference_stored);
    if (pbh)
    {
        SG_ERR_IGNORE(  _sg_blob_handle__close(pCtx, pbh)  );
    }
}

static void _sg_blob_handle__close(SG_context * pCtx, sg_blob_fs3_handle_fetch * pbh)
{
	// close and free blob handle.
	//
	// this can happen when either they have completely read the contents
	// of the BLOB -or- they are aborting because of an error (or lack of
	// interest in the rest of the blob).

	if (!pbh)
		return;

	inflateEnd(&pbh->zStream);

	if (pbh->pRHH_VerifyOnFetch)
	{
		// if this fetch-handle is being freed with an active repo_hash_handle, we destory it too.
		// if everything went as expected, our caller should have finalized the hash (called __end)
		// and gotten the computed HID (and already freed the repo_hash_handle).  so if we still have
		// one, we must assume that there was a problem and call __abort on it.

		SG_ERR_IGNORE(  sg_repo__fs3__hash__abort(pCtx, pbh->pData->pRepo, &pbh->pRHH_VerifyOnFetch)  );
	}

	if (pbh->b_we_own_file)
		SG_FILE_NULLCLOSE(pCtx, pbh->m_pFileBlob);

    SG_NULLFREE(pCtx, pbh->psz_hid_vcdiff_reference_stored_freeme);

	SG_NULLFREE(pCtx, pbh);
}

void sg_blob_fs3__fetch_blob__chunk_wrapper(
    SG_context * pCtx,
    sg_blob_fs3_handle_fetch* pbh,
    SG_uint32 len_buf,
    SG_byte* p_buf,
    SG_uint32* p_len_got
    );

void sg_blob_fs3__setup_undeltify(
    SG_context * pCtx,
    my_instance_data* pData,
    const char* psz_hid_blob,
    sg_blob_fs3_handle_fetch* pbh
    )
{
    /* The reference blob needs to get into a seekreader. */
    SG_ERR_CHECK(  SG_seekreader__alloc__for_file(pCtx, pbh->pPath_tempfile_vcdiff_reference, 0, &pbh->psr_reference)  );

    /* The delta blob needs to be a readstream */
    SG_ERR_CHECK(  sg_blob_fs3__fetch_blob__begin(pCtx,
                pData,
                psz_hid_blob,
                SG_FALSE,
                NULL,
                NULL,
                NULL,
                NULL,
				SG_TRUE,
                &pbh->pbh_delta)  );
    SG_ERR_CHECK(  SG_readstream__alloc(pCtx,
                pbh->pbh_delta,
                (SG_stream__func__read*) sg_blob_fs3__fetch_blob__chunk_wrapper,
                NULL,
                0,
                &pbh->pstrm_delta)  );

    /* Now begin the undeltify operation */
    SG_ERR_CHECK(  SG_vcdiff__undeltify__begin(pCtx, pbh->psr_reference, pbh->pstrm_delta, &pbh->pst)  );
    pbh->p_buf = NULL;
    pbh->count = 0;
    pbh->next = 0;

    return;
fail:
    return;
}

void sg_blob_fs3__fetch_blob__begin(
    SG_context * pCtx,
    my_instance_data* pData,
    const char* psz_hid_blob,
    SG_bool b_convert_to_full,
    SG_blob_encoding* p_blob_encoding,
    char** ppsz_hid_vcdiff_reference,
    SG_uint64* pLenEncoded,
    SG_uint64* pLenFull,
	SG_bool b_open_file,
    sg_blob_fs3_handle_fetch** ppHandle
    )
{
    char* psz_hid_vcdiff_reference_returning = NULL;

	sg_blob_fs3_handle_fetch * pbh = NULL;

	SG_ERR_CHECK_RETURN(  sg_fs3__open_blob_for_reading(pCtx,
                pData,
                psz_hid_blob,
                b_convert_to_full,
				b_open_file,
                &pbh
                )  );

    if (
            b_convert_to_full
            && (!SG_IS_BLOBENCODING_FULL(pbh->blob_encoding_stored))
            )
    {
        pbh->blob_encoding_returning = SG_BLOBENCODING__FULL;

        if (SG_IS_BLOBENCODING_ZLIB(pbh->blob_encoding_stored))
        {
            int zError;

            pbh->b_uncompressing = SG_TRUE;

            memset(&pbh->zStream,0,sizeof(pbh->zStream));
            zError = inflateInit(&pbh->zStream);
            if (zError != Z_OK)
            {
                SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
            }
        }
        else if (SG_BLOBENCODING__VCDIFF == pbh->blob_encoding_stored)
        {
            pbh->b_undeltifying = SG_TRUE;
            SG_ASSERT(pbh->psz_hid_vcdiff_reference_stored_freeme);
            SG_ERR_CHECK(  sg_blob_fs3__setup_undeltify(pCtx, pData, psz_hid_blob, pbh)  );
        }
        else
        {
            /* should never happen */
            SG_ERR_THROW(  SG_ERR_ASSERT  );
        }
    }
    else
    {
        pbh->b_undeltifying = SG_FALSE;
        pbh->b_uncompressing = SG_FALSE;

        pbh->blob_encoding_returning = pbh->blob_encoding_stored;
        psz_hid_vcdiff_reference_returning = pbh->psz_hid_vcdiff_reference_stored_freeme;
    }

    if (p_blob_encoding)
    {
        *p_blob_encoding = pbh->blob_encoding_returning;
    }
    if (ppsz_hid_vcdiff_reference)
    {
        if (psz_hid_vcdiff_reference_returning)
        {
            SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_hid_vcdiff_reference_returning, ppsz_hid_vcdiff_reference)  );
        }
        else
        {
            *ppsz_hid_vcdiff_reference = NULL;
        }
    }
    if (pLenEncoded)
    {
        *pLenEncoded = pbh->len_encoded_stored;
    }
    if (pLenFull)
    {
        *pLenFull = pbh->len_full_stored;
    }

    if (ppHandle)
    {
        *ppHandle = pbh;
    }
    else
    {
        SG_ERR_IGNORE(  _sg_blob_handle__close(pCtx, pbh)  );
    }

    return;
fail:
    return;
}

void sg_blob_fs3__fetch_blob__chunk(
    SG_context * pCtx,
    sg_blob_fs3_handle_fetch* pbh,
    SG_uint32 len_buf,
    SG_byte* p_buf,
    SG_uint32* p_len_got,
    SG_bool* pb_done
    )
{
    SG_uint32 nbr = 0;
    SG_bool b_done = SG_FALSE;

    if (pbh->b_uncompressing)
    {
        int zError;

        if (0 == pbh->zStream.avail_in)
        {
            SG_uint32 want = sizeof(pbh->bufCompressed);
            if (want > (pbh->len_encoded_stored - pbh->len_encoded_observed))
            {
                want = (SG_uint32)(pbh->len_encoded_stored - pbh->len_encoded_observed);
            }

            SG_file__read(pCtx,pbh->m_pFileBlob,want,pbh->bufCompressed,&nbr);
			if(SG_CONTEXT__HAS_ERR(pCtx) && !SG_context__err_equals(pCtx, SG_ERR_EOF))
				SG_ERR_RETHROW_RETURN;

            pbh->len_encoded_observed += nbr;

            pbh->zStream.next_in = pbh->bufCompressed;
            pbh->zStream.avail_in = nbr;
        }

        pbh->zStream.next_out = p_buf;
        pbh->zStream.avail_out = len_buf;

		while (1)
		{
			// let decompressor decompress what it can of our input.  it may or
			// may not take all of it.  this will update next_in, avail_in,
			// next_out, and avail_out.
			//
			// WARNING: we get Z_STREAM_END when the decompressor is finished
			// WARNING: processing all of the input.  it gives this immediately
			// WARNING: (with the last partial buffer) rather than returning OK
			// WARNING: for the last partial buffer and then returning END with
			// WARNING: zero bytes transferred.

			zError = inflate(&pbh->zStream,Z_NO_FLUSH);
			if ((zError != Z_OK)  &&  (zError != Z_STREAM_END))
			{
				SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
			}

			if (zError == Z_STREAM_END)
            {
                b_done = SG_TRUE;
                break;
            }

			// if decompressor didn't take all of our input, let it continue
			// with the existing buffer (next_in was advanced).

			if (pbh->zStream.avail_in == 0)
            {
				break;
            }

			if (pbh->zStream.avail_out == 0)
            {
                /* no room left */
				break;
            }
		}

        if (pbh->zStream.avail_out < len_buf)
        {
            nbr = len_buf - pbh->zStream.avail_out;
        }
        else
        {
            nbr = 0;
        }
    }
    else if (pbh->b_undeltifying)
    {
        if (!pbh->p_buf || !pbh->count || (pbh->next >= pbh->count))
        {
            SG_ASSERT(pbh->pst);
            SG_ERR_CHECK(  SG_vcdiff__undeltify__chunk(pCtx, pbh->pst, &pbh->p_buf, &pbh->count)  );
            if (0 == pbh->count)
            {
                b_done = SG_TRUE;
            }
            pbh->next = 0;
        }

        nbr = pbh->count - pbh->next;
        if (nbr > len_buf)
        {
            nbr = len_buf;
        }

        memcpy(p_buf, pbh->p_buf + pbh->next, nbr);
        pbh->next += nbr;

        /* We don't have a way of knowing how many bytes the vcdiff code has read
         * from our readstream.  But the readstream keeps track of this, so we just
         * ask it.
         */
        SG_ERR_CHECK(  SG_readstream__get_count(pCtx, pbh->pstrm_delta, &pbh->len_encoded_observed)  );
    }
    else
    {
        SG_uint32 want;

        want = len_buf;
        if (want > (pbh->len_encoded_stored - pbh->len_encoded_observed))
        {
            want = (SG_uint32)(pbh->len_encoded_stored - pbh->len_encoded_observed);
        }

        SG_file__read(pCtx, pbh->m_pFileBlob, want, p_buf, &nbr);
        if(SG_CONTEXT__HAS_ERR(pCtx) && !SG_context__err_equals(pCtx, SG_ERR_EOF))
        {
			SG_ERR_RETHROW_RETURN;
        }

        pbh->len_encoded_observed += nbr;

        if (pbh->len_encoded_observed == pbh->len_encoded_stored)
        {
            b_done = SG_TRUE;
        }
    }

    if (nbr)
    {
        if (pbh->pRHH_VerifyOnFetch)
        {
            SG_ERR_CHECK_RETURN(  sg_repo__fs3__hash__chunk(pCtx, pbh->pData->pRepo, pbh->pRHH_VerifyOnFetch, nbr, p_buf)  );
        }
        if (SG_IS_BLOBENCODING_FULL(pbh->blob_encoding_returning))
        {
            pbh->len_full_observed += nbr;
        }
    }

    *p_len_got = nbr;
    *pb_done = b_done;

    return;
fail:
    return;
}

void sg_blob_fs3__fetch_blob__chunk_wrapper(
    SG_context * pCtx,
    sg_blob_fs3_handle_fetch* pbh,
    SG_uint32 len_buf,
    SG_byte* p_buf,
    SG_uint32* p_len_got
    )
{
    SG_bool b_unused = SG_FALSE;

    SG_ERR_CHECK_RETURN(  sg_blob_fs3__fetch_blob__chunk(pCtx, pbh, len_buf, p_buf, p_len_got, &b_unused)  );
}

void sg_blob_fs3__fetch_blob__end(
    SG_context * pCtx,
    sg_blob_fs3_handle_fetch** ppbh
    )
{
    sg_blob_fs3_handle_fetch* pbh = NULL;

	SG_NULLARGCHECK_RETURN(ppbh);
    pbh = *ppbh;

    if (pbh->b_uncompressing)
    {
        SG_ASSERT( (pbh->zStream.avail_in == 0) );

        inflateEnd(&pbh->zStream);
    }
    else if (pbh->b_undeltifying)
    {
        SG_ERR_CHECK(  SG_vcdiff__undeltify__end(pCtx, pbh->pst)  );
        pbh->pst = NULL;

        SG_ERR_CHECK(  sg_blob_fs3__fetch_blob__end(pCtx, &pbh->pbh_delta)  );

        SG_ERR_CHECK(  SG_readstream__close(pCtx, pbh->pstrm_delta)  );
        pbh->pstrm_delta = NULL;

        SG_ERR_CHECK(  SG_seekreader__close(pCtx, pbh->psr_reference)  );
        pbh->psr_reference = NULL;

        if (pbh->pPath_tempfile_vcdiff_reference)
        {
            SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pbh->pPath_tempfile_vcdiff_reference)  );
            SG_PATHNAME_NULLFREE(pCtx, pbh->pPath_tempfile_vcdiff_reference);
        }

        pbh->p_buf = NULL;
        pbh->count = 0;
        pbh->next = 0;
    }

	if (pbh->b_we_own_file)
		SG_ERR_CHECK(  SG_file__close(pCtx, &pbh->m_pFileBlob)  );

	if (pbh->len_full_stored != 0 && pbh->len_encoded_observed != pbh->len_encoded_stored)
    {
        SG_ERR_THROW_RETURN(SG_ERR_BLOB_NOT_VERIFIED_INCOMPLETE);
    }

    if (SG_IS_BLOBENCODING_FULL(pbh->blob_encoding_returning))
    {
        if (pbh->len_full_observed != pbh->len_full_stored)
        {
            SG_ERR_THROW_RETURN(SG_ERR_BLOB_NOT_VERIFIED_INCOMPLETE);
        }
    }

	if (pbh->pRHH_VerifyOnFetch)
	{
		SG_ERR_CHECK_RETURN(  _sg_blob_handle__check_digest(pCtx, pbh)  );
	}

	SG_ERR_IGNORE(  _sg_blob_handle__close(pCtx, pbh)  );

    *ppbh = NULL;

    return;
fail:
    return;
}

void sg_blob_fs3__fetch_blob__abort(
    SG_context * pCtx,
    sg_blob_fs3_handle_fetch** ppbh
    )
{
	SG_NULLARGCHECK_RETURN(ppbh);

	SG_ERR_IGNORE(  _sg_blob_handle__close(pCtx, *ppbh)  );
    *ppbh = NULL;
}

static void sg_blob_fs3__sql__find_by_hid_prefix(SG_context * pCtx, sqlite3 * psql, const char * psz_hid_prefix, SG_rbtree** pprb)
{
	sqlite3_stmt * pStmt = NULL;
    SG_rbtree* prb = NULL;
    int rc;
    char buf_prefix_plus_one[SG_HID_MAX_BUFFER_LENGTH];

	SG_NULLARGCHECK_RETURN(psql);
	SG_NULLARGCHECK_RETURN(psz_hid_prefix);
	SG_NULLARGCHECK_RETURN(pprb);

    SG_ERR_CHECK(  SG_strcpy(pCtx, buf_prefix_plus_one, sizeof(buf_prefix_plus_one), psz_hid_prefix)  );

	/* This will throw if the provided hid prefix has non-hex in it.
	   SG_dag_sqlite3__find_by_prefix returns an empty rbtree in this case.
	   Consider doing that here. */
	SG_ERR_CHECK(  SG_hex__fake_add_one(pCtx, buf_prefix_plus_one)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql,&pStmt,
									  "SELECT \"hid\" FROM \"blobs\" WHERE (\"hid\" >= ?) AND (\"hid\" < ?)")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, psz_hid_prefix)  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, buf_prefix_plus_one)  );

    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb)  );

    while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        const char * szHid = (const char *)sqlite3_column_text(pStmt,0);

        SG_ERR_CHECK(  SG_rbtree__add(pCtx, prb,szHid)  );
    }
    if (rc != SQLITE_DONE)
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    *pprb = prb;

	return;
fail:
    SG_RBTREE_NULLFREE(pCtx, prb);
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}

void sg_blob_fs3__list_blobs(
    SG_context * pCtx,
        sqlite3* psql,
        SG_blob_encoding blob_encoding, /**< only return blobs of this encoding */
        SG_uint32 limit,                /**< only return this many entries */
        SG_uint32 offset,               /**< skip the first group of the sorted entries */
        SG_blobset** ppbs
        )
{
    int rc;
	sqlite3_stmt * pStmt = NULL;
    SG_blobset* pbs = NULL;

    if (blob_encoding)
    {
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql,&pStmt,
                                          "SELECT \"hid\",\"filename\",\"offset\",\"encoding\",\"len_encoded\",\"len_full\",\"hid_vcdiff\" FROM \"blobs\" WHERE \"encoding\" = ? LIMIT ? OFFSET ?")  );
        SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pStmt,1,blob_encoding)  );
        SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pStmt,2,limit ? (SG_int32) limit : -1)  );
        SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pStmt,3,offset)  );
    }
    else
    {
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql,&pStmt,
                                          "SELECT \"hid\",\"filename\",\"offset\",\"encoding\",\"len_encoded\",\"len_full\",\"hid_vcdiff\" FROM \"blobs\" LIMIT ? OFFSET ?")  );
        SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pStmt,1,limit ? (SG_int32) limit : -1)  );
        SG_ERR_CHECK(  sg_sqlite__bind_int(pCtx, pStmt,2,offset)  );
    }

    SG_ERR_CHECK(  SG_blobset__create(pCtx, &pbs)  );
    SG_ERR_CHECK(  SG_blobset__begin_inserting(pCtx, pbs)  );

    while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        const char * psz_hid = (const char *)sqlite3_column_text(pStmt,0);
        const char * psz_filename = (const char *)sqlite3_column_text(pStmt,1);
        SG_uint64 offset = sqlite3_column_int64(pStmt, 2);
        SG_blob_encoding encoding = (SG_blob_encoding) sqlite3_column_int(pStmt, 3);
        SG_uint64 len_encoded = sqlite3_column_int64(pStmt, 4);
        SG_uint64 len_full = sqlite3_column_int64(pStmt, 5);
        const char * psz_hid_vcdiff = (const char *)sqlite3_column_text(pStmt,6);

        SG_ERR_CHECK(  SG_blobset__insert(
                    pCtx,
                    pbs,
                    psz_hid,
                    psz_filename,
                    offset,
                    encoding,
                    len_encoded,
                    len_full,
                    psz_hid_vcdiff
                    )  );
    }
    if (rc != SQLITE_DONE)
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    SG_ERR_CHECK(  SG_blobset__finish_inserting(pCtx, pbs)  );

    *ppbs = pbs;
    pbs = NULL;

    return;
fail:
    SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
    SG_BLOBSET_NULLFREE(pCtx, pbs);
}


static void _my_make_db_pathname(SG_context * pCtx, my_instance_data* pData, SG_pathname ** ppResult)
{
	// create the pathname for the SQL DB file.

	SG_ERR_CHECK_RETURN(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, ppResult, pData->pPathMyDir, "fs3.sqlite3")  );
}

void sg_repo__fs3__open_repo_instance(SG_context * pCtx, SG_repo * pRepo)
{
	my_instance_data * pData = NULL;
	SG_pathname * pPathnameSqlDb = NULL;
    sqlite3* psql = NULL;
    const char* pszParentDir = NULL;
    const char* psz_subdir_name = NULL;
    SG_string* pstr_prop = NULL;
	char * pszTrivialHash_Computed = NULL;
	SG_bool bEqual_TrivialHash;
    void* instance_data = NULL;
    SG_vhash* pvh_descriptor = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);

	// Only open/create the db once.
    SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, &instance_data)  );
	if (instance_data)
    {
		SG_ERR_THROW2_RETURN(SG_ERR_INVALIDARG, (pCtx, "instance data should be empty"));
    }

	SG_ERR_CHECK(  SG_alloc1(pCtx, pData)  );
    pData->pRepo = pRepo;

    SG_ERR_CHECK(  SG_rbtree__alloc(pCtx, &pData->prb_paths)  );
    SG_ERR_CHECK(  SG_rbtree__alloc(pCtx, &pData->prb_sql)  );

    SG_ERR_CHECK(  SG_repo__get_descriptor__ref(pCtx, pRepo, &pvh_descriptor)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_descriptor, SG_RIDESC_FSLOCAL__PATH_PARENT_DIR, &pszParentDir)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pData->pPathParentDir, pszParentDir)  );

	// make sure that <repo_parent_dir> directory exists on disk.

	SG_ERR_CHECK(  SG_fsobj__verify_directory_exists_on_disk__pathname(pCtx, pData->pPathParentDir)  );

	// for the FS implementation, we have a <repo_parent_dir>/<subdir>
	// directory that actually contains the contents of the REPO.
	// construct the pathname to the subdir and verify that it exists on disk.

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_descriptor, SG_RIDESC_FSLOCAL__DIR_NAME, &psz_subdir_name)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pData->pPathMyDir, pData->pPathParentDir, psz_subdir_name)  );
	SG_ERR_CHECK(  SG_fsobj__verify_directory_exists_on_disk__pathname(pCtx, pData->pPathMyDir)  );

    SG_ERR_CHECK(  SG_repo__set_instance_data(pCtx, pRepo, pData)  );

	SG_ERR_CHECK_RETURN(  _my_make_db_pathname(pCtx, pData,&pPathnameSqlDb)  );

    SG_ERR_CHECK(  SG_dag_sqlite3__open(pCtx, pPathnameSqlDb, &psql)  );

	SG_PATHNAME_NULLFREE(pCtx, pPathnameSqlDb);

    SG_ERR_CHECK(  SG_sqlite__table_exists(pCtx, psql, "audits", &pData->b_new_audits)  );

    // TODO fetch b_indexes

	// Get repo and admin ids
	SG_ERR_CHECK(  sg_sqlite__exec__va__string(pCtx, psql, &pstr_prop, "SELECT \"value\" FROM \"props\" WHERE \"name\"='hashmethod'")  );
	SG_ERR_CHECK( SG_strcpy(pCtx, pData->buf_hash_method, sizeof(pData->buf_hash_method), SG_string__sz(pstr_prop))  );
	SG_STRING_NULLFREE(pCtx, pstr_prop);

	SG_ERR_CHECK(  sg_sqlite__exec__va__string(pCtx, psql, &pstr_prop, "SELECT \"value\" FROM \"props\" WHERE \"name\"='repoid'")  );
	SG_ERR_CHECK( SG_strcpy(pCtx, pData->buf_repo_id, sizeof(pData->buf_repo_id), SG_string__sz(pstr_prop))  );
	SG_STRING_NULLFREE(pCtx, pstr_prop);

	SG_ERR_CHECK(  sg_sqlite__exec__va__string(pCtx, psql, &pstr_prop, "SELECT \"value\" FROM \"props\" WHERE \"name\"='adminid'")  );
	SG_ERR_CHECK( SG_strcpy(pCtx, pData->buf_admin_id, sizeof(pData->buf_admin_id), SG_string__sz(pstr_prop))  );
	SG_STRING_NULLFREE(pCtx, pstr_prop);

	// Verify the TRIVIAL-HASH.  This is one last sanity check before we return
	// the pRepo and let them start playing with it.
	//
	// When the first instance of this REPO was originally created on disk
	// (see __create_repo_instance()), a HASH-METHOD was selected.  This CANNOT
	// EVER be changed; not for that instance on disk NOR for any clone instance;
	// ALL HASHES within a REPO must be computed using the SAME hash-method.
	//
	// At this point, we must verify that the currently running implementation
	// also supports the hash-method that was registered when the REPO was
	// created on disk.
	//
	// First, we use the registered hash-method and compute a trivial hash; this
	// ensures that the hash-method is installed and available.   (I call it "trivial"
	// because we are computing the hash of a zero-length buffer.)  This throws
	// SG_ERR_UNKNOWN_HASH_METHOD.
	//
	// Second, verify that the answer matches; this is really obscure but might
	// catch a configuration error or something.

	SG_ERR_CHECK(  sg_repo_utils__one_step_hash__from_sghash(pCtx, pData->buf_hash_method, 0, NULL, &pszTrivialHash_Computed)  );
	SG_ERR_CHECK(  sg_sqlite__exec__va__string(pCtx, psql, &pstr_prop, "SELECT \"value\" FROM \"props\" WHERE \"name\"='trivialhash'")  );
	bEqual_TrivialHash = (strcmp(pszTrivialHash_Computed,SG_string__sz(pstr_prop)) == 0);
	if (!bEqual_TrivialHash)
		SG_ERR_THROW(  SG_ERR_TRIVIAL_HASH_DIFFERENT  );
	pData->strlen_hashes = SG_string__length_in_bytes(pstr_prop);
	SG_STRING_NULLFREE(pCtx, pstr_prop);
	SG_NULLFREE(pCtx, pszTrivialHash_Computed);

	// all is well.

    pData->psql = psql;
	return;

fail:
    SG_ERR_IGNORE(  SG_dag_sqlite3__nullclose(pCtx, &psql)  );
	SG_STRING_NULLFREE(pCtx, pstr_prop);
	SG_NULLFREE(pCtx, pszTrivialHash_Computed);
	SG_PATHNAME_NULLFREE(pCtx, pPathnameSqlDb);
    if(pData!=NULL)
    {
        SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pData->prb_sql, (SG_free_callback *)sg_sqlite__close);
        SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pData->prb_paths, (SG_free_callback *)SG_pathname__free);
        SG_PATHNAME_NULLFREE(pCtx, pData->pPathParentDir);
        SG_PATHNAME_NULLFREE(pCtx, pData->pPathMyDir);
    	SG_NULLFREE(pCtx, pData);
    }

	// cannot call SG_ERR_CHECK() in fail block.
    SG_ERR_IGNORE(  SG_repo__set_instance_data(pCtx, pRepo, NULL)  );
}

//////////////////////////////////////////////////////////////////

// For FS3-based REPOs stored in the CLOSET, we create a unique
// directory and store everything within it.

// TODO review these names and see if we want to define/register
// TODO more meaningful suffixes.  For example, on the Mac we can
// TODO tell Finder to hide the contents of a directory (like it
// TODO does for .app directories).

#define sg_MY_CLOSET_DIR_LENGTH_RANDOM		4
#define sg_MY_CLOSET_DIR_LENGTH_DESCRIPTOR_NAME		8
#define sg_MY_CLOSET_DIR_BUFFER_LENGTH		(sg_MY_CLOSET_DIR_LENGTH_DESCRIPTOR_NAME + 1 + sg_MY_CLOSET_DIR_LENGTH_RANDOM + 4 + 1)	// "fs3_" + <hex_digits> + ".fs3" + NULL

static void _my_create_name_dir(SG_context * pCtx, SG_repo* pRepo, char * pBuf, SG_uint32 lenBuf)
{
	char bufTid[SG_TID_MAX_BUFFER_LENGTH];
    char buf_clean_descriptor_name[sg_MY_CLOSET_DIR_LENGTH_DESCRIPTOR_NAME + 1];
    const char* psz_descriptor_name = NULL;
    const char* p = NULL;
    char* q = NULL;

	SG_ASSERT(  (lenBuf >= sg_MY_CLOSET_DIR_BUFFER_LENGTH)  );

    SG_ERR_CHECK_RETURN(  SG_repo__get_descriptor_name(pCtx, pRepo, &psz_descriptor_name)  );

    if (psz_descriptor_name)
    {
        p = psz_descriptor_name;
        q = buf_clean_descriptor_name;

        while (
                (*p)
                && ( (q - buf_clean_descriptor_name) < sg_MY_CLOSET_DIR_LENGTH_DESCRIPTOR_NAME)
              )
        {
            char c = *p++;

            switch (c)
            {
                case ' ':
                case '\t':
                case '\r':
                case '\n':
                case ':':
                case '/':
                case '\\':
                case '~':
                case '!':
                case '@':
                case '#':
                case '$':
                case '%':
                case '^':
                case '&':
                case '*':
                case '(':
                case ')':
                case '{':
                case '}':
                case '[':
                case ']':
                case '.':
                case '?':
                case '<':
                case '>':
                case '|':
                case '"':
                case '\'':
                    c = '_';
            }

            *q++ = c;
        }
        *q = 0;
    }
    else
    {
        buf_clean_descriptor_name[0] = 0;
    }

	SG_ERR_CHECK_RETURN(  SG_tid__generate2(pCtx, bufTid, sizeof(bufTid), sg_MY_CLOSET_DIR_LENGTH_RANDOM)  );

	SG_ERR_CHECK_RETURN(  SG_strcpy(pCtx, pBuf, lenBuf, buf_clean_descriptor_name)  );
	SG_ERR_CHECK_RETURN(  SG_strcat(pCtx, pBuf, lenBuf, "_")  );	// skip leading 't' in TID
	SG_ERR_CHECK_RETURN(  SG_strcat(pCtx, pBuf, lenBuf, &bufTid[SG_TID_LENGTH_PREFIX])  );	// skip leading 't' in TID
}

static void sg_repo__fs3__create_deferred_sqlite_indexes(
        SG_context * pCtx,
        my_instance_data* pData
        )
{
    SG_uint64* paDagNums = NULL;
    SG_uint32 count_dagnums = 0;
    SG_uint32 i = 0;

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pData->psql, "CREATE INDEX IF NOT EXISTS audits_idx_dagnum ON audits (dagnum)")  );
    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pData->psql, "CREATE UNIQUE INDEX IF NOT EXISTS no_dup_audits ON audits (csid, userid, timestamp)")  );
	
    SG_ERR_CHECK(  SG_dag_sqlite3__list_dags(pCtx, pData->psql, &count_dagnums, &paDagNums)  );

    for (i = 0; i < count_dagnums; i++)
    {
        SG_ERR_CHECK(  sg_dag_sqlite3__create_deferred_indexes(pCtx, pData->psql, paDagNums[i])  );
    }

fail:
	SG_NULLFREE(pCtx, paDagNums);
}

void sg_repo__fs3__create_repo_instance(
        SG_context * pCtx,
        SG_repo * pRepo,
        SG_bool b_indexes,
        const char* psz_hash_method,
        const char* psz_repo_id,
        const char* psz_admin_id
        )
{
	my_instance_data * pData = NULL;
	SG_pathname * pPathnameSqlDb = NULL;
    const char* pszParentDir = NULL;
	char * pszTrivialHash = NULL;
	char buf_subdir_name[sg_MY_CLOSET_DIR_BUFFER_LENGTH];
    SG_pathname* pPath_maybe = NULL;
    SG_vhash* pvh_descriptor = NULL;

	SG_UNUSED(b_indexes);

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(psz_repo_id);
	SG_NULLARGCHECK_RETURN(psz_admin_id);

	SG_ERR_CHECK_RETURN(  SG_gid__argcheck(pCtx, psz_repo_id)  );
	SG_ERR_CHECK_RETURN(  SG_gid__argcheck(pCtx, psz_admin_id)  );

    {
        void* pDataLocal = NULL;

        SG_ERR_CHECK_RETURN(  SG_repo__get_instance_data(pCtx, pRepo, &pDataLocal)  );
        SG_ARGCHECK(pDataLocal == NULL, pRepo);
    }

	// Validate the HASH-METHOD.  that is, does this repo implementation support that hash-method.
	// We use the given hash-method to compute a TRIVIAL-HASH.  This does 2 things.  First, it
	// verifies that the hash-method is not bogus.  Second, it gives us a "known answer" that we
	// can put in the "props" (aka "id") table for later consistency checks when pushing/pulling.
	// Note that we cannot just use the REPO API hash routines because we haven't set pRepo->pData
	// and we haven't set pRepo->pData->buf_hash_method, so we do it directly.

	SG_ERR_CHECK(  sg_repo_utils__one_step_hash__from_sghash(pCtx, psz_hash_method, 0, NULL, &pszTrivialHash)  );

	// allocate and install pData
	SG_ERR_CHECK(  SG_alloc1(pCtx, pData)  );
    pData->pRepo = pRepo;

    SG_ERR_CHECK(  SG_rbtree__alloc(pCtx, &pData->prb_paths)  );
    SG_ERR_CHECK(  SG_rbtree__alloc(pCtx, &pData->prb_sql)  );

    SG_ERR_CHECK(  SG_repo__get_descriptor__ref(pCtx, pRepo, &pvh_descriptor)  );

    /* We were given a parent dir.  Make sure it exists. */
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_descriptor, SG_RIDESC_FSLOCAL__PATH_PARENT_DIR, &pszParentDir)  );
	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pData->pPathParentDir, pszParentDir)  );
	SG_ERR_CHECK(  SG_fsobj__verify_directory_exists_on_disk__pathname(pCtx, pData->pPathParentDir)  );

	/* Create the subdir name and make sure it exists on disk. */
    while (1)
    {
        SG_bool b_exists = SG_FALSE;

        SG_ERR_CHECK(  _my_create_name_dir(pCtx, pRepo, buf_subdir_name, sizeof(buf_subdir_name))  );
        SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_maybe, pData->pPathParentDir, buf_subdir_name)  );
        SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_maybe, &b_exists, NULL, NULL)  );
        if (!b_exists)
        {
            pData->pPathMyDir = pPath_maybe;
            pPath_maybe = NULL;
            break;
        }
        SG_PATHNAME_NULLFREE(pCtx, pPath_maybe);
    }
	SG_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pData->pPathMyDir)  );

    /* store the subdir name in the descriptor */
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_descriptor, SG_RIDESC_FSLOCAL__DIR_NAME, buf_subdir_name)  );

    /* Tell the dag code to create its sql db */
	SG_ERR_CHECK(  _my_make_db_pathname(pCtx, pData,&pPathnameSqlDb)  );
    SG_ERR_CHECK(  SG_dag_sqlite3__create(pCtx, pPathnameSqlDb, &pData->psql)  );

    // Now we add our stuff into that same db
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pData->psql, "BEGIN TRANSACTION")  );
    pData->b_in_sqlite_transaction = SG_TRUE;

    SG_ERR_CHECK(  SG_blobset__create_blobs_table(pCtx, pData->psql)  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pData->psql, "CREATE TABLE audits (csid VARCHAR NOT NULL, dagnum INTEGER NOT NULL, userid VARCHAR NOT NULL, timestamp INTEGER NOT NULL)")  );

    pData->b_new_audits = SG_TRUE;

    // We could put a UNIQUE constraint on (filenumber, offset, len_encoded)
    // but it slows things down a lot.
    //
    // We don't need to explicitly ask for an INDEX on hid because the UNIQUE
    // constraint implies one.
    //
    // Any other indexes on this table are not necessary and would slow
    // things down.

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pData->psql, ("CREATE TABLE \"props\""
		"   ("
		"     \"name\" VARCHAR PRIMARY KEY,"
		"     \"value\" VARCHAR NOT NULL"
		"   )"))  );
    // TODO surely we don't really need this index?
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pData->psql, ("CREATE INDEX \"props_name\" on \"props\" ( \"name\" )"))  );

	/* Store repoid and adminid in the DB and in memory. */
	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pData->psql, "INSERT INTO \"props\" (\"name\", \"value\") VALUES ('%s', '%s')", "hashmethod", psz_hash_method)  );
	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pData->psql, "INSERT INTO \"props\" (\"name\", \"value\") VALUES ('%s', '%s')", "repoid", psz_repo_id)  );
	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pData->psql, "INSERT INTO \"props\" (\"name\", \"value\") VALUES ('%s', '%s')", "adminid", psz_admin_id)  );
	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pData->psql, "INSERT INTO \"props\" (\"name\", \"value\") VALUES ('%s', '%s')", "trivialhash", pszTrivialHash)  );

	SG_ERR_CHECK(  SG_strcpy(pCtx, pData->buf_hash_method, sizeof(pData->buf_hash_method), psz_hash_method)  );
	SG_ERR_CHECK(  SG_strcpy(pCtx, pData->buf_repo_id, sizeof(pData->buf_repo_id), psz_repo_id)  );
	SG_ERR_CHECK(  SG_strcpy(pCtx, pData->buf_admin_id, sizeof(pData->buf_admin_id), psz_admin_id)  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pData->psql, "COMMIT TRANSACTION")  );
    pData->b_in_sqlite_transaction = SG_FALSE;

	pData->strlen_hashes = SG_STRLEN(pszTrivialHash);

	SG_PATHNAME_NULLFREE(pCtx, pPathnameSqlDb);
	SG_NULLFREE(pCtx, pszTrivialHash);

    SG_ERR_CHECK(  SG_repo__set_instance_data(pCtx, pRepo, pData)  );

	return;

fail:
    /* TODO close psql */
    if (pData && pData->b_in_sqlite_transaction)
    {
        SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, pData->psql, "ROLLBACK TRANSACTION")  );
        pData->b_in_sqlite_transaction = SG_FALSE;
    }
	SG_NULLFREE(pCtx, pData);
	SG_NULLFREE(pCtx, pszTrivialHash);
    SG_PATHNAME_NULLFREE(pCtx, pPath_maybe);
}

void sg_repo__fs3__delete_repo_instance(
    SG_context * pCtx,
    SG_repo * pRepo
    )
{
	my_instance_data * pData = NULL;
    SG_pathname * pPath = NULL;

    SG_ASSERT(pCtx!=NULL);
	SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__COPY(pCtx, &pPath, pData->pPathMyDir)  );

    SG_ERR_CHECK(  sg_repo__fs3__close_repo_instance(pCtx, pRepo)  );

    SG_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname(pCtx, pPath)  );

    SG_PATHNAME_NULLFREE(pCtx, pPath);

    return;
fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath);
}

void sg_fs3__remove_lock_files(
	SG_context * pCtx,
	my_tx_data* ptx
    );

void sg_fs3__close_file_handles(
	SG_context * pCtx,
	my_tx_data* ptx
    );

static void sg_fs3__nullfree_tx_data(SG_context* pCtx, my_instance_data* pData)
{
    if (!pData->ptx)
    {
        return;
    }

    SG_VHASH_NULLFREE(pCtx, pData->ptx->cloning.pvh_templates);
    SG_ERR_CHECK_RETURN(  sg_sqlite__nullfinalize(pCtx, &pData->ptx->cloning.pStmt_insert)  );
    SG_ERR_CHECK_RETURN(  sg_sqlite__nullfinalize(pCtx, &pData->ptx->pStmt_audits)  );
    if (pData->ptx->psql_new_audits)
    {
        SG_ERR_CHECK_RETURN(  sg_sqlite__close(pCtx, pData->ptx->psql_new_audits)  );
        pData->ptx->psql_new_audits = NULL;
    }
	SG_ERR_CHECK(  sg_fs3__close_file_handles(pCtx, pData->ptx)  );
    SG_RBTREE_NULLFREE(pCtx, (pData->ptx)->prb_file_handles);
	SG_ERR_CHECK(  sg_fs3__remove_lock_files(pCtx, pData->ptx)  );
    SG_RBTREE_NULLFREE(pCtx, (pData->ptx)->prb_append_locks);
    SG_BLOBSET_NULLFREE(pCtx, pData->ptx->pbs_new_blobs);

	if (pData->ptx->flags & SG_REPO_TX_FLAG__CLONING)
	{
		SG_RBTREE_NULLFREE(pCtx, pData->ptx->cloning.prb_dbndx_update);
		SG_VHASH_NULLFREE(pCtx, pData->ptx->cloning.pvh_templates);
		SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pData->ptx->cloning.pStmt_insert));
	}

    SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, (pData->ptx)->prb_frags, (SG_free_callback *)SG_dagfrag__free);

    SG_ERR_CHECK_RETURN(  sg_blob_fs3_handle_store__free(pCtx, pData->ptx->pBlobStoreHandle)  );

    SG_NULLFREE(pCtx, pData->ptx);
    pData->ptx = NULL;

fail:
    ;
}

void sg_repo__fs3__close_repo_instance(SG_context * pCtx, SG_repo * pRepo)
{
	my_instance_data * pData = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK_RETURN(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );
    SG_ERR_CHECK_RETURN(  SG_repo__set_instance_data(pCtx, pRepo, NULL)  );

	if (!pData)					// no instance data implies that the sql db is not open
		return;					// just ignore close.

	// TODO should we assert that we are not in a transaction?  YES, or error code.

    SG_ERR_CHECK_RETURN(  sg_sqlite__exec(pCtx, pData->psql, "PRAGMA journal_mode=WAL")  );

    SG_ERR_CHECK_RETURN(  SG_dag_sqlite3__nullclose(pCtx, &pData->psql)  );

    if (pData->ptx)
    {
        SG_ERR_CHECK_RETURN(  sg_fs3__nullfree_tx_data(pCtx, pData)  );
    }

	SG_PATHNAME_NULLFREE(pCtx, pData->pPathParentDir);
	SG_PATHNAME_NULLFREE(pCtx, pData->pPathMyDir);

    SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pData->prb_paths, (SG_free_callback *)SG_pathname__free);
    SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pData->prb_sql, (SG_free_callback *)sg_sqlite__close);

	SG_NULLFREE(pCtx, pData);

	SG_ERR_REPLACE(SG_ERR_SQLITE(SQLITE_BUSY), SG_ERR_DB_BUSY);
}

void sg_repo__fs3__fetch_dagnode(SG_context * pCtx, SG_repo * pRepo, SG_uint64 iDagNum, const char* pszidHidChangeset, SG_dagnode ** ppNewDagnode)	// must match FN_
{
	my_instance_data * pData = NULL;

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_in_sqlite_transaction)
    {
        SG_RETRY_THINGIE(
                SG_dag_sqlite3__fetch_dagnode(pCtx,pData->psql, iDagNum, pszidHidChangeset, ppNewDagnode)
                );
    }
    else
    {
        SG_ERR_CHECK(  SG_dag_sqlite3__fetch_dagnode(pCtx,pData->psql, iDagNum, pszidHidChangeset, ppNewDagnode)  );
    }

fail:
    return;
}

void sg_repo__fs3__find_dag_path(
	SG_context* pCtx,
	SG_repo * pRepo, 
	SG_uint64 iDagNum,
    const char* psz_id_min,
    const char* psz_id_max,
    SG_varray** ppva
	)
{
	my_instance_data * pData = NULL;
	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

	if (!pData->b_in_sqlite_transaction)
	{
		SG_RETRY_THINGIE(
			SG_dag_sqlite3__find_dag_path(pCtx, pData->psql, iDagNum, psz_id_min, psz_id_max, ppva)
			);
	}
	else
	{
		SG_ERR_CHECK(  SG_dag_sqlite3__find_dag_path(pCtx, pData->psql, iDagNum, psz_id_min, psz_id_max, ppva)  );
	}

fail:
	;
}

void sg_repo__fs3__fetch_dagnodes__begin(
	SG_context* pCtx,
	SG_repo * pRepo, 
	SG_uint64 iDagNum,
	SG_repo_fetch_dagnodes_handle ** ppHandle)
{
	my_instance_data * pData = NULL;
	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

	if (!pData->b_in_sqlite_transaction)
	{
		SG_RETRY_THINGIE(
			SG_dag_sqlite3__fetch_dagnodes__begin(pCtx, iDagNum, (SG_dag_sqlite3_fetch_dagnodes_handle**)ppHandle)
			);
	}
	else
	{
		SG_ERR_CHECK(  SG_dag_sqlite3__fetch_dagnodes__begin(pCtx, iDagNum, (SG_dag_sqlite3_fetch_dagnodes_handle**)ppHandle)  );
	}

fail:
	;
}

void sg_repo__fs3__fetch_dagnodes__one(
	SG_context* pCtx,
	SG_repo * pRepo,
	SG_repo_fetch_dagnodes_handle * pHandle,
	const char * pszidHidChangeset,
	SG_dagnode** ppdn)
{
	my_instance_data * pData = NULL;

	SG_ERR_CHECK_RETURN(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );
	SG_ERR_CHECK_RETURN(  SG_dag_sqlite3__fetch_dagnodes__one(pCtx, pData->psql, (SG_dag_sqlite3_fetch_dagnodes_handle*)pHandle, pszidHidChangeset, ppdn)  );
}

void sg_repo__fs3__fetch_dagnodes__end(
	SG_context* pCtx,
	SG_repo * pRepo, 
	SG_repo_fetch_dagnodes_handle ** ppHandle)
{
	SG_UNUSED(pRepo);
	SG_ERR_CHECK_RETURN(  SG_dag_sqlite3__fetch_dagnodes__end(pCtx, (SG_dag_sqlite3_fetch_dagnodes_handle**)ppHandle)  );
}

void sg_repo__fs3__list_dags(SG_context * pCtx, SG_repo* pRepo, SG_uint32* piCount, SG_uint64** paDagNums)
{
	my_instance_data * pData = NULL;
	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_RETRY_THINGIE(
            SG_dag_sqlite3__list_dags(pCtx, pData->psql, piCount, paDagNums)
            );

fail:
    return;
}

void sg_fs3__list_dags(SG_context * pCtx, SG_repo* pRepo, SG_uint32* piCount, SG_uint64** paDagNums)
{
	my_instance_data * pData = NULL;
	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_ERR_CHECK(  SG_dag_sqlite3__list_dags(pCtx, pData->psql, piCount, paDagNums)  );

fail:
    return;
}

void sg_repo__fs3__fetch_dag_leaves(SG_context * pCtx, SG_repo * pRepo, SG_uint64 iDagNum, SG_rbtree ** ppIdsetReturned)	// must match FN_
{
	my_instance_data * pData = NULL;
	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_in_sqlite_transaction)
    {
        SG_RETRY_THINGIE(
                SG_dag_sqlite3__fetch_leaves(pCtx, pData->psql, iDagNum, ppIdsetReturned)
                );
    }
    else
    {
        SG_ERR_CHECK(  SG_dag_sqlite3__fetch_leaves(pCtx, pData->psql, iDagNum, ppIdsetReturned)  );
    }

fail:
    return;
}

void sg_repo__fs3__fetch_dagnode_children(SG_context * pCtx, SG_repo * pRepo, SG_uint64 iDagNum, const char* pszDagnodeID, SG_rbtree ** ppIdsetReturned)	// must match FN_
{
	my_instance_data * pData = NULL;
	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_RETRY_THINGIE(
            SG_dag_sqlite3__fetch_dagnode_children(pCtx, pData->psql, iDagNum, pszDagnodeID, ppIdsetReturned)
            );

fail:
    return;
}

void sg_repo__fs3__find_new_dagnodes_since(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint64 iDagNum,
    SG_varray* pva_starting,
    SG_ihash** ppih
	)
{
	my_instance_data * pData = NULL;
	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

	SG_RETRY_THINGIE(
		SG_dag_sqlite3__find_new_dagnodes_since(pCtx, pData->psql, iDagNum, pva_starting, ppih)
		);

fail:
	return;
}

void sg_repo__fs3__fetch_dagnode_ids(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint64 iDagNum,
    SG_int32 gen_min,
    SG_int32 gen_max,
    SG_ihash** ppih
    )
{
	my_instance_data * pData = NULL;
	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

	SG_RETRY_THINGIE(
		SG_dag_sqlite3__fetch_dagnode_ids(pCtx, pData->psql, iDagNum, gen_min, gen_max, ppih)
		);

fail:
	return;
}

void sg_repo__fs3__fetch_chrono_dagnode_list(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_uint64 iDagNum,
	SG_uint32 iStartRevNo,
	SG_uint32 iNumRevsRequested,
	SG_uint32* piNumRevsReturned,
	char*** ppaszHidsReturned)
{
	my_instance_data * pData = NULL;
	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

	SG_RETRY_THINGIE(
		SG_dag_sqlite3__fetch_chrono_dagnode_list(pCtx, pData->psql, iDagNum, iStartRevNo, iNumRevsRequested, piNumRevsReturned, ppaszHidsReturned)
		);

fail:
	return;
}

void sg_repo__fs3__get_dag_lca(
    SG_context * pCtx,
    SG_repo* pRepo,
    SG_uint64 iDagNum,
    const SG_rbtree* prbNodes,
    SG_daglca ** ppDagLca
    )
{
	my_instance_data * pData = NULL;
	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (pData->b_in_sqlite_transaction)
    {
        SG_ERR_CHECK(  SG_dag_sqlite3__get_lca(pCtx, pData->psql, iDagNum, prbNodes, ppDagLca)  );
    }
    else
    {
        SG_RETRY_THINGIE(
            SG_dag_sqlite3__get_lca(pCtx, pData->psql, iDagNum, prbNodes, ppDagLca)
            );
    }

fail:
    return;
}

void _fs3_my_get_treendx(SG_context * pCtx, my_instance_data* pData, SG_uint64 iDagNum, SG_bool bQueryOnly, SG_treendx** ppResult)
{
    SG_pathname* pPath = NULL;
    SG_treendx* pndx = NULL;

    SG_ERR_CHECK(  sg_fs3__get_treendx_path(pCtx, pData, iDagNum, &pPath)  );
    SG_ERR_CHECK(  SG_treendx__open(pCtx, pData->pRepo, iDagNum, pPath, bQueryOnly, &pndx)  );
    *ppResult = pndx;

    return;

fail:
    ;
    /* don't free pPath here because the treendx should own it */
}

void sg_repo__fs3__find_dagnodes_by_prefix(SG_context * pCtx, SG_repo * pRepo, SG_uint64 iDagNum, const char* psz_hid_prefix, SG_rbtree** pprb)
{
	my_instance_data * pData = NULL;
	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_RETRY_THINGIE(
        SG_dag_sqlite3__find_by_prefix(pCtx, pData->psql, iDagNum, psz_hid_prefix, pprb)
        );

fail:
    return;
}

void sg_repo__fs3__find_dagnode_by_rev_id(SG_context * pCtx, SG_repo * pRepo, SG_uint64 iDagNum, SG_uint32 iRevisionId, char** ppsz_hid)
{
	my_instance_data * pData = NULL;
	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

	SG_RETRY_THINGIE(
		SG_dag_sqlite3__find_by_rev_id(pCtx, pData->psql, iDagNum, iRevisionId, ppsz_hid)
		);

fail:
	return;
}

void sg_repo__fs3__list_blobs(
    SG_context * pCtx,
    SG_repo * pRepo,
    SG_blob_encoding blob_encoding, /**< only return blobs of this encoding */
    SG_uint32 limit,                /**< only return this many entries */
    SG_uint32 offset,               /**< skip the first group of the sorted entries */
    SG_blobset** ppbs               /**< Caller must free */
    )
{
	my_instance_data * pData = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(ppbs);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_RETRY_THINGIE(
        sg_blob_fs3__list_blobs(pCtx, pData->psql, blob_encoding, limit, offset, ppbs)
        );

fail:
    return;
}

void sg_repo__fs3__find_blobs_by_prefix(SG_context * pCtx, SG_repo * pRepo, const char* psz_hid_prefix, SG_rbtree** pprb)
{
	my_instance_data * pData = NULL;

	SG_NULLARGCHECK_RETURN(psz_hid_prefix);
	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(pprb);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_RETRY_THINGIE(
        sg_blob_fs3__sql__find_by_hid_prefix(pCtx, pData->psql, psz_hid_prefix, pprb)
        );

fail:
    return;
}

void sg_repo__fs3__get_repo_id(SG_context * pCtx, SG_repo * pRepo, char** ppsz_id)
{
	my_instance_data * pData = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );
#if defined(DEBUG)
	// buf_repo_id is a GID.  it should have been checked when it was stored.
	// checking now for sanity during GID/HID conversion.
	SG_ERR_CHECK(  SG_gid__argcheck(pCtx, pData->buf_repo_id)  );
#endif
    SG_ERR_CHECK(  SG_STRDUP(pCtx, pData->buf_repo_id, ppsz_id)  );

    return;
fail:
    return;
}

void sg_repo__fs3__get_admin_id(SG_context * pCtx, SG_repo * pRepo, char** ppsz_id)
{
	my_instance_data * pData = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );
#if defined(DEBUG)
	// buf_admin_id is a GID.  it should have been checked when it was stored.
	// checking now for sanity during GID/HID conversion.
	SG_ERR_CHECK(  SG_gid__argcheck(pCtx, pData->buf_admin_id)  );
#endif
    SG_ERR_CHECK(  SG_STRDUP(pCtx, pData->buf_admin_id, ppsz_id)  );

    return;
fail:
    return;
}

void sg_repo__fs3__store_blob__begin(
    SG_context * pCtx,
    SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    SG_blob_encoding blob_encoding,
    const char* psz_hid_vcdiff_reference,
    SG_uint64 lenFull,
	SG_uint64 lenEncoded,
    const char* psz_hid_known,
    SG_repo_store_blob_handle** ppHandle
    )
{
    sg_blob_fs3_handle_store* pbh = NULL;
	my_instance_data * pData = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(pTx);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

	if (!pData->ptx)
	{
		SG_ERR_THROW(  SG_ERR_NO_REPO_TX  );
	}

    if (pTx != (SG_repo_tx_handle*) pData->ptx)
    {
		SG_ERR_THROW(  SG_ERR_BAD_REPO_TX_HANDLE  );
    }

    SG_ERR_CHECK(  sg_blob_fs3__store_blob__begin(pCtx, pData, blob_encoding, psz_hid_vcdiff_reference, lenFull, lenEncoded, psz_hid_known, &pbh)  );

    *ppHandle = (SG_repo_store_blob_handle*) pbh;

    return;
fail:
    return;
}

void sg_repo__fs3__store_blob__chunk(
    SG_context * pCtx,
    SG_repo * pRepo,
    SG_repo_store_blob_handle* pHandle,
    SG_uint32 len_chunk,
    const SG_byte* p_chunk,
    SG_uint32* piWritten
    )
{
	SG_NULLARGCHECK_RETURN(pRepo);

    SG_ERR_CHECK(  sg_blob_fs3__store_blob__chunk(pCtx, (sg_blob_fs3_handle_store*) pHandle, len_chunk, p_chunk, piWritten)  );

    return;
fail:
    return;
}

void sg_repo__fs3__store_blob__end(
    SG_context * pCtx,
    SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    SG_repo_store_blob_handle** ppHandle,
    char** ppsz_hid_returned
    )
{
	my_instance_data * pData = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(pTx);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );
	pData->ptx->pBlobStoreHandle = NULL;

    SG_ERR_CHECK(  sg_blob_fs3__store_blob__end(pCtx, (sg_blob_fs3_handle_store**) ppHandle, ppsz_hid_returned)  );

    return;
fail:
    return;
}

void sg_repo__fs3__store_blob__abort(
    SG_context * pCtx,
    SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    SG_repo_store_blob_handle** ppHandle
    )
{
	my_instance_data * pData = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(pTx);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );
	pData->ptx->pBlobStoreHandle = NULL;

    SG_ERR_CHECK(  sg_blob_fs3__store_blob__abort(pCtx, (sg_blob_fs3_handle_store**) ppHandle)  );

    return;
fail:
    return;
}

void sg_repo__fs3__fetch_blob__begin(
    SG_context * pCtx,
    SG_repo * pRepo,
    const char* psz_hid_blob,
    SG_bool b_convert_to_full,
    SG_blob_encoding* pBlobFormat,
    char** ppsz_hid_vcdiff_reference,
    SG_uint64* pLenRawData,
    SG_uint64* pLenFull,
    SG_repo_fetch_blob_handle** ppHandle
    )
{
	my_instance_data * pData = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK_RETURN(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_ERR_CHECK_RETURN(  sg_blob_fs3__fetch_blob__begin(
                pCtx,
                pData,
                psz_hid_blob,
                b_convert_to_full,
                pBlobFormat,
                ppsz_hid_vcdiff_reference,
                pLenRawData,
                pLenFull,
				SG_TRUE,
                (sg_blob_fs3_handle_fetch**) ppHandle
                )  );
}

void sg_repo__fs3__fetch_blob__chunk(
    SG_context * pCtx,
    SG_repo * pRepo,
    SG_repo_fetch_blob_handle* pHandle,
    SG_uint32 len_buf,
    SG_byte* p_buf,
    SG_uint32* p_len_got,
    SG_bool* pb_done
    )
{
	SG_NULLARGCHECK_RETURN(pRepo);

    SG_ERR_CHECK(  sg_blob_fs3__fetch_blob__chunk(pCtx, (sg_blob_fs3_handle_fetch*) pHandle, len_buf, p_buf, p_len_got, pb_done)  );

    return;
fail:
    return;
}

void sg_repo__fs3__fetch_blob__end(
    SG_context * pCtx,
    SG_repo * pRepo,
    SG_repo_fetch_blob_handle** ppHandle
    )
{
	SG_NULLARGCHECK_RETURN(pRepo);

    SG_ERR_CHECK(  sg_blob_fs3__fetch_blob__end(pCtx, (sg_blob_fs3_handle_fetch**) ppHandle)  );

    return;
fail:
    return;
}

void sg_repo__fs3__fetch_blob__abort(
    SG_context * pCtx,
    SG_repo * pRepo,
    SG_repo_fetch_blob_handle** ppHandle
    )
{
	my_instance_data * pData = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_ERR_CHECK(  sg_blob_fs3__fetch_blob__abort(pCtx, (sg_blob_fs3_handle_fetch**) ppHandle)  );

    // TODO I don't think the following is needed
    if (pData->b_in_sqlite_transaction)
    {
        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pData->psql, ("ROLLBACK TRANSACTION"))  );
        pData->b_in_sqlite_transaction = SG_FALSE;
    }

    return;
fail:
    return;
}

static void sg_fs3__get_committed_blobfile_lengths(
        SG_context* pCtx, 
        my_instance_data* pData,
        SG_vhash** ppvhLens
        )
{
	SG_vhash* pvh = NULL;
	sqlite3_stmt* pStmt = NULL;
	int rc = -1;

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, 
		pData->psql,
		&pStmt,
		"SELECT blobs.filename, blobs.offset, blobs.len_encoded "
		"FROM blobs "
		"INNER JOIN (SELECT filename, MAX(offset) offset FROM blobs GROUP BY filename) mo "
		"ON blobs.filename = mo.filename AND blobs.offset = mo.offset;")  );

	while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
	{
		SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, (const char*)sqlite3_column_text(pStmt, 0), sqlite3_column_int64(pStmt, 1) + sqlite3_column_int64(pStmt, 2))  );
	}
	if (rc != SQLITE_DONE)
	{
		SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
	}

	*ppvhLens = pvh;
	pvh = NULL;

	/* fall through */
fail:
	SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
	SG_VHASH_NULLFREE(pCtx, pvh);
}

static void is_hardwired_without_template(
	SG_context* pCtx,
    SG_uint64 dagnum,
    SG_bool *pb
    )
{
    SG_bool b = SG_FALSE;

    if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(dagnum))
    {
        SG_zingtemplate* pzt = NULL;
        SG_zing__get_cached_template__static_dagnum(pCtx, dagnum, &pzt);
        if (
                SG_CONTEXT__HAS_ERR(pCtx)
                && (SG_context__err_equals(pCtx, SG_ERR_ZING_TEMPLATE_NOT_FOUND))
                )
        {
            SG_context__err_reset(pCtx);
            b = SG_TRUE;
        }
        else
        {
            SG_ERR_CHECK_CURRENT;
        }
    }

    *pb = b;

fail:
    ;
}

static void _build_bindex(
	SG_context* pCtx,
	my_instance_data* pData,
    SG_pathname* pPath_dir,
	SG_fragball_writer* pFragballWriter,
    SG_pathname** pp,
	SG_vhash** ppvh_offsets
    )
{
	sqlite3 * psql = NULL;
    char buf_tid[SG_TID_MAX_BUFFER_LENGTH];
    SG_pathname* pPath = NULL;
    SG_uint32 count_files = 0;
    SG_uint32 i = 0;
    SG_uint64* paDagNums = NULL;
    SG_uint32 count_dagnums;
    SG_varray* pva_templates = NULL;

	SG_vhash* pvh_blobfile_offsets = NULL;
	SG_vhash* pvh_blobfile_lens = NULL;
	SG_pathname* pPath_bfile = NULL;

	// This index is only useful to clone, so we create it when a repo is first cloned. Desktop repos that are never cloned from never get it, on purpose.
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pData->psql, "CREATE INDEX IF NOT EXISTS blobs_idx_filename_offset ON blobs (filename, offset)")  );

	SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_tid, sizeof(buf_tid))  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pPath_dir, buf_tid)  );
    SG_ERR_CHECK(  sg_sqlite__create__pathname(pCtx, pPath, SG_SQLITE__SYNC__OFF, &psql)  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "CREATE TABLE dags (dagnum VARCHAR NOT NULL)")  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "CREATE TABLE templates (hid VARCHAR NOT NULL, dagnum VARCHAR NOT NULL)")  );
	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, psql, "CREATE TABLE \"blobs\""
                              "   ("
                              "     \"hid\"                VARCHAR        NOT NULL,"
                              "     \"offset\"             INTEGER            NULL,"
                              "     \"encoding\"           INTEGER            NULL,"
                              "     \"len_encoded\"        INTEGER            NULL,"
                              "     \"len_full\"           INTEGER            NULL,"
                              "     \"hid_vcdiff\"         VARCHAR            NULL"
                              "   )")  );
    SG_ERR_CHECK(  sg_sqlite__close(pCtx, psql)  );
    psql = NULL;

    SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pData->psql, "ATTACH DATABASE '%s' AS %s", SG_pathname__sz(pPath), buf_tid )  );

	SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pData->psql, "BEGIN TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pData->b_in_sqlite_transaction = SG_TRUE;

	SG_ERR_CHECK(  sg_fs3__get_committed_blobfile_lengths(pCtx, pData, &pvh_blobfile_lens)  );
	SG_ERR_CHECK(  SG_VHASH__ALLOC__COPY(pCtx, &pvh_blobfile_offsets, pvh_blobfile_lens)  );
	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_blobfile_offsets, &count_files)  );

	SG_ERR_CHECK(  SG_dag_sqlite3__list_dags(pCtx, pData->psql, &count_dagnums, &paDagNums)  );
    for (i = 0; i < count_dagnums; i++)
    {
        {
            char buf_dagnum[48];

            SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, paDagNums[i], buf_dagnum, sizeof(buf_dagnum))  );

            if (SG_DAGNUM__IS_DB(paDagNums[i]))
            {
                if (!SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(paDagNums[i]))
                {
                    SG_uint32 count_templates = 0;
                    SG_uint32 i_template = 0;

                    SG_ERR_CHECK(  sg_fs3_get_list_of_templates(pCtx, pData->pRepo, paDagNums[i], &pva_templates)  );
                    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_templates, &count_templates)  );
                    for (i_template=0; i_template<count_templates; i_template++)
                    {
                        const char* psz_hid = NULL;

                        SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_templates, i_template, &psz_hid)  );

                        SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pData->psql, "INSERT INTO %s.templates (hid, dagnum) VALUES ('%s', '%s')", buf_tid, psz_hid, buf_dagnum)  );
                    }
                    SG_VARRAY_NULLFREE(pCtx, pva_templates);
                }
            }
        }
    }

    for (i = 0; i < count_dagnums; i++)
    {
        {
            char buf_dagnum[48];
            SG_bool b_skip = SG_FALSE;

            SG_ERR_CHECK(  is_hardwired_without_template(pCtx, paDagNums[i], &b_skip)  );
            if (!b_skip)
            {
                SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, paDagNums[i], buf_dagnum, sizeof(buf_dagnum))  );

                SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pData->psql, "INSERT INTO %s.dags (dagnum) VALUES ('%s')", buf_tid, buf_dagnum)  );

                SG_ERR_CHECK(  sg_fs3_get_fragball_stuff(pCtx, pData->psql, paDagNums[i], buf_tid)  );
            }
        }
    }

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pData->psql, ("COMMIT TRANSACTION"))  );
	pData->b_in_sqlite_transaction = SG_FALSE;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Writing", SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_log__set_steps(pCtx, count_files+1, "chunks")  );

	for (i=0; i<count_files; i++)
	{
		const char* psz_filename = NULL;
		const SG_variant* pv = NULL;
		SG_uint64 len = 0;
		SG_uint64 offset = 0;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_blobfile_offsets, i, &psz_filename, &pv)  );
		SG_ERR_CHECK(  SG_vhash__get__uint64(pCtx, pvh_blobfile_lens, psz_filename, &len)  );

		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_bfile, pData->pPathMyDir, psz_filename)  );
		//SG_ERR_CHECK(  sg_fs3__lock__retry(pCtx, pData, psz_filename, MY_SLEEP_MS, MY_TIMEOUT_MS)  );
		SG_ERR_CHECK(  SG_fragball__write__bfile(pCtx, pFragballWriter, pPath_bfile, psz_filename, len, &offset)  );
		//SG_ERR_CHECK(  sg_fs3__unlock(pCtx, pData->ptx, psz_filename)  );
		SG_ERR_CHECK(  SG_vhash__update__int64(pCtx, pvh_blobfile_offsets, psz_filename, (SG_int64) offset)  );

		SG_PATHNAME_NULLFREE(pCtx, pPath_bfile);
		SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );
	}

	SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pData->psql, "BEGIN TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
	pData->b_in_sqlite_transaction = SG_TRUE;

	for (i=0; i<count_files; i++)
	{
		const char* psz_filename = NULL;
		SG_uint64 offset = 0;
		SG_uint64 len = 0;
		const SG_variant* pv = NULL;
		SG_int_to_string_buffer sz_offset;
		SG_int_to_string_buffer sz_len;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_blobfile_offsets, i, &psz_filename, &pv)  );
		SG_ERR_CHECK(  SG_variant__get__uint64(pCtx, pv, &offset)  );
		SG_int64_to_sz(offset, sz_offset);
		SG_ERR_CHECK(  SG_vhash__get__uint64(pCtx, pvh_blobfile_lens, psz_filename, &len)  );
		SG_int64_to_sz(len, sz_len);

		SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pData->psql, 
			"INSERT INTO %s.blobs (\"hid\", \"offset\", \"encoding\", \"len_encoded\", \"len_full\", \"hid_vcdiff\") "
			"SELECT \"hid\", \"offset\" + %s, \"encoding\", \"len_encoded\", \"len_full\", \"hid_vcdiff\" "
			"FROM blobs b WHERE b.filename = '%s' AND b.offset < %s", 
			buf_tid, sz_offset, psz_filename, sz_len)  );
	}
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pData->psql, ("COMMIT TRANSACTION"))  );
	pData->b_in_sqlite_transaction = SG_FALSE;
	SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pData->psql, "DETACH DATABASE %s", buf_tid)  );

	*pp = pPath;
    pPath = NULL;
	*ppvh_offsets = pvh_blobfile_offsets;
	pvh_blobfile_offsets = NULL;

fail:
    if (pData && pData->b_in_sqlite_transaction)
    {
        SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, pData->psql, ("ROLLBACK TRANSACTION"))  );
		pData->b_in_sqlite_transaction = SG_FALSE;
    }
    SG_VARRAY_NULLFREE(pCtx, pva_templates);
    if (psql)
    {
        SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
    }
    if (pPath && SG_CONTEXT__HAS_ERR(pCtx))
    {
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPath)  );
    }
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_NULLFREE(pCtx, paDagNums);

	SG_VHASH_NULLFREE(pCtx, pvh_blobfile_offsets);
	SG_VHASH_NULLFREE(pCtx, pvh_blobfile_lens);
	SG_PATHNAME_NULLFREE(pCtx, pPath_bfile);
}

static void _build_fragball__dagnodes(
	SG_context* pCtx,
	my_instance_data* pData,
	SG_fragball_writer* pFragballWriter
    )
{
    SG_uint64* paDagNums = NULL;
    SG_uint32 count_dagnums;
    SG_uint32 count_nodes;
	SG_dagfrag* pFrag = NULL;
	char** paszHids = NULL;
    SG_uint32 i, j;
	SG_dag_sqlite3_fetch_dagnodes_handle* pdh = NULL;
	SG_dagnode* pDagnode = NULL;
    SG_bool b_inside_operation = SG_FALSE;
    SG_int32 count_steps = 0;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Writing", SG_LOG__FLAG__NONE)  );
    b_inside_operation = SG_TRUE;

    SG_ERR_CHECK(  SG_dag_sqlite3__list_dags(pCtx, pData->psql, &count_dagnums, &paDagNums)  );

    for (i = 0; i < count_dagnums; i++)
    {
        SG_uint32 count = 0;

        SG_ERR_CHECK(  SG_dag_sqlite3__count_all_dagnodes(pCtx, pData->psql, paDagNums[i], &count)  );
        count_steps += count;
    }
    SG_ERR_CHECK(  SG_log__set_steps(pCtx, (SG_uint32) count_steps, "changesets")  );

    for (i = 0; i < count_dagnums; i++)
    {
        const char* pszHid = NULL;

        SG_ERR_CHECK(  SG_dagfrag__alloc(pCtx, &pFrag, pData->buf_repo_id, pData->buf_admin_id, paDagNums[i])  );
        SG_ERR_CHECK(  SG_dag_sqlite3__fetch_chrono_dagnode_list(pCtx, pData->psql, paDagNums[i], 0, 0, &count_nodes, &paszHids)  );
        
        SG_ERR_CHECK(  SG_dag_sqlite3__fetch_dagnodes__begin(pCtx, paDagNums[i], &pdh)  );
        pszHid = (const char*)paszHids;
        for (j = 0; j < count_nodes; j++)
        {
            SG_ERR_CHECK(  SG_dag_sqlite3__fetch_dagnodes__one(pCtx, pData->psql, pdh, pszHid, &pDagnode)  );
            SG_ERR_CHECK(  SG_dagfrag__add_dagnode(pCtx, pFrag, &pDagnode)  );
            SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );
            pszHid += SG_HID_MAX_BUFFER_LENGTH;
        }
        SG_NULLFREE(pCtx, paszHids);
        SG_ERR_CHECK(  SG_dag_sqlite3__fetch_dagnodes__end(pCtx, &pdh)  );

        SG_ERR_CHECK(  SG_fragball__write__frag(pCtx, pFragballWriter, pFrag)  );
        SG_DAGFRAG_NULLFREE(pCtx, pFrag);
    }

    SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
    b_inside_operation = SG_FALSE;

fail:
    SG_NULLFREE(pCtx, paDagNums);
    SG_NULLFREE(pCtx, paszHids);
	SG_DAGFRAG_NULLFREE(pCtx, pFrag);
	SG_DAGNODE_NULLFREE(pCtx, pDagnode);
	if (pdh)
    {
		SG_ERR_IGNORE(  SG_dag_sqlite3__fetch_dagnodes__end(pCtx, &pdh)  );
    }
    if (b_inside_operation)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
    }
}

static void _build_fragball__v3__blobs(
	SG_context* pCtx,
	my_instance_data* pData,
	SG_fragball_writer* pFragballWriter
    )
{
    SG_pathname* pPath_sqlite = NULL;
    SG_vhash* pvh_offsets = NULL;
    char buf_tid[SG_TID_MAX_BUFFER_LENGTH];
    SG_pathname* pPath_zlib_sqlite = NULL;

    SG_ERR_CHECK(  _build_bindex(pCtx, pData, pData->pPathMyDir, pFragballWriter, &pPath_sqlite, &pvh_offsets)  );

    SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_tid, sizeof(buf_tid))  );
    SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath_zlib_sqlite, pData->pPathMyDir, buf_tid)  );
    SG_ERR_CHECK(  SG_zlib__deflate__file(pCtx, pPath_sqlite, pPath_zlib_sqlite)  );
    SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath_sqlite)  );
    SG_PATHNAME_NULLFREE(pCtx, pPath_sqlite);

    // TODO what vhash should we store with the bindex?  it's not used.
    SG_ERR_CHECK(  SG_fragball__write__bindex(pCtx, pFragballWriter, pPath_zlib_sqlite, pvh_offsets)  );
    SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath_zlib_sqlite)  );
    SG_PATHNAME_NULLFREE(pCtx, pPath_zlib_sqlite);
    SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

fail:
    SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
    SG_VHASH_NULLFREE(pCtx, pvh_offsets);
    if (pPath_sqlite && SG_CONTEXT__HAS_ERR(pCtx))
    {
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPath_sqlite)  );
    }
    if (pPath_zlib_sqlite && SG_CONTEXT__HAS_ERR(pCtx))
    {
		SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPath_zlib_sqlite)  );
    }
    SG_PATHNAME_NULLFREE(pCtx, pPath_sqlite);
    SG_PATHNAME_NULLFREE(pCtx, pPath_zlib_sqlite);
}

static void _build_fragball__blobs(
	SG_context* pCtx,
	my_instance_data* pData,
	SG_fragball_writer* pFragballWriter
    )
{
	sqlite3_stmt* pStmt = NULL;
	int rc;
	sg_blob_fs3_handle_fetch* pbh = NULL;
	SG_file* pBlobFile = NULL;
    SG_uint32 currentFilenum = 0;
    SG_uint64 next_offset = 0;
    SG_bool b_inside_operation = SG_FALSE;
    SG_int32 count_blobs = 0;

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Writing", SG_LOG__FLAG__NONE)  );
    b_inside_operation = SG_TRUE;

	SG_ERR_CHECK(  sg_sqlite__exec__va__int32(pCtx, pData->psql, &count_blobs,
		"SELECT COUNT(hid) FROM blobs"
		)  );
    SG_ERR_CHECK(  SG_log__set_steps(pCtx, (SG_uint32) count_blobs, "blobs")  );

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pData->psql,&pStmt,
                                      "SELECT \"encoding\", \"len_encoded\", \"len_full\", \"hid_vcdiff\", \"filename\", \"offset\", \"hid\" FROM \"blobs\" ORDER BY \"filename\", \"offset\"")  );

    while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        SG_uint32 filenumber = 0;
        SG_uint64 offset = 0;
        SG_blob_encoding blob_encoding = 0;
        SG_uint64 len_encoded = 0;
        SG_uint64 len_full = 0;
        const char* psz_hid_vcdiff_reference = NULL;
        char* psz_hid_vcdiff_reference_freeme = NULL;
        const char* psz_hid_blob = NULL;

        blob_encoding = (SG_blob_encoding) sqlite3_column_int(pStmt, 0);

        len_encoded = sqlite3_column_int64(pStmt, 1);
        len_full = sqlite3_column_int64(pStmt, 2);
        psz_hid_vcdiff_reference = (const char *)sqlite3_column_text(pStmt,3);
        filenumber = sqlite3_column_int(pStmt,4); // will convert string to int
        offset = sqlite3_column_int64(pStmt, 5);
        psz_hid_blob = (const char*)sqlite3_column_text(pStmt, 6);

        if (psz_hid_vcdiff_reference)
        {
            SG_ERR_CHECK(  SG_STRDUP(pCtx, psz_hid_vcdiff_reference, &psz_hid_vcdiff_reference_freeme)  );
        }

        SG_ERR_CHECK(  sg_fs3__open_blob_for_reading__already_got_info(
                    pCtx, 
                    pData,
                    psz_hid_blob,
                    SG_FALSE,
                    SG_FALSE,
                    filenumber,
                    offset,
                    blob_encoding,
                    len_encoded,
                    len_full,
                    &psz_hid_vcdiff_reference_freeme,
                    &pbh
                    )  );

#if 0
        printf("fetch_blob__begin: %s -- %d -- %d\n", psz_hid_blob, (SG_uint32) lenEncoded, (SG_uint32) lenFull);
        total_lenEncoded += lenEncoded;
        total_lenFull += lenFull;
#endif

        /* open blob file and/or seek only when necessary */
        if ( !pBlobFile || (currentFilenum != pbh->filenumber) )
        {
            if (pBlobFile)
                SG_FILE_NULLCLOSE(pCtx, pBlobFile);

            SG_ERR_CHECK(  _open_blobfile_for_reading(pCtx, pData, pbh)  );
            pBlobFile = pbh->m_pFileBlob;
            currentFilenum = pbh->filenumber;
        }
        else
        {
            // Even if we haven't switched files, we'll sometimes have to seek past a hole.
            if (pbh->offset != next_offset)
            {
                SG_ERR_CHECK(  SG_file__seek(pCtx, pBlobFile, pbh->offset)  );
            }

            pbh->m_pFileBlob = pBlobFile;
        }
        next_offset = pbh->offset + len_encoded;

        SG_ERR_CHECK(  SG_fragball__write_blob__from_handle(pCtx, pFragballWriter,
            (SG_repo_fetch_blob_handle**)&pbh, psz_hid_blob, blob_encoding, psz_hid_vcdiff_reference, len_encoded, len_full)  );
        SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );
    }
    if (rc != SQLITE_DONE)
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

	SG_FILE_NULLCLOSE(pCtx, pBlobFile);

    SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
    b_inside_operation = SG_FALSE;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	if (pbh)
    {
		SG_ERR_IGNORE(  sg_blob_fs3__fetch_blob__abort(pCtx, &pbh)  );
    }
	SG_FILE_NULLCLOSE(pCtx, pBlobFile);
    if (b_inside_operation)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
    }
}

static void _build_repo_fragball__v1(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_pathname* pFragballPath
    )
{
	my_instance_data* pData = NULL;
	SG_fragball_writer* pFragballWriter = NULL;

	// Caller should ensure we're inside a sqlite tx.
    
	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_ERR_CHECK(  SG_fragball_writer__alloc(pCtx, pRepo, pFragballPath, SG_TRUE, 1, &pFragballWriter)  );

    SG_ERR_CHECK(  _build_fragball__dagnodes(pCtx, pData, pFragballWriter)  );
    SG_ERR_CHECK(  _build_fragball__blobs(pCtx, pData, pFragballWriter)  );

	SG_ERR_IGNORE(  SG_fragball_writer__free(pCtx, pFragballWriter)  );
    pFragballWriter = NULL;

fail:
	SG_ERR_IGNORE(  SG_fragball_writer__free(pCtx, pFragballWriter)  );
}

static void _build_repo_fragball__v3(
	SG_context* pCtx,
	SG_repo* pRepo,
	SG_pathname* pFragballPath
    )
{
	my_instance_data* pData = NULL;
	SG_fragball_writer* pFragballWriter = NULL;

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_new_audits)
    {
        SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
    }

    SG_ERR_CHECK(  SG_fragball_writer__alloc(pCtx, pRepo, pFragballPath, SG_TRUE, 3, &pFragballWriter)  );

    SG_ERR_CHECK(  _build_fragball__v3__blobs(pCtx, pData, pFragballWriter)  );

	SG_ERR_IGNORE(  SG_fragball_writer__free(pCtx, pFragballWriter)  );
    pFragballWriter = NULL;

fail:
	SG_ERR_IGNORE(  SG_fragball_writer__free(pCtx, pFragballWriter)  );
}

void sg_repo__fs3__fetch_repo__fragball(
	SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint32 version,
	const SG_pathname* pFragballDirPathname,
	char** ppszFragballName)
{
	my_instance_data* pData = NULL;
	SG_pathname* pFragballPathname = NULL;
	SG_string* pstrFragballName = NULL;

    SG_UNUSED(version);

	SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Creating fragball", SG_LOG__FLAG__NONE)  );
	SG_ERR_CHECK(  SG_log__set_value__sz(pCtx, "Repository Type", "fs3", SG_LOG__FLAG__NONE)  );

	SG_NULLARGCHECK(pRepo);
	SG_NULLARGCHECK(pFragballDirPathname);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    {
        char buf_filename[SG_TID_MAX_BUFFER_LENGTH];
        SG_ERR_CHECK(  SG_tid__generate(pCtx, buf_filename, sizeof(buf_filename))  );
        SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pFragballPathname, pFragballDirPathname, buf_filename)  );
    }

    if (!pData->b_new_audits)
    {
        SG_RETRY_THINGIE(  _build_repo_fragball__v1(pCtx, pRepo, pFragballPathname)  );
    }
    else
    {
		SG_ERR_CHECK(  _build_repo_fragball__v3(pCtx, pRepo, pFragballPathname)  );
    }

	SG_ERR_CHECK(  SG_pathname__get_last(pCtx, pFragballPathname, &pstrFragballName)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, SG_string__sz(pstrFragballName), ppszFragballName)  );

	/* fallthru */

fail:
	// If we had an error, delete the half-baked fragball.
	if (pFragballPathname && SG_CONTEXT__HAS_ERR(pCtx))
			SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pFragballPathname)  );

	if (pData)
		SG_ERR_IGNORE(  sg_fs3__nullfree_tx_data(pCtx, pData)  );

	SG_PATHNAME_NULLFREE(pCtx, pFragballPathname);
	SG_STRING_NULLFREE(pCtx, pstrFragballName);
	SG_log__pop_operation(pCtx);
}

void sg_fs3__begin_tx(
    SG_context * pCtx,
    my_instance_data* pData,
    SG_uint32 flags,
    my_tx_data** pptx
    )
{
    if (pData->ptx)
    {
        SG_ERR_THROW(  SG_ERR_ONLY_ONE_REPO_TX  );
    }

    SG_ERR_CHECK(  SG_alloc(pCtx, 1, sizeof(my_tx_data), &pData->ptx)  );
    pData->ptx->flags = flags;

    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pData->ptx->prb_append_locks)  );
    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pData->ptx->prb_file_handles)  );

    if (pData->ptx->flags & SG_REPO_TX_FLAG__CLONING)
    {
        SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pData->ptx->cloning.prb_dbndx_update)  );
        SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pData->ptx->cloning.pvh_templates)  );
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pData->psql, &pData->ptx->pStmt_audits,
                                          "INSERT INTO \"audits\" (\"csid\", \"dagnum\", \"userid\", \"timestamp\") VALUES (?, ?, ?, ?)")  );
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pData->psql,&pData->ptx->cloning.pStmt_insert,
                                          "INSERT OR IGNORE INTO \"blobs\" (\"hid\", \"filename\", \"offset\", \"encoding\", \"len_encoded\", \"len_full\", \"hid_vcdiff\") VALUES (?, ?, ?, ?, ?, ?, ?)")  );

        SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pData->psql, "BEGIN IMMEDIATE TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
        pData->b_in_sqlite_transaction = SG_TRUE;
    }
    else
    {
        SG_ERR_CHECK(  sg_repo__fs3__create_deferred_sqlite_indexes(pCtx, pData)  );
        SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pData->ptx->prb_frags)  );
        SG_ERR_CHECK(  sg_sqlite__open__temporary(pCtx, &pData->ptx->psql_new_audits)  );
        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pData->ptx->psql_new_audits, "CREATE TABLE audits (csid VARCHAR NOT NULL, dagnum INTEGER NOT NULL, userid VARCHAR NOT NULL, timestamp INTEGER NOT NULL)")  );
        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pData->ptx->psql_new_audits,&pData->ptx->pStmt_audits,
                                          "INSERT INTO \"audits\" (\"csid\", \"dagnum\", \"userid\", \"timestamp\") VALUES (?, ?, ?, ?)")  );
        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pData->ptx->psql_new_audits, ("BEGIN TRANSACTION"))  );

        SG_ERR_CHECK(  SG_blobset__create(pCtx, &pData->ptx->pbs_new_blobs)  );
        SG_ERR_CHECK(  SG_blobset__begin_inserting(pCtx, pData->ptx->pbs_new_blobs)  );
    }

    *pptx = pData->ptx;

    return;

fail:
    SG_ERR_IGNORE(  sg_fs3__nullfree_tx_data(pCtx, pData)  );
}

void sg_repo__fs3__begin_tx(
	SG_context * pCtx,
	SG_repo* pRepo,
    SG_uint32 flags,
	SG_repo_tx_handle** ppTx
	)
{
	my_instance_data * pData = NULL;
    my_tx_data* ptx = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(ppTx);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_new_audits)
    {
        SG_ERR_THROW(  SG_ERR_OLD_AUDITS  );
    }

    SG_ERR_CHECK(  sg_fs3__begin_tx(pCtx, pData, flags, &ptx)  );

    *ppTx = (SG_repo_tx_handle*) ptx;

    return;
fail:
    SG_ERR_IGNORE(  sg_fs3__nullfree_tx_data(pCtx, pData)  );
}

void sg_fs3__close_file_handles(
	SG_context * pCtx,
	my_tx_data* ptx
    )
{
    SG_bool b = SG_FALSE;
    SG_rbtree_iterator* pit = NULL;
    const char* psz_filenumber = NULL;
    SG_file* pFile = NULL;

    if (ptx->prb_file_handles)
    {
        /* remove the lock files here.  */
        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, ptx->prb_file_handles, &b, &psz_filenumber, (void**) &pFile)  );
        while (b)
        {
            SG_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

            SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_filenumber, (void**) &pFile)  );
        }
        SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
        SG_RBTREE_NULLFREE(pCtx, ptx->prb_file_handles);
    }

    return;

fail:
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    SG_ERR_IGNORE(  SG_file__close(pCtx, &pFile)  );

    return;
}

void sg_fs3__remove_lock_files(
	SG_context * pCtx,
	my_tx_data* ptx
    )
{
    SG_bool b = SG_FALSE;
    SG_rbtree_iterator* pit = NULL;
    const char* psz_filenumber = NULL;
    SG_pathname* pPath_lock = NULL;

    if (ptx->prb_append_locks)
    {
        /* remove the lock files here.  */
        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, ptx->prb_append_locks, &b, &psz_filenumber, (void**) &pPath_lock)  );
        while (b)
        {
            SG_ERR_IGNORE(  SG_fsobj__remove__pathname(pCtx, pPath_lock)  );
            SG_PATHNAME_NULLFREE(pCtx, pPath_lock);

            SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_filenumber, (void**) &pPath_lock)  );
        }
        SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
        SG_RBTREE_NULLFREE(pCtx, ptx->prb_append_locks);
    }

    return;

fail:
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    SG_PATHNAME_NULLFREE(pCtx, pPath_lock);

    return;
}

static void sg_fs3__get_dbndx_path__cb(
        SG_context* pCtx,
        void * pVoidData,
        SG_uint64 iDagNum,
        SG_pathname** pp
        )
{
	my_instance_data* pData = (my_instance_data*) pVoidData;

    SG_ERR_CHECK_RETURN(  sg_fs3__get_dbndx_path(pCtx, pData, iDagNum, pp)  );
}

static void sg_fs3__get_sql__cb(
        SG_context* pCtx,
        void * pVoidData,
        SG_pathname* pPath,
        sqlite3** pp
        )
{
	my_instance_data* pData = (my_instance_data*) pVoidData;

    SG_ERR_CHECK_RETURN(  fs3__get_sql(pCtx, pData, pPath, SG_SQLITE__SYNC__OFF, pp)  );
}

static void sg_fs3__get_treendx__cb(
        SG_context* pCtx,
        void * pVoidData,
        SG_uint64 iDagNum,
        SG_treendx** ppndx
        )
{
	my_instance_data* pData = (my_instance_data*) pVoidData;

    SG_ERR_CHECK_RETURN(  _fs3_my_get_treendx(pCtx, pData, iDagNum, SG_FALSE, ppndx)  );
}

static void sg_repo__fs3__rebuild_some_dbndxes(
	SG_context * pCtx,
	SG_repo * pRepo,
    SG_vhash* pvh,
    SG_vhash* pvh_fresh
	)
{
	my_instance_data * pData = NULL;
    SG_uint32 count = 0;
    SG_uint32 i = 0;
    SG_rbtree* prb = NULL;
    SG_stringarray* psa = NULL;
    SG_pathname* pPath = NULL;

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh, &count)  );
    SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb)  );

    for (i=0; i<count; i++)
    {
        SG_bool bExists = SG_FALSE;
        const char* psz_dagnum = NULL;
        SG_uint64 dagnum = 0;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh, i, &psz_dagnum, NULL)  );
        SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );

        SG_ERR_CHECK(  sg_fs3__get_dbndx_path(pCtx, pData, dagnum, &pPath)  );

        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_fresh, psz_dagnum, SG_pathname__sz(pPath))  );

        bExists = SG_FALSE;
        SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bExists, NULL, NULL)  );
        if (bExists)
        {
            sqlite3* psql = NULL;
            SG_bool b_found_sql = SG_FALSE;

            SG_ERR_CHECK(  SG_rbtree__find(pCtx, pData->prb_sql, SG_pathname__sz(pPath), &b_found_sql, (void**) &psql)  );
            if (psql)
            {
                SG_ERR_CHECK(  sg_sqlite__close(pCtx, psql)  );
                SG_ERR_CHECK(  SG_rbtree__remove(pCtx, pData->prb_sql, SG_pathname__sz(pPath))  );
            }

            SG_ERR_CHECK(  SG_dbndx__remove(pCtx, dagnum, pPath)  );
        }
        SG_PATHNAME_NULLFREE(pCtx, pPath);

        // add all the changesets for this dag
        SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psa, 1)  );
        SG_ERR_CHECK(  SG_dag_sqlite3__fetch_all_dagnodes(pCtx, pData->psql, dagnum, psa)  );
        SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb, psz_dagnum, psa)  );
        psa = NULL;
    }

    SG_ERR_CHECK(  sg_repo__rebuild_dbndxes(pCtx, pData->pRepo, prb,
                sg_fs3__get_dbndx_path__cb, pData,
                sg_fs3__get_sql__cb, pData
                )  );

fail:
    SG_STRINGARRAY_NULLFREE(pCtx, psa);
    SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, prb,  (SG_free_callback *) SG_stringarray__free);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
}

void sg_repo__fs3__count_all_dagnodes(
	SG_context * pCtx,
	SG_repo * pRepo,
    SG_uint64 dagnum,
    SG_uint32* pi
    )
{
	my_instance_data * pData = NULL;

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_ERR_CHECK(  SG_dag_sqlite3__count_all_dagnodes(pCtx, pData->psql, dagnum, pi)  );

fail:
    ;
}

void sg_repo__fs3__fetch_all_dagnodes(
	SG_context * pCtx,
	SG_repo * pRepo,
    SG_uint64 dagnum,
    SG_stringarray** ppsa
    )
{
	my_instance_data * pData = NULL;
    SG_stringarray* psa = NULL;

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_ERR_CHECK(  SG_stringarray__alloc(pCtx, &psa, 50)  );
    SG_ERR_CHECK(  SG_dag_sqlite3__fetch_all_dagnodes(pCtx, pData->psql, dagnum, psa)  );

    *ppsa = psa;
    psa = NULL;

fail:
    SG_STRINGARRAY_NULLFREE(pCtx, psa);
}

static void sg_fs3__update_shadow_users( SG_context* pCtx, sqlite3* psql, SG_varray* pva_users)
{
    sqlite3_stmt* pStmt = NULL;
    SG_uint32 count_users = 0;
    SG_uint32 i_user = 0;
    SG_bool b_in_transaction = SG_FALSE;

    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, psql, "BEGIN TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
    b_in_transaction = SG_TRUE;

    SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "CREATE TABLE IF NOT EXISTS shadow_users (userid VARCHAR NOT NULL UNIQUE, name VARCHAR NOT NULL UNIQUE)")  );
	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql, "DELETE FROM shadow_users")  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, psql, &pStmt, "INSERT INTO shadow_users (userid, name) VALUES (?, ?)")  );

    // clear the shadow table and stuff everything in t
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_users, &count_users)  );
    for (i_user=0; i_user<count_users; i_user++)
    {
        SG_vhash* pvh_user = NULL;
        const char* psz_name = NULL;
        const char* psz_userid = NULL;

        SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_users, i_user, &pvh_user)  );

        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_user, "name", &psz_name)  );
        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_user, "recid", &psz_userid)  );

        SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );
        SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, psz_userid)  );
        SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, psz_name)  );
        SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
    }
    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, psql, "COMMIT TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
    b_in_transaction = SG_FALSE;

fail:
    if (pStmt)
    {
        SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
    }
    if (b_in_transaction)
    {
        SG_ERR_IGNORE(  sg_sqlite__exec__retry(pCtx, psql, "ROLLBACK TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
    }
}

void sg_repo__fs3__update_all_shadow_users(
	SG_context * pCtx,
	SG_repo * pRepo
	)
{
	my_instance_data * pData = NULL;
    sqlite3* psql = NULL;
    SG_pathname* pPath = NULL;
    SG_pathname* pPath_users = NULL;
    SG_dbndx_query* pndx_users = NULL;
    SG_uint32 count_users_dag_leaves = 0;
    SG_rbtree* prb_users_dag_leaves = NULL;
    SG_varray* pva_users = NULL;
    SG_varray* pva_fields = NULL;
    const char* psz_first_leaf = NULL;

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_ERR_CHECK(  SG_dag_sqlite3__fetch_leaves(pCtx, pData->psql, SG_DAGNUM__USERS, &prb_users_dag_leaves)  );
    SG_ERR_CHECK(  SG_rbtree__count(pCtx, prb_users_dag_leaves, &count_users_dag_leaves)  );
    if (count_users_dag_leaves > 0)
    {
        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, NULL, prb_users_dag_leaves, NULL, &psz_first_leaf, NULL)  );

        {
            sqlite3* psql_users = NULL;

            SG_ERR_CHECK(  sg_fs3__get_dbndx_path(pCtx, pData, SG_DAGNUM__USERS, &pPath_users)  );
            SG_ERR_CHECK(  fs3__get_sql(pCtx, pData, pPath_users, SG_SQLITE__SYNC__OFF, &psql_users)  );
            SG_ERR_CHECK(  SG_dbndx_query__open(pCtx, pRepo, SG_DAGNUM__USERS, psql_users, pPath_users, &pndx_users)  );
            SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_fields)  );
            SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, SG_ZING_FIELD__RECID)  );
            SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_fields, "name")  );
            SG_ERR_CHECK(  SG_dbndx_query__query(pCtx, pndx_users, psz_first_leaf, "user", NULL, NULL, 0, 0, pva_fields, &pva_users)  );
            SG_DBNDX_QUERY_NULLFREE(pCtx, pndx_users);
            SG_PATHNAME_NULLFREE(pCtx, pPath_users);
            SG_VARRAY_NULLFREE(pCtx, pva_fields);
        }

        SG_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPath, pData->pPathMyDir, "usernames.sqlite3")  );
        SG_ERR_CHECK(  sg_sqlite__open__pathname(pCtx, pPath, SG_SQLITE__SYNC__OFF, &psql)  );

        SG_ERR_CHECK(  sg_fs3__update_shadow_users(pCtx, psql, pva_users)  );
        
        SG_ERR_CHECK(  sg_sqlite__close(pCtx, psql)  );
        psql = NULL;
    }

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_VARRAY_NULLFREE(pCtx, pva_fields);
    SG_DBNDX_QUERY_NULLFREE(pCtx, pndx_users);
    SG_VARRAY_NULLFREE(pCtx, pva_users);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_PATHNAME_NULLFREE(pCtx, pPath_users);
    SG_RBTREE_NULLFREE(pCtx, prb_users_dag_leaves);
}

static void sg_fs3__store_the_audits(
	SG_context * pCtx,
    my_instance_data* pData
    )
{
    sqlite3_stmt* pStmt_read = NULL;
    sqlite3_stmt* pStmt_write = NULL;
    int rc = -1;

    SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, 
                pData->psql,
                &pStmt_write,
                "INSERT OR IGNORE INTO \"audits\" (\"csid\", \"dagnum\", \"userid\", \"timestamp\") VALUES (?, ?, ?, ?)")  );

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, 
                pData->ptx->psql_new_audits,
                &pStmt_read,
                "SELECT csid, dagnum, userid, timestamp FROM audits")  );

    while ((rc=sqlite3_step(pStmt_read)) == SQLITE_ROW)
    {
        SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt_write)  );
        SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt_write)  );
        SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt_write, 1, (char*) sqlite3_column_text(pStmt_read, 0))  );
        SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt_write, 2, sqlite3_column_int64(pStmt_read, 1))  );
        SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt_write, 3, (char*) sqlite3_column_text(pStmt_read, 2))  );
        SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt_write, 4, sqlite3_column_int64(pStmt_read, 3))  );
        SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt_write, SQLITE_DONE)  );
    }
    if (rc != SQLITE_DONE)
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

fail:
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt_read)  );
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt_write)  );
}

static void sg_fs3__store_the_frags(
	SG_context * pCtx,
	my_instance_data* pData,
	my_tx_data* ptx,
    SG_rbtree** pprb,
    SG_bool* pb
    )
{
    SG_bool b = SG_FALSE;
    SG_rbtree_iterator* pit = NULL;
    SG_rbtree* prb_new_dagnodes = NULL;
    SG_rbtree* prb_missing = NULL;
    SG_stringarray* psa_new = NULL;
    SG_bool b_users_dag_changed = SG_FALSE;
    SG_bool b_inside_operation = SG_FALSE;

    SG_ERR_CHECK(  SG_log__push_operation(pCtx, "Storing", SG_LOG__FLAG__NONE)  );
    b_inside_operation = SG_TRUE;

    /* store the frags */
    if (ptx->prb_frags)
    {
        SG_dagfrag* pFrag = NULL;
        const char* psz_dagnum = NULL;
        SG_uint32 count_steps = 0;

        SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prb_new_dagnodes)  );

        SG_ERR_CHECK(  SG_rbtree__count(pCtx, ptx->prb_frags, &count_steps)  );

        SG_ERR_CHECK(  SG_log__set_steps(pCtx, count_steps, "dags")  );

        SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pit, ptx->prb_frags, &b, &psz_dagnum, (void**) &pFrag)  );
        while (b)
        {
            SG_ERR_CHECK(  SG_dag_sqlite3__insert_frag(pCtx, pData->psql, pFrag, &prb_missing, &psa_new, ptx->flags)  );
            SG_RBTREE_NULLFREE(pCtx, prb_missing);

            if (psa_new)
            {
                SG_uint64 dagnum = 0;
                SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );
                if (SG_DAGNUM__USERS == dagnum)
                {
                    b_users_dag_changed = SG_TRUE;
                }
                if (prb_new_dagnodes)
                {
                    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, prb_new_dagnodes, psz_dagnum, psa_new)  );
                    psa_new = NULL;
                }
            }
            SG_STRINGARRAY_NULLFREE(pCtx, psa_new);
            SG_ERR_CHECK(  SG_log__finish_step(pCtx)  );

            SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pit, &b, &psz_dagnum, (void**) &pFrag)  );
        }
        SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    }

    SG_ERR_CHECK(  SG_log__pop_operation(pCtx)  );
    b_inside_operation = SG_FALSE;

    *pprb = prb_new_dagnodes;
    prb_new_dagnodes = NULL;

    *pb = b_users_dag_changed;

fail:
    SG_RBTREE_ITERATOR_NULLFREE(pCtx, pit);
    SG_RBTREE_NULLFREE(pCtx, prb_missing);
    SG_RBTREE_NULLFREE(pCtx, prb_new_dagnodes);
    SG_STRINGARRAY_NULLFREE(pCtx, psa_new);
    if (b_inside_operation)
    {
        SG_ERR_IGNORE(  SG_log__pop_operation(pCtx)  );
    }
}

void sg_fs3__commit_tx(
	SG_context * pCtx,
	my_instance_data* pData,
	my_tx_data** pptx
	)
{
    SG_rbtree* prb_new_dagnodes = NULL;
    SG_dagnode* pdn = NULL;
    SG_vhash* pvh_rebuild = NULL;
    SG_vhash* pvh_fresh = NULL;
    SG_bool b_users_dag_changed = SG_FALSE;

	if (!pData->ptx)
	{
		SG_ERR_THROW(  SG_ERR_NO_REPO_TX  );
	}

    if (*pptx != pData->ptx)
    {
		SG_ERR_THROW(  SG_ERR_BAD_REPO_TX_HANDLE  );
    }

    if (pData->ptx->flags & SG_REPO_TX_FLAG__CLONING)
    {
        SG_RBTREE_NULLFREE(pCtx, pData->ptx->cloning.prb_dbndx_update);
        SG_ERR_CHECK(  sg_repo__fs3__create_deferred_sqlite_indexes(pCtx, pData)  );
        SG_ERR_CHECK(  sg_repo__update_ndx__all__tree(pCtx, pData->pRepo,
                    sg_fs3__get_treendx__cb, pData
                    )  );
        SG_ERR_CHECK(  sg_repo__fs3__update_all_shadow_users(pCtx, pData->pRepo)  );
    }
    else
    {
        SG_ERR_CHECK(  SG_blobset__finish_inserting(pCtx, pData->ptx->pbs_new_blobs)  );

        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pData->ptx->psql_new_audits, ("COMMIT TRANSACTION"))  );
        SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pData->ptx->pStmt_audits)  );

        SG_ERR_CHECK(  sg_sqlite__exec__retry(pCtx, pData->psql, "BEGIN IMMEDIATE TRANSACTION", MY_SLEEP_MS, MY_TIMEOUT_MS)  );
        pData->b_in_sqlite_transaction = SG_TRUE;

        SG_ERR_CHECK(  SG_blobset__copy_into_fs3(pCtx, pData->ptx->pbs_new_blobs, pData->psql)  );

        SG_BLOBSET_NULLFREE(pCtx, pData->ptx->pbs_new_blobs);

        SG_ERR_CHECK(  sg_fs3__store_the_frags(pCtx, pData, pData->ptx, &prb_new_dagnodes, &b_users_dag_changed)  );

        SG_ERR_CHECK(  sg_fs3__store_the_audits(pCtx, pData)  );

        SG_ERR_CHECK(  sg_sqlite__close(pCtx, pData->ptx->psql_new_audits)  );
        pData->ptx->psql_new_audits = NULL;

        if (prb_new_dagnodes)
        {
            SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_fresh)  );
            SG_ERR_CHECK(  sg_repo__update_ndx(pCtx, pData->pRepo, prb_new_dagnodes,
                        sg_fs3__get_dbndx_path__cb, pData,
                        sg_fs3__get_sql__cb, pData,
                        sg_fs3__get_treendx__cb, pData,
                        &pvh_rebuild,
                        pvh_fresh
                        )  );

            if (pvh_rebuild)
            {
                SG_ERR_CHECK(  sg_repo__fs3__rebuild_some_dbndxes(pCtx, pData->pRepo, pvh_rebuild, pvh_fresh)  );
            }

            if (b_users_dag_changed)
            {
                SG_ERR_CHECK(  sg_repo__fs3__update_all_shadow_users(pCtx, pData->pRepo)  );
            }
        }
    }

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pData->psql, ("COMMIT TRANSACTION"))  );
    pData->b_in_sqlite_transaction = SG_FALSE;

    SG_ERR_CHECK(  sg_fs3__nullfree_tx_data(pCtx, pData)  );
    *pptx = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_fresh);
    SG_VHASH_NULLFREE(pCtx, pvh_rebuild);
    SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, prb_new_dagnodes,  (SG_free_callback *) SG_stringarray__free);
    SG_DAGNODE_NULLFREE(pCtx, pdn);
    if (pData->b_in_sqlite_transaction)
    {
        SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, pData->psql, ("ROLLBACK TRANSACTION"))  );
        pData->b_in_sqlite_transaction = SG_FALSE;
    }
}

void sg_repo__fs3__check_dagfrag(
	SG_context * pCtx,
	SG_repo * pRepo,
	SG_dagfrag * pFrag,
	SG_bool * pbConnected,
	SG_rbtree ** ppIdsetMissing,
	SG_stringarray ** ppsa_new
    )
{
	my_instance_data * pData = NULL;
	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );
    SG_RETRY_THINGIE(  SG_dag_sqlite3__check_frag(pCtx, pData->psql, pFrag, pbConnected, ppIdsetMissing, ppsa_new)  );

fail:
	;
}

void sg_repo__fs3__rebuild_indexes(
	SG_context * pCtx,
	SG_repo * pRepo
	)
{
	my_instance_data * pData = NULL;
    SG_uint32 count = 0;
    SG_uint64* pa_dagnums = NULL;
    SG_uint32 i = 0;
    SG_pathname* pPath = NULL;
    SG_pathname* pPath_filters_dir = NULL;
    SG_bool bExists = SG_FALSE;

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_new_audits)
    {
        SG_ERR_THROW(  SG_ERR_OLD_AUDITS  );
    }

    // first remove all the dbndx filters
    SG_ERR_CHECK(  sg_fs3__get_dbndx_path(pCtx, pData, SG_DAGNUM__VC_TAGS, &pPath)  );
    SG_ERR_CHECK( sg_dbndx__get_filters_dir_path(pCtx, pPath, &pPath_filters_dir)  );
    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_filters_dir, &bExists, NULL, NULL)  );
    if (bExists)
    {
        SG_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname(pCtx, pPath_filters_dir)  );
    }
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_PATHNAME_NULLFREE(pCtx, pPath_filters_dir);

    SG_ERR_CHECK(  SG_dag_sqlite3__list_dags(pCtx, pData->psql, &count, &pa_dagnums)  );

    for (i=0; i<count; i++)
    {
        SG_ERR_CHECK(  sg_fs3__get_dbndx_path(pCtx, pData, pa_dagnums[i], &pPath)  );
        bExists = SG_FALSE;
        SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bExists, NULL, NULL)  );
        if (bExists)
        {
            sqlite3* psql = NULL;
            SG_bool b_found_sql = SG_FALSE;

            SG_ERR_CHECK(  SG_rbtree__find(pCtx, pData->prb_sql, SG_pathname__sz(pPath), &b_found_sql, (void**) &psql)  );
            if (psql)
            {
                SG_ERR_CHECK(  sg_sqlite__close(pCtx, psql)  );
                SG_ERR_CHECK(  SG_rbtree__remove(pCtx, pData->prb_sql, SG_pathname__sz(pPath))  );
            }

            SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath)  );
        }
        SG_PATHNAME_NULLFREE(pCtx, pPath);

        SG_ERR_CHECK(  sg_fs3__get_treendx_path(pCtx, pData, pa_dagnums[i], &pPath)  );
        bExists = SG_FALSE;
        SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &bExists, NULL, NULL)  );
        if (bExists)
        {
            SG_ERR_CHECK(  SG_fsobj__remove__pathname(pCtx, pPath)  );
        }
        SG_PATHNAME_NULLFREE(pCtx, pPath);
    }

    SG_ERR_CHECK(  sg_repo__update_ndx__all__tree(pCtx, pRepo,
                sg_fs3__get_treendx__cb, pData
                )  );

    SG_ERR_CHECK(  sg_repo__update_ndx__all__db(pCtx, pRepo,
                sg_fs3__get_dbndx_path__cb, pData
                )  );

    // we must update all the shadow users here because everything
    // is getting rebuilt.

    SG_ERR_CHECK(  sg_repo__fs3__update_all_shadow_users(pCtx, pRepo)  );

fail:
    SG_NULLFREE(pCtx, pa_dagnums);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
}

void sg_repo__fs3__query_audits(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        SG_int64 min_timestamp,
        SG_int64 max_timestamp,
        const char* psz_userid,
        SG_varray** ppva
        )
{
    SG_string* pstr = NULL;
	my_instance_data * pData = NULL;
    SG_varray* pva = NULL;
    sqlite3_stmt* pStmt = NULL;
    int rc = -1;
    SG_bool b_where = SG_FALSE;

    SG_NULLARGCHECK_RETURN(pRepo);
    SG_NULLARGCHECK_RETURN(ppva);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_new_audits)
    {
        SG_ERR_THROW(  SG_ERR_OLD_AUDITS  );
    }

    SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr, "SELECT csid, userid, timestamp, dagnum FROM audits")  );

    if (dagnum)
    {
        if (b_where)
        {
            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, " AND ")  );
        }
        else
        {
            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, " WHERE ")  );
            b_where = SG_TRUE;
        }

        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, "dagnum=?")  );
    }

    if (min_timestamp >= 0)
    {
        SG_int_to_string_buffer ibuf;

        if (b_where)
        {
            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, " AND ")  );
        }
        else
        {
            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, " WHERE ")  );
            b_where = SG_TRUE;
        }

        SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr, "(timestamp >= %s)", SG_int64_to_sz(min_timestamp, ibuf))  );
    }

    if (max_timestamp >= 0)
    {
        SG_int_to_string_buffer ibuf;

        if (b_where)
        {
            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, " AND ")  );
        }
        else
        {
            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, " WHERE ")  );
            b_where = SG_TRUE;
        }

        SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr, "(timestamp <= %s)", SG_int64_to_sz(max_timestamp, ibuf))  );
    }

    if (psz_userid)
    {
        if (b_where)
        {
            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, " AND ")  );
        }
        else
        {
            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, " WHERE ")  );
            b_where = SG_TRUE;
        }

        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, "(userid = ?)")  );
    }

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pData->psql, &pStmt, "%s", SG_string__sz(pstr))  );
    if (dagnum)
    {
        SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pStmt, 1, dagnum)  );
    }

    if (psz_userid)
    {
        if (dagnum)
        {
            SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 2, psz_userid)  );
        }
        else
        {
            SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, psz_userid)  );
        }
    }

    SG_ERR_CHECK(  SG_varray__alloc(pCtx, &pva)  );
    while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        const char * psz_csid = (const char *)sqlite3_column_text(pStmt,0);
        const char * psz_userid = (const char *)sqlite3_column_text(pStmt,1);
        SG_int64 timestamp = (SG_int64) sqlite3_column_int64(pStmt, 2);
        SG_int64 this_dagnum = (SG_int64) sqlite3_column_int64(pStmt, 3);
        SG_vhash* pvh = NULL;

        SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "csid", psz_csid)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, SG_AUDIT__USERID, psz_userid)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, SG_AUDIT__TIMESTAMP, timestamp)  );
        if (!dagnum)
        {
            SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "dagnum", this_dagnum)  );
        }
    }
    if (rc != SQLITE_DONE)
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    *ppva = pva;
    pva = NULL;

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
    SG_VARRAY_NULLFREE(pCtx, pva);
}

void sg_repo__fs3__list_audits_for_given_changesets(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        SG_varray* pva_csid_list,
        SG_varray** ppva
        )
{
	my_instance_data * pData = NULL;
    SG_varray* pva = NULL;
    sqlite3_stmt* pStmt = NULL;
    int rc = -1;
    SG_uint32 count_changesets = 0;
    SG_string* pstr = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);
    SG_NULLARGCHECK_RETURN(pva_csid_list);
    SG_NULLARGCHECK_RETURN(ppva);
    SG_UNUSED(dagnum);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_new_audits)
    {
        SG_ERR_THROW(  SG_ERR_OLD_AUDITS  );
    }

    SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr, "SELECT csid, userid, timestamp FROM audits WHERE ")  );
    SG_ERR_CHECK(  SG_varray__count(pCtx, pva_csid_list, &count_changesets)  );

    if (count_changesets > 10)
    {
        char buf[SG_TID_MAX_BUFFER_LENGTH];
        SG_uint32 i = 0;

        SG_ERR_CHECK(  SG_tid__generate(pCtx, buf, sizeof(buf))  );

        SG_ERR_CHECK(  sg_sqlite__exec__va(pCtx, pData->psql, "CREATE TEMP TABLE %s (val VARCHAR NOT NULL UNIQUE)", buf)  );

        SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pData->psql, &pStmt, "INSERT OR IGNORE INTO %s (val) VALUES (?)", buf)  );
        for (i=0; i<count_changesets; i++)
        {
            const char* psz_val = NULL;

            SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_csid_list, i, &psz_val)  );

            SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
            SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );

            SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, psz_val)  );

            SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_DONE)  );
        }
        SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

        SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr, "EXISTS (SELECT * FROM \"%s\" WHERE val = csid)", buf)  );
    }
    else
    {
        SG_uint32 i = 0;

        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, "csid IN (")  );
        for (i=0; i<count_changesets; i++)
        {
            const char* psz_val = NULL;

            SG_ERR_CHECK(  SG_varray__get__sz(pCtx, pva_csid_list, i, &psz_val)  );
            if (i > 0)
            {
                SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, ",")  );
            }
            SG_ERR_CHECK(  SG_string__append__format(pCtx, pstr, "'%s'", psz_val)  );
        }
        SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr, ")")  );
    }

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pData->psql, &pStmt, "%s", SG_string__sz(pstr))  );

    SG_ERR_CHECK(  SG_varray__alloc(pCtx, &pva)  );
    while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        const char * psz_csid = (const char *)sqlite3_column_text(pStmt,0);
        const char * psz_userid = (const char *)sqlite3_column_text(pStmt,1);
        SG_int64 timestamp = (SG_int64) sqlite3_column_int64(pStmt, 2);
        SG_vhash* pvh = NULL;

        SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "csid", psz_csid)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, SG_AUDIT__USERID, psz_userid)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, SG_AUDIT__TIMESTAMP, timestamp)  );
    }
    if (rc != SQLITE_DONE)
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    *ppva = pva;
    pva = NULL;

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

void sg_repo__fs3__lookup_audits(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 dagnum,
        const char* psz_csid,
        SG_varray** ppva
        )
{
	my_instance_data * pData = NULL;
    SG_varray* pva = NULL;
    sqlite3_stmt* pStmt = NULL;
    int rc = -1;

    SG_NULLARGCHECK_RETURN(pRepo);
    SG_NULLARGCHECK_RETURN(psz_csid);
    SG_NULLARGCHECK_RETURN(ppva);
    SG_UNUSED(dagnum);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_new_audits)
    {
        SG_ERR_THROW(  SG_ERR_OLD_AUDITS  );
    }

	SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pData->psql,&pStmt,
									  "SELECT userid, timestamp FROM \"audits\" WHERE csid = ?")  );
	SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, psz_csid)  );

    SG_ERR_CHECK(  SG_varray__alloc(pCtx, &pva)  );
    while ((rc=sqlite3_step(pStmt)) == SQLITE_ROW)
    {
        const char * psz_userid = (const char *)sqlite3_column_text(pStmt,0);
        SG_int64 timestamp = (SG_int64) sqlite3_column_int64(pStmt, 1);
        SG_vhash* pvh = NULL;

        SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pva, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, SG_AUDIT__USERID, psz_userid)  );
        SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, SG_AUDIT__TIMESTAMP, timestamp)  );
    }
    if (rc != SQLITE_DONE)
    {
        SG_ERR_THROW(  SG_ERR_SQLITE(rc)  );
    }

    SG_ERR_CHECK(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );

    *ppva = pva;
    pva = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_ERR_IGNORE(  sg_sqlite__nullfinalize(pCtx, &pStmt)  );
}

void sg_repo__fs3__store_audit(
	SG_context * pCtx,
	SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    const char* psz_csid,
    SG_uint64 dagnum,
    const char* psz_userid,
    SG_int64 when
	)
{
	my_instance_data * pData = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);
    SG_NULLARGCHECK_RETURN(pTx);
    SG_NULLARGCHECK_RETURN(psz_csid);
    SG_NULLARGCHECK_RETURN(psz_userid);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );
    SG_ARGCHECK_RETURN(pData->ptx == ((my_tx_data*) pTx), ptx);

    SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pData->ptx->pStmt_audits)  );
    SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pData->ptx->pStmt_audits)  );
    SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pData->ptx->pStmt_audits, 1, psz_csid)  );
    SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pData->ptx->pStmt_audits, 2, dagnum)  );
    SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pData->ptx->pStmt_audits, 3, psz_userid)  );
    SG_ERR_CHECK(  sg_sqlite__bind_int64(pCtx, pData->ptx->pStmt_audits, 4, when)  );
    SG_ERR_CHECK(  sg_sqlite__step(pCtx, pData->ptx->pStmt_audits, SQLITE_DONE)  );

fail:
    ;
}

void sg_repo__fs3__done_with_dag(
	SG_context * pCtx,
	SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    SG_uint64 dagnum
	)
{
	my_instance_data * pData = NULL;
    SG_dbndx_update* pndx = NULL;
    char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX + 10];
    SG_bool b_found = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(pTx);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_ARGCHECK_RETURN(pData->ptx == ((my_tx_data*) pTx), ptx);
    if (!(pData->ptx->flags & SG_REPO_TX_FLAG__CLONING))
    {
		SG_ERR_THROW2_RETURN(SG_ERR_INVALIDARG, (pCtx, "done_with_dag is only for cloning"));
    }

    SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, dagnum, buf_dagnum, sizeof(buf_dagnum))  );
    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pData->ptx->cloning.prb_dbndx_update, buf_dagnum, &b_found, (void**) &pndx)  );
    if (pndx)
    {
        SG_ERR_CHECK(  SG_dbndx_update__do_audits(pCtx, pndx, NULL)  );

        SG_ERR_CHECK(  SG_dbndx_update__end(pCtx, pndx)  );
        SG_DBNDX_UPDATE_NULLFREE(pCtx, pndx);
        SG_ERR_CHECK(  SG_rbtree__remove(pCtx, pData->ptx->cloning.prb_dbndx_update, buf_dagnum)  );
    }

fail:
    ;
}

void sg_repo__fs3__store_dagfrag(
	SG_context * pCtx,
	SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
	SG_dagfrag * pFrag
	)
{
    SG_uint64 iDagNum = 0;
    SG_rbtree* prb_missing = NULL;
    SG_stringarray* psa_new = NULL;
    char buf[SG_DAGNUM__BUF_MAX__HEX];
	my_instance_data * pData = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(pTx);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );
    SG_ARGCHECK_RETURN(pData->ptx == ((my_tx_data*) pTx), ptx);

    if (pData->ptx->flags & SG_REPO_TX_FLAG__CLONING)
    {
        SG_ERR_CHECK(  SG_dag_sqlite3__insert_frag(pCtx, pData->psql, pFrag, &prb_missing, &psa_new, pData->ptx->flags)  );
        SG_RBTREE_NULLFREE(pCtx, prb_missing);
        SG_STRINGARRAY_NULLFREE(pCtx, psa_new);
        SG_DAGFRAG_NULLFREE(pCtx, pFrag);
    }
    else
    {
        /* We don't actually store the frag now.  We just tuck it away
         * so it can be stored at the end when commit_tx gets called. */

        SG_ERR_CHECK(  SG_dagfrag__get_dagnum(pCtx, pFrag, &iDagNum)  );
        if (SG_DAGNUM__IS_OLDAUDIT(iDagNum))
        {
            SG_ERR_THROW(  SG_ERR_OLD_AUDITS  );
        }

        SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, iDagNum, buf, sizeof(buf))  );
        SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pData->ptx->prb_frags, buf, pFrag)  );
    }

fail:
    SG_RBTREE_NULLFREE(pCtx, prb_missing);
    SG_STRINGARRAY_NULLFREE(pCtx, psa_new);
}

void sg_repo__fs3__commit_tx(
	SG_context * pCtx,
	SG_repo* pRepo,
	SG_repo_tx_handle** ppTx
	)
{
	my_instance_data * pData = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULL_PP_CHECK_RETURN(ppTx);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );
    SG_ARGCHECK_RETURN(pData->ptx == ((my_tx_data*) *ppTx), ppTx);

	if (pData->ptx->pBlobStoreHandle)
	{
		SG_ERR_IGNORE(  sg_repo__fs3__abort_tx(pCtx, pRepo, ppTx)  );
		SG_ERR_THROW_RETURN(SG_ERR_INCOMPLETE_BLOB_IN_REPO_TX);
	}

    SG_ERR_CHECK(  sg_fs3__commit_tx(pCtx, pData, (my_tx_data**) ppTx)  );

    return;
fail:
    if (pData && (pData->b_in_sqlite_transaction))
    {
        SG_ERR_IGNORE(  sg_sqlite__exec(pCtx, pData->psql, ("ROLLBACK TRANSACTION"))  );
        pData->b_in_sqlite_transaction = SG_FALSE;
    }

    if (ppTx)
    {
        SG_ERR_IGNORE(  sg_fs3__nullfree_tx_data(pCtx, pData)  );
    }
}

void sg_repo__fs3__abort_tx(
	SG_context * pCtx,
	SG_repo* pRepo,
	SG_repo_tx_handle** pptx
	)
{
	my_instance_data * pData = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULL_PP_CHECK_RETURN(pptx);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

	if (!pData->ptx)
	{
		SG_ERR_THROW(  SG_ERR_NO_REPO_TX  );
	}

    if (*pptx != (SG_repo_tx_handle*) pData->ptx)
    {
		SG_ERR_THROW(  SG_ERR_BAD_REPO_TX_HANDLE  );
    }

	if (pData->ptx->pBlobStoreHandle)
	{
        SG_ERR_IGNORE(  sg_blob_fs3__store_blob__abort(pCtx, &pData->ptx->pBlobStoreHandle)  );
	}

    if (pData->b_in_sqlite_transaction)
    {
        SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pData->psql, ("ROLLBACK TRANSACTION"))  );
        pData->b_in_sqlite_transaction = SG_FALSE;
    }

    /* TODO rollback truncate the data files */

    SG_ERR_IGNORE(  sg_fs3__nullfree_tx_data(pCtx, pData)  );
    *pptx = NULL;

    return;
fail:
    return;
}

static void fs3__get_sql(
	SG_context* pCtx,
    my_instance_data* pData,
    SG_pathname* pPath,
    SG_uint32 synchro,
    sqlite3** ppsql
    )
{
    SG_bool b_exists = SG_FALSE;
    SG_bool b_found = SG_FALSE;
    sqlite3* psql = NULL;

    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pData->prb_sql, SG_pathname__sz(pPath), &b_found, (void**) &psql)  );
    if (psql)
    {
        *ppsql = psql;
        psql = NULL;
        goto done;
    }

    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &b_exists, NULL, NULL)  );
    if (!b_exists)
    {
        *ppsql = NULL;
        goto done;
    }

    SG_ERR_CHECK(  sg_sqlite__open__pathname(pCtx, pPath, synchro, &psql)  );
    SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pData->prb_sql, SG_pathname__sz(pPath), psql)  );

    *ppsql = psql;
    psql = NULL;

done:
fail:
    SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql)  );
}

void sg_repo__fs3__dbndx__query__fts(
	SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint64 iDagNum,
	const char* pidState,
    const char* psz_rectype,
    const char* psz_field_name,
    const char* psz_keywords,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* psa_fields,
    SG_varray** ppva
	)
{
    SG_dbndx_query* pndx = NULL;
	my_instance_data * pData = NULL;
    SG_pathname* pPath = NULL;
    SG_bool b_exists = SG_FALSE;
    SG_varray* pva = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_new_audits)
    {
        SG_ERR_THROW(  SG_ERR_OLD_AUDITS  );
    }

    SG_ERR_CHECK(  sg_fs3__get_dbndx_path(pCtx, pData, iDagNum, &pPath)  );
    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &b_exists, NULL, NULL)  );
    if (b_exists)
    {
        sqlite3* psql = NULL;

        SG_ERR_CHECK(  fs3__get_sql(pCtx, pData, pPath, SG_SQLITE__SYNC__OFF, &psql)  );
        SG_ERR_CHECK(  SG_dbndx_query__open(pCtx, pData->pRepo, iDagNum, psql, pPath, &pndx)  );
        SG_ERR_CHECK(  SG_dbndx_query__fts(pCtx, pndx, pidState, psz_rectype, psz_field_name, psz_keywords, iNumRecordsToReturn, iNumRecordsToSkip, psa_fields, &pva)  );
        SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
    }
    else
    {
        SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );
    }

    *ppva = pva;
    pva = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
}

void sg_repo__fs3__dbndx__query__one(
	SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint64 iDagNum,
	const char* pidState,
    const char* psz_rectype,
    const char* psz_field_name,
    const char* psz_field_value,
    SG_varray* psa_fields,
    SG_vhash** ppvh
	)
{
    SG_dbndx_query* pndx = NULL;
	my_instance_data * pData = NULL;
    SG_pathname* pPath = NULL;
    SG_bool b_exists = SG_FALSE;
    SG_vhash* pvh = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_new_audits)
    {
        SG_ERR_THROW(  SG_ERR_OLD_AUDITS  );
    }

    SG_ERR_CHECK(  sg_fs3__get_dbndx_path(pCtx, pData, iDagNum, &pPath)  );
    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &b_exists, NULL, NULL)  );
    if (b_exists)
    {
        sqlite3* psql = NULL;

        SG_ERR_CHECK(  fs3__get_sql(pCtx, pData, pPath, SG_SQLITE__SYNC__OFF, &psql)  );
        SG_ERR_CHECK(  SG_dbndx_query__open(pCtx, pData->pRepo, iDagNum, psql, pPath, &pndx)  );
        SG_ERR_CHECK(  SG_dbndx_query__query__one(pCtx, pndx, pidState, psz_rectype, psz_field_name, psz_field_value, psa_fields, &pvh)  );
        SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
    }
    else
    {
        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );
    }

    *ppvh = pvh;
    pvh = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
}

void sg_repo__fs3__dbndx__query__raw_history(
	SG_context* pCtx,
    SG_repo* pRepo, 
    SG_uint64 iDagNum,
    SG_int64 min_timestamp,
    SG_int64 max_timestamp,
    SG_vhash** ppvh
	)
{
    SG_dbndx_query* pndx = NULL;
	my_instance_data * pData = NULL;
    SG_pathname* pPath = NULL;
    SG_bool b_exists = SG_FALSE;
    SG_vhash* pvh = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_new_audits)
    {
        SG_ERR_THROW(  SG_ERR_OLD_AUDITS  );
    }

    SG_ERR_CHECK(  sg_fs3__get_dbndx_path(pCtx, pData, iDagNum, &pPath)  );
    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &b_exists, NULL, NULL)  );
    if (b_exists)
    {
        sqlite3* psql = NULL;

        SG_ERR_CHECK(  fs3__get_sql(pCtx, pData, pPath, SG_SQLITE__SYNC__OFF, &psql)  );
        SG_ERR_CHECK(  SG_dbndx_query__open(pCtx, pData->pRepo, iDagNum, psql, pPath, &pndx)  );
        SG_ERR_CHECK(  SG_dbndx_query__raw_history(pCtx, pndx, min_timestamp, max_timestamp, &pvh)  );
        SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
    }
    else
    {
        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );
    }

    *ppvh = pvh;
    pvh = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
}

void sg_repo__fs3__dbndx__query__recent(
	SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint64 iDagNum,
    const char* psz_rectype,
    const SG_varray* pcrit,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* psa_fields,
    SG_varray** ppva
	)
{
    SG_dbndx_query* pndx = NULL;
	my_instance_data * pData = NULL;
    SG_pathname* pPath = NULL;
    SG_bool b_exists = SG_FALSE;
    SG_varray* pva = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_new_audits)
    {
        SG_ERR_THROW(  SG_ERR_OLD_AUDITS  );
    }

    SG_ERR_CHECK(  sg_fs3__get_dbndx_path(pCtx, pData, iDagNum, &pPath)  );
    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &b_exists, NULL, NULL)  );
    if (b_exists)
    {
        sqlite3* psql = NULL;

        SG_ERR_CHECK(  fs3__get_sql(pCtx, pData, pPath, SG_SQLITE__SYNC__OFF, &psql)  );
        SG_ERR_CHECK(  SG_dbndx_query__open(pCtx, pData->pRepo, iDagNum, psql, pPath, &pndx)  );
        SG_ERR_CHECK(  SG_dbndx_query__query__recent(pCtx, pndx, psz_rectype, pcrit, iNumRecordsToReturn, iNumRecordsToSkip, psa_fields, &pva)  );
        SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
    }
    else
    {
        SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );
    }

    *ppva = pva;
    pva = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
}

void sg_repo__fs3__dbndx__query__prep(
	SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint64 iDagNum,
	const char* pidState
    )
{
    SG_dbndx_query* pndx = NULL;
	my_instance_data * pData = NULL;
    SG_pathname* pPath = NULL;
    SG_bool b_exists = SG_FALSE;

    SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_new_audits)
    {
        SG_ERR_THROW(  SG_ERR_OLD_AUDITS  );
    }

    SG_ERR_CHECK(  sg_fs3__get_dbndx_path(pCtx, pData, iDagNum, &pPath)  );
    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &b_exists, NULL, NULL)  );
    if (b_exists)
    {
        sqlite3* psql = NULL;

        SG_ERR_CHECK(  fs3__get_sql(pCtx, pData, pPath, SG_SQLITE__SYNC__OFF, &psql)  );
        SG_ERR_CHECK(  SG_dbndx_query__open(pCtx, pData->pRepo, iDagNum, psql, pPath, &pndx)  );
        SG_ERR_CHECK(  SG_dbndx_query__prep(pCtx, pndx, pidState)  );
        SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
    }
    else
    {
        // TODO throw
    }

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
}

void sg_repo__fs3__dbndx__query__count(
	SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint64 iDagNum,
	const char* pidState,
    const char* psz_rectype,
	const SG_varray* pcrit,
    SG_uint32* pi_count
    )
{
    SG_dbndx_query* pndx = NULL;
	my_instance_data * pData = NULL;
    SG_pathname* pPath = NULL;
    SG_bool b_exists = SG_FALSE;
    SG_uint32 count = 0;

    SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_new_audits)
    {
        SG_ERR_THROW(  SG_ERR_OLD_AUDITS  );
    }

    SG_ERR_CHECK(  sg_fs3__get_dbndx_path(pCtx, pData, iDagNum, &pPath)  );
    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &b_exists, NULL, NULL)  );
    if (b_exists)
    {
        sqlite3* psql = NULL;

        SG_ERR_CHECK(  fs3__get_sql(pCtx, pData, pPath, SG_SQLITE__SYNC__OFF, &psql)  );
        SG_ERR_CHECK(  SG_dbndx_query__open(pCtx, pData->pRepo, iDagNum, psql, pPath, &pndx)  );
        SG_ERR_CHECK(  SG_dbndx_query__count(pCtx, pndx, pidState, psz_rectype, pcrit, &count)  );
        SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
    }

    *pi_count = count;

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
}

void sg_repo__fs3__dbndx__query(
	SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint64 iDagNum,
	const char* pidState,
    const char* psz_rectype,
	const SG_varray* pcrit,
	const SG_varray* pSort,
	SG_uint32 iNumRecordsToReturn,
	SG_uint32 iNumRecordsToSkip,
    SG_varray* psa_fields,
    SG_varray** ppva
	)
{
    SG_dbndx_query* pndx = NULL;
	my_instance_data * pData = NULL;
    SG_pathname* pPath = NULL;
    SG_bool b_exists = SG_FALSE;
    SG_varray* pva = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_new_audits)
    {
        SG_ERR_THROW(  SG_ERR_OLD_AUDITS  );
    }

    SG_ERR_CHECK(  sg_fs3__get_dbndx_path(pCtx, pData, iDagNum, &pPath)  );
    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &b_exists, NULL, NULL)  );
    if (b_exists)
    {
        sqlite3* psql = NULL;

        SG_ERR_CHECK(  fs3__get_sql(pCtx, pData, pPath, SG_SQLITE__SYNC__OFF, &psql)  );
        SG_ERR_CHECK(  SG_dbndx_query__open(pCtx, pData->pRepo, iDagNum, psql, pPath, &pndx)  );
        SG_ERR_CHECK(  SG_dbndx_query__query(pCtx, pndx, pidState, psz_rectype, pcrit, pSort, iNumRecordsToReturn, iNumRecordsToSkip, psa_fields, &pva)  );
        SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
    }
    else
    {
        SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );
    }

    *ppva = pva;
    pva = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
}

void sg_repo__fs3__dbndx__query_record_history(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_uint64 iDagNum,
    const char* psz_recid,  // this is NOT the hid of a record.  it is the zing recid
    const char* psz_rectype,
    SG_varray** ppva
	)
{
    SG_dbndx_query* pndx = NULL;
	my_instance_data * pData = NULL;
    SG_pathname* pPath = NULL;
    sqlite3* psql = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_new_audits)
    {
        SG_ERR_THROW(  SG_ERR_OLD_AUDITS  );
    }

    SG_ERR_CHECK(  sg_fs3__get_dbndx_path(pCtx, pData, iDagNum, &pPath)  );
    SG_ERR_CHECK(  fs3__get_sql(pCtx, pData, pPath, SG_SQLITE__SYNC__OFF, &psql)  );
    SG_ERR_CHECK(  SG_dbndx_query__open(pCtx, pData->pRepo, iDagNum, psql, pPath, &pndx)  );
    SG_ERR_CHECK(  SG_dbndx_query__query_record_history(pCtx, pndx, psz_recid, psz_rectype, ppva)  );

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
}

void sg_repo__fs3__dbndx__query_multiple_record_history(
	SG_context* pCtx,
    SG_repo* pRepo,
    SG_uint64 iDagNum,
    const char* psz_rectype,
    SG_varray* pva_recids,
    const char* psz_field,
    SG_vhash** ppvh
	)
{
    SG_dbndx_query* pndx = NULL;
	my_instance_data * pData = NULL;
    SG_pathname* pPath = NULL;
    sqlite3* psql = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_new_audits)
    {
        SG_ERR_THROW(  SG_ERR_OLD_AUDITS  );
    }

    SG_ERR_CHECK(  sg_fs3__get_dbndx_path(pCtx, pData, iDagNum, &pPath)  );
    SG_ERR_CHECK(  fs3__get_sql(pCtx, pData, pPath, SG_SQLITE__SYNC__OFF, &psql)  );
    SG_ERR_CHECK(  SG_dbndx_query__open(pCtx, pData->pRepo, iDagNum, psql, pPath, &pndx)  );
    SG_ERR_CHECK(  SG_dbndx_query__query_multiple_record_history(pCtx, pndx, psz_rectype, pva_recids, psz_field, ppvh)  );

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
}

void sg_repo__fs3__hash__begin(
	SG_context* pCtx,
    SG_repo * pRepo,
    SG_repo_hash_handle** ppHandle
    )
{
	my_instance_data * pData;

	SG_NULLARGCHECK_RETURN(pRepo);
	
	SG_ERR_CHECK_RETURN(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );
    SG_ARGCHECK_RETURN(pData != NULL, pRepo);

	SG_ERR_CHECK_RETURN(  sg_repo_utils__hash_begin__from_sghash(pCtx, pData->buf_hash_method, ppHandle)  );
}

void sg_repo__fs3__hash__chunk(
    SG_context* pCtx,
	SG_repo * pRepo,
    SG_repo_hash_handle* pHandle,
    SG_uint32 len_chunk,
    const SG_byte* p_chunk
    )
{
	SG_NULLARGCHECK_RETURN(pRepo);
	
    {
        void* pData = NULL;

        SG_ERR_CHECK_RETURN(  SG_repo__get_instance_data(pCtx, pRepo, &pData)  );
        SG_ARGCHECK_RETURN(pData != NULL, pRepo);
    }

	SG_ERR_CHECK_RETURN(  sg_repo_utils__hash_chunk__from_sghash(pCtx, pHandle, len_chunk, p_chunk)  );
}

void sg_repo__fs3__hash__end(
    SG_context* pCtx,
	SG_repo * pRepo,
    SG_repo_hash_handle** ppHandle,
    char** ppsz_hid_returned
    )
{
	SG_NULLARGCHECK_RETURN(pRepo);
	
    {
        void* pData = NULL;

        SG_ERR_CHECK_RETURN(  SG_repo__get_instance_data(pCtx, pRepo, &pData)  );
        SG_ARGCHECK_RETURN(pData != NULL, pRepo);
    }

	SG_ERR_CHECK_RETURN(  sg_repo_utils__hash_end__from_sghash(pCtx, ppHandle, ppsz_hid_returned)  );
}

void sg_repo__fs3__hash__abort(
    SG_context* pCtx,
	SG_repo * pRepo,
    SG_repo_hash_handle** ppHandle
    )
{
	SG_NULLARGCHECK_RETURN(pRepo);
	
    {
        void* pData = NULL;

        SG_ERR_CHECK_RETURN(  SG_repo__get_instance_data(pCtx, pRepo, &pData)  );
        SG_ARGCHECK_RETURN(pData != NULL, pRepo);
    }

	SG_ERR_CHECK_RETURN(  sg_repo_utils__hash_abort__from_sghash(pCtx, ppHandle)  );
}

//////////////////////////////////////////////////////////////////

void sg_repo__fs3__query_implementation(
    SG_context* pCtx,
    SG_uint16 question,
    SG_bool* p_bool,
    SG_int64* p_int,
    char* p_buf_string,
    SG_uint32 len_buf_string,
    SG_vhash** pp_vhash
    )
{
  SG_UNUSED(p_int);
  SG_UNUSED(p_buf_string);
  SG_UNUSED(len_buf_string);

    switch (question)
    {
	case SG_REPO__QUESTION__VHASH__LIST_HASH_METHODS:
		{
			SG_NULLARGCHECK_RETURN(pp_vhash);
			SG_ERR_CHECK_RETURN(  sg_repo_utils__list_hash_methods__from_sghash(pCtx, pp_vhash)  );
			return;
		}

	case SG_REPO__QUESTION__BOOL__SUPPORTS_LIVE_WITH_WORKING_DIRECTORY:
		{
			SG_NULLARGCHECK_RETURN(p_bool);
			*p_bool = SG_TRUE;
			return;
		}

	case SG_REPO__QUESTION__BOOL__SUPPORTS_ZLIB:
		{
			SG_NULLARGCHECK_RETURN(p_bool);
			*p_bool = SG_TRUE;
			return;
		}

	case SG_REPO__QUESTION__BOOL__SUPPORTS_VCDIFF:
		{
			SG_NULLARGCHECK_RETURN(p_bool);
			*p_bool = SG_TRUE; // TODO make this false
			return;
		}

	default:
		SG_ERR_THROW_RETURN(  SG_ERR_NOTIMPLEMENTED  );
	}
}

void sg_repo__fs3__query_blob_existence(
	SG_context* pCtx,
	SG_repo * pRepo,
	SG_stringarray* psaQueryBlobHids,
	SG_stringarray** ppsaNonexistentBlobs
	)
{
	my_instance_data * pData = NULL;
	SG_stringarray* psaNonexistentBlobs = NULL;
	sqlite3_stmt* pStmt = NULL;
	const char* pszHid = NULL;
	SG_uint32 count, i;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(psaQueryBlobHids);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

	SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaQueryBlobHids, &count)  );
	SG_ERR_CHECK(  SG_STRINGARRAY__ALLOC(pCtx, &psaNonexistentBlobs, count)  );

	SG_RETRY_THINGIE
	(
		SG_ERR_CHECK(  sg_sqlite__prepare(pCtx, pData->psql, &pStmt,
			"SELECT EXISTS(SELECT \"filename\" from \"blobs\" where \"hid\" = ?)")  );

		for	(i = 0; i < count; i++)
		{
			SG_bool exists;

			SG_ERR_CHECK(  sg_sqlite__reset(pCtx, pStmt)  );
			SG_ERR_CHECK(  sg_sqlite__clear_bindings(pCtx, pStmt)  );

			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaQueryBlobHids, i, &pszHid)  );

			SG_ERR_CHECK(  sg_sqlite__bind_text(pCtx, pStmt, 1, pszHid)  );
			SG_ERR_CHECK(  sg_sqlite__step(pCtx, pStmt, SQLITE_ROW)  );
			exists = sqlite3_column_int(pStmt, 0);
			if (!exists)
				SG_ERR_CHECK(  SG_stringarray__add(pCtx, psaNonexistentBlobs, pszHid)  );
		}

		SG_RETURN_AND_NULL(psaNonexistentBlobs, ppsaNonexistentBlobs);

		SG_ERR_CHECK(  sg_sqlite__finalize(pCtx, pStmt)  );
	);

	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
	SG_STRINGARRAY_NULLFREE(pCtx, psaNonexistentBlobs);
}

void sg_repo__fs3__treendx__get_paths(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        SG_vhash* pvh_gids,
        SG_vhash** ppvh
        )
{
    SG_treendx* pTreeNdx = NULL;
	my_instance_data * pData = NULL;
    SG_vhash* pvh = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_ERR_CHECK(  _fs3_my_get_treendx(pCtx, pData, iDagNum, SG_TRUE, &pTreeNdx)  );

    SG_ERR_CHECK(  SG_treendx__get_paths(pCtx, pTreeNdx, pvh_gids, &pvh)  );

    *ppvh = pvh;
    pvh = NULL;

fail:
    SG_TREENDX_NULLFREE(pCtx, pTreeNdx);
    SG_VHASH_NULLFREE(pCtx, pvh);
}

void sg_repo__fs3__treendx__search_changes(
        SG_context* pCtx,
        SG_repo* pRepo,
        SG_uint64 iDagNum,
        SG_stringarray* psa_gids,
        SG_vhash** ppvh
        )
{
    SG_treendx* pTreeNdx = NULL;
	my_instance_data * pData = NULL;

    SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_ERR_CHECK(  _fs3_my_get_treendx(pCtx, pData, iDagNum, SG_TRUE, &pTreeNdx)  );

    SG_ERR_CHECK(  SG_treendx__get_changesets_where_any_of_these_gids_changed(pCtx, pTreeNdx, psa_gids, ppvh)  );
    SG_TREENDX_NULLFREE(pCtx, pTreeNdx);

    return;

fail:
    SG_TREENDX_NULLFREE(pCtx, pTreeNdx);
    return;
}

void sg_repo__fs3__get_hash_method(
        SG_context* pCtx,
        SG_repo* pRepo,
        char** ppsz_result  /* caller must free */
        )
{
	my_instance_data * pData = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );
    SG_ERR_CHECK(  SG_STRDUP(pCtx, pData->buf_hash_method, ppsz_result)  );

    return;
fail:
    return;
}

void sg_repo__fs3__verify__dag_consistency(
    SG_context* pCtx,
    SG_repo * pRepo,
    SG_uint64 dagnum,
    SG_vhash** pp_results
    )
{
	my_instance_data * pData = NULL;

	SG_UNUSED(pp_results); // for future use

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_ERR_CHECK(  SG_dag_sqlite3__check_consistency(pCtx, pData->psql, dagnum)  );

fail:
    ;
}

void sg_repo__fs3__verify__dbndx_states(
    SG_context* pCtx,
    SG_repo * pRepo,
    SG_uint64 dagnum,
    SG_vhash** pp_results
    )
{
	my_instance_data * pData = NULL;
    SG_pathname* pPath_dbndx = NULL;
    SG_dbndx_query* pndx = NULL;
    sqlite3* psql = NULL;

	SG_UNUSED(pp_results); // for future use

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (SG_DAGNUM__IS_DB(dagnum))
    {
        SG_bool b_exists = SG_FALSE;

        SG_ERR_CHECK(  sg_fs3__get_dbndx_path(pCtx, pData, dagnum, &pPath_dbndx)  );
        SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath_dbndx, &b_exists, NULL, NULL)  );
        if (b_exists)
        {
            SG_ERR_CHECK(  fs3__get_sql(pCtx, pData, pPath_dbndx, SG_SQLITE__SYNC__OFF, &psql)  );
            SG_ERR_CHECK(  SG_dbndx_query__open(pCtx, pRepo, dagnum, psql, pPath_dbndx, &pndx)  );

            SG_ERR_CHECK(  sg_dbndx__verify_all_filters(pCtx, pndx)  );

            SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
        }
        SG_PATHNAME_NULLFREE(pCtx, pPath_dbndx);
    }

fail:
    ;
}

void sg_repo__fs3__get_blob(
    SG_context* pCtx,
	SG_repo * pRepo,
    const char* psz_hid_blob,
    SG_blob** ppHandle
    )
{
    SG_blob* pb = NULL;
    sg_blob_fs3_handle_fetch* pbh = NULL;
    SG_uint32 got = 0;
    SG_bool b_done = SG_FALSE;

    SG_uint32 filenumber = 0;
    SG_uint64 offset = 0;
    SG_blob_encoding blob_encoding_stored = 0;
    SG_uint64 len_encoded_stored = 0;
    SG_uint64 len_full_stored = 0;
    char* psz_hid_vcdiff_reference_stored = NULL;
	my_instance_data * pData = NULL;

	SG_NULLARGCHECK_RETURN(psz_hid_blob);
	SG_NULLARGCHECK_RETURN(ppHandle);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_ERR_CHECK(  do_fetch_info(pCtx, pData, psz_hid_blob, &filenumber, &offset, &blob_encoding_stored, &len_encoded_stored, &len_full_stored, &psz_hid_vcdiff_reference_stored)  );

    if (!pb)
    {
        SG_uint64 so_far = 0;

        SG_ERR_CHECK(  sg_fs3__open_blob_for_reading__already_got_info(
                    pCtx, 
                    pData,
                    psz_hid_blob,
                    SG_TRUE,
                    SG_TRUE, // TODO b_open_file,
                    filenumber,
                    offset,
                    blob_encoding_stored,
                    len_encoded_stored,
                    len_full_stored,
                    &psz_hid_vcdiff_reference_stored,
                    &pbh
                    )  );

        pbh->blob_encoding_returning = SG_BLOBENCODING__FULL;

        if (SG_IS_BLOBENCODING_ZLIB(pbh->blob_encoding_stored))
        {
            int zError;

            pbh->b_uncompressing = SG_TRUE;

            memset(&pbh->zStream,0,sizeof(pbh->zStream));
            zError = inflateInit(&pbh->zStream);
            if (zError != Z_OK)
            {
                SG_ERR_THROW(  SG_ERR_ZLIB(zError)  );
            }
        }
        else if (SG_BLOBENCODING__VCDIFF == pbh->blob_encoding_stored)
        {
            pbh->b_undeltifying = SG_TRUE;
            SG_ASSERT(pbh->psz_hid_vcdiff_reference_stored_freeme);
            SG_ERR_CHECK(  sg_blob_fs3__setup_undeltify(pCtx, pData, psz_hid_blob, pbh)  );
        }
        else
        {
            pbh->b_undeltifying = SG_FALSE;
            pbh->b_uncompressing = SG_FALSE;
        }

        SG_ERR_CHECK(  SG_alloc1(pCtx, pb)  );
        pb->length = pbh->len_full_stored;
        SG_ERR_CHECK(  SG_alloc(pCtx, (SG_uint32) (pbh->len_full_stored), 1, &pb->data)  );
        while (!b_done)
        {
            SG_ERR_CHECK(  sg_blob_fs3__fetch_blob__chunk(pCtx, pbh, (SG_uint32) (pbh->len_full_stored - so_far), pb->data + so_far, &got, &b_done)  );
            so_far += got;
        }
        SG_ERR_CHECK(  sg_blob_fs3__fetch_blob__end(pCtx, &pbh)  );
    }

    *ppHandle = pb;
    pb = NULL;

fail:
    if (pbh)
    {
        SG_ERR_CHECK(  sg_blob_fs3__fetch_blob__abort(pCtx, &pbh)  );
    }
    if (pb)
    {
        SG_NULLFREE(pCtx, pb->data);
        SG_NULLFREE(pCtx, pb);
    }
}
             
void sg_repo__fs3__release_blob(
    SG_context* pCtx,
	SG_repo * pRepo,
    SG_blob** ppBlob
    )
{
    SG_blob* pb = NULL;

    SG_UNUSED(pRepo);
    if (!ppBlob || !*ppBlob)
    {
        return;
    }

    pb = *ppBlob;

    SG_NULLFREE(pCtx, pb->data);
    SG_NULLFREE(pCtx, pb);
    *ppBlob = NULL;
}

DCL__REPO_VTABLE("fs3", fs3);

sg_repo__vtable* SG_fs3__get_vtable(void)
{
    return &s_repo_vtable__fs3;
}

void sg_repo__fs3__dbndx__make_delta_from_path(
	SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint64 dagnum,
    SG_varray* pva_path,
    SG_uint32 flags,
    SG_vhash* pvh_add,
    SG_vhash* pvh_remove
	)
{
    SG_dbndx_query* pndx = NULL;
	my_instance_data * pData = NULL;
    SG_pathname* pPath = NULL;
    SG_bool b_exists = SG_FALSE;

    SG_NULLARGCHECK_RETURN(pRepo);

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    if (!pData->b_new_audits)
    {
        SG_ERR_THROW(  SG_ERR_OLD_AUDITS  );
    }

    SG_ERR_CHECK(  sg_fs3__get_dbndx_path(pCtx, pData, dagnum, &pPath)  );
    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &b_exists, NULL, NULL)  );
    if (b_exists)
    {
        sqlite3* psql = NULL;

        SG_ERR_CHECK(  fs3__get_sql(pCtx, pData, pPath, SG_SQLITE__SYNC__OFF, &psql)  );
        SG_ERR_CHECK(  SG_dbndx_query__open(pCtx, pData->pRepo, dagnum, psql, pPath, &pndx)  );
        SG_ERR_CHECK(  sg_dbndx_query__make_delta_from_path(pCtx, pndx, pva_path, flags, pvh_add, pvh_remove)  );
        SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
    }

fail:
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_DBNDX_QUERY_NULLFREE(pCtx, pndx);
}

static void sg_fs3__get_vec_templates_from_vhashes(
	SG_context* pCtx,
    SG_uint64 dagnum,
    SG_vhash* pvh_templates,
    SG_vector** ppv
    )
{
    SG_uint32 count = 0;
    SG_uint32 i = 0;
    SG_vector* pvec_templates = NULL;
    SG_zingtemplate* pzt = NULL;
    SG_vhash* pvh_copy = NULL;

    SG_NULLARGCHECK_RETURN(pvh_templates);
    SG_NULLARGCHECK_RETURN(ppv);

    SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_templates, &count)  );
    SG_ERR_CHECK(  SG_vector__alloc(pCtx, &pvec_templates, count)  );
    for (i=0; i<count; i++)
    {
        SG_vhash* pvh_t = NULL;

        SG_ERR_CHECK(  SG_vhash__get_nth_pair__vhash(pCtx, pvh_templates, i, NULL, &pvh_t)  );

        SG_ERR_CHECK(  SG_vhash__alloc__copy(pCtx, &pvh_copy, pvh_t)  );
        SG_ERR_CHECK(  SG_zingtemplate__alloc(pCtx, dagnum, &pvh_copy, &pzt)  );
        SG_ERR_CHECK(  SG_vector__append(pCtx, pvec_templates, pzt, NULL)  );
        pzt = NULL;
    }

    *ppv = pvec_templates;
    pvec_templates = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_copy);
    SG_ZINGTEMPLATE_NULLFREE(pCtx, pzt);
    SG_VECTOR_NULLFREE_WITH_ASSOC(pCtx, pvec_templates, (SG_free_callback *)SG_zingtemplate__free);
}

static void sg_fs3__cloning__get_dbndx_update(
	SG_context* pCtx,
    my_instance_data* pData,
    SG_uint64 dagnum,
    SG_dbndx_update** ppndx
    )
{
    char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX + 10];
    SG_bool b_exists = SG_FALSE;
    SG_bool b_found = SG_FALSE;
    SG_dbndx_update* pndx = NULL;
    SG_pathname* pPath = NULL;
    sqlite3* psql = NULL;
    sqlite3* psql_new = NULL;
    SG_varray* pva_template_hids = NULL;
    SG_string* pstr_new_schema_token = NULL;
    SG_vhash* pvh_schema = NULL;
    SG_vector* pvec_templates = NULL;
    SG_bool b_no_index = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pData);
	SG_NULLARGCHECK_RETURN(ppndx);

    SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, dagnum, buf_dagnum, sizeof(buf_dagnum))  );

    SG_ERR_CHECK(  SG_rbtree__find(pCtx, pData->ptx->cloning.prb_dbndx_update, buf_dagnum, &b_found, (void**) &pndx)  );
    if (pndx)
    {
        *ppndx = pndx;
        pndx = NULL;
        goto done;
    }

    // not there yet.  let's get things set up

    SG_ERR_CHECK(  sg_fs3__get_dbndx_path(pCtx, pData, dagnum, &pPath)  );

    b_exists = SG_FALSE;
    SG_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPath, &b_exists, NULL, NULL)  );
    if (!b_exists)
    {
        if (SG_DAGNUM__HAS_HARDWIRED_TEMPLATE(dagnum))
        {
            SG_zingtemplate* pzt = NULL;

            SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva_template_hids)  );
            SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva_template_hids, "1")  );

            SG_zing__get_cached_template__static_dagnum(pCtx, dagnum, &pzt);
            if (
                    SG_CONTEXT__HAS_ERR(pCtx)
                    && (SG_context__err_equals(pCtx, SG_ERR_ZING_TEMPLATE_NOT_FOUND))
                    )
            {
                SG_context__err_reset(pCtx);
                SG_VECTOR_NULLFREE(pCtx, pvec_templates);
            }
            else
            {
                SG_ERR_CHECK(  SG_vector__alloc(pCtx, &pvec_templates, 1)  );
                SG_ERR_CHECK(  SG_vector__append(pCtx, pvec_templates, pzt, NULL)  );
                SG_ERR_CHECK(  SG_dbndx__create_composite_schema_for_all_templates(pCtx, dagnum, pvec_templates, &pvh_schema)  );
                SG_VECTOR_NULLFREE(pCtx, pvec_templates);
            }
        }
        else
        {
            SG_vhash* pvh_templates = NULL;

            // the templates are supposed to be there by now
            SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pData->ptx->cloning.pvh_templates, buf_dagnum, &pvh_templates)  );
            if (pvh_templates)
            {
                SG_ERR_CHECK(  SG_vhash__sort(pCtx, pvh_templates, SG_FALSE, SG_vhash_sort_callback__increasing)  );
                SG_ERR_CHECK(  SG_vhash__get_keys(pCtx, pvh_templates, &pva_template_hids)  );

                SG_ERR_CHECK(  sg_fs3__get_vec_templates_from_vhashes(pCtx, dagnum, pvh_templates, &pvec_templates)  );
                SG_ERR_CHECK(  SG_dbndx__create_composite_schema_for_all_templates(pCtx, dagnum, pvec_templates, &pvh_schema)  );
                SG_VECTOR_NULLFREE_WITH_ASSOC(pCtx, pvec_templates, (SG_free_callback *)SG_zingtemplate__free);
            }
        }

        if (pvh_schema)
        {
            SG_ERR_CHECK(  sg_sqlite__create__pathname(pCtx, pPath, SG_SQLITE__SYNC__OFF, &psql_new)  );
            SG_ERR_CHECK(  sg_sqlite__exec(pCtx, psql_new, "PRAGMA temp_store=2")  );

            SG_ERR_CHECK(  sg_fs3_create_schema_token(pCtx, pva_template_hids, &pstr_new_schema_token)  );
            SG_ERR_CHECK(  SG_dbndx__create_new(pCtx, dagnum, psql_new, SG_string__sz(pstr_new_schema_token), pvh_schema)  );
            SG_ERR_CHECK(  sg_sqlite__close(pCtx, psql_new)  );
            psql_new = NULL;
            SG_VHASH_NULLFREE(pCtx, pvh_schema);
            SG_STRING_NULLFREE(pCtx, pstr_new_schema_token);
        }
        else
        {
            b_no_index = SG_TRUE;
        }
    }

    if (b_no_index)
    {
        pndx = NULL;
    }
    else
    {
        SG_ERR_CHECK(  fs3__get_sql(pCtx, pData, pPath, SG_SQLITE__SYNC__OFF, &psql)  );

        SG_ERR_CHECK(  SG_dbndx_update__open(pCtx, pData->pRepo, dagnum, psql, &pndx)  );
        SG_ERR_CHECK(  SG_dbndx_update__begin(pCtx, pndx)  );

        SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pData->ptx->cloning.prb_dbndx_update, buf_dagnum, pndx)  );
    }

    *ppndx = pndx;
    pndx = NULL;

done:
fail:
    if (psql_new)
    {
        SG_ERR_IGNORE(  sg_sqlite__close(pCtx, psql_new)  );
    }
    SG_VARRAY_NULLFREE(pCtx, pva_template_hids);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_STRING_NULLFREE(pCtx, pstr_new_schema_token);
    SG_VHASH_NULLFREE(pCtx, pvh_schema);
}

static void store_json_blob(
	SG_context* pCtx,
    SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    const char* psz_json,
    SG_uint32 len_full,
    const char* psz_hid_known,
    char** ppsz_hid_returned
    )
{
    SG_repo_store_blob_handle* pbh = NULL;

	SG_NULLARGCHECK_RETURN(pTx);
	SG_NULLARGCHECK_RETURN(psz_json);
	SG_NULLARGCHECK_RETURN(ppsz_hid_returned);

    SG_ERR_CHECK(  SG_repo__store_blob__begin(pCtx, pRepo, pTx, SG_BLOBENCODING__FULL, NULL, len_full, 0, psz_hid_known, &pbh)  );
    SG_ERR_CHECK(  SG_repo__store_blob__chunk(pCtx, pRepo, pbh, len_full, (SG_byte*) psz_json, NULL)  );
    SG_ERR_CHECK(  SG_repo__store_blob__end(pCtx, pRepo, pTx, &pbh, ppsz_hid_returned)  );
	pbh = NULL;

    /* fall through */

fail:
    if (pbh)
    {
        SG_ERR_IGNORE(  SG_repo__store_blob__abort(pCtx, pRepo, pTx, &pbh)  );
	}
}

void sg_repo__fs3__store_blob__changeset(
	SG_context* pCtx,
    SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    const char* psz_json,
    SG_uint32 len_full,
    const char* psz_hid_known,
    SG_vhash** ppvh,
    char** ppsz_hid_returned
    )
{
    char* psz_hid_returned = NULL;
	my_instance_data * pData = NULL;
    SG_vhash* pvh = NULL;

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    SG_ARGCHECK_RETURN(pData->ptx == ((my_tx_data*) pTx), pTx);

    // TODO require psz_hid_known or ppsz_hid_returned

    SG_ERR_CHECK(  store_json_blob(pCtx, pRepo, pTx, psz_json, len_full, psz_hid_known, &psz_hid_returned)  );

    if (ppsz_hid_returned)
    {
        *ppsz_hid_returned = psz_hid_returned;
        psz_hid_returned = NULL;
    }

    SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, &pvh, psz_json));

    if (pData->ptx->flags & SG_REPO_TX_FLAG__CLONING)
    {
        const char* psz_dagnum = NULL;
        SG_uint64 dagnum = 0;

        SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, "dagnum", &psz_dagnum)  );
        SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, psz_dagnum, &dagnum)  );

        if (SG_DAGNUM__IS_DB(dagnum))
        {
            SG_dbndx_update* pndx = NULL;

            SG_ERR_CHECK(  sg_fs3__cloning__get_dbndx_update(pCtx, pData, dagnum, &pndx)  );
            if (pndx)
            {
                SG_vhash* pvh_db = NULL;
                SG_int32 gen = -1;
                SG_int64 csidrow = -1;

                SG_ERR_CHECK(  SG_vhash__get__int32(pCtx, pvh, "generation", &gen)  );

                SG_ERR_CHECK(  sg_dbndx__add_csid(pCtx, pndx, psz_hid_returned, gen, &csidrow)  );

                SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh, "db", &pvh_db)  );
                if (pvh_db)
                {
                    SG_vhash* pvh_changes = NULL;

                    SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvh_db, "changes", &pvh_changes)  );
                    if (pvh_changes)
                    {
                        SG_ERR_CHECK(  sg_dbndx__add_deltas(pCtx, pndx, csidrow, pvh_changes)  );
                        SG_ERR_CHECK(  sg_dbndx__add_db_history_for_new_records(pCtx, pndx, csidrow, pvh_changes)  );
                    }
                }
            }
        }
    }

    if (ppvh)
    {
        *ppvh = pvh;
        pvh = NULL;
    }

fail:
    SG_NULLFREE(pCtx, psz_hid_returned);
    SG_VHASH_NULLFREE(pCtx, pvh);
}

void sg_repo__fs3__store_blob__dbrecord(
	SG_context* pCtx,
    SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    const char* psz_json,
    SG_uint32 len_full,
    SG_uint64 dagnum,
    const char* psz_hid_known,
    SG_vhash** ppvh,
    char** ppsz_hid_returned
    )
{
    char* psz_hid_returned = NULL;
	my_instance_data * pData = NULL;
    SG_vhash* pvh = NULL;

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    // TODO require psz_hid_known or ppsz_hid_returned

    SG_ERR_CHECK(  store_json_blob(pCtx, pRepo, pTx, psz_json, len_full, psz_hid_known, &psz_hid_returned)  );

    if (ppsz_hid_returned)
    {
        *ppsz_hid_returned = psz_hid_returned;
        psz_hid_returned = NULL;
    }

    if (pData->ptx->flags & SG_REPO_TX_FLAG__CLONING)
    {
        SG_dbndx_update* pndx = NULL;

        SG_ERR_CHECK(  SG_vhash__alloc__from_json__buflen__utf8_fix(pCtx, &pvh, psz_json, len_full));
        SG_ERR_CHECK(  sg_fs3__cloning__get_dbndx_update(pCtx, pData, dagnum, &pndx)  );
        if (pndx)
        {
            SG_ERR_CHECK(  sg_dbndx_update__one_record(pCtx, pndx, psz_hid_returned, pvh)  );
        }
    }

    if (ppvh)
    {
        if (!pvh)
        {
            SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, &pvh, psz_json));
        }

        *ppvh = pvh;
        pvh = NULL;
    }

fail:
    SG_NULLFREE(pCtx, psz_hid_returned);
    SG_VHASH_NULLFREE(pCtx, pvh);
}

void sg_repo__fs3__store_blob__dbtemplate(
	SG_context* pCtx,
    SG_repo * pRepo,
	SG_repo_tx_handle* pTx,
    const char* psz_json,
    SG_uint32 len_full,
    SG_uint64 dagnum,
    const char* psz_hid_known,
    SG_vhash** ppvh,
    char** ppsz_hid_returned
    )
{
    char* psz_hid_returned = NULL;
	my_instance_data * pData = NULL;
    SG_vhash* pvh = NULL;

	SG_ERR_CHECK(  SG_repo__get_instance_data(pCtx, pRepo, (void**) &pData)  );

    // TODO require psz_hid_known or ppsz_hid_returned

    SG_ERR_CHECK(  store_json_blob(pCtx, pRepo, pTx, psz_json, len_full, psz_hid_known, &psz_hid_returned)  );

    if (ppsz_hid_returned)
    {
        *ppsz_hid_returned = psz_hid_returned;
        psz_hid_returned = NULL;
    }

    if (pData->ptx->flags & SG_REPO_TX_FLAG__CLONING)
    {
        SG_vhash* pvh_templates = NULL;
        char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX + 10];

        SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, &pvh, psz_json));
        SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, dagnum, buf_dagnum, sizeof(buf_dagnum))  );
        SG_ERR_CHECK(  SG_vhash__ensure__vhash(pCtx, pData->ptx->cloning.pvh_templates, buf_dagnum, &pvh_templates)  );
        SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_templates, psz_hid_known, &pvh)  );
    }

    if (ppvh)
    {
        if (!pvh)
        {
            SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, &pvh, psz_json));
        }

        *ppvh = pvh;
        pvh = NULL;
    }

fail:
    SG_NULLFREE(pCtx, psz_hid_returned);
    SG_VHASH_NULLFREE(pCtx, pvh);
}

