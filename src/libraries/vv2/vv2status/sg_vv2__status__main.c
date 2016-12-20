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
 * @file sg_vv2__status__main.c
 *
 * @details Compute a COMPLETE STATUS between 2 changesets.
 * We DO NOT take an ARGV or non-recursive options.
 *
 * We DO NOT have nor use a WD (which means that we DO NOT know
 * about wc.db nor gid-aliases nor know about a current baseline).
 *
 * 2011/12/15 This code was refactored out of SG_treediff2.c from
 * the PendingTree version of sglib.  That version could take 0,
 * 1, or 2 csets, do composite diffs, and returned a pTreeDiff2
 * which could then be used to generate 'vv status' and/or 'vv diff'
 * output.  It would always compute the full diff and then could
 * be post-filtered before status/diff reports were created.  It
 * also suffered from being intertangled with PendingTree and
 * forced the caller to deal with a pTreeDiff.
 *
 * The version here ***ONLY*** does a 2-way compare of 2 CSETS
 * and it returns the results in a canonical-status format.
 * 
 * This version does not do any filtering like the original
 * TreeDiff2 did.  See W3440.
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

void sg_vv2__status__make_legend(SG_context * pCtx,
								 SG_vhash ** ppvhLegend,
								 const char * pszHid_0,   const char * pszHid_1,
								 const char * pszLabel_0, const char * pszLabel_1)
{
	SG_vhash * pvhLegend = NULL;

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhLegend)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhLegend, pszLabel_0, pszHid_0)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhLegend, pszLabel_1, pszHid_1)  );

	SG_RETURN_AND_NULL( pvhLegend, ppvhLegend );

fail:
	SG_VHASH_NULLFREE(pCtx, pvhLegend);
}

//////////////////////////////////////////////////////////////////

/**
 * Compute basic status (diff) of 2 csets without any
 * assumptions on how/if they are related.
 *
 * This is {cset[0]}==>{cset[1]} and we label items in terms
 * of cset[1].  For example, an item present in cset[0] and not
 * present in cset[1] is said to be: "deleted from cset[1]"
 * as opposed to "added to cset[0]".
 *
 * All repo-paths in the resulting status will be properly
 * domain-prefixed with the given domain chars.
 *
 * We return a VARRAY containing a VHASH for each changed
 * item.  Within each VHASH we have common (top-level) fields
 * and a per-cset sub-section for each cset that the item
 * appears in.  These sub-sections are labelled with the given
 * labels.
 *
 */
void sg_vv2__status__main(SG_context * pCtx,
						  SG_repo * pRepo,
						  SG_rbtree * prbTreenodeCache_Shared,	// optional
						  const char * pszHidCSet_0,
						  const char * pszHidCSet_1,
						  const char chDomain_0,
						  const char chDomain_1,
						  const char * pszLabel_0,
						  const char * pszLabel_1,
						  const char * pszWasLabel_0,
						  const char * pszWasLabel_1,
						  SG_bool bNoSort,
						  SG_varray ** ppvaStatus,
						  SG_vhash ** ppvhLegend)
{
	sg_vv2status * pST = NULL;
	SG_rbtree * prbTreenodeCache_Allocated = NULL;	// we allocate one if not given one.
	SG_varray * pvaStatus = NULL;
	SG_vhash * pvhLegend = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	// prbTreenodeCache_Shared is optional
	SG_NONEMPTYCHECK_RETURN(pszHidCSet_0);
	SG_NONEMPTYCHECK_RETURN(pszHidCSet_1);
	SG_NULLARGCHECK_RETURN(ppvaStatus);
	// ppvhLegend is optional

	if (strcmp(pszHidCSet_0, pszHidCSet_1) == 0)		// must be 2 different changesets
		SG_ERR_THROW2_RETURN(  SG_ERR_INVALIDARG,
							   (pCtx,"Comparing changset %s with itself.",pszHidCSet_0)  );

#if TRACE_VV2_STATUS
	SG_ERR_IGNORE(  SG_console(pCtx,SG_CS_STDERR,
							   "vv2status: CSET0_VS_CSET1 [cset0 %s][cset1 %s]\n",
							   pszHidCSet_0, pszHidCSet_1)  );
#endif

	if (!prbTreenodeCache_Shared)
		SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prbTreenodeCache_Allocated)  );

	SG_ERR_CHECK(  sg_vv2__status__alloc(pCtx, pRepo,
										 ((prbTreenodeCache_Shared)
										  ? prbTreenodeCache_Shared
										  : prbTreenodeCache_Allocated),
										 chDomain_0, chDomain_1,
										 pszLabel_0, pszLabel_1,
										 pszWasLabel_0, pszWasLabel_1,
										 &pST)  );
	SG_ERR_CHECK(  sg_vv2__status__compare_cset_vs_cset(pCtx, pST, pszHidCSet_0, pszHidCSet_1)  );
	SG_ERR_CHECK(  sg_vv2__status__summarize(pCtx, pST, &pvaStatus)  );
	if (!bNoSort)
		SG_ERR_CHECK(  SG_wc__status__sort_by_repopath(pCtx, pvaStatus)  );

	if (ppvhLegend)
		SG_ERR_CHECK(  sg_vv2__status__make_legend(pCtx, &pvhLegend,
												   pszHidCSet_0, pszHidCSet_1,
												   pszLabel_0, pszLabel_1)  );

#if TRACE_VV2_STATUS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "vv2status: computed cset-vs-cset result:\n")  );
	SG_ERR_IGNORE(  SG_varray_debug__dump_varray_of_vhashes_to_console(pCtx, pvaStatus, "")  );
#endif

	SG_RETURN_AND_NULL( pvaStatus, ppvaStatus );
	SG_RETURN_AND_NULL( pvhLegend, ppvhLegend );

fail:
	SG_VV2__STATUS__NULLFREE(pCtx, pST);
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, prbTreenodeCache_Allocated,((SG_free_callback *)SG_treenode__free));
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_VHASH_NULLFREE(pCtx, pvhLegend);
}

