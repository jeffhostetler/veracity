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
 * @file sg_pendingdb_prototypes.h
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_PENDINGDB_PROTOTYPES_H
#define H_SG_PENDINGDB_PROTOTYPES_H

BEGIN_EXTERN_C;

void SG_pendingdb__alloc(
    SG_context* pCtx,
	SG_repo* pRepo,
    SG_uint64 iDagNum,
    const char* psz_hid_baseline,
    const char* psz_hid_template,
	SG_pendingdb** ppThis
	);

void SG_pendingdb__free(SG_context* pCtx, SG_pendingdb* pPendingdb);

void SG_pendingdb__add(
    SG_context* pCtx,
	SG_pendingdb* pPendingDb,
    SG_dbrecord ** ppRecord,
    const char** ppsz_hid
    );

// This adds a record by only adding its HID to the list.
// It is assumed that the record already existed.  Trivial
// merge uses this.
void SG_pendingdb__add__hidrec(
    SG_context* pCtx,
	SG_pendingdb* pPendingDb,
    const char* psz_hid_rec
    );

void SG_pendingdb__set_template(
	SG_context* pCtx,
	SG_pendingdb* pPendingDb,
    SG_vhash* pvh
    );

void SG_pendingdb__add_attachment__from_memory(
    SG_context* pCtx,
	SG_pendingdb * pPendingDb,
    void* pBytes,
    SG_uint32 len,
    char** ppsz_hid
    );

void SG_pendingdb__add_attachment__from_pathname(
    SG_context* pCtx,
	SG_pendingdb * pPendingDb,
    SG_pathname* pPath,
    char** ppsz_hid
    );

void SG_pendingdb__remove(
    SG_context* pCtx,
	SG_pendingdb* pPendingDb,
    const char* pszidHidRecord
    );

void SG_pendingdb__add_parent(
    SG_context* pCtx,
	SG_pendingdb* pPendingDb,
    const char* psz_hid_cs_parent
    );

void SG_pendingdb__set_baseline_delta(
	SG_context* pCtx,
	SG_pendingdb* pdb,
    const char* psz_csid_parent,
    struct sg_zing_merge_delta* pd
    );

/** After calling this, any dbrecords you passed in to pendingdb__add will be
 * freed.
 * */
void SG_pendingdb__commit(
	SG_context* pCtx,
	SG_pendingdb* pThis,
    const SG_audit* pq,
    SG_changeset** ppcs,
    SG_dagnode** ppdn
    );

END_EXTERN_C;

#endif //H_SG_PENDINGDB_PROTOTYPES_H
