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

#ifndef H_SG_VV_VERBS__PULL_H
#define H_SG_VV_VERBS__PULL_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

#if 0
void sg_force_dbndx_prep(
        SG_context* pCtx,
        SG_repo* pRepo
        )
{
	SG_uint32  i;
	SG_uint32  count_dagnums = 0;
	SG_uint64* paDagNums     = NULL;
    char* psz_leaf = NULL;

	SG_ERR_CHECK(  SG_repo__list_dags(pCtx, pRepo, &count_dagnums, &paDagNums)  );

	for (i=0; i<count_dagnums; i++)
	{
        if (SG_DAGNUM__IS_DB(paDagNums[i]))
        {
            SG_uint32 count = 0;
            char buf_dagnum[SG_DAGNUM__BUF_MAX__HEX];

            SG_ERR_CHECK(  SG_dagnum__to_sz__hex(pCtx, paDagNums[i], buf_dagnum, sizeof(buf_dagnum))  );
            fprintf(stderr, "%s\n", buf_dagnum);

            SG_ERR_CHECK(  SG_zing__get_leaf(pCtx, pRepo, NULL, paDagNums[i], &psz_leaf)  );
            SG_ERR_IGNORE(  SG_repo__dbndx__query__count(pCtx, pRepo, paDagNums[i], psz_leaf, "item", NULL, &count)  );
            SG_NULLFREE(pCtx, psz_leaf);

        }
	}

fail:
    SG_NULLFREE(pCtx, psz_leaf);
	SG_NULLFREE(pCtx, paDagNums);
}
#endif

void SG_vv_verbs__pull(SG_context * pCtx,
					   const char * pszSrcRepoSpec,
					   const char* pszUsername,
					   const char* pszPassword,
					   const char * pszDestRepoDescriptorName,
					   SG_vhash * pvh_area_names,
					   SG_rev_spec ** ppRevSpec,
					   SG_vhash** ppvhAccountInfo,
					   SG_vhash** ppvhStats)
{
	SG_repo* pRepoDest = NULL;
	SG_pull* pPull = NULL;
	SG_uint32 count_area_args = 0;
	SG_uint32 nCountRevSpecs = 0;
    SG_string* pstr_area_names = NULL;

	if(strcmp(pszSrcRepoSpec, pszDestRepoDescriptorName)==0)
		SG_ERR_THROW_RETURN(SG_ERR_SOURCE_AND_DEST_REPO_SAME);

	/* TODO: resolve branches in remote repo */

	if (pvh_area_names)
		SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh_area_names, &count_area_args)  );
	if (ppRevSpec && *ppRevSpec)
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, *ppRevSpec, &nCountRevSpecs)  );

	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszDestRepoDescriptorName, &pRepoDest)  );
	
	// If criteria were specified, make sure they're requesting the version control DAG.
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

        SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Pulling from %s:\n", pszSrcRepoSpec)  );
		
		SG_ERR_CHECK(  SG_pull__begin(pCtx, pRepoDest, pszSrcRepoSpec, pszUsername, pszPassword, &pPull)  );

		SG_ERR_CHECK(  SG_pull__add__area__version_control(pCtx, pPull, ppRevSpec)  );

		SG_ERR_CHECK(  SG_pull__commit(pCtx, &pPull, ppvhAccountInfo, ppvhStats)  );
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
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Pulling %s from %s:\n", SG_string__sz(pstr_area_names), pszSrcRepoSpec)  );
        SG_STRING_NULLFREE(pCtx, pstr_area_names);

		SG_ERR_CHECK(  SG_pull__begin(pCtx, pRepoDest, pszSrcRepoSpec, pszUsername, pszPassword, &pPull)  );

		for (i = 0; i < count_area_args; i++)
		{
            const char* psz_area_name = NULL;

			SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh_area_names, i, &psz_area_name, NULL)  );

			SG_ERR_CHECK(  SG_pull__add__area(pCtx, pPull, psz_area_name)  );
		}
		SG_ERR_CHECK(  SG_pull__commit(pCtx, &pPull, ppvhAccountInfo, ppvhStats)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "Pulling from %s:\n", pszSrcRepoSpec)  );
        // TODO not sure we actually want to pull ALL here.
		SG_ERR_CHECK(  SG_pull__all(pCtx, pRepoDest, pszSrcRepoSpec, pszUsername, pszPassword, ppvhAccountInfo, ppvhStats)  );
	}

#if 0
    SG_ERR_CHECK(  sg_force_dbndx_prep(pCtx, pRepoDest)  );
#endif

	if (! SG_CONTEXT__HAS_ERR(pCtx))
	{
		SG_ERR_CHECK(  SG_sync__remember_sync_target(pCtx, pszDestRepoDescriptorName, pszSrcRepoSpec)  );
	}

	/* fall through */
fail:
	if (pPull)
		SG_ERR_IGNORE(  SG_pull__abort(pCtx, &pPull)  );
	SG_REPO_NULLFREE(pCtx, pRepoDest);
    SG_STRING_NULLFREE(pCtx, pstr_area_names);
}

END_EXTERN_C;

#endif//H_SG_VV_VERBS__CHECKOUT_H
