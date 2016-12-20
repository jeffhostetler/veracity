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
 * @file sg__wc__do_cmd_merge.c
 *
 * @details Handle the MERGE command.
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

static void _report_merge_stats(SG_context * pCtx,
								const SG_vhash * pvhStats)
{
	const char * pszSynopsys = NULL;
	SG_bool bHasFF = SG_FALSE;
	SG_bool bIsFF  = SG_FALSE;

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhStats, "isFastForwardMerge", &bHasFF)  );
	if (bHasFF)
		SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvhStats, "isFastForwardMerge", &bIsFF)  );

	// synopsys should always be set, but i don't want
	// to chance a throw on the finish line....
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhStats, "synopsys", &pszSynopsys)  );
	if (pszSynopsys)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT, "%s%s\n",
								   ((bIsFF) ? "Fast Forward Merge: " : ""),
								   pszSynopsys)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

static void _report_merge_details(SG_context * pCtx)
{
	SG_vhash * pvhInfo = NULL;
	SG_vhash * pvhCSets = NULL;		// we do not own this

	SG_ERR_CHECK(  SG_wc__get_wc_info(pCtx, NULL, &pvhInfo)  );
	SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvhInfo, "csets", &pvhCSets)  );
	if (pvhCSets)
	{
		const char * pszL0 = NULL;	// we do not own these
		const char * pszL1 = NULL;
		const char * pszA  = NULL;

		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhCSets, "L0", &pszL0)  );
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhCSets, "L1", &pszL1)  );
		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhCSets, "A" , &pszA )  );

		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Merging changeset: %s\n", ((pszL1) ? pszL1 : ""))  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "    into baseline: %s\n", ((pszL0) ? pszL0 : ""))  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "   using ancestor: %s\n", ((pszA ) ? pszA  : ""))  );
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pvhInfo);
}

//////////////////////////////////////////////////////////////////

void wc__do_cmd_merge(SG_context * pCtx,
					  const SG_option_state * pOptSt)
{
	SG_wc_merge_args mergeArgs;
	SG_vhash * pvhStats = NULL;
	SG_varray * pvaJournal = NULL;
	SG_varray * pvaStatus = NULL;
	SG_string * pString = NULL;

	memset(&mergeArgs, 0, sizeof(mergeArgs));
	mergeArgs.pRevSpec                   = pOptSt->pRevSpec;
	mergeArgs.bNoFFMerge                 = pOptSt->bNoFFMerge;
	mergeArgs.bNoAutoMergeFiles          = pOptSt->bNoAutoMerge;
	mergeArgs.bComplainIfBaselineNotLeaf = SG_FALSE;	// TODO 2012/01/20 see note in SG_wc_tx__merge().
	mergeArgs.bDisallowWhenDirty         = (!pOptSt->bAllowWhenDirty);

	SG_wc__merge(pCtx, NULL, &mergeArgs,
				 pOptSt->bTest,
				 ((pOptSt->bVerbose) ? &pvaJournal : NULL),
				 &pvhStats,
				 NULL,
				 ((pOptSt->bStatus) ? &pvaStatus : NULL));
	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		if (SG_context__err_equals(pCtx, SG_ERR_MERGE_TARGET_EQUALS_BASELINE)
			|| SG_context__err_equals(pCtx, SG_ERR_MERGE_TARGET_IS_ANCESTOR_OF_BASELINE))
		{
			SG_context__err_reset(pCtx);
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT,
									  "The baseline already includes the merge target. No merge is needed.\n")  );
			goto done;
		}
		else
		{
			SG_ERR_RETHROW;
		}
	}

	if (pvhStats)
		SG_ERR_IGNORE(  _report_merge_stats(pCtx, pvhStats)  );

	if (pvaJournal)
	{
		SG_ERR_IGNORE(  _report_merge_details(pCtx)  );
		SG_ERR_IGNORE(  sg_report_journal(pCtx, pvaJournal)  );
	}

	if (pvaStatus)
	{
		// TODO 2012/06/26 We have a bit of a conflict here.  'vv merge' takes a
		// TODO            --verbose argument and it means that we should print
		// TODO            the journal of what we had/would have to do to the WD
		// TODO            to actually update it.
		// TODO
		// TODO            The addition of --status allows us to print the what-if
		// TODO            differences, but the status-formatter also has a verbose
		// TODO            argument (which is used by the 'vv status' command to
		// TODO            mean to print the GIDs and stuff).
		// TODO
		// TODO            I'm going to not force a verbose status report here.
		// TODO            (Just like I don't allow us to pass down any NoTSC,
		// TODO            ListSparse, NoSort, and etc. args.)

		SG_ERR_CHECK(  SG_wc__status__classic_format(pCtx, pvaStatus, SG_FALSE, &pString)  );
		SG_ERR_CHECK(  SG_console__raw(pCtx, SG_CS_STDOUT, SG_string__sz(pString))  );

		// TODO 2012/06/26 Should we print the legend too?
	}

done:
	;
fail:
	SG_VARRAY_NULLFREE(pCtx, pvaJournal);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_STRING_NULLFREE(pCtx, pString);
}

