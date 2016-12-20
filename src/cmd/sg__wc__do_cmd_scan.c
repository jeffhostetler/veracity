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
 * @file sg__wc__do_cmd_scan.c
 *
 * @details 
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

/**
 * The 'vv scan' command is a relic from the way we
 * originally handled things in PendingTree.  With the
 * WC re-write of PendingTree, we don't really need it.
 * I'm going to leave it in as an 'advanced' command
 * to allow the user to flush and rebuild the timestamp
 * cache.
 *
 */
void wc__do_cmd_scan(SG_context * pCtx,
					 const SG_option_state * pOptSt)
{
	SG_varray * pvaStatus = NULL;
	SG_vhash * pvhLegend = NULL;
	SG_string * pString = NULL;

	SG_bool bListSparse = SG_TRUE;			// See P1579

	// TODO 2011/10/13 Should we do both of these operations
	// TODO            in the same TX (using the wx7txapi layer) ?

	SG_ERR_CHECK(  SG_wc__flush_timestamp_cache(pCtx, NULL)  );

	// See also do_cmd_status__do_baseline_vs_wc().

	SG_ERR_CHECK(  SG_wc__status(pCtx, NULL,
								 "@/",
								 SG_INT32_MAX,	// assume recursive
								 SG_FALSE,		// no list-unchanged
								 WC__GET_NO_IGNORES(pOptSt),
								 SG_FALSE,		// assume use TSC
								 bListSparse,
								 SG_FALSE,		// bListReserved
								 SG_FALSE,		// bNoSort
								 &pvaStatus,
								 &pvhLegend)  );

	SG_ERR_CHECK(  SG_wc__status__classic_format(pCtx,
												 pvaStatus,
												 pOptSt->bVerbose,
												 &pString)  );
	SG_ERR_CHECK(  SG_console__raw(pCtx, SG_CS_STDOUT, SG_string__sz(pString))  );

	// TODO 2012/02/28 Do a nicer job of printing the legend.
#if defined(DEBUG)
	SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhLegend, "Legend")  );
#endif

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_VHASH_NULLFREE(pCtx, pvhLegend);
	SG_STRING_NULLFREE(pCtx, pString);
}

