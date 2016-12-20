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
 * @file sg__wc__do_cmd_commit.c
 *
 * @details Handle 'vv commit'.
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

void wc__do_cmd_commit(SG_context * pCtx,
					   const SG_option_state * pOptSt,
					   const SG_stringarray * psaArgs)
{
	SG_wc_commit_args ca;
	char * pszHidNewCSet = NULL;
	SG_repo * pRepo = NULL;
    SG_vhash * pvhPileOfCleanBranches = NULL;
	SG_varray * pvaJournal = NULL;

	memset(&ca, 0, sizeof(ca));
	ca.bDetached = pOptSt->bAllowDetached;
	ca.pszUser = pOptSt->psz_username;
	ca.pszWhen = pOptSt->psz_when;
	ca.pszMessage = pOptSt->psz_message;
	ca.pfnPrompt = ((pOptSt->bPromptForDescription) ? SG_cmd_util__get_comment_from_editor : NULL);
	ca.psaInputs = psaArgs;		// null for a complete commit; non-null for a partial commit.
	ca.depth = WC__GET_DEPTH(pOptSt);
	ca.psaAssocs = pOptSt->psa_assocs;
	ca.bAllowLost = pOptSt->bAllowLost;
	ca.psaStamps = pOptSt->psa_stamps;

	SG_ERR_CHECK(  SG_wc__commit(pCtx,
								 NULL,
								 &ca,
								 pOptSt->bTest,
								 ((pOptSt->bVerbose) ? &pvaJournal : NULL),
								 &pszHidNewCSet)  );

	if (pvaJournal)
		SG_ERR_IGNORE(  sg_report_journal(pCtx, pvaJournal)  );

	if (!pOptSt->bTest)
	{
		// after the commit is finished, display the details
		// of the new changeset.

		SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, NULL)  );
		SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvhPileOfCleanBranches)  );
		SG_ERR_CHECK(  SG_cmd_util__dump_log(pCtx,
											 SG_CS_STDOUT,
											 pRepo,
											 pszHidNewCSet,
											 pvhPileOfCleanBranches,
											 SG_FALSE,
											 SG_TRUE)  );
	}

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
    SG_VHASH_NULLFREE(pCtx, pvhPileOfCleanBranches);
	SG_NULLFREE(pCtx, pszHidNewCSet);
	SG_VARRAY_NULLFREE(pCtx, pvaJournal);
}
