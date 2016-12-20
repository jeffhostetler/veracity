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

#ifndef H_SG_BLOBSET_PROTOTYPES_H
#define H_SG_BLOBSET_PROTOTYPES_H

BEGIN_EXTERN_C;

void SG_blobset__insert(
	SG_context * pCtx,
    SG_blobset* pbc,
    const char* psz_hid,
    const char* psz_filename,
    SG_uint64 offset,
    SG_blob_encoding encoding,
    SG_uint64 len_encoded,
    SG_uint64 len_full,
    const char* psz_hid_vcdiff
    );

void SG_blobset__create(
	SG_context * pCtx,
    SG_blobset** pp
    );

void SG_blobset__begin_inserting(
	SG_context * pCtx,
    SG_blobset* pbc
    );

void SG_blobset__finish_inserting(
	SG_context * pCtx,
    SG_blobset* pbc
    );

void SG_blobset__inserting(
	SG_context* pCtx,
    SG_blobset* pbs,
    SG_bool* pb
    );

void SG_blobset__free(
	SG_context * pCtx,
    SG_blobset* pbc
    );

void SG_blobset__copy_into_fs3(
	SG_context * pCtx,
    SG_blobset* pbc,
    sqlite3* psql_to
    );

void SG_blobset__lookup__vhash(
        SG_context * pCtx,
        SG_blobset* pbc,
        const char * szHidBlob,
        SG_vhash** ppvh
        );

void SG_blobset__lookup(
        SG_context * pCtx,
        SG_blobset* pbc,
        const char * szHidBlob,
        SG_bool* pb_found,
        char** ppsz_filename,
        SG_uint64* p_offset,
        SG_blob_encoding* p_blob_encoding,
        SG_uint64* p_len_encoded,
        SG_uint64* p_len_full,
        char** ppsz_hid_vcdiff_reference
        );

void SG_blobset__count(
	SG_context* pCtx,
    SG_blobset* pbs,
    SG_uint32* pi_count
    );

void SG_blobset__compare_to(
	SG_context* pCtx,
    SG_blobset* pbs,
    SG_blobset* pbs_other,
    SG_bool* pb
    );

void SG_blobset__get_stats(
        SG_context * pCtx,
        SG_blobset* pbs,
        SG_blob_encoding encoding,
        SG_uint32* p_count,
        SG_uint64* p_len_encoded,
        SG_uint64* p_len_full,
        SG_uint64* p_max_len_encoded,
        SG_uint64* p_max_len_full
        );

void SG_blobset__get(
        SG_context * pCtx,
        SG_blobset* pbs,
        SG_uint32 limit,
        SG_uint32 skip,
        SG_vhash** ppvh
        );

void SG_blobset__create_blobs_table(
        SG_context* pCtx, 
        sqlite3* psql
        );

END_EXTERN_C;

#endif //H_SG_BLOBSET_PROTOTYPES_H

