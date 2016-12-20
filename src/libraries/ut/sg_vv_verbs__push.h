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
 * @file sg_vv_verbs__pull.h
 *
 * @details Routines to perform most of 'vv pull'.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_VV_VERBS__PUSH_H
#define H_SG_VV_VERBS__PUSH_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

static void _check_locks_for_push(SG_context* pCtx, 
	const char* pszPushDestRepoSpec,
	const char* pszUsername,
	const char* pszPassword,
	SG_repo* pRepo)
{
	SG_ERR_CHECK_RETURN(  SG_log__silence(pCtx, SG_TRUE)  );
	SG_ERR_CHECK_RETURN(  SG_vc_locks__pull_locks(pCtx, pszPushDestRepoSpec, pszUsername, pszPassword, pRepo)  );
	SG_ERR_CHECK_RETURN(  SG_log__silence(pCtx, SG_FALSE)  );

	SG_ERR_CHECK_RETURN(  SG_vc_locks__ensure__okay_to_push(
		pCtx, 
		pRepo, 
		NULL,
		NULL
		)  );
}

void SG_vv_verbs__push(
	SG_context * pCtx,
	const char * pszDestRepoSpec,
	const char * pszUsername,
	const char * pszPassword,
	const char* pszSrcRepoDescriptorName,
	SG_vhash * pvh_area_names,
	SG_rev_spec * pRevSpec,
    SG_bool bForce,
	SG_vhash** ppvhAccountInfo,
	SG_vhash** ppvhStats)
{
	SG_repo* pThisRepo = NULL;
	char* pszRevFullHid = NULL;
	SG_stringarray* psaFullHids = NULL;
	SG_rbtree* prbDagnodes = NULL;
	SG_push* pPush = NULL;
	SG_uint32 count_area_args = 0;
    SG_string* pstr_area_names = NULL;
	SG_uint32 nCountRevSpecs = 0;

	if(strcmp(pszDestRepoSpec, pszSrcRepoDescriptorName)==0)
		SG_ERR_THROW_RETURN(SG_ERR_SOURCE_AND_DEST_REPO_SAME);

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszSrcRepoDescriptorName, &pThisRepo)  );

	if (pvh_area_names)
		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_area_names, &count_area_args)  );
	if (pRevSpec)
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &nCountRevSpecs)  );

	// If a rev spec was provided, lookup its hids.
	if (nCountRevSpecs)
	{
		if (1 == count_area_args)
        {
            const char* psz_area_name = NULL;

			SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_area_names, 0, &psz_area_name, NULL)  );
            if (0 != strcmp(psz_area_name, SG_AREA_NAME__VERSION_CONTROL))
            {
                SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
                    "\nThe rev, tag and branch options are valid only for version control.\n\n")  );
                SG_ERR_THROW(SG_ERR_USAGE);
            }
        }
		else if (0 == count_area_args)
        {
            // ok
        }
		else
		{
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
				"\nThe rev, tag and branch options are valid only for version control.\n\n")  );
			SG_ERR_THROW(SG_ERR_USAGE);
		}

        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Pushing to %s:\n", pszDestRepoSpec)  );

		if (!bForce)
			SG_ERR_CHECK(  _check_locks_for_push(pCtx, pszDestRepoSpec, pszUsername, pszPassword, pThisRepo)  );

		SG_ERR_CHECK(  SG_push__begin(pCtx, pThisRepo, pszDestRepoSpec, pszUsername, pszPassword, &pPush)  );
		SG_ERR_CHECK(  SG_push__add__area__version_control(pCtx, pPush, pRevSpec)  );
		SG_ERR_CHECK(  SG_push__commit(pCtx, &pPush, bForce, ppvhAccountInfo, ppvhStats)  );
	}
	else if (count_area_args)
	{
		SG_uint32 i;

		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pstr_area_names)  );
		for (i = 0; i < count_area_args; i++)
		{
            const char* psz_area_name = NULL;

			SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_area_names, i, &psz_area_name, NULL)  );
            SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_area_names, psz_area_name)  );
			if ((i+1) < count_area_args)
            {
                SG_ERR_CHECK(  SG_string__append__sz(pCtx, pstr_area_names, ", ")  );
            }
		}
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Pushing %s to %s:\n", SG_string__sz(pstr_area_names), pszDestRepoSpec)  );
        SG_STRING_NULLFREE(pCtx, pstr_area_names);

		{
			SG_bool bHasVersionControlArea = SG_FALSE;
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pvh_area_names, SG_AREA_NAME__VERSION_CONTROL, &bHasVersionControlArea)  );
			if (!bForce && bHasVersionControlArea)
				SG_ERR_CHECK(  _check_locks_for_push(pCtx, pszDestRepoSpec, pszUsername, pszPassword, pThisRepo)  );
		}

		SG_ERR_CHECK(  SG_push__begin(pCtx, pThisRepo, pszDestRepoSpec, pszUsername, pszPassword, &pPush)  );

		for (i = 0; i < count_area_args; i++)
		{
            const char* psz_area_name = NULL;

			SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_area_names, i, &psz_area_name, NULL)  );
			SG_ERR_CHECK(  SG_push__add__area(pCtx, pPush, psz_area_name)  );
		}
		SG_ERR_CHECK(  SG_push__commit(pCtx, &pPush, bForce, ppvhAccountInfo, ppvhStats)  );
	}
	else
	{
		/* No arguments constraining the push were provided: push all */
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Pushing to %s:\n", pszDestRepoSpec)  );
		if (!bForce)
			SG_ERR_CHECK(  _check_locks_for_push(pCtx, pszDestRepoSpec, pszUsername, pszPassword, pThisRepo)  );
		SG_ERR_CHECK(  SG_push__all(pCtx, pThisRepo, pszDestRepoSpec, pszUsername, pszPassword, bForce, ppvhAccountInfo, ppvhStats)  );
	}

	SG_ERR_CHECK(  SG_sync__remember_sync_target(pCtx, pszSrcRepoDescriptorName, pszDestRepoSpec )  );

	/* fall through */
fail:
	if (pPush)
		SG_ERR_IGNORE(  SG_push__abort(pCtx, &pPush)  );
	SG_NULLFREE(pCtx, pszRevFullHid);
	SG_REPO_NULLFREE(pCtx, pThisRepo);
	SG_RBTREE_NULLFREE(pCtx, prbDagnodes);
    SG_STRING_NULLFREE(pCtx, pstr_area_names);
	SG_STRINGARRAY_NULLFREE(pCtx, psaFullHids);
	SG_ERR_IGNORE(  SG_log__silence(pCtx, SG_FALSE)  );
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_VV_VERBS__CHECKOUT_H
