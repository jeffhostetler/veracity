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
 * @file sg__vvwc__do_cmd_history.c
 *
 * @details Support the 'vv history' command.
 *
 * This has 2 forms:
 *
 * [] When '--repo' given, we use the strictly historical form and directly
 *    open the repo and require that any files/folders be full repo-paths
 *    "@/abc/def" which will be interpreted relative to *SOME/WHICH* cset.
 *    We do not require a WD to be present.
 *
 *    Note that this code was written before the extended-prefix repo-path
 *    stuff, so we don't yet have a way to properly specify things.
 *
 * [] When no '--repo' given, we assume the existence of a WD and use it
 *    to get the repo *and* we interpret any files/folders as "input"
 *    paths (like all other SG_wc__ routines).
 *
 *    Note that this code was written before the extended-prefix repo-path
 *    stuff, so we support absolute- and relative-paths and "@/abc/def" live
 *    repo-paths and "@b/abc/def" for queued-deleted items.  We don't have
 *    code to let you give an extended-prefix repo-path that refers to an
 *    arbitrary historical cset.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include <sg_vv2__public_typedefs.h>

#include <sg_wc__public_prototypes.h>
#include <sg_vv2__public_prototypes.h>

#include "sg_typedefs.h"
#include "sg_prototypes.h"
#include "sg_cmd_util_prototypes.h"

//////////////////////////////////////////////////////////////////

void vv2__do_cmd_history(SG_context * pCtx, 
						 const SG_option_state * pOptSt,
						 const SG_stringarray * psaArgs,
						 SG_bool b_graph)
{
	SG_vv2_history_args historyArgs;
	SG_rev_spec * pRevSpec_single_revisions = NULL;
	SG_history_result * pHistoryResult = NULL;
	SG_vhash* pvhBranchPile = NULL;
    SG_string* pstr_comment = NULL;
    SG_string* pstr_wrapped = NULL;
	SG_uint32 nResultsSoFar = 0;

	SG_ERR_CHECK(  memset(&historyArgs, 0, sizeof(historyArgs))  );

	historyArgs.pszRepoDescriptorName = pOptSt->psz_repo;

	if (psaArgs)
	{
		SG_uint32 count_args = 0;

		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaArgs, &count_args)  );
		if (count_args > 1)
			SG_ERR_THROW2(  SG_ERR_USAGE,
							(pCtx, "History queries only support one file/folder argument at a time.")  );

		historyArgs.psaSrcArgs = psaArgs;
	}

	historyArgs.bListAll = pOptSt->bListAll;
	if (pOptSt->bListAll)
	{
		// See J8493.  If they gave one or more --rev or --tag options,
		// then we normally just print info for that/those csets only
		// and stop -- we don't print the full history from each cset.
		//
		// --list-all indicates that they want the full history *even
		// when* they give us --rev or --tag options.
		//
		// So we can just leave all of the --rev and --tag options in
		// the main pRevSpec.
	}
	else
	{
		// split out the --rev and --tag options into the alternate
		// pRevSpec.  This alters the callers rev-spec.

		SG_ERR_CHECK(  SG_rev_spec__split_single_revisions(pCtx, pOptSt->pRevSpec, &pRevSpec_single_revisions)  );
	}
	historyArgs.pRevSpec = pOptSt->pRevSpec;
	historyArgs.pRevSpec_single_revisions = pRevSpec_single_revisions;

	historyArgs.pszUser = pOptSt->psz_username;

	if (pOptSt->psa_stamps)
	{
		SG_uint32 count_stamps = 0;

		// History only supports one stamp argument at the moment.

		SG_ERR_CHECK(  SG_stringarray__count(pCtx, pOptSt->psa_stamps, &count_stamps)  );
		if (count_stamps > 1)
			SG_ERR_THROW2(  SG_ERR_USAGE,
							(pCtx, "History queries only support one stamp at a time.")  );

		else if (count_stamps == 1)
			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, pOptSt->psa_stamps, 0, &historyArgs.pszStamp)  );
	}

	if (pOptSt->psz_from_date && *pOptSt->psz_from_date)
		SG_ERR_CHECK(  SG_time__parse__informal__local(pCtx, pOptSt->psz_from_date, &historyArgs.nFromDate, SG_FALSE)  );

	if (pOptSt->psz_to_date && *pOptSt->psz_to_date)
		SG_ERR_CHECK(  SG_time__parse__informal__local(pCtx, pOptSt->psz_to_date, &historyArgs.nToDate, SG_TRUE)  );
	else
		historyArgs.nToDate = SG_INT64_MAX;

	historyArgs.nResultLimit = pOptSt->iMaxHistoryResults;

	historyArgs.bHideObjectMerges = pOptSt->bHideMerges;

	historyArgs.bLeaves = pOptSt->bLeaves;

	historyArgs.bReverse = pOptSt->bReverseOutput;
	
	SG_ERR_CHECK(  SG_vv2__history(pCtx, &historyArgs, NULL, &pvhBranchPile, &pHistoryResult)  );

	if (pHistoryResult != NULL)
	{
		while(nResultsSoFar < historyArgs.nResultLimit)
		{
			SG_uint32 nHistItemsReturned = 0;
			SG_ERR_CHECK(  SG_history_result__count(  pCtx, pHistoryResult, &nHistItemsReturned)  );
			nResultsSoFar += nHistItemsReturned;

			if (b_graph)
			{
				// this code dumps history in the form of a graphviz "dot" file instead
				// of the usual way.
				//print the information for each
				SG_bool bFound = SG_TRUE;
				const char* psz_cs_hid = NULL;
				SG_uint32 count_parents = 0;
				SG_uint32 nCount = 0;
				SG_uint32 nIndex = 0;
				const char* psz_comment = NULL;
				const char* psz_who = NULL;
				char buf_time_formatted[256];

				SG_console(pCtx, SG_CS_STDOUT, "digraph %s\n", "history");
				SG_console(pCtx, SG_CS_STDOUT, "{\n");
				SG_console(pCtx, SG_CS_STDOUT, "rankdir=BT;\n");

				while (bFound)
				{
					SG_ERR_CHECK(  SG_history_result__get_cshid(pCtx, pHistoryResult, &psz_cs_hid)  );
					SG_ERR_CHECK(  SG_history_result__get_parent__count(pCtx, pHistoryResult, &count_parents)  );

					SG_ERR_CHECK(  SG_history_result__get_audit__count(pCtx, pHistoryResult, &nCount)  );
					if (nCount)
					{
						SG_int64 itime = -1;
						SG_time tmLocal;

						SG_ERR_CHECK(  SG_history_result__get_audit__who(pCtx, pHistoryResult, nCount-1, &psz_who)  );

						SG_ERR_CHECK(  SG_history_result__get_audit__when(pCtx, pHistoryResult, nCount-1, &itime)  );
						SG_ERR_CHECK(  SG_time__decode__local(pCtx, itime, &tmLocal)  );
						SG_ERR_CHECK(  SG_sprintf(pCtx, 
												  buf_time_formatted, 
												  sizeof(buf_time_formatted),
												  "%4d-%02d-%02d", 
												  tmLocal.year,
												  tmLocal.month,
												  tmLocal.mday
										   )  );
					}

					SG_ERR_CHECK(  SG_history_result__get_first_comment__text(pCtx, pHistoryResult, &psz_comment)  );

					SG_ERR_CHECK(  SG_string__alloc__sz(pCtx, &pstr_comment, psz_comment)  );
					SG_ERR_CHECK(  SG_string__replace_bytes(pCtx, pstr_comment,
															(const SG_byte*)"\n", 1,
															(const SG_byte*)" ", 1,
															SG_UINT32_MAX, SG_TRUE, NULL)  );
					SG_ERR_CHECK(  my_wrap_text(pCtx, pstr_comment, 40, &pstr_wrapped)  );
					SG_ERR_CHECK(  SG_string__replace_bytes(pCtx, pstr_wrapped,
															(const SG_byte*)"\n", 1,
															(const SG_byte*)"\\n", 2,
															SG_UINT32_MAX, SG_TRUE, NULL)  );
					SG_ERR_CHECK(  SG_string__replace_bytes(pCtx, pstr_wrapped,
															(const SG_byte*)"\\\"", 2,
															(const SG_byte*)"\"", 1,
															SG_UINT32_MAX, SG_TRUE, NULL)  );
					SG_ERR_CHECK(  SG_string__replace_bytes(pCtx, pstr_wrapped,
															(const SG_byte*)"\"", 1,
															(const SG_byte*)"\\\"", 2,
															SG_UINT32_MAX, SG_TRUE, NULL)  );
					SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "_%s [%s label=\"%s\\n%s\\n%s\"];\n", 
											  psz_cs_hid, 
											  (count_parents > 1) ? "style=filled, fillcolor=yellow," : "",
											  psz_who,
											  buf_time_formatted,
											  SG_string__sz(pstr_wrapped)
									   )  );
					SG_STRING_NULLFREE(pCtx, pstr_comment);
					SG_STRING_NULLFREE(pCtx, pstr_wrapped);

					for (nIndex = 0; nIndex < count_parents; nIndex++)
					{
						const char * pszParent = NULL;
						SG_ERR_CHECK(  SG_history_result__get_parent(pCtx, pHistoryResult, nIndex, &pszParent, NULL)  );
						SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "_%s -> _%s\n", psz_cs_hid, pszParent)  );
					}

					SG_ERR_CHECK(  SG_history_result__next(pCtx, pHistoryResult, &bFound)  );
				}
				SG_console(pCtx, SG_CS_STDOUT, "}\n");
			}
			else
			{
				SG_ERR_CHECK(  SG_cmd_util__dump_history_results(pCtx, SG_CS_STDOUT, pHistoryResult, pvhBranchPile, SG_FALSE,
																 pOptSt->bVerbose, SG_FALSE) );
			}
			break;
		}

	}
fail:
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec_single_revisions);
	SG_STRING_NULLFREE(pCtx, pstr_wrapped);
	SG_STRING_NULLFREE(pCtx, pstr_comment);
	SG_VHASH_NULLFREE(pCtx, pvhBranchPile);
	SG_HISTORY_RESULT_NULLFREE(pCtx, pHistoryResult);
}
