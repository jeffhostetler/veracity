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
 * @file sg_vv_verbs__incoming.h
 *
 * @details Routines to perform most of 'vv incoming'.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_VV_VERBS__INCOMING_H
#define H_SG_VV_VERBS__INCOMING_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

void SG_vv_verbs__incoming(SG_context * pCtx,
						   const char* pszSrcRepoSpec,
						   const char* pszUsername,
						   const char* pszPassword,
						   const char * pszDestRepoDescriptorName,
						   SG_history_result** pp_incomingChangesets,
						   SG_vhash** ppvhStats)
{
	SG_repo* pDestRepo = NULL;

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszDestRepoDescriptorName, &pDestRepo)  );

	SG_ERR_CHECK(  SG_pull__list_incoming_vc(pCtx, pDestRepo, pszSrcRepoSpec, pszUsername, pszPassword, pp_incomingChangesets, ppvhStats)  );

	/* fall through */
fail:
	SG_REPO_NULLFREE(pCtx, pDestRepo);
}

END_EXTERN_C;

#endif//H_SG_VV_VERBS__INCOMING_H
