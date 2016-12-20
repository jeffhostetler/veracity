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
 * @file sg_clone_typedefs.h
 *
 */

#ifndef H_SG_CLONE_TYPEDEFS_H
#define H_SG_CLONE_TYPEDEFS_H

BEGIN_EXTERN_C

#define SG_PACK_DEFAULT__KEYFRAMEDENSITY      512
#define SG_PACK_DEFAULT__REVISIONSTOLEAVEFULL 1
#define SG_PACK_DEFAULT__MINREVISIONS         2
#define SG_PACK_DEFAULT__ZLIBKEYFRAMES        SG_TRUE

#define SG_CLONE_FLAG__OVERRIDE_ALWAYSFULL (1)

#define SG_CLONE_CHANGES__OMIT       "omit"
#define SG_CLONE_CHANGES__ENCODING   "encoding"
#define SG_CLONE_CHANGES__HID_VCDIFF "hid_vcdiff"

#define SG_CLONE_DEFAULT__DEMAND_VCDIFF_SAVINGS_OVER_FULL   10
#define SG_CLONE_DEFAULT__DEMAND_VCDIFF_SAVINGS_OVER_ZLIB   10
#define SG_CLONE_DEFAULT__DEMAND_ZLIB_SAVINGS_OVER_FULL   10

#define SG_CLONE_CHANGES__DEMAND_VCDIFF_SAVINGS_OVER_FULL   "demand_vcdiff_savings_over_full"
#define SG_CLONE_CHANGES__DEMAND_VCDIFF_SAVINGS_OVER_ZLIB   "demand_vcdiff_savings_over_zlib"
#define SG_CLONE_CHANGES__DEMAND_ZLIB_SAVINGS_OVER_FULL   "demand_zlib_savings_over_full"

struct SG_clone__params__pack
{
    SG_int32 nKeyframeDensity;
    SG_int32 nRevisionsToLeaveFull;
    SG_uint32 nMinRevisions;
    SG_uint64 low_pass;
    SG_uint64 high_pass;
};

struct SG_clone__params__all_to_something
{
    SG_blob_encoding new_encoding;
    SG_uint64 low_pass;
    SG_uint64 high_pass;
    SG_uint32 flags;
};

struct SG_clone__demands
{
    SG_int32 vcdiff_savings_over_full;
    SG_int32 vcdiff_savings_over_zlib;
    SG_int32 zlib_savings_over_full;
};

END_EXTERN_C;

#endif

