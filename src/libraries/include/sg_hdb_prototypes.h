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

#ifndef H_SG_HDB_PROTOTYPES_H
#define H_SG_HDB_PROTOTYPES_H

BEGIN_EXTERN_C

void SG_hdb__open(
    SG_context* pCtx, 
    SG_pathname* pPath,
    SG_uint32 num_plan_to_add,
    SG_bool allow_rollback,
    SG_hdb** ppResult
    );

void SG_hdb__close_free(
    SG_context* pCtx, 
    SG_hdb* pdb
    );

void SG_hdb__rollback_close_free(
    SG_context* pCtx, 
    SG_hdb* pdb
    );

void SG_hdb__create(
    SG_context* pCtx, 
    SG_pathname* pPath,
    SG_uint8 key_length,
    SG_uint8 num_bits,
    SG_uint16 data_size
    );

#define SG_HDB__ON_COLLISION__OVERWRITE  (1)
#define SG_HDB__ON_COLLISION__ERROR      (2)
#define SG_HDB__ON_COLLISION__IGNORE     (3)
#define SG_HDB__ON_COLLISION__MULTIPLE   (4)

void SG_hdb__put(
    SG_context* pCtx, 
    SG_hdb* pdb,
    SG_byte* pkey,
    SG_byte* pdata,
    SG_uint8 on_collision
    );

void SG_hdb__find(
    SG_context* pCtx, 
    SG_hdb* pdb,
    SG_byte* pkey,
    SG_uint32 starting_offset,
    SG_byte** ppdata,
    SG_uint32* pi_next_offset
    );

#define SG_HDB_TO_VHASH__HEADER  (1)
#define SG_HDB_TO_VHASH__BUCKETS (2)
#define SG_HDB_TO_VHASH__PAIRS   (4)
#define SG_HDB_TO_VHASH__STATS   (8)

void SG_hdb__to_vhash(
    SG_context* pCtx, 
    SG_pathname* pPath,
    SG_uint8 flags,
    SG_vhash** ppvh
    );

void SG_hdb__rehash(
    SG_context* pCtx, 
    SG_pathname* pPath_old,
    SG_uint8 num_bits_new,
    SG_pathname* pPath_new
    );

END_EXTERN_C

#endif

