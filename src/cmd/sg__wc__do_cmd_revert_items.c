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
 * @file sg__wc__do_cmd_revert_items.c
 *
 * @details Handle 'vv revert' of one or more files/folders.
 * This is NOT the REVERT-ALL code.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_typedefs.h"
#include "sg_prototypes.h"
#include "sg_cmd_util_prototypes.h"

//////////////////////////////////////////////////////////////////

static void _make_flag_mask(const SG_option_state * pOptSt,
							SG_wc_status_flags * pFlagMask)
{
	SG_wc_status_flags flagMask_type = SG_WC_STATUS_FLAGS__T__MASK;
	SG_wc_status_flags flagMask_dirt = (SG_WC_STATUS_FLAGS__U__MASK
										|SG_WC_STATUS_FLAGS__S__MASK
										|SG_WC_STATUS_FLAGS__C__MASK);

	if (pOptSt->RevertItems_flagMask_type)
		flagMask_type = pOptSt->RevertItems_flagMask_type;

	if (pOptSt->RevertItems_flagMask_dirt)
		flagMask_dirt = pOptSt->RevertItems_flagMask_dirt;
	
	*pFlagMask = (flagMask_type | flagMask_dirt);
}

//////////////////////////////////////////////////////////////////

void wc__do_cmd_revert_items(SG_context * pCtx,
							 const SG_option_state * pOptSt,
							 const SG_stringarray * psaArgs)
{
	SG_varray * pvaJournal = NULL;
	SG_uint32 nrArgs = 0;
	SG_wc_status_flags flagMask = SG_WC_STATUS_FLAGS__ZERO;

	_make_flag_mask(pOptSt, &flagMask);

	if (psaArgs)
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaArgs, &nrArgs)  );
	if (nrArgs == 0)
		SG_ERR_THROW2(  SG_ERR_USAGE, (pCtx, "At least one pathname is required")  );

	SG_ERR_CHECK(  SG_wc__revert_items__stringarray(pCtx, NULL, psaArgs,
													WC__GET_DEPTH(pOptSt),
													flagMask,
													pOptSt->bNoBackups,
													pOptSt->bTest,
													((pOptSt->bVerbose) ? &pvaJournal : NULL))  );

	if (pvaJournal)
		SG_ERR_IGNORE(  sg_report_journal(pCtx, pvaJournal)  );
	
fail:
	SG_VARRAY_NULLFREE(pCtx, pvaJournal);
}

