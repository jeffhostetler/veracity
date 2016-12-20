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
 * @file sg_treendx_prototypes.h
 *
 */

#ifndef H_SG_TREENDX_H
#define H_SG_TREENDX_H

BEGIN_EXTERN_C

/**
 * Create a new treendx object.  If the sql file doesn't exist yet,
 * it will be created.
 *
 */
void SG_treendx__open(
	SG_context*,

	SG_repo* pRepo,                         /**< You still own this, but don't
	                                          free it until the treendx is
	                                          freed.  It will retain this
	                                          pointer. */

	SG_uint64 iDagNum,                      /**< Which dag in that repo are we
	                                          indexing? */

	SG_pathname* pPath,

	SG_bool bQueryOnly,

	SG_treendx** ppNew
	);

/**
 * Release the memory for an SG_treendx object.  This does
 * nothing at all to the treendx on disk.
 *
 */
void SG_treendx__free(SG_context* pCtx, SG_treendx* pdbc);

void SG_treendx__begin_update_multiple(SG_context* pCtx, SG_treendx* pdbc);
void SG_treendx__end_update_multiple(SG_context* pCtx, SG_treendx* pdbc);
void SG_treendx__abort_update_multiple(SG_context* pCtx, SG_treendx* pdbc);
void SG_treendx__update_one_changeset(SG_context* pCtx, SG_treendx* pdbc, const char* psz_csid, SG_vhash* pvh);

void SG_treendx__get_paths(
    SG_context* pCtx, 
    SG_treendx* pndx, 
    SG_vhash* pvh_gids,
    SG_vhash** ppvh
    );

void SG_treendx__get_changesets_where_any_of_these_gids_changed(
        SG_context* pCtx, 
        SG_treendx* pTreeNdx, 
        SG_stringarray* psa_gids,
        SG_vhash** ppvh
        );

END_EXTERN_C;

#endif
