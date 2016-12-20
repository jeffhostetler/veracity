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
 * @file sg__wc__do_cmd_update.c
 *
 * @details Handle the UPDATE command.
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
 * TODO 2012/05/09 This code is identical to that in sg__wc__do_cmd_merge.c
 * TODO            I'm not sure yet if/whether I want UPDATE to report stats
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

void wc__do_cmd_update(SG_context * pCtx,
					   const SG_option_state * pOptSt)
{
	SG_wc_update_args ua;
	SG_varray * pvaJournal = NULL;
	SG_vhash * pvhStats = NULL;
	SG_varray * pvaStatus = NULL;
	SG_string * pString = NULL;

	memset(&ua, 0, sizeof(ua));
	ua.pRevSpec = pOptSt->pRevSpec;
	ua.pszAttach = pOptSt->psz_attach;
	ua.pszAttachNew = pOptSt->psz_attach_new;
	ua.bDetached = pOptSt->bAllowDetached;
	ua.bAttachCurrent = pOptSt->bAttachCurrent;
	// TODO 2013/01/04 decide if we want an --allow-dirty option and change the default
	ua.bDisallowWhenDirty = SG_FALSE;

	SG_ERR_CHECK(  SG_wc__update(pCtx, NULL, &ua,
								 pOptSt->bTest,
								 ((pOptSt->bVerbose) ? &pvaJournal : NULL),
								 &pvhStats,
								 ((pOptSt->bStatus) ? &pvaStatus : NULL),
								 NULL)  );
	if (pvhStats)
		SG_ERR_IGNORE(  _report_merge_stats(pCtx, pvhStats)  );

	if (pvaJournal)
		SG_ERR_IGNORE(  sg_report_journal(pCtx, pvaJournal)  );

	if (pvaStatus)
	{
		// TODO 2012/06/15 We have a bit of a conflict here.  'vv update' takes a
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

		// TODO 2012/06/15 Should we print the legend too?
	}

	if (!pOptSt->bTest)
	{
		// TODO 2012/01/10 Consider printing info for the new (branch name, baseline).
		// TODO            Something like 'vv branch' and 'vv parents' would.
		// TODO
		// TODO 2012/06/15 With the changes for --status, we also have the pszHidChosen
		// TODO            available now.
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaJournal);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_STRING_NULLFREE(pCtx, pString);
}

//////////////////////////////////////////////////////////////////

/**
 * This is used for "vv pull -u" to do an UPDATE after the PULL.
 * It is separate from the main wc__do_cmd_update() because PULL
 * owns the args in pOptSt; the subsequent UPDATE runs with
 * defaults.
 *
 */
void wc__do_cmd_update__after_pull(SG_context * pCtx,
								   SG_rev_spec * pRevSpec)
{
	SG_wc_update_args ua;
	SG_vhash * pvhStats = NULL;
	SG_uint32 countRevSpecs;

	SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &countRevSpecs)  );
	if (countRevSpecs > 1)
	{
		SG_ERR_CHECK(  SG_console__raw(pCtx, SG_CS_STDOUT, "Skipping update: multiple revisions were specified.\n")  );
		return;
	}

	memset(&ua, 0, sizeof(ua));
	ua.pRevSpec = pRevSpec;
	// TODO 2013/01/04 decide if we want an --allow-dirty option and change the default
	ua.bDisallowWhenDirty = SG_FALSE;

	SG_ERR_CHECK(  SG_console__raw(pCtx, SG_CS_STDOUT, "Pull completed.  Updating...")  );
	SG_ERR_CHECK(  SG_wc__update(pCtx, NULL, &ua,
								 SG_FALSE,        // bTest
								 SG_FALSE,        // bJournal
								 &pvhStats,
								 NULL,            // pvaStatus
								 NULL)  );
	SG_ERR_CHECK(  SG_console__raw(pCtx, SG_CS_STDOUT, " Done.\n")  );

	if (pvhStats)
		SG_ERR_IGNORE(  _report_merge_stats(pCtx, pvhStats)  );

	// TODO 2012/12/13 Consider printing info for the new (branch name, baseline).
	// TODO            Something like 'vv branch' and 'vv parents' would.

fail:
	SG_VHASH_NULLFREE(pCtx, pvhStats);
}
