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
 * @file sg_wc__private.h
 *
 * @details Declarations visible in more than one .c file, but NOT
 * visible outside of this directory.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_VC__PRIVATE_H
#define H_SG_VC__PRIVATE_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
#define TRACE_WC_TX_ADD 0
#define TRACE_WC_TX_ADDREMOVE 0
#define TRACE_WC_TX_LVD 0
#define TRACE_WC_TX_SCAN 0
#define TRACE_WC_TX_APPLY 0
#define TRACE_WC_TX_STATUS 0
#define TRACE_WC_TX_STATUS1 0
#define TRACE_WC_TX_MOVE_RENAME 0
#define TRACE_WC_TX_REMOVE 0
#define TRACE_WC_READDIR 0
#define TRACE_WC_DB 0
#define TRACE_WC_GID 0
#define TRACE_WC_PORT 0
#define TRACE_WC_TX 0
#define TRACE_WC_TX_JRNL 0
#define TRACE_WC_CHECKOUT 0
#define TRACE_WC_COMMIT 0
#define TRACE_WC_UPDATE 0
#define TRACE_WC_MERGE 0
#define TRACE_WC_ISSUE 0
#define TRACE_WC_MSTATUS 0
#define TRACE_WC_RESOLVE 0
#define TRACE_WC_LIE 0
#define TRACE_WC_TSC 0
#define TRACE_WC_ABORT 0
#define TRACE_WC_REVERT 0
#define TRACE_WC_DIFF 0
#define TRACE_WC_TNE 0
#define TRACE_WC_ATTRBITS 0
#else
#define TRACE_WC_TX_ADD 0
#define TRACE_WC_TX_ADDREMOVE 0
#define TRACE_WC_TX_LVD 0
#define TRACE_WC_TX_SCAN 0
#define TRACE_WC_TX_APPLY 0
#define TRACE_WC_TX_STATUS 0
#define TRACE_WC_TX_STATUS1 0
#define TRACE_WC_TX_MOVE_RENAME 0
#define TRACE_WC_TX_REMOVE 0
#define TRACE_WC_READDIR 0
#define TRACE_WC_DB 0
#define TRACE_WC_GID 0
#define TRACE_WC_PORT 0
#define TRACE_WC_TX 0
#define TRACE_WC_TX_JRNL 0
#define TRACE_WC_CHECKOUT 0
#define TRACE_WC_COMMIT 0
#define TRACE_WC_UPDATE 0
#define TRACE_WC_MERGE 0
#define TRACE_WC_ISSUE 0
#define TRACE_WC_MSTATUS 0
#define TRACE_WC_RESOLVE 0
#define TRACE_WC_LIE 0
#define TRACE_WC_TSC 0
#define TRACE_WC_ABORT 0
#define TRACE_WC_REVERT 0
#define TRACE_WC_DIFF 0
#define TRACE_WC_TNE 0
#define TRACE_WC_ATTRBITS 0
#endif

//////////////////////////////////////////////////////////////////

typedef struct _sg_wc_prescan_row  sg_wc_prescan_row;
typedef struct _sg_wc_prescan_dir  sg_wc_prescan_dir;

#include     "wc0util/sg_wc_attrbits__private_typedefs.h"
#include     "wc0util/sg_wc_port__private_typedefs.h"
#include     "wc0util/sg_wc_readdir__private_typedefs.h"
#include       "wc0db/sg_wc_db__private_typedefs.h"
#include       "wc1pt/sg_wc_pctne__private_typedefs.h"
#include     "wc2scan/sg_wc_prescan__private_typedefs.h"
#include     "wc3live/sg_wc_liveview__private_typedefs.h"
#include       "wc4tx/sg_wc_tx__private_typedefs.h"
#include   "wc6commit/sg_wc6commit__private_typedefs.h"
#include     "wc6diff/sg_wc6diff__private_typedefs.h"
#include    "wc6merge/sg_wc6merge__private_typedefs.h"
#if 1 // delete these
#include  "wc6resolve/sg_wc_tx__resolve__private_typedefs.h"
#endif
#include  "wc6status1/sg_wc6status1__private_typedefs.h"
#include   "wc6update/sg_wc6update__private_typedefs.h"

#include     "wc0util/sg_wc_attrbits__private_prototypes.h"
#include     "wc0util/sg_wc_diff_utils__private_prototypes.h"
#include     "wc0util/sg_wc_park__private_prototypes.h"
#include     "wc0util/sg_wc_path__private_prototypes.h"
#include     "wc0util/sg_wc_port__private_prototypes.h"
#include     "wc0util/sg_wc_readdir__private_prototypes.h"
#include       "wc0db/sg_wc_db__private_prototypes.h"
#include       "wc1pt/sg_wc_pctne__private_prototypes.h"
#include     "wc2scan/sg_wc_prescan__private_prototypes.h"
#include     "wc3live/sg_wc_liveview__private_prototypes.h"
#include   "wc4status/sg_wc__status__private_prototypes.h"
#include       "wc4tx/sg_wc_tx__private_prototypes.h"
#include    "wc5apply/sg_wc_tx__apply__private_prototypes.h"
#include    "wc5queue/sg_wc_tx__queue__private_prototypes.h"
#include   "wc6commit/sg_wc6commit__private_prototypes.h"
#include     "wc6diff/sg_wc6diff__private_prototypes.h"
#include  "wc6ffmerge/sg_wc6ffmerge__private_prototypes.h"
#include    "wc6rpapi/sg_wc_tx__rp__private_prototypes.h"
#include    "wc6merge/sg_wc6merge__private_prototypes.h"
#include  "wc6mstatus/sg_wc6mstatus__private_prototypes.h"
#if 1 // delete these
#include  "wc6resolve/sg_wc_tx__resolve__private_prototypes.h"
#include  "wc6resolve/sg_wc_tx__resolve__private_prototypes2.h"
#include  "wc6resolve/sg_wc_tx__resolve__choice__private_prototypes.h"
#include  "wc6resolve/sg_wc_tx__resolve__item__private_prototypes.h"
#include  "wc6resolve/sg_wc_tx__resolve__state__private_prototypes.h"
#include  "wc6resolve/sg_wc_tx__resolve__value__private_prototypes.h"
#include  "wc6resolve/sg_wc_tx__resolve__to_vhash__private_prototypes.h"
#endif
#include  "wc6status1/sg_wc6status1__private_prototypes.h"
#include   "wc6update/sg_wc6update__private_prototypes.h"
#include      "wc8api/sg_wc8api__private_prototypes.h"
#include       "wc9js/sg_wc__jsglue__private_prototypes.h"

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VC__PRIVATE_H
