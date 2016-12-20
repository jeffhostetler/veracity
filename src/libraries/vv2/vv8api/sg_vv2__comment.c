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
 * @file sg_vv2__comment.c
 *
 * @details Record a comment on a changeset.
 *
 * WE DO NOT ASSUME THAT A WD IS PRESENT.
 * If the RepoName is omitted, we will try
 * to get it from the WD if we can.
 *
 *
 * Note that COMMENTS are written directly/immediately
 * to the REPO on an *EXISTING* CHANGESET.  Therefore
 * they have nothing to do with the current WD (if present).
 * That is, we DO NOT "pend" a comment for the next commit.
 * And that is why this routine is in SG_vv2__ rather than
 * in SG_wc__.
 * 
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

void SG_vv2__comment(SG_context * pCtx,
					 const char * pszRepoName,
					 SG_rev_spec * pRevSpec,
					 const char * pszUser,
					 const char * pszWhen,
					 const char * pszMessage,
					 SG_ut__prompt_for_comment__callback * pfnPrompt)
{
	SG_audit q;
	SG_vhash * pvhWcInfo = NULL;
	SG_vhash * pvhUser = NULL;
	SG_repo * pRepo = NULL;
	char * pszHidCSet = NULL;
	char * pszCandidate_Allocated = NULL;
	char * pszTrimmedMessage = NULL;
	const char * pszCandidate = NULL;
	SG_bool bAllIsWhitespace = SG_FALSE;

	// pszRepoName is optional
	SG_NULLARGCHECK_RETURN( pRevSpec );
	// pszUser is optional
	// pszWhen is optional
	SG_ARGCHECK_RETURN( (pszMessage || pfnPrompt), pszMessage );

	if (!pszRepoName || !*pszRepoName)
	{
		// If they didn't give us a RepoName, try to get it from the WD.

		SG_wc__get_wc_info(pCtx, NULL, &pvhWcInfo);
		if (SG_CONTEXT__HAS_ERR(pCtx))
			SG_ERR_RETHROW2(  (pCtx, "Use 'repo' option or be in a working copy.")  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhWcInfo, "repo", &pszRepoName)  );

		// The wc-info data also just happens to have the default
		// whoami for this (repo,user).  If they didn't give us a
		// value, we can substitute it in now (and save another
		// lookup in the audit-init code).

		if (!pszUser || !*pszUser)
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhWcInfo, "user", &pszUser)  );
	}
	
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, &pRepo)  );

	memset(&q, 0, sizeof(q));
	SG_ERR_CHECK(  SG_audit__init__friendly(pCtx, &q, pRepo, pszWhen, pszUser)  );

	SG_ERR_CHECK(  SG_rev_spec__get_one__repo(pCtx, pRepo, pRevSpec, SG_TRUE, &pszHidCSet, NULL)  );

	if (pszMessage)
		pszCandidate = pszMessage;
	else if (pfnPrompt)
	{
		// Prompt the user for the comment text.

		const char * pszNormalizedUserName = NULL;

		// Show a "normalized" username in their editor.

		SG_ERR_CHECK(  SG_user__lookup_by_userid(pCtx, pRepo, &q, q.who_szUserId, &pvhUser)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhUser, "name", &pszNormalizedUserName)  );

		SG_ERR_CHECK(  (*pfnPrompt)(pCtx, SG_FALSE, pszNormalizedUserName,
									NULL, NULL, NULL,
									&pszCandidate_Allocated)  );
		pszCandidate = pszCandidate_Allocated;
	}

	if (!pszCandidate || !*pszCandidate)
		SG_ERR_THROW(  SG_ERR_EMPTYCOMMITMESSAGE  );

	SG_ERR_CHECK(  SG_sz__is_all_whitespace(pCtx,
											pszCandidate,
											&bAllIsWhitespace)  );
	if (bAllIsWhitespace)
		SG_ERR_THROW(  SG_ERR_EMPTYCOMMITMESSAGE  );

	SG_ERR_CHECK(  SG_sz__trim(pCtx,
							   pszCandidate,
							   NULL,
							   &pszTrimmedMessage)  );

	SG_ERR_CHECK(  SG_vc_comments__add(pCtx, pRepo, pszHidCSet, pszTrimmedMessage, &q)  );

fail:
	SG_NULLFREE(pCtx, pszCandidate_Allocated);
	SG_NULLFREE(pCtx, pszHidCSet);
	SG_NULLFREE(pCtx, pszTrimmedMessage);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_VHASH_NULLFREE(pCtx, pvhWcInfo);
	SG_VHASH_NULLFREE(pCtx, pvhUser);
}
					 
