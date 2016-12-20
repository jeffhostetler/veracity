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
 * @file sg_attributes_prototypes.h
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_ATTRIBUTES_PROTOTYPES_H
#define H_SG_ATTRIBUTES_PROTOTYPES_H

BEGIN_EXTERN_C;

void SG_attributes__bits__apply(
	SG_context* pCtx,
    const SG_pathname* pPath,
    SG_int64 iBits
    );

void SG_attributes__bits__read(
	SG_context* pCtx,
    const SG_pathname* pPath,
    SG_int64 iBaselineAttributeBits,
    SG_int64* piBits
    );

void SG_attributes__bits__from_perms(
    SG_context* pCtx,
	SG_fsobj_type type,
	SG_fsobj_perms perms,
    SG_int64 iBaselineAttributeBits,
    SG_int64* piBits
    );

END_EXTERN_C;

#endif //H_SG_ATTRIBUTES_PROTOTYPES_H
