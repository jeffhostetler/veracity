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
 * @file sg_fragball_prototypes.h
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_FRAGBALL_PROTOTYPES_H
#define H_SG_FRAGBALL_PROTOTYPES_H

BEGIN_EXTERN_C;

void SG_fragball__write__bindex(SG_context * pCtx, SG_fragball_writer* pfb, SG_pathname* pPath, const SG_vhash* pvh_offsets);
void SG_fragball__write__bfile(SG_context * pCtx, SG_fragball_writer* pfb, SG_pathname* pPath, const char* psz_name, SG_uint64 maxlen, SG_uint64* pi_offset);

void SG_fragball__v3__read_object_header(
        SG_context * pCtx, 
        SG_file* pFile, 
        SG_uint16* pi16_type,
        SG_uint16* pi16_flags,
        SG_uint32* pi32_len_json,
        SG_uint32* pi32_len_zlib_json,
        SG_uint64* pi64_len_payload
        );

void SG_fragball__v3__read_json(
        SG_context* pCtx,
        SG_file* pFile,
        SG_uint16 flags,
        SG_uint32 len_json,
        SG_uint32 len_zlib_json,
        SG_vhash** ppvh
        );

void SG_fragball__v1__read_object_header(SG_context * pCtx, SG_file* pFile, SG_vhash** ppvh);

void SG_fragball_writer__alloc(SG_context * pCtx, SG_repo* pRepo, const SG_pathname* pPathFragball, SG_bool bCreateNewFile, SG_uint32 version, SG_fragball_writer** ppResult);
void SG_fragball_writer__close(SG_context * pCtx, SG_fragball_writer* pfb);
void SG_fragball_writer__free(SG_context * pCtx, SG_fragball_writer* pfb);

void SG_fragball__write_blob__from_handle(
	SG_context* pCtx,
	SG_fragball_writer* pWriter,
	SG_repo_fetch_blob_handle** ppBlob,
	const char* pszHid,
	SG_blob_encoding encoding,
	const char* pszHidVcdiffRef,
	SG_uint64 lenEncoded,
	SG_uint64 lenFull);

void SG_fragball__write__blobs(SG_context * pCtx, SG_fragball_writer* pfb, const char* const* pasz_blob_hids, SG_uint32 countHids);
void SG_fragball__write__dagnodes(SG_context * pCtx, SG_fragball_writer* pfb, SG_uint64 iDagNum, SG_rbtree* prb_ids);

void SG_fragball__write_blob(SG_context * pCtx, SG_fragball_writer* pfb, const char* psz_hid);
void SG_fragball__write__frag(SG_context * pCtx, SG_fragball_writer* pfb, SG_dagfrag* pFrag);
void SG_fragball__tell(SG_context * pCtx, SG_fragball_writer* pfb, SG_uint64* pi);
void SG_fragball__get_repo(SG_context * pCtx, SG_fragball_writer* pfb, SG_repo** pp);

END_EXTERN_C;

#endif //H_SG_FRAGBALL_PROTOTYPES_H

