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
 * @file sg__vvwc__do_cmd_status.c
 *
 * @details Support the 'vv status' command.
 * 
 * The 'vv status' command has 3 different variants depending on
 * the number of REV-SPEC args given.
 * 
 * [0] baseline-cset vs live-wd
 * 
 * Usage:
 *
 * vv status
 *       [--no-ignores]
 *       [--list-unchanged]
 *       [--list-sparse]
 *       [--verbose]
 *       [--nonrecursive]
 *       [item1 [item2 [...]]]
 *
 * Where 'item1', 'item2', ... are entrynames/pathnames/repo-paths.
 * If no items are given, we display all changes in the WD.
 * If one or more items are given, we only display info for them.
 * The --nonrecursive option only makes sense when given one or
 * more items.
 *
 * [1] arbitrary cset-vs-live-wd (composite)
 *
 * TODO 2011/12/21
 * 
 * [2] cset-vs-cset (historical) status (no wd required)
 *
 * Usage:
 *
 * vv status
 *       [--repo=REPOSITORY]
 *       [<rev_spec_0>]
 *       [<rev_spec_1>]
 *       [--verbose]
 *       TODO [--nonrecursive]
 *       TODO [item1 [item2 [...]]]
 *
 * If the REPOSITORY name is omitted, we'll try to get it
 * from the WD, if present.
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

static void _s0__do_baseline_vs_wc(SG_context * pCtx,
								   const SG_option_state * pOptSt,											 
								   const SG_stringarray * psaArgs)
{
	SG_varray * pvaStatus = NULL;
	SG_vhash * pvhLegend = NULL;
	SG_string * pString = NULL;

	// TODO            Do we have a way of marking an *arg* as
	// TODO            advanced (like we do for some of the
	// TODO            commands) so that it won't show up in
	// TODO            the basic help?
	SG_bool bNoTSC = SG_FALSE;				// TODO 2011/10/13 Do we want a --no-tsc argument?
	SG_bool bListReserved = SG_FALSE;		// TODO 2012/10/04 Do we want a --list-reserved argument?
	SG_bool bNoSort = SG_FALSE;				// TODO 2012/04/26 Do we want a --no-sort argument?

	SG_bool bListSparse = SG_TRUE;			// See P1579

	SG_ERR_CHECK(  SG_wc__status__stringarray(pCtx, NULL, psaArgs,
											  WC__GET_DEPTH(pOptSt),
											  pOptSt->bListUnchanged,
											  WC__GET_NO_IGNORES(pOptSt),
											  bNoTSC,
											  bListSparse,
											  bListReserved,
											  bNoSort,
											  &pvaStatus,
											  &pvhLegend)  );

	SG_ERR_CHECK(  SG_wc__status__classic_format(pCtx,
												 pvaStatus,
												 pOptSt->bVerbose,
												 &pString)  );
	SG_ERR_CHECK(  SG_console__raw(pCtx, SG_CS_STDOUT, SG_string__sz(pString))  );

	// TODO 2012/02/28 Do a nicer job of printing the legend.
#if 0 && defined(DEBUG)
	SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhLegend, "Legend")  );
#endif
	
fail:
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_VHASH_NULLFREE(pCtx, pvhLegend);
	SG_STRING_NULLFREE(pCtx, pString);
}

//////////////////////////////////////////////////////////////////

static void _s1__do_cset_vs_wc(SG_context * pCtx,
							   const SG_option_state * pOptSt,											 
							   const SG_stringarray * psaArgs)
{
	SG_varray * pvaStatus = NULL;
	SG_string * pString = NULL;

	// TODO            Do we have a way of marking an *arg* as
	// TODO            advanced (like we do for some of the
	// TODO            commands) so that it won't show up in
	// TODO            the basic help?
	SG_bool bNoTSC = SG_FALSE;				// TODO 2011/10/13 Do we want a --no-tsc argument?
	SG_bool bListReserved = SG_FALSE;		// TODO 2012/10/04 Do we want a --list-reserved argument?
	SG_bool bNoSort = SG_FALSE;				// TODO 2012/04/26 Do we want a --no-sort argument?

	SG_bool bListSparse = SG_TRUE;			// See P1579

	SG_ERR_CHECK(  SG_wc__status1__stringarray(pCtx, NULL, pOptSt->pRevSpec,
											   psaArgs,
											   WC__GET_DEPTH(pOptSt),
											   pOptSt->bListUnchanged,
											   WC__GET_NO_IGNORES(pOptSt),
											   bNoTSC,
											   bListSparse,
											   bListReserved,
											   bNoSort,
											   &pvaStatus)  );

	SG_ERR_CHECK(  SG_wc__status__classic_format(pCtx,
												 pvaStatus,
												 pOptSt->bVerbose,
												 &pString)  );
	SG_ERR_CHECK(  SG_console__raw(pCtx, SG_CS_STDOUT, SG_string__sz(pString))  );

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_STRING_NULLFREE(pCtx, pString);
}

//////////////////////////////////////////////////////////////////

static void _s2__do_cset_vs_cset(SG_context * pCtx,
								 const SG_option_state * pOptSt,
								 const SG_stringarray * psaArgs)
{
	SG_varray * pvaStatus = NULL;
	SG_vhash * pvhLegend = NULL;
	SG_string * pString = NULL;
	SG_bool bNoSort = SG_FALSE;		// TODO 2012/04/26 Do we want a --no-sort argument?

	// We DO NOT support --list-unchanged for historical status.
	// We could with a little work, but I'm not going to do so today.

	SG_ERR_CHECK(  SG_vv2__status(pCtx,
								  pOptSt->psz_repo,
								  pOptSt->pRevSpec,
								  psaArgs,
								  WC__GET_DEPTH(pOptSt),
								  bNoSort,
								  &pvaStatus,
								  &pvhLegend)  );

	// TODO 2012/01/13 I just created a pvaStatus using VV2
	// TODO            and am going to format it using WC.
	// TODO            This feels odd -- like maybe we need
	// TODO            to some of the status stuff out of WC.

	SG_ERR_CHECK(  SG_wc__status__classic_format(pCtx,
												 pvaStatus,
												 pOptSt->bVerbose,
												 &pString)  );
	SG_ERR_CHECK(  SG_console__raw(pCtx, SG_CS_STDOUT, SG_string__sz(pString))  );

	// TODO 2012/02/28 Do a nicer job of printing the legend.
#if 0 && defined(DEBUG)
	SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhLegend, "Legend")  );
#endif
	
fail:
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_STRING_NULLFREE(pCtx, pString);
	SG_VHASH_NULLFREE(pCtx, pvhLegend);
}

//////////////////////////////////////////////////////////////////

/**
 * Main 'vv status' entry point.  We decide which variant is intended.
 *
 * TODO 2011/12/21 Should we split out the [1] and [2] forms into a
 * TODO            different top-level command and/or use the sub-command
 * TODO            form for them so it is a little less confusing in the
 * TODO            usage/help ?
 * 
 */
void vvwc__do_cmd_status(SG_context * pCtx,
						 const SG_option_state * pOptSt,
						 const SG_stringarray * psaArgs)
{
	SG_uint32 nrRevSpecs = 0;

	if (pOptSt->pRevSpec)
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pOptSt->pRevSpec, &nrRevSpecs)  );

	switch (nrRevSpecs)
	{
	case 0:
		SG_ERR_CHECK(  _s0__do_baseline_vs_wc(pCtx, pOptSt, psaArgs)  );
		break;

	case 1:
		SG_ERR_CHECK(  _s1__do_cset_vs_wc(pCtx, pOptSt, psaArgs)  );
		break;

	case 2:
		SG_ERR_CHECK(  _s2__do_cset_vs_cset(pCtx, pOptSt, psaArgs)  );
		break;

	default:
		SG_ERR_THROW( SG_ERR_USAGE );
	}

fail:
	return;
}

