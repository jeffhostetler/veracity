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
 * @file sg__wc__do_cmd_revert_all.c
 *
 * @details Handle 'vv revert --all'
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

/**
 * TODO 2012/07/23 This code is identical to that in sg__wc__do_cmd_merge.c
 * TODO            I'm not sure yet if/whether I want REVERT to report stats
 * TODO            like MERGE.
 *
 */
static void _report_merge_stats(SG_context * pCtx,
								const SG_vhash * pvhStats)
{
	const char * pszSynopsys = NULL;

	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhStats, "synopsys", &pszSynopsys)  );
	if (pszSynopsys)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%s\n", pszSynopsys)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

void wc__do_cmd_revert_all(SG_context * pCtx,
						   const SG_option_state * pOptSt,
						   const SG_stringarray * psaArgs)
{
	SG_varray * pvaJournal = NULL;
	SG_vhash * pvhStats = NULL;
	SG_uint32 nrArgs = 0;

	if (pOptSt->RevertItems_flagMask_type | pOptSt->RevertItems_flagMask_dirt)
	{
		SG_ERR_THROW2(  SG_ERR_USAGE, (pCtx, "Cannot use any of the '--revert-*' options with '--all'")  );
	}

	if (psaArgs)
	{
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaArgs, &nrArgs)  );
		if (nrArgs > 0)
			SG_ERR_THROW2(  SG_ERR_USAGE, (pCtx, "Cannot give pathnames with '--all'")  );
	}

	if (!pOptSt->bRecursive)
		SG_ERR_THROW2(  SG_ERR_USAGE, (pCtx, "Cannot give '-N' with '--all'")  );

	SG_ERR_CHECK(  SG_wc__revert_all(pCtx, NULL, pOptSt->bNoBackups, pOptSt->bTest,
									 ((pOptSt->bVerbose) ? &pvaJournal : NULL),
									 &pvhStats)  );
	if (pvhStats)
		SG_ERR_IGNORE(  _report_merge_stats(pCtx, pvhStats)  );

	if (pvaJournal)
		SG_ERR_IGNORE(  sg_report_journal(pCtx, pvaJournal)  );
	
fail:
	SG_VARRAY_NULLFREE(pCtx, pvaJournal);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
}

