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

//////////////////////////////////////////////////////////////////
// Notes during conversion from PendingTree to WC.
//////////////////////////////////////////////////////////////////
//
// WARNING: 2012/03/23 With the conversion from PendingTree to the new WC code,
// WARNING:            we should never call a SG_fsobj__ or SG_file__ routine on
// WARNING:            a controlled item.  We should always ask WC to give us
// WARNING:            that info or do an operation.
// WARNING:
// WARNING:            As long as we maintain the one-operation-per-sg_resolve
// WARNING:            restriction (that we inherited from PendingTree) we'll
// WARNING:            be fine, but when we actuall get RESOLVE playing nice in
// WARNING:            the WC TX, we'll have problems.
// WARNING:
// WARNING:            It *is* OK to call SG_fsobj__ and SG_file__ routines on 
// WARNING:            TEMP files that automerge and/or manual merge created.
//
//
//
// 2012/04/03 [2] pVariantFile now contains an extended-prefix repo-path.
//                For items in TEMP (created by auto- or manual-merges),
//                these are of the form "@/.sgdrawer/t/merge_*...."
//                For live/working items, these are GID-domain repo-paths
//                (which saves us from having to know about pre-tx vs in-tx
//                and/or know about QUEUED overwrite-file operations).
//
//
//
// 2012/04/05 [4] Think about having MERGE and RESOLVE define a new domain
//                prefix for TEMP the files used by the mergetools.  I'm not
//                sure that I like having .sgdrawer appear in the repo-path
//                that we stuff in pVariantFile.  W4679.
//
//
//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////
// Types

#include "wc6resolve/sg_wc_tx__resolve__value__inline0.h"
#include "wc6resolve/sg_wc_tx__resolve__choice__inline0.h"
#include "wc6resolve/sg_wc_tx__resolve__item__inline0.h"

/**
 * All of the resolve data about a single working directory.
 * This is the basic data structure used by the resolve system.
 * All other resolve data is stored within it somewhere.
 */
struct _sg_resolve
{
	SG_wc_tx * pWcTx;             //< The WC TX handle we were given; we DO NOT own this.

	SG_vector*      pItems;       //< The set of items that are/were in conflict.
	                              //< Vector contains SG_resolve__item* values.
};

//////////////////////////////////////////////////////////////////
// Global declarations

#include "wc6resolve/sg_wc_tx__resolve__globals__inline0.h"

//////////////////////////////////////////////////////////////////
// Internal functions

#include "wc6resolve/sg_wc_tx__resolve__variant__inline1.h"
#include "wc6resolve/sg_wc_tx__resolve__utils__inline1.h"

#include "wc6resolve/sg_wc_tx__resolve__value__inline1.h"
#include "wc6resolve/sg_wc_tx__resolve__choice__inline1.h"
#include "wc6resolve/sg_wc_tx__resolve__item__inline1.h"

#include "wc6resolve/sg_wc_tx__resolve__merge__inline1.h"
#include "wc6resolve/sg_wc_tx__resolve__accept__inline1.h"

//////////////////////////////////////////////////////////////////
// Routines that were originally PUBLIC, but should become PRIVATE.

#include "wc6resolve/sg_wc_tx__resolve__state__inline2.h"

#include "wc6resolve/sg_wc_tx__resolve__alloc__inline2.h"
#include "wc6resolve/sg_wc_tx__resolve__diff__inline2.h"
#include "wc6resolve/sg_wc_tx__resolve__merge__inline2.h"
#include "wc6resolve/sg_wc_tx__resolve__accept__inline2.h"

#include "wc6resolve/sg_wc_tx__resolve__utils__inline2.h"

#include "wc6resolve/sg_wc_tx__resolve__item__inline2.h"
#include "wc6resolve/sg_wc_tx__resolve__choice__inline2.h"
#include "wc6resolve/sg_wc_tx__resolve__value__inline2.h"

#include "wc6resolve/sg_wc_tx__resolve__to_vhash__inline2.h"
