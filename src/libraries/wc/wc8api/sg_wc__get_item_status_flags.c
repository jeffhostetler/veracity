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
 * @file sg_wc__get_item_status_flags.c
 *
 * @details Routine to do a quick status on an individual item
 * so that caller an answer questions like "Is controlled?" or
 * "Is ignored?".  This is a single-item STATUS, but without the
 * vhash/varray of details -- just the net status flags.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Ask basic "is-controlled" question for a single item.
 * This is a convenience wrapper for callers that don't
 * need to do multiple operations and/or don't want to
 * bother setting up their own transaction.
 *
 * IF YOU WANT TO ASK A SERIES OF THESE QUESTIONS, CREATE
 * YOUR OWN TRANSACTION AND USE THE ACTUAL FUNCTION RATHER
 * THAN THIS WRAPPER (because the TX will cache the scandir/
 * readdir results).
 *
 * WARNING: See note in sg_wc_tx__get_item_status_flags.c for
 * ambiguity in this call.
 * 
 */
void SG_wc__get_item_status_flags(SG_context * pCtx,
								  const SG_pathname* pPathWc,
								  const char * pszInput,
								  SG_bool bNoIgnores,
								  SG_bool bNoTSC,
								  SG_wc_status_flags * pStatusFlags,
								  SG_vhash ** ppvhProperties)
{
	SG_WC__WRAPPER_TEMPLATE__ro(
		pPathWc,
		SG_ERR_CHECK(  SG_wc_tx__get_item_status_flags(pCtx, pWcTx,
													   pszInput,
													   bNoIgnores, bNoTSC,
													   pStatusFlags,
													   ppvhProperties)  )  );
}

