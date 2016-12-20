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

#include <sg.h>

/* TODO put a magic number at the top of a fragball file? */

/* TODO #defines for the vhash keys */

struct _sg_fragball_writer
{
    SG_file* pFile;
    SG_repo* pRepo;
    SG_byte* buf;
    SG_uint32 buf_size;
    SG_uint32 version;
};

/*
 * A fragball is a byte stream (usually a file) which contains
 * a collection of dagfrags and blobs.  The structure of a
 * fragball is a series of objects.  Each object is:
 *
 * 4 bytes containing the length of the header
 *
 * A header, which is an uncompressed JSON object.
 * The length includes a trailing zero.
 *
 * Optional:  Payload, as specified in the object header.
 *
 *
 * The contents of the object header vary depending on
 * what kind of object it is.
 *
 * The first object in every fragball specifies what
 * version of the fragball format is being used.  For
 * now, this is version 1.
 *
 * Other kinds of objects:
 *
 * frag (header contains a dagnum and a frag)
 *
 * blob (header contains encoding and lengths.  payload
 * contains the encoded blob)
 *
 */

void SG_fragball__v3__read_object_header(
        SG_context * pCtx, 
        SG_file* pFile, 
        SG_uint16* pi16_type,
        SG_uint16* pi16_flags,
        SG_uint32* pi32_len_json,
        SG_uint32* pi32_len_zlib_json,
        SG_uint64* pi64_len_payload
        )
{
    SG_byte ba[20];

    SG_file__read(pCtx,pFile, 20, ba, NULL);

	if (SG_context__err_equals(pCtx,SG_ERR_EOF))
	{
		SG_context__err_reset(pCtx);
        *pi16_type = 0;
		return;
	}

	SG_ERR_CHECK_CURRENT;

    *pi16_type = 
          (ba[0] << 8)
        | (ba[1] << 0);

    *pi16_flags = 
          (ba[2] << 8)
        | (ba[3] << 0);

    *pi32_len_json = 
          (ba[4] << 24)
        | (ba[5] << 16)
        | (ba[6] <<  8)
        | (ba[7] <<  0);

    *pi32_len_zlib_json = 
          (ba[ 8] << 24)
        | (ba[ 9] << 16)
        | (ba[10] <<  8)
        | (ba[11] <<  0);

    *pi64_len_payload = 
          (((SG_uint64) ba[12]) << 56)
        | (((SG_uint64) ba[13]) << 48)
        | (((SG_uint64) ba[14]) << 40)
        | (((SG_uint64) ba[15]) << 32)
        | (((SG_uint64) ba[16]) << 24)
        | (((SG_uint64) ba[17]) << 16)
        | (((SG_uint64) ba[18]) <<  8)
        | (((SG_uint64) ba[19]) <<  0);

fail:
    ;
}

void SG_fragball__v3__read_json(
        SG_context* pCtx,
        SG_file* pFile,
        SG_uint16 flags,
        SG_uint32 len_json,
        SG_uint32 len_zlib_json,
        SG_vhash** ppvh
        )
{
    SG_byte* p_zlib_json = NULL;
    char* psz_json = NULL;
    SG_vhash* pvh = NULL;

    SG_UNUSED(flags);

    SG_ERR_CHECK(  SG_malloc(pCtx, len_zlib_json, &p_zlib_json)  );
    SG_ERR_CHECK(  SG_file__read(pCtx, pFile, len_zlib_json, p_zlib_json, NULL)  );

    SG_ERR_CHECK(  SG_malloc(pCtx, len_json+1, &psz_json)  );
    psz_json[len_json] = 0;
    SG_ERR_CHECK(  SG_zlib__inflate__memory(pCtx,
                p_zlib_json, len_zlib_json,
                (SG_byte*) psz_json, len_json
                )  );
    SG_NULLFREE(pCtx, p_zlib_json);

	SG_ERR_CHECK(  SG_vhash__alloc__from_json__sz(pCtx, &pvh, psz_json));
    SG_NULLFREE(pCtx, psz_json);

    *ppvh = pvh;
    pvh = NULL;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
    SG_NULLFREE(pCtx, psz_json);
    SG_NULLFREE(pCtx, p_zlib_json);
}

static void sg_fragball__v3__write_object_header(
        SG_context * pCtx, 
        SG_file* pFile, 
        SG_uint16 type, 
        SG_uint16 flags, 
        SG_uint64 len_payload, 
        SG_vhash* pvh
        )
{
    SG_string* pstr = NULL;
    SG_uint32 len_json = 0;
    SG_byte ba[20];
    SG_byte* p_zlib_json = NULL;
    SG_uint32 len_zlib_json = 0;

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx,&pstr)  );
    SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvh, pstr)  );
    len_json = SG_string__length_in_bytes(pstr);
    len_json++;

    SG_ERR_CHECK(  SG_zlib__deflate__memory(
                pCtx,
                (SG_byte*) SG_string__sz(pstr),
                len_json,
                &p_zlib_json,
                &len_zlib_json
                )  );
    SG_STRING_NULLFREE(pCtx, pstr);

    // type
    ba[ 0] = (SG_byte) ( (type >> 8) & 0xff );
    ba[ 1] = (SG_byte) ( (type >> 0) & 0xff );

    // flags
    ba[ 2] = (SG_byte) ( (flags >>  8) & 0xff );
    ba[ 3] = (SG_byte) ( (flags >>  0) & 0xff );

    // len_json
    ba[ 4] = (SG_byte) ( (len_json >> 24) & 0xff );
    ba[ 5] = (SG_byte) ( (len_json >> 16) & 0xff );
    ba[ 6] = (SG_byte) ( (len_json >>  8) & 0xff );
    ba[ 7] = (SG_byte) ( (len_json >>  0) & 0xff );

    // len_zlib_json
    ba[ 8] = (SG_byte) ( (len_zlib_json >> 24) & 0xff );
    ba[ 9] = (SG_byte) ( (len_zlib_json >> 16) & 0xff );
    ba[10] = (SG_byte) ( (len_zlib_json >>  8) & 0xff );
    ba[11] = (SG_byte) ( (len_zlib_json >>  0) & 0xff );

    // len_payload
    ba[12] = (SG_byte) ( (len_payload >> 56) & 0xff );
    ba[13] = (SG_byte) ( (len_payload >> 48) & 0xff );
    ba[14] = (SG_byte) ( (len_payload >> 40) & 0xff );
    ba[15] = (SG_byte) ( (len_payload >> 32) & 0xff );
    ba[16] = (SG_byte) ( (len_payload >> 24) & 0xff );
    ba[17] = (SG_byte) ( (len_payload >> 16) & 0xff );
    ba[18] = (SG_byte) ( (len_payload >>  8) & 0xff );
    ba[19] = (SG_byte) ( (len_payload >>  0) & 0xff );
    SG_ERR_CHECK(  SG_file__write(pCtx, pFile, 20, ba, NULL)  );

    SG_ERR_CHECK(  SG_file__write(pCtx, pFile, len_zlib_json, (SG_byte*) p_zlib_json, NULL)  );
    SG_NULLFREE(pCtx, p_zlib_json);

    return;

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

void SG_fragball__v1__read_object_header(SG_context * pCtx, SG_file* pFile, SG_vhash** ppvh)
{
    SG_byte ba_len[4];
    SG_uint32 len = 0;
    SG_byte* p = NULL;
    SG_vhash* pvh = NULL;

    SG_file__read(pCtx,pFile, 4, ba_len, NULL);

	if (SG_context__err_equals(pCtx,SG_ERR_EOF))
	{
		SG_context__err_reset(pCtx);
		*ppvh = NULL;
		return;
	}

	SG_ERR_CHECK_CURRENT;

    len = (ba_len[0] << 24)
        | (ba_len[1] << 16)
        | (ba_len[2] <<  8)
        | (ba_len[3] <<  0);

    SG_ERR_CHECK(  SG_alloc(pCtx, len, 1, &p)  );
    SG_ERR_CHECK(  SG_file__read(pCtx, pFile, len, p, NULL)  );
	SG_ERR_CHECK(  SG_VHASH__ALLOC__FROM_JSON__BUFLEN(pCtx, &pvh, (char*)p, len));
    SG_NULLFREE(pCtx, p);

    *ppvh = pvh;

    return;

fail:
    SG_NULLFREE(pCtx, p);
}

static void sg_fragball__v1__write_object_header(SG_context * pCtx, SG_file* pFile, SG_vhash* pvh)
{
    SG_string* pstr = NULL;
    SG_uint32 len = 0;
    SG_byte ba_len[4];

    SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx,&pstr)  );
    SG_ERR_CHECK(  SG_vhash__to_json(pCtx, pvh, pstr)  );
    len = SG_string__length_in_bytes(pstr);
    len++;
    ba_len[0] = (SG_byte) ( (len >> 24) & 0xff );
    ba_len[1] = (SG_byte) ( (len >> 16) & 0xff );
    ba_len[2] = (SG_byte) ( (len >>  8) & 0xff );
    ba_len[3] = (SG_byte) ( (len >>  0) & 0xff );
    SG_ERR_CHECK(  SG_file__write(pCtx, pFile, 4, ba_len, NULL)  );
    SG_ERR_CHECK(  SG_file__write(pCtx, pFile, len, (SG_byte*) SG_string__sz(pstr), NULL)  );
    SG_STRING_NULLFREE(pCtx, pstr);

    return;

fail:
    SG_STRING_NULLFREE(pCtx, pstr);
}

void SG_fragball__write_blob__from_handle(
	SG_context* pCtx,
	SG_fragball_writer* pWriter,
	SG_repo_fetch_blob_handle** ppBlob,
	const char* pszHid,
	SG_blob_encoding encoding,
	const char* pszHidVcdiffRef,
	SG_uint64 lenEncoded,
	SG_uint64 lenFull)
{
	SG_vhash* pvh = NULL;
	SG_uint64 left = 0;
	SG_repo_fetch_blob_handle* pBlob;
    SG_bool b_done = SG_FALSE;

	SG_NULLARGCHECK_RETURN(pWriter);
	SG_NULL_PP_CHECK_RETURN(ppBlob);

	pBlob = *ppBlob;

	/* write the header */
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );
    if (pWriter->version < 3)
    {
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "op", "blob")  );
    }
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "hid", pszHid)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "encoding", (SG_int64) encoding)  ); /* TODO enum string */
    if (pszHidVcdiffRef)
    {
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "vcdiff_ref", pszHidVcdiffRef)  );
    }
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "len_encoded", (SG_int64) lenEncoded)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "len_full", (SG_int64) lenFull)  );

    if (pWriter->version < 3)
    {
        SG_ERR_CHECK(  sg_fragball__v1__write_object_header(pCtx, pWriter->pFile, pvh)  );
    }
    else
    {
        SG_ERR_CHECK(  sg_fragball__v3__write_object_header(
                    pCtx, 
                    pWriter->pFile, 
                    SG_FRAGBALL_V3_TYPE__BLOB,
                    SG_FRAGBALL_V3_FLAGS__NONE,
                    lenFull,
                    pvh
                    )  );
    }
	SG_VHASH_NULLFREE(pCtx, pvh);

	left = lenEncoded;
	while (!b_done)
	{
		SG_uint32 want = pWriter->buf_size;
		SG_uint32 got = 0;

		if (want > left)
		{
			want = (SG_uint32) left;
		}
		SG_ERR_CHECK(  SG_repo__fetch_blob__chunk(pCtx, pWriter->pRepo, pBlob, want, pWriter->buf, &got, &b_done)  );
		SG_ERR_CHECK(  SG_file__write(pCtx, pWriter->pFile, got, pWriter->buf, NULL)  );

		left -= got;
	}
	SG_ERR_CHECK(  SG_repo__fetch_blob__end(pCtx, pWriter->pRepo, ppBlob)  );

    SG_ASSERT(0 == left);

	return;

fail:
	SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_fragball__write_blob(SG_context * pCtx, SG_fragball_writer* pfb, const char* psz_hid)
{
	SG_uint64 len_full;
	SG_repo_fetch_blob_handle* pbh = NULL;
	SG_blob_encoding encoding;
	char* psz_hid_vcdiff_reference = NULL;
	SG_uint64 len_encoded;

	SG_ERR_CHECK(  SG_repo__fetch_blob__begin(pCtx, pfb->pRepo, psz_hid, SG_FALSE, &encoding, &psz_hid_vcdiff_reference, &len_encoded, &len_full, &pbh)  );
	SG_ERR_CHECK(  SG_fragball__write_blob__from_handle(pCtx, pfb, &pbh, psz_hid, encoding, psz_hid_vcdiff_reference, len_encoded, len_full)  );


fail:
    SG_NULLFREE(pCtx, psz_hid_vcdiff_reference);
}

void SG_fragball_writer__close(SG_context * pCtx, SG_fragball_writer* pfb)
{
	SG_ERR_CHECK_RETURN(  SG_file__close(pCtx, &pfb->pFile)  );
}

void SG_fragball_writer__free(SG_context * pCtx, SG_fragball_writer* pfb)
{
	SG_file* pFile = NULL;

	if (!pfb)
	{
		return;
	}

	pFile = pfb->pFile;

	SG_NULLFREE(pCtx, pfb->buf);
	SG_NULLFREE(pCtx, pfb);

	SG_ERR_CHECK_RETURN(  SG_file__close(pCtx, &pFile)  );
}

void SG_fragball_writer__alloc(
        SG_context * pCtx, 
        SG_repo* pRepo, 
        const SG_pathname* pPathFragball, 
        SG_bool bCreateNewFile, 
        SG_uint32 version, 
        SG_fragball_writer** ppResult
        )
{
    SG_fragball_writer* pfb = NULL;
    SG_vhash* pvh = NULL;
    char* psz_repo_id = NULL;
    char* psz_admin_id = NULL;
    char* psz_hash_method = NULL;

	SG_NULLARGCHECK_RETURN(pPathFragball);
	SG_NULLARGCHECK_RETURN(ppResult);
    // TODO verify that version is 1 or 2 or 0

    SG_ERR_CHECK(  SG_alloc(pCtx, 1, sizeof(SG_fragball_writer), &pfb)  );

    if (version)
    {
        pfb->version = version;
    }
    else
    {
        pfb->version = 2;
    }

    pfb->pRepo = pRepo;
    pfb->buf_size = SG_STREAMING_BUFFER_SIZE;
    SG_ERR_CHECK(  SG_alloc(pCtx, pfb->buf_size, 1, &pfb->buf)  );

	if (bCreateNewFile)
	{
        char buf_version[32];

        SG_ERR_CHECK(  SG_sprintf(pCtx, buf_version, sizeof(buf_version), "%d", (int) pfb->version)  );

		SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathFragball, SG_FILE_WRONLY | SG_FILE_CREATE_NEW, 0644, &pfb->pFile)  );

        SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "op", "version")  );
        SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "version", buf_version)  );
        if (pfb->version > 1)
        {
            SG_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pfb->pRepo, &psz_repo_id)  );
            SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pfb->pRepo, &psz_admin_id)  );
            SG_ERR_CHECK(  SG_repo__get_hash_method(pCtx, pfb->pRepo, &psz_hash_method)  );

            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, SG_SYNC_REPO_INFO_KEY__REPO_ID, psz_repo_id)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, SG_SYNC_REPO_INFO_KEY__ADMIN_ID, psz_admin_id)  );
            SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, SG_SYNC_REPO_INFO_KEY__HASH_METHOD, psz_hash_method)  );
        
            SG_NULLFREE(pCtx, psz_repo_id);
            SG_NULLFREE(pCtx, psz_admin_id);
            SG_NULLFREE(pCtx, psz_hash_method);
        }

        // the main header of a fragball is always a v1 header
        SG_ERR_CHECK(  sg_fragball__v1__write_object_header(pCtx, pfb->pFile, pvh)  );
        SG_VHASH_NULLFREE(pCtx, pvh);
	}
	else
	{
		SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathFragball, SG_FILE_WRONLY | SG_FILE_OPEN_EXISTING, SG_FSOBJ_PERMS__UNUSED, &pfb->pFile)  );
		SG_ERR_CHECK(  SG_file__seek_end(pCtx, pfb->pFile, NULL)  );
	}

    *ppResult = pfb;

    return;

fail:
    SG_NULLFREE(pCtx, psz_repo_id);
    SG_NULLFREE(pCtx, psz_admin_id);
    SG_NULLFREE(pCtx, psz_hash_method);
    SG_VHASH_NULLFREE(pCtx, pvh);
	if (pfb)
	{
		SG_FILE_NULLCLOSE(pCtx, pfb->pFile);
		SG_NULLFREE(pCtx, pfb->buf);
		SG_NULLFREE(pCtx, pfb);
	}
}

void SG_fragball__tell(SG_context * pCtx, SG_fragball_writer* pfb, SG_uint64* pi)
{
    SG_ERR_CHECK_RETURN(  SG_file__tell(pCtx, pfb->pFile, pi)  );
}

static void get_dagfrag_audits(SG_context* pCtx, SG_fragball_writer* pfb, SG_dagfrag* pFrag, SG_vhash** ppvh)
{
    SG_vhash* pvh = NULL;
    SG_uint64 dagnum = 0;
    SG_varray* pva_changesets = NULL;
    SG_varray* pva_audits = NULL;

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "op", "audits")  );
    SG_ERR_CHECK(  SG_dagfrag__get_dagnum(pCtx, pFrag, &dagnum)  );
    SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh, "dagnum", dagnum)  );

    // this frag CAN have a fringe, but we do not need to write audits for
    // the fringe nodes into the fragball
	SG_ERR_CHECK(  SG_dagfrag__get_members__not_including_the_fringe(pCtx, pFrag, &pva_changesets)  );

    // TODO wouldn't it be nice if this frag had some efficient way to know
    // if it contained ALL the dagnodes (for this dag) in this repository instance?
    // Then we could call SG_repo__query_audits(dagnum) instead, which would be faster.

    SG_ERR_CHECK(  SG_repo__list_audits_for_given_changesets(pCtx, pfb->pRepo, dagnum, pva_changesets, &pva_audits)  );
    SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvh, "audits", &pva_audits)  );
 
    *ppvh = pvh;
    pvh = NULL;

fail:
    SG_VARRAY_NULLFREE(pCtx, pva_audits);
    SG_VARRAY_NULLFREE(pCtx, pva_changesets);
    SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_fragball__write__frag(SG_context * pCtx, SG_fragball_writer* pfb, SG_dagfrag* pFrag)
{
    SG_vhash* pvh = NULL;
    SG_vhash* pvh_frag = NULL;

    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "op", "frag")  );

    SG_ERR_CHECK(  SG_dagfrag__to_vhash__shared(pCtx, pFrag, pvh, &pvh_frag)  );

    SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh, "frag", &pvh_frag)  );

    if (pfb->version < 3)
    {
        SG_ERR_CHECK(  sg_fragball__v1__write_object_header(pCtx, pfb->pFile, pvh)  );
    }
    else
    {
        SG_ERR_CHECK(  sg_fragball__v3__write_object_header(
                    pCtx, 
                    pfb->pFile, 
                    SG_FRAGBALL_V3_TYPE__FRAG,
                    SG_FRAGBALL_V3_FLAGS__NONE,
                    0,
                    pvh
                    )  );
    }
    SG_VHASH_NULLFREE(pCtx, pvh);

    if (pfb->version > 1)
    {
        SG_ERR_CHECK(  get_dagfrag_audits(pCtx, pfb, pFrag, &pvh)  );
        if (pfb->version < 3)
        {
            SG_ERR_CHECK(  sg_fragball__v1__write_object_header(pCtx, pfb->pFile, pvh)  );
        }
        else
        {
            SG_ERR_CHECK(  sg_fragball__v3__write_object_header(
                        pCtx, 
                        pfb->pFile, 
                        SG_FRAGBALL_V3_TYPE__AUDITS,
                        SG_FRAGBALL_V3_FLAGS__NONE,
                        0,
                        pvh
                        )  );
        }
        SG_VHASH_NULLFREE(pCtx, pvh);
    }

    return;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_frag);
    SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_fragball__write__blobs(SG_context * pCtx, SG_fragball_writer*pfb, const char* const* pasz_blob_hids, SG_uint32 countHids)
{
    SG_uint32 i = 0;

    for (i=0; i<countHids; i++)
    {
        SG_ERR_CHECK(  SG_fragball__write_blob(pCtx, pfb, pasz_blob_hids[i])  );
    }

fail:
	// REVIEW: Jeff says:  The original version of this had nothing but "return err" after the label.
	//                     I added the following after a quick glance at the code.  Is it sufficient?
	//                     Do we care that we may have only written a partial blob to the file?

    ;
}

void SG_fragball__write__dagnodes(SG_context * pCtx, SG_fragball_writer* pfb, SG_uint64 iDagNum, SG_rbtree* prb_ids)
{
    SG_dagfrag* pFrag = NULL;
    char* psz_repo_id = NULL;
    char* psz_admin_id = NULL;

    SG_ERR_CHECK(  SG_repo__get_repo_id(pCtx, pfb->pRepo, &psz_repo_id)  );
    SG_ERR_CHECK(  SG_repo__get_admin_id(pCtx, pfb->pRepo, &psz_admin_id)  );

	// NOTE: We're trusting that the caller has verified that the dagnode hids exist in iDagNum's dag.  Should we?
	//       It's easy to introduce a subtle bug by calling this with a valid dagnode hid but the wrong dagnum.

    SG_ERR_CHECK(  SG_dagfrag__alloc(pCtx, &pFrag, psz_repo_id, psz_admin_id, iDagNum)  );
    SG_NULLFREE(pCtx, psz_repo_id);
    SG_NULLFREE(pCtx, psz_admin_id);

    SG_ERR_CHECK(  SG_dagfrag__load_from_repo__simple(pCtx, pFrag, pfb->pRepo, prb_ids)  );

    SG_ERR_CHECK(  SG_fragball__write__frag(pCtx, pfb, pFrag)  );

    SG_DAGFRAG_NULLFREE(pCtx, pFrag);

	return;

fail:
    SG_NULLFREE(pCtx, psz_repo_id);
    SG_NULLFREE(pCtx, psz_admin_id);
    SG_DAGFRAG_NULLFREE(pCtx, pFrag);
}

void SG_fragball__get_repo(SG_context * pCtx, SG_fragball_writer* pfb, SG_repo** pp)
{
    SG_NULLARGCHECK_RETURN(pfb);
    SG_NULLARGCHECK_RETURN(pp);

    *pp = pfb->pRepo;
}

static void x_copy_file_into_fragball(
        SG_context* pCtx,
        SG_fragball_writer* pfb,
        SG_pathname* pPathSrc,
		SG_uint64 maxlen
        )
{
	SG_file * pFileSrc = NULL;
	SG_byte * pBuf = NULL;
	SG_uint32 lenBuf;
	SG_uint64 lenGotTotal = 0;

	lenBuf = 32 * 1024;
	SG_ERR_CHECK(  SG_malloc(pCtx, lenBuf, &pBuf)  );

	// TODO 2012/03/23 Should we assert/verify that the 2
	// TODO            pathnames are not equal and/or
	// TODO            refer to the same actual file.

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx,
										   pPathSrc,
										   SG_FILE_OPEN_EXISTING | SG_FILE_RDONLY,
										   SG_FSOBJ_PERMS__UNUSED,
										   &pFileSrc)  );

	while (lenGotTotal < maxlen)
	{
		SG_uint32 lenReadThisTime;
		SG_uint64 lenTotalLeft = maxlen - lenGotTotal;
		SG_uint32 lenToRead;

		if (lenTotalLeft > lenBuf)
			lenToRead = lenBuf;
		else
			lenToRead = (SG_uint32)lenTotalLeft;

		SG_ERR_CHECK(  SG_file__read(pCtx, pFileSrc, lenToRead, pBuf, &lenReadThisTime)  );
		lenGotTotal += lenReadThisTime;

		{
			SG_uint32 lenWritten = 0;
			while (lenWritten < lenReadThisTime)
			{
				SG_uint32 writtenThisWrite = 0;
				SG_ERR_CHECK(  SG_file__write(pCtx, pfb->pFile, lenReadThisTime-lenWritten, pBuf+lenWritten, &writtenThisWrite)  );
				lenWritten += writtenThisWrite;
			}
		}
	}

fail:
	SG_NULLFREE(pCtx, pBuf);
	SG_FILE_NULLCLOSE(pCtx, pFileSrc);
}

void SG_fragball__write__bindex(SG_context * pCtx, SG_fragball_writer* pfb, SG_pathname* pPath, const SG_vhash* pvh_offsets)
{
	SG_uint64 len = 0;
    SG_vhash* pvh = NULL;

    if (pfb->version < 3)
    {
        SG_ERR_THROW(  SG_ERR_UNSPECIFIED  ); // TODO
    }

	SG_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPath, &len, NULL)  );

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh)  );
    SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pvh, "files", pvh_offsets)  );
    SG_ERR_CHECK(  sg_fragball__v3__write_object_header(
                pCtx, 
                pfb->pFile, 
                SG_FRAGBALL_V3_TYPE__BINDEX,
                SG_FRAGBALL_V3_FLAGS__NONE,
                len,
                pvh
                )  );


    SG_ERR_CHECK(  x_copy_file_into_fragball(pCtx, pfb, pPath, len)  );

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
}

void SG_fragball__write__bfile(SG_context * pCtx, SG_fragball_writer* pfb, SG_pathname* pPath, const char* psz_name, SG_uint64 maxlen, SG_uint64* pi_offset)
{
    SG_vhash* pvh = NULL;
    SG_uint64 offset = 0;

    if (pfb->version < 3)
    {
        SG_ERR_THROW(  SG_ERR_UNSPECIFIED  ); // TODO
    }

    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh)  );
    SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "name", psz_name)  );
    SG_ERR_CHECK(  sg_fragball__v3__write_object_header(
                pCtx, 
                pfb->pFile, 
                SG_FRAGBALL_V3_TYPE__BFILE,
                SG_FRAGBALL_V3_FLAGS__NONE,
                maxlen,
                pvh
                )  );

    SG_ERR_CHECK(  SG_file__tell(pCtx, pfb->pFile, &offset)  );

    SG_ERR_CHECK(  x_copy_file_into_fragball(pCtx, pfb, pPath, maxlen)  );

    *pi_offset = offset;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh);
}


