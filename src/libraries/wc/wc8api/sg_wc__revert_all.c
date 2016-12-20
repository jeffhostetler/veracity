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
 * @file sg_wc__revert_all.c
 *
 * @details Handle a REVERT-ALL.  This is a complete REVERT.
 * We treat this differently than a revert on an individual
 * item because a revert-all must be able undo the changes
 * *AND* be able to cancel a pending MERGE.  Whereas an
 * individual item revert is more like a simple undo change.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Revert everything (cancelling any pending merge).
 * We optionally backup the contents of edited files.
 *
 * We use the MERGE engine to unwind the pending changes/dirt
 * in the WD.  A REVERT-ALL should, in theory, not have any
 * merge-conflicts, *UNLESS* there are ADDED/FOUND/UNCONTROLLED
 * items that collide/conflict with baseline items.  Since they
 * are ADDED/FOUND/UNCONTROLLED in the current WD, they will be
 * FOUND/UNCONTROLLED from the point of view of the baseline
 * and we don't like to alter/move/rename uncontrolled items.
 * So we may abort the REVERT if get any of these, rather than
 * creating an ISSUE and getting RESOLVE involved (just feels
 * weird).
 *
 */
void SG_wc__revert_all(SG_context * pCtx,
					   const SG_pathname* pPathWc,
					   SG_bool bNoBackups,
					   SG_bool bTest,
					   SG_varray ** ppvaJournal,
					   SG_vhash ** ppvhStats)
{
	SG_WC__WRAPPER_TEMPLATE__tj(
		pPathWc,
		bTest,
		ppvaJournal,
		SG_ERR_CHECK(  SG_wc_tx__revert_all(pCtx, pWcTx, bNoBackups, ppvhStats)  ));
}

