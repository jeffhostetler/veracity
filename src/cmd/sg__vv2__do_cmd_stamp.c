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
 * @file sg__vv2__do_cmd_stamp.c
 *
 * @details Handle 'vv stamp' and 'vv stamps' and all sub-commands.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_vv2__public_typedefs.h>
#include <sg_vv2__public_prototypes.h>

#include "sg_typedefs.h"
#include "sg_prototypes.h"
#include "sg_cmd_util_prototypes.h"

//////////////////////////////////////////////////////////////////

/**
 * 'vv stamps'
 *
 * Print a summary of the stamps in use and their frequency.
 *
 */
void vv2__do_cmd_stamps(SG_context * pCtx,
						const SG_option_state * pOptSt)
{
	SG_varray * pva_results = NULL;
	SG_uint32 count_results = 0;
	SG_uint32 index_results = 0;

#if 1 // Need W6606
	{
		// TODO 2012/06/28 Until W6606 is fixed we manually cross-check
		// TODO            for invalid options from other subcommands.
		// TODO
		// TODO            stamp <subcommand> takes: --repo --all --rev --branch --tag
		// TODO            stamps             takes: --repo

		if (pOptSt->pRevSpec)
		{
			SG_uint32 countRevSpecs = 0;
			SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pOptSt->pRevSpec, &countRevSpecs)  );
			if (countRevSpecs > 0)
				SG_ERR_THROW(  SG_ERR_USAGE  );
		}
		if (pOptSt->bAll)
			SG_ERR_THROW(  SG_ERR_USAGE  );
	}
#endif

	SG_ERR_CHECK(  SG_vv2__stamps(pCtx, pOptSt->psz_repo, &pva_results)  );

	SG_ERR_CHECK(  SG_varray__count(pCtx, pva_results, &count_results)  );
	for (index_results = 0; index_results < count_results; index_results++)
	{
		SG_vhash * pvhThisResult = NULL;
		const char * psz_thisStamp = NULL;
		SG_int64 thisCount = 0;

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pva_results, index_results, &pvhThisResult)  );

		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhThisResult, "stamp", &psz_thisStamp)  );
		SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhThisResult, "count", &thisCount)  );
		SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDOUT, "%s:\t%d\n", psz_thisStamp, (int)thisCount)  );
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pva_results);
}

//////////////////////////////////////////////////////////////////

/**
 * 'vv stamp add <stamp>'
 *
 * Add a STAMP to one or more changesets.
 *
 */
static void vv2__do_cmd_stamp__add(SG_context * pCtx,
								   const SG_option_state * pOptSt,
								   SG_uint32 argc,
								   const char ** argv)
{
	SG_varray * pvaInfo = NULL;
	SG_uint32 k, count;
	const char * pszStampName;

#if 1 // Need W6606
	{
		// TODO 2012/06/28 Until W6606 is fixed we manually cross-check
		// TODO            for invalid options from other subcommands.
		// TODO
		// TODO            stamp <subcommand> takes: --repo --all --rev --branch --tag
		// TODO            stamp add          takes: --repo       --rev --branch --tag

		if (pOptSt->bAll)
			SG_ERR_THROW(  SG_ERR_USAGE  );
	}
#endif

	// argv[0] is subcommand name
	// argv[1] is stamp name

	if (argc != 2)
		SG_ERR_THROW(  SG_ERR_USAGE  );

	pszStampName = argv[1];

	SG_ERR_CHECK(  SG_vv2__stamp__add(pCtx, pOptSt->psz_repo, pOptSt->pRevSpec, pszStampName, &pvaInfo)  );

	// TODO 2012/06/27 Should this be verbose output only ?

	if (pvaInfo)
	{
		SG_ERR_CHECK(  SG_varray__count(pCtx, pvaInfo, &count)  );
		for (k=0; k<count; k++)
		{
			SG_vhash * pvh_k;
			const char * pszHid_k;
			SG_bool bRedundant_k;

			SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaInfo, k, &pvh_k)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_k, "hid", &pszHid_k)  );
			SG_ERR_CHECK(  SG_vhash__get__bool(pCtx, pvh_k, "redundant", &bRedundant_k)  );

			if (bRedundant_k)
				SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT,
										   "(Changeset %s was already stamped \"%s\".)\n",
										   pszHid_k, pszStampName)  );
			else
				SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT,
										   "Stamp \"%s\" added to changeset %s\n",
										   pszStampName, pszHid_k)  );
		}
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaInfo);
}

/**
 * 'vv stamp remove <stamp>'
 *
 * Remove the named STAMP from either ALL changesets or the list of named changesets.
 *
 */
static void vv2__do_cmd_stamp__remove(SG_context * pCtx,
									  const SG_option_state * pOptSt,
									  SG_uint32 argc,
									  const char ** argv)
{
	SG_varray * pvaInfo = NULL;
	SG_uint32 k, count;
	const char * pszStampName;

#if 1 // Need W6606
	{
		// TODO 2012/06/28 Until W6606 is fixed we manually cross-check
		// TODO            for invalid options from other subcommands.
		// TODO
		// TODO            stamp <subcommand> takes: --repo --all --rev --branch --tag
		// TODO            stamp remove       takes: --repo --all --rev --branch --tag
	}
#endif

	// argv[0] is subcommand name
	// argv[1] is stamp name

	// TODO 2012/06/28 should we allow stamp name be null to
	// TODO            mean all of the stamps on a given cset?
	// TODO            see W5430.

	if (argc != 2)
		SG_ERR_THROW(  SG_ERR_USAGE  );

	pszStampName = argv[1];

	SG_ERR_CHECK(  SG_vv2__stamp__remove(pCtx, pOptSt->psz_repo, pOptSt->pRevSpec, pszStampName, pOptSt->bAll, &pvaInfo)  );

	// TODO 2012/06/27 Should this be verbose output only ?

	if (pvaInfo)
	{
		SG_ERR_CHECK(  SG_varray__count(pCtx, pvaInfo, &count)  );
		for (k=0; k<count; k++)
		{
			SG_vhash * pvh_k;
			const char * pszHid_k;

			SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaInfo, k, &pvh_k)  );
			SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_k, "hid", &pszHid_k)  );

			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDOUT,
									   "Stamp \"%s\" removed from changeset %s\n",
									   pszStampName, pszHid_k)  );
		}
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaInfo);
}

/**
 * 'vv stamp list <stamp>'
 *
 * Print full changeset details for all changesets having this STAMP.
 *
 */
static void vv2__do_cmd_stamp__list(SG_context * pCtx,
									const SG_option_state * pOptSt,
									SG_uint32 argc,
									const char ** argv)
{
	SG_stringarray * psaHidChangesets = NULL;
	SG_repo * pRepo = NULL;
	SG_vhash * pvhPileOfCleanBranches = NULL;
	SG_uint32 k, countChangesets;
	const char * pszStampName;


#if 1 // Need W6606
	{
		// TODO 2012/06/28 Until W6606 is fixed we manually cross-check
		// TODO            for invalid options from other subcommands.
		// TODO
		// TODO            stamp <subcommand> takes: --repo --all --rev --branch --tag
		// TODO            stamp list         takes: --repo

		if (pOptSt->pRevSpec)
		{
			SG_uint32 countRevSpecs = 0;
			SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pOptSt->pRevSpec, &countRevSpecs)  );
			if (countRevSpecs > 0)
				SG_ERR_THROW(  SG_ERR_USAGE  );
		}
		if (pOptSt->bAll)
			SG_ERR_THROW(  SG_ERR_USAGE  );
	}
#endif

	// argv[0] is subcommand name
	// argv[1] is stamp name

	if (argc != 2)
		SG_ERR_THROW(  SG_ERR_USAGE  );

	pszStampName = argv[1];

	SG_ERR_CHECK(  SG_vv2__stamp__list(pCtx, pOptSt->psz_repo, pszStampName, &psaHidChangesets)  );

	if (psaHidChangesets)
	{
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaHidChangesets, &countChangesets)  );
		if (countChangesets > 0)
		{
			// TODO 2012/06/27 This is a bit redundant.  The SG_vv2__stamp__() code opened
			// TODO            the repo (named or default), looked up stuff, built the list
			// TODO            of changesets and returned it to us.  Now *WE* have to open
			// TODO            the repo so that we can print log-details for each changeset.
			// TODO            This pattern happens many places (not just here).
			// TODO
			// TODO            It'd be nice if we could maybe refactor __dump_log() and push
			// TODO            some of it to a lower level (in WC or VV2, rather than having
			// TODO            cmd_util know that much detail).

			if (pOptSt->psz_repo && *pOptSt->psz_repo)
				SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pOptSt->psz_repo, &pRepo)  );
			else
				SG_ERR_CHECK(  SG_cmd_util__get_repo_from_cwd(pCtx, &pRepo, NULL)  );

			SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvhPileOfCleanBranches)  );
			for (k=0; k<countChangesets; k++)
			{
				const char * pszHid_k;

				SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaHidChangesets, k, &pszHid_k)  );
				SG_ERR_CHECK(  SG_cmd_util__dump_log(pCtx, SG_CS_STDOUT, pRepo, pszHid_k,
													 pvhPileOfCleanBranches, SG_TRUE, SG_FALSE)  );
			}
		}
	}

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psaHidChangesets);
	SG_REPO_NULLFREE(pCtx, pRepo);
    SG_VHASH_NULLFREE(pCtx, pvhPileOfCleanBranches);
}

//////////////////////////////////////////////////////////////////

void vv2__do_cmd_stamp__subcommand(SG_context * pCtx,
								   const SG_option_state * pOptSt,
								   SG_uint32 argc,
								   const char ** argv)
{
	if (strcmp(argv[0], "add") == 0)
	{
		SG_ERR_CHECK(  vv2__do_cmd_stamp__add(pCtx, pOptSt, argc, argv)  );
	}
	else if (strcmp(argv[0], "remove") == 0)
	{
		SG_ERR_CHECK(  vv2__do_cmd_stamp__remove(pCtx, pOptSt, argc, argv)  );
	}
	else if (strcmp(argv[0], "list") == 0)
	{
		if (argc == 1)
			SG_ERR_CHECK(  vv2__do_cmd_stamps(pCtx, pOptSt)  );
		else
			SG_ERR_CHECK(  vv2__do_cmd_stamp__list(pCtx, pOptSt, argc, argv)  );
	}
	else
	{
		SG_ERR_THROW2(  SG_ERR_USAGE,
						(pCtx, "Unknown subcommand '%s'.", argv[0])  );
	}

fail:
	return;
}
