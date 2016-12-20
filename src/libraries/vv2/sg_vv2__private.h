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
 * @file sg_vv2__private.h
 *
 * @details Private declaraions for VV2 routines that should NOT
 * be visible outside of the vv2 directory tree.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_VV2__PRIVATE_H
#define H_SG_VV2__PRIVATE_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
#define TRACE_VV2_EXPORT 0
#define TRACE_VV2_STATUS 0
#define TRACE_VV2_CAT 0
#else
#define TRACE_VV2_EXPORT 0
#define TRACE_VV2_STATUS 0
#define TRACE_VV2_CAT 0
#endif

//////////////////////////////////////////////////////////////////

#include "vv2status/sg_vv2__status__private_typedefs.h"
#include   "vv6diff/sg_vv6diff__private_typedefs.h"

#include    "vv0util/sg_vv0util__private_prototypes.h"
#include  "vv2status/sg_vv2__status__private_prototypes.h"
#include    "vv6diff/sg_vv6diff__private_prototypes.h"
#include "vv6history/sg_vv6history__private_prototypes.h"
#include   "vv6locks/sg_vv6locks__private_prototypes.h"
#include "vv6mstatus/sg_vv6mstatus__private_prototypes.h"
#include      "vv9js/sg_vv2__jsglue__private_prototypes.h"

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VV2__PRIVATE_H
