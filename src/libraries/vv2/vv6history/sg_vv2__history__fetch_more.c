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

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

void sg_vv2__history__fetch_more__repo(
	SG_context * pCtx,
	const char * pszRepoDescriptorName,
	SG_history_token * pHistoryToken,
	SG_uint32 nResultLimit, 
	SG_vhash ** ppvhBranchPile,
	SG_history_result ** ppHistoryResults,
	SG_history_token **ppNewToken)
{
	SG_repo * pRepo = NULL;

	SG_ERR_CHECK( SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoDescriptorName, &pRepo) );

	SG_ERR_CHECK( SG_history__fetch_more(pCtx, pRepo, pHistoryToken, nResultLimit,
										 ppHistoryResults, ppNewToken) );
	
	/* This is kind of a hack. History callers often need branch data to format ouput.
	 * But we open the repo down here. I didn't want to open/close it again. And there's logic
	 * in here about which repo to open. So instead, we do this. */
	if (ppvhBranchPile)
	{
		// TODO 2012/07/02 Wait. Didn't we give them a set of branches when
		// TODO            we started the iteration?  Do we really need to
		// TODO            give it to them for *each* page?
		SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, ppvhBranchPile)  );
	}

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
}

void sg_vv2__history__fetch_more__repo2(
	SG_context * pCtx,
	SG_repo * pRepo,
	SG_history_token * pHistoryToken,
	SG_uint32 nResultLimit,
	SG_history_result ** ppHistoryResults,
	SG_history_token **ppNewToken)
{
	SG_ERR_CHECK_RETURN( SG_history__fetch_more(pCtx, pRepo, pHistoryToken, nResultLimit,
												ppHistoryResults, ppNewToken) );
}

void sg_vv2__history__fetch_more__working_folder(
	SG_context * pCtx,
	SG_history_token * pHistoryToken,
	SG_uint32 nResultLimit,
	SG_vhash ** ppvhBranchPile,
	SG_history_result ** ppHistoryResults,
	SG_history_token **ppNewToken)
{
	SG_repo * pRepo = NULL;
	SG_bool bRepoNamed;

	SG_ERR_CHECK(  sg_vv2__util__get_repo(pCtx, NULL, &pRepo, &bRepoNamed)  );

	SG_ERR_CHECK( SG_history__fetch_more(pCtx, pRepo, pHistoryToken, nResultLimit,
										 ppHistoryResults, ppNewToken) );

	/* This is kind of a hack. History callers often need branch data to format ouput.
	 * But we open the repo down here. I didn't want to open/close it again. And there's logic
	 * in here about which repo to open. So instead, we do this. */
	if (ppvhBranchPile)
	{
		// TODO 2012/07/02 Wait. Didn't we give them a set of branches when
		// TODO            we started the iteration?  Do we really need to
		// TODO            give it to them for *each* page?
		SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, ppvhBranchPile)  );
	}

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
}
