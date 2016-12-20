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
 * @file sg__vvwc__do_cmd_mstatus.c
 *
 * @details Support the 'vv mstatus' command.
 * 
 * The 'vv mstatus' command has 2 different variants depending on
 * the number of REV-SPEC args given.
 * 
 * [0] live/wc -- start with the WC and get the 1 or 2 parents.
 *                if 2 parents, compute LCA and do MSTATUS(A,B,C,WC).
 *                if 1 parent (and fallback allowed), do regular STATUS(B,WC).
 * 
 * Usage:
 *
 * vv status
 *       [--no-ignores]
 *       [--no-fallback]
 *       [--verbose]
 *
 * TODO 2012/02/21 we do not currently support (and I'm not sure I want to):
 * TODO            [--list-sparse]
 * TODO            [--nonrecursive]
 * TODO            [item1 [item2 [...]]]
 * TODO
 * TODO Where 'item1', 'item2', ... are entrynames/pathnames/repo-paths.
 * TODO If no items are given, we display all changes in the WD.
 * TODO If one or more items are given, we only display info for them.
 * TODO The --nonrecursive option only makes sense when given one or
 * TODO more items.
 *
 * 
 * [1] historical -- no wd required -- start with the given M cset.
 *                   compute the parents of M.
 *                   if 2 parents, compute LCA and do MSTATUS(A,B,C,M).
 *                   if 1 parent (and fallback allowed), do regular STATUS(B,M).
 *
 * Usage:
 *
 * vv status
 *       [--repo=REPOSITORY]
 *       [<rev_spec_0>]
 *       [--no-fallback]
 *       [--verbose]
 *
 * TODO 2012/02/21 we do not currently support (and I'm not sure I want to):
 * TODO            [--nonrecursive]
 * TODO            [item1 [item2 [...]]]
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

static void _mstatus__s0__wc(SG_context * pCtx,
							 const SG_option_state * pOptSt)
{
	SG_varray * pvaStatus = NULL;
	SG_vhash * pvhLegend = NULL;
	SG_string * pString = NULL;
	SG_bool bNoSort = SG_FALSE;				// TODO 2012/04/26 Do we want a --no-sort argument?

	SG_ERR_CHECK(  SG_wc__mstatus(pCtx, NULL, WC__GET_NO_IGNORES(pOptSt), pOptSt->bNoFallback, bNoSort,
								  &pvaStatus, &pvhLegend)  );

	// TODO 2012/02/23 Legend for WC MSTATUS ?

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

static void _mstatus__s1__historical(SG_context * pCtx,
									 const SG_option_state * pOptSt)
{
	SG_varray * pvaStatus = NULL;
	SG_vhash * pvhLegend = NULL;
	SG_string * pString = NULL;
	SG_bool bNoSort = SG_FALSE;				// TODO 2012/04/26 Do we want a --no-sort argument?

	SG_ERR_CHECK(  SG_vv2__mstatus(pCtx,
								   pOptSt->psz_repo,
								   pOptSt->pRevSpec,
								   pOptSt->bNoFallback,
								   bNoSort,
								   &pvaStatus,
								   &pvhLegend)  );

	// TODO 2012/02/23 print the legend.

	// TODO 2012/01/13 I just created a pvaStatus using VV2
	// TODO            and am going to format it using WC.
	// TODO            This feels odd -- like maybe we need
	// TODO            to move some of the status stuff out of WC.

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

/**
 * Main 'vv mstatus' entry point.  We decide which variant is intended.
 * 
 */
void vvwc__do_cmd_mstatus(SG_context * pCtx,
						  const SG_option_state * pOptSt)
{
	SG_uint32 nrRevSpecs = 0;

	if (pOptSt->pRevSpec)
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pOptSt->pRevSpec, &nrRevSpecs)  );

	switch (nrRevSpecs)
	{
	case 0:
		SG_ERR_CHECK(  _mstatus__s0__wc(pCtx, pOptSt)  );
		break;

	case 1:
		SG_ERR_CHECK(  _mstatus__s1__historical(pCtx, pOptSt)  );
		break;

	default:
		SG_ERR_THROW( SG_ERR_USAGE );
	}

fail:
	return;
}

