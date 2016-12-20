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

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * PARK this item.
 *
 * In the PendingTree version of MERGE, we would PRE-PARK
 * the BASELINE version of any/all items that had transient
 * or final collisions *before* we started juggling the
 * content of the WD.
 *
 * With the WC code, we can reduce this and only PARK items
 * with transient collisions as we bump into them as we are
 * queuing/journalling the juggling using the LVI code.  So
 * we can park using the item's GID and not worry about
 * pre- or post- or transient-pathnames.
 *
 * Also, the PendingTree version parked things in a "parking-lot"
 * directory within .sgdrawer.  This hid the details, but required
 * a hack to let us temporarily add .sgdrawer and various TEMP
 * directories to PendingTree so that we maintained a connected
 * tree during the juggle phase.  In the WC version, I'm just
 * going to park the items in the @/ root directory since it
 * is guaranteed to be there and is already under version
 * control and won't cause fits for the layer-7-txapi routines.
 *
 * NOTE: Parking is a bit of a lie.  We tell the LVI/LVD layer
 * to move/rename the item to a tmp name/location, but we do not
 * alter the MrgCSetEntry. Then when we go to un-park it, we use
 * gid-domain extended-prefix repo-paths to tell the LVI/LVD
 * layer to move/rename it to the desired final location (still)
 * stored in the MrgCSetEntry.
 * 
 */
void sg_wc_tx__merge__queue_plan__park(SG_context * pCtx,
									   SG_mrg * pMrg,
									   SG_mrg_cset_entry * pMrgCSetEntry)
{
	SG_ERR_CHECK(  sg_wc_tx__park__move_rename(pCtx, pMrg->pWcTx, pMrgCSetEntry->bufGid_Entry)  );

	pMrgCSetEntry->markerValue |= _SG_MRG__PLAN__MARKER__FLAGS__PARKED;

fail:
	return;
}

	
	
