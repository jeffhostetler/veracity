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

struct _vv2_mstatus_data
{
	SG_repo * pRepo;				// we do not own this

	SG_dagnode * pDagnode_M;		// we own this
	SG_daglca * pDagLca;			// we own this
	SG_rbtree * prbTreenodeCache;	// map[hid ==> pTreenode]   we own this

	SG_vector * pvecPairwiseStatus;	// vec[pvaStatus]  we own this and the contents
	SG_rbtree * prbItems;			// map[gid ==> _item]  we own this and the contents

	SG_varray * pvaStatusResult;	// we own this
	SG_vhash * pvhLegend;			// we own this
};

//////////////////////////////////////////////////////////////////
// The first 6 slots in pvecPairwiseStatus are known/pre-assigned.
// We reserve the right to allocate additional pairs to handle any
// SPCAs that we may need to load, but they won't have fixed locations;
// they would just be appended to the end of the vector as we see them.

#define NDX_VEC__A_B	0
#define NDX_VEC__A_C	1
#define NDX_VEC__B_C	2
#define NDX_VEC__B_M	3
#define NDX_VEC__C_M	4
#define NDX_VEC__A_M	5
#define COUNT_NDX_KNOWN 6

//////////////////////////////////////////////////////////////////

static void _vv2__mstatus__build_legend(SG_context * pCtx,
										struct _vv2_mstatus_data * pData,
										const char * pszHid_M)
{
	const char ** paszHidParents;
	SG_uint32 cParents;
	const char * pszHid_A;
	SG_rbtree * prbLeaves = NULL;
	SG_daglca_node_type nodeType;

	SG_ERR_CHECK(  SG_dagnode__get_parents__ref(pCtx, pData->pDagnode_M, &cParents, &paszHidParents)  );
	SG_ASSERT(  (cParents == 2)  );

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &prbLeaves)  );
	SG_ERR_CHECK(  SG_rbtree__add(pCtx, prbLeaves, paszHidParents[0])  );
	SG_ERR_CHECK(  SG_rbtree__add(pCtx, prbLeaves, paszHidParents[1])  );
	SG_ERR_CHECK(  SG_repo__get_dag_lca(pCtx, pData->pRepo, SG_DAGNUM__VERSION_CONTROL, prbLeaves,
										&pData->pDagLca)  );

	SG_ERR_CHECK(  SG_daglca__iterator__first(pCtx, NULL, pData->pDagLca, SG_FALSE,
											  &pszHid_A, &nodeType, NULL, NULL)  );
	SG_ASSERT_RELEASE_FAIL(  (nodeType == SG_DAGLCA_NODE_TYPE__LCA)  );

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pData->pvhLegend)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pData->pvhLegend, "A", pszHid_A)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pData->pvhLegend, "B", paszHidParents[0])  );	// aka "L0"
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pData->pvhLegend, "C", paszHidParents[1])  );	// aka "L1"
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pData->pvhLegend, "M", pszHid_M)  );

	// Keep pData->pDagLca around so that eventually we
	// can look at SPCAs when we need to.

#if TRACE_VV2_STATUS
	SG_ERR_IGNORE(  SG_vhash_debug__dump_to_console__named(pCtx, pData->pvhLegend, "MSTATUS Legend")  );
#endif

fail:
	SG_RBTREE_NULLFREE(pCtx, prbLeaves);
}

//////////////////////////////////////////////////////////////////

#define BIT_A 0x8
#define BIT_B 0x4
#define BIT_C 0x2
#define BIT_M 0x1

struct _item
{
	SG_vhash * pvhResult;				// owned by pData->pvaStatusResult
	SG_varray * pvaResultHeadings;		// contained with this.pvhResult

	SG_wc_status_flags asf[COUNT_NDX_KNOWN];
	SG_wc_status_flags sfR;

	SG_uint32 existBits;			// see BIT_*
	SG_uint32 nrBits;				// count of significant statusFlags bits (so we can set __A__MULTIPLE)
};

static void _item__free(SG_context * pCtx, struct _item * pItem)
{
	if (!pItem)
		return;

	pItem->pvhResult = NULL;
	pItem->pvaResultHeadings = NULL;

	SG_NULLFREE(pCtx, pItem);
}

static void _item__alloc(SG_context * pCtx,
						 struct _vv2_mstatus_data * pData,
						 const char * pszGid,
						 SG_wc_status_flags statusFlags,	// for __T__ bits onlly
						 struct _item ** ppItem)
{
	struct _item * pItemAllocated = NULL;
	struct _item * pItem;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pItemAllocated)  );
	SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pData->prbItems, pszGid, pItemAllocated)  );
	pItem = pItemAllocated;
	pItemAllocated = NULL;

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pData->pvaStatusResult, &pItem->pvhResult)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pItem->pvhResult, "gid", pszGid)  );
	SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx,  pItem->pvhResult, "headings", &pItem->pvaResultHeadings)  );

	pItem->sfR = (statusFlags & SG_WC_STATUS_FLAGS__T__MASK);

	*ppItem = pItem;
	return;

fail:
	_item__free(pCtx, pItemAllocated);
}

//////////////////////////////////////////////////////////////////

/**
 * Get the "path" field from the requested sub-section
 * and make it the official "path" of the item.
 *
 */
static void _item__set_path_from(SG_context * pCtx,
								 struct _item * pItem,
								 const char * pszLabel)
{
	/*const*/ SG_vhash * pvhSub;
	const char * pszPath;

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pItem->pvhResult, pszLabel, &pvhSub)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSub, "path", &pszPath)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pItem->pvhResult, "path", pszPath)  );

fail:
	return;
}

static void _item__set_sf_bit(SG_context * pCtx,
							  struct _item * pItem,
							  SG_wc_status_flags sfBit,
							  const char * pszHeading)
{
	if ((pItem->sfR & sfBit) != sfBit)
	{
		pItem->sfR |= sfBit;
		pItem->nrBits++;
	}

	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx,
												 pItem->pvaResultHeadings,
												 pszHeading)  );
#if TRACE_VV2_STATUS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "MSTATUS:Heading: %s\n",
							   pszHeading)  );
#endif	

fail:
	return;
}

static void _item__insert_prop(SG_context * pCtx,
							   struct _item * pItem)
{
	SG_vhash * pvhProp = NULL;

	if (pItem->nrBits > 1)
		pItem->sfR |= SG_WC_STATUS_FLAGS__A__MULTIPLE_CHANGE;

	SG_ERR_CHECK(  SG_wc__status__flags_to_properties(pCtx, pItem->sfR, &pvhProp)  );
	SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pItem->pvhResult, "status", &pvhProp)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pvhProp);
}

//////////////////////////////////////////////////////////////////

/**
 * If the 'b' is set in the given pairwise status, set the 'b'
 * bit in the final result, increment the bit counter, and give
 * the item the given heading.
 */
#define TestAndSet(sf, b, h)											\
	SG_STATEMENT(														\
		if ((sf) & (b))													\
			SG_ERR_CHECK(  _item__set_sf_bit(pCtx,						\
											 pItem,						\
											 ((sf) & (b)),				\
											 (h))  );					\
		)																\

//////////////////////////////////////////////////////////////////

/**
 * Inspect (B,C,M) (in the absense of A or a SPCA)
 * and see if the requested bit should be set in the
 * final result.  And QUALIFY the change.  That is, did
 * M make an original change or did M inherit the change
 * from B or C.
 *
 */
static void TestAndSet_BCM(SG_context * pCtx,
						   struct _item * pItem,
						   SG_wc_status_flags sfBit,
						   const char * pszHeadingPrefix)
{
	SG_wc_status_flags sf__b_c = (pItem->asf[NDX_VEC__B_C] & sfBit);
	SG_wc_status_flags sf__b_m = (pItem->asf[NDX_VEC__B_M] & sfBit);
	SG_wc_status_flags sf__c_m = (pItem->asf[NDX_VEC__C_M] & sfBit);
	char bufHeader[100];

	// TODO 2012/02/27 Currently, we don't have any code in here to
	// TODO            lookup a SPCA (and/or the best SPCA) to get
	// TODO            alternate (A-prime) values for the item, so
	// TODO            we can't be as accurate as we could be when
	// TODO            reporting the various __S__/__C__ bits relative
	// TODO            to M.
	// TODO
	// TODO            So for now, we just report on {B,C}-vs-M.

	if (sf__b_c)		// B.foo != C.foo
	{
		if (sf__b_m && sf__c_m)		// B.foo != M.foo *and* C.foo != M.foo
		{
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(M!=B,M!=C,B!=C)")  );
			SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, (sf__b_m|sf__c_m), bufHeader)  );
		}
		else if (sf__b_m)			// B.foo != M.foo *and* C.foo == M.foo
		{
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(M!=B,M==C)")  ); // B!=C is implicit
			SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, sf__b_m, bufHeader)  );
		}
		else if (sf__c_m)			// B.foo == M.foo *and* C.foo != M.foo
		{
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(M==B,M!=C)")  ); // B!=C is implicit
			SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, sf__c_m, bufHeader)  );
		}
		else						// B.foo == M.foo *and* C.foo == M.foo
		{
			// Not possible, since B.foo != C.foo
		}
	}
	else				// B.foo == C.foo
	{
		SG_ASSERT(  ((sf__b_m != 0) == (sf__c_m != 0))  );

		if (sf__b_m)				// [BC].foo != M.foo
		{
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(M!=B,M!=C,B==C)")  );
			SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, sf__b_m, bufHeader)  );
		}
		else			// B.foo == C.foo == M.foo
		{
			// Nothing to report here.
		}
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Inspect (A,x,M) (where x is either B or C) and see if
 * the requested bit should be set in the final result.
 * And QUALIFY the change.  That is, did M make an
 * original change or did M inherit the change from x.
 *
 */
static void TestAndSet_AxM(SG_context * pCtx,
						   struct _item * pItem,
						   SG_uint32 ndx_vec__A_x,
						   SG_uint32 ndx_vec__x_M,
						   const char * pszLabel_x,
						   SG_wc_status_flags sfBit,
						   const char * pszHeadingPrefix)
{
	SG_wc_status_flags sf__a_m = (pItem->asf[NDX_VEC__A_M] & sfBit);
	SG_wc_status_flags sf__a_x = (pItem->asf[ndx_vec__A_x] & sfBit);
	SG_wc_status_flags sf__x_m = (pItem->asf[ndx_vec__x_M] & sfBit);
	char bufHeader[100];

	if (sf__a_m)				// A.foo != M.foo
	{
		if (sf__a_x && sf__x_m)		// A.foo != x.foo *and* x.foo != M.foo
		{
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s (M!=A,%s!=A,M!=%s)",
									  pszHeadingPrefix, pszLabel_x, pszLabel_x)  );
			SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, (sf__a_x|sf__x_m), bufHeader)  );
		}
		else if (sf__a_x)			// A.foo != x.foo *and* x.foo == M.foo
		{
			// TODO 2012/02/28 The value of M.foo was inherited from x.foo, so
			// TODO            it didn't *really* change in the merge; rather,
			// TODO            it just got carried forward.  Yes, it did change
			// TODO            relative to A, but I'm not sure if the viewer
			// TODO            cares about that.
			// TODO
			// TODO            Do we want to report this as a stock 'foo' or
			// TODO            do we want to use a special __M__ bit like we
			// TODO            did for "Existence"?
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s (%s!=A,M==%s)", // M!=A implicit
									  pszHeadingPrefix, pszLabel_x, pszLabel_x)  );
			SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, sf__a_x, bufHeader)  );
		}
		else if (sf__x_m)			// A.foo == x.foo *and* x.foo != M.foo
		{
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s (%s==A,M!=%s)", // M!=A implicit
									  pszHeadingPrefix, pszLabel_x, pszLabel_x)  );
			SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, sf__x_m, bufHeader)  );
		}
		else						// A.foo == x.foo == M.foo
		{
			// Not possible, since A.foo != M.foo
		}
	}
	else				// A.foo == M.foo (no net change)
	{
		SG_ASSERT(  ((sf__a_x != 0) == (sf__x_m != 0))  );

		if (sf__a_x)
		{
			// If we get here, M undid the change that x made
			// such that there is no net change.
			//
			// TODO 2012/02/28 Do we want to report it?

			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s (M!=%s,M==A)", // x!=A implicit
									  pszHeadingPrefix, pszLabel_x)  );
			SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, (sf__a_x|sf__x_m), bufHeader)  );
		}
		else						// A.foo == x.foo == M.foo
		{
			// Nothing to report here.
		}
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Inspect (A,B,C,M) 
 * and see if the requested bit should be set in the
 * final result.  And QUALIFY the change.  That is, did
 * M make an original change or did M inherit the change
 * from B or C.
 *
 */
static void TestAndSet_ABCM(SG_context * pCtx,
						   struct _item * pItem,
						   SG_wc_status_flags sfBit,
						   const char * pszHeadingPrefix)
{
	SG_wc_status_flags sf__a_m = (pItem->asf[NDX_VEC__A_M] & sfBit);
	SG_wc_status_flags sf__b_m = (pItem->asf[NDX_VEC__B_M] & sfBit);
	SG_wc_status_flags sf__c_m = (pItem->asf[NDX_VEC__C_M] & sfBit);
	char bufHeader[100];

	if (sf__a_m)				// A.foo != M.foo
	{
		if (sf__b_m && sf__c_m)		// B.foo != M.foo *and* C.foo != M.foo
		{
			// TODO 2012/02/28 Do we want to report on whether B.foo == C.foo also?

			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(M!=A,M!=B,M!=C)")  );
			SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, (sf__a_m|sf__b_m|sf__c_m), bufHeader)  );
		}
		else if (sf__b_m)			// B.foo != M.foo *and* C.foo == M.foo
		{
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(M!=A,M!=B,M==C)")  );
			SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, (sf__a_m|sf__b_m), bufHeader)  );
		}
		else if (sf__c_m)			// B.foo == M.foo *and* C.foo != M.foo
		{
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(M!=A,M==B,M!=C)")  );
			SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, (sf__a_m|sf__c_m), bufHeader)  );
		}
		else						// B.foo == M.foo *and* C.foo == M.foo
		{
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(M!=A,M==B,M==C)")  );
			SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, sf__a_m, bufHeader)  );
		}
	}
	else				// A.foo == M.foo (no net change)
	{
		if (sf__b_m && sf__c_m)		// B.foo != M.foo *and* C.foo != M.foo
		{
			// If we get here, M undid the changes that B and C
			// made such that there is no net change.
			//
			// TODO 2012/02/28 Do we want to report it?
			//
			// TODO 2012/02/28 Do we want to report on whether B.foo == C.foo also?

			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(M==A,M!=B,M!=C)")  );
			SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, (sf__b_m|sf__c_m), bufHeader)  );
		}
		else if (sf__b_m)			// B.foo != M.foo *and* C.foo == M.foo (implies C.foo == A.foo)
		{
			// If we get here, M ignored the changes in B and
			// carried forward the unchanged C version relative
			// to A.
			//
			// TODO 2012/02/28 Do we want to report it?

			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(M==A,M!=B,M==C)")  );
			SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, sf__b_m, bufHeader)  );
		}
		else if (sf__c_m)			// B.foo == M.foo *and* C.foo != M.foo (implies B.foo == A.foo)
		{
			// If we get here, M ignored the changes in C and
			// carried forward the unchanged B version relative
			// to A.
			//
			// TODO 2012/02/28 Do we want to report it?

			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(M==A,M==B,M!=C)")  );
			SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, sf__c_m, bufHeader)  );
		}
		else			// A.foo == B.foo == C.foo == M.foo
		{
			// Nothing to report here.
		}
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

typedef void (_fn_case)(SG_context * pCtx,
						struct _item * pItem);

		
#define DCL(_fn_)											\
	static _fn_case _fn_;									\
	static void _fn_(SG_context * pCtx,						\
					 struct _item * pItem)

DCL(_fn_case__zero)		// The entry did not exist in any of the 4 CSETs.
{
	SG_ERR_THROW2_RETURN(  SG_ERR_NOTIMPLEMENTED,
						   (pCtx, "MSTATUS case 0")  );
	SG_UNUSED( pItem );
}

DCL(_fn_case_____M)		// The entry only exists in M.
{
	SG_ERR_CHECK(  _item__set_path_from(pCtx, pItem, "M")  );

	SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, SG_WC_STATUS_FLAGS__S__ADDED, "Added (M)")  );

	// No __S__/__C__ bits possible because no reference version.

fail:
	return;
}

DCL(_fn_case____C_)		// The entry only exists in C.
{
	// So it was added in C and deleted from M.
	// Set path from C because that is the last place we saw it.
	SG_ERR_CHECK(  _item__set_path_from(pCtx, pItem, "C")  );

	SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, SG_WC_STATUS_FLAGS__S__ADDED,   "Added (C)")  );
	SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, SG_WC_STATUS_FLAGS__S__DELETED, "Removed (M)")  );

	// No __S__/__C__ bits possible because no final version.

fail:
	return;
}

DCL(_fn_case____CM)		// The entry exists in both C and M.
{
	// So it was added in C.
	// It may or may not have changed in M.

	SG_wc_status_flags sf__c_m = pItem->asf[NDX_VEC__C_M];

	SG_ERR_CHECK(  _item__set_path_from(pCtx, pItem, "M")  );

	SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, SG_WC_STATUS_FLAGS__S__ADDED,   "Added (C)")  );

	// set __S__/__C__ bits between C and M.
	// Note that __S__MERGE_CREATED is not possible because we
	// computed a simple 2-way historical status.

	TestAndSet( sf__c_m, SG_WC_STATUS_FLAGS__S__RENAMED, "Renamed (M)" );
	TestAndSet( sf__c_m, SG_WC_STATUS_FLAGS__S__MOVED,   "Moved (M)" );
	TestAndSet( sf__c_m, SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED, "Attributes (M)" );
	if ((pItem->sfR & SG_WC_STATUS_FLAGS__T__DIRECTORY) == 0)
		TestAndSet( sf__c_m, SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED, "Modified (M)" );

fail:
	return;
}

DCL(_fn_case___B__)		// The entry only exists in B.
{
	// So it was added in B and deleted from M.

	SG_ERR_CHECK(  _item__set_path_from(pCtx, pItem, "B")  );

	SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, SG_WC_STATUS_FLAGS__S__ADDED,   "Added (B)")  );
	SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, SG_WC_STATUS_FLAGS__S__DELETED, "Removed (M)")  );

	// No __S__/__C__ bits possible because no final version.

fail:
	return;
}

DCL(_fn_case___B_M)		// The entry exists in both B and M.
{
	// So it was added in B.
	// It may or may not have changed in M.

	SG_wc_status_flags sf__b_m = pItem->asf[NDX_VEC__B_M];

	SG_ERR_CHECK(  _item__set_path_from(pCtx, pItem, "M")  );

	SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, SG_WC_STATUS_FLAGS__S__ADDED,   "Added (B)")  );

	// set __S__/__C__ bits between B and M.
	// 
	// Note that __S__MERGE_CREATED is not possible because we
	// computed a simple 2-way historical status.

	TestAndSet( sf__b_m, SG_WC_STATUS_FLAGS__S__RENAMED, "Renamed (M)" );
	TestAndSet( sf__b_m, SG_WC_STATUS_FLAGS__S__MOVED,   "Moved (M)" );
	TestAndSet( sf__b_m, SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED, "Attributes (M)" );
	if ((pItem->sfR & SG_WC_STATUS_FLAGS__T__DIRECTORY) == 0)
		TestAndSet( sf__b_m, SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED, "Modified (M)" );

fail:
	return;
}

DCL(_fn_case___BC_)		// The entry exists in B and C, but not in A nor M.
{
	// Therefore it was added in an SPCA between A and {B,C}.
	// And it was deleted in M.

	// "path" could be set using either B or C.  Just pick one.
	SG_ERR_CHECK(  _item__set_path_from(pCtx, pItem, "B")  );

	// Note: reporting it as "ADDED in B *and* C" is a bit of a
	//       lie.  Technically, it first existed in some cset
	//       between A and B which is also between A and C and
	//       because we haven't looked at any of the SPCAs
	//       we are first seeing the item in both B and C.
	//
	//       But from the point of view of A, the item has
	//       been added.
	SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, SG_WC_STATUS_FLAGS__S__ADDED,   "Added (B,C)")  );
	SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, SG_WC_STATUS_FLAGS__S__DELETED, "Removed (M)")  );

	// We DO NOT report __S__/__C__ changes between B and C.

	// No __S__/__C__ bits possible because no final version.

fail:
	return;
}

DCL(_fn_case___BCM)		// The entry exists in B, C and M, but not in A.
{
	// Therefore it was added in an SPCA between A and {B,C}.
	// It may or may not have changed in M.

	// "path" could be set using either B or C.  Just pick one.
	SG_ERR_CHECK(  _item__set_path_from(pCtx, pItem, "B")  );

	SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, SG_WC_STATUS_FLAGS__S__ADDED,   "Added (B,C)")  );

	// set __S__/__C__ bits between {B,C} and M.

	SG_ERR_CHECK(  TestAndSet_BCM(pCtx, pItem, SG_WC_STATUS_FLAGS__S__RENAMED, "Renamed")  );
	SG_ERR_CHECK(  TestAndSet_BCM(pCtx, pItem, SG_WC_STATUS_FLAGS__S__MOVED,   "Moved")  );
	SG_ERR_CHECK(  TestAndSet_BCM(pCtx, pItem, SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED, "Attributes")  );
	if ((pItem->sfR & SG_WC_STATUS_FLAGS__T__DIRECTORY) == 0)
		SG_ERR_CHECK(  TestAndSet_BCM(pCtx, pItem, SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED, "Modified")  );

fail:
	return;
}

DCL(_fn_case__A___)		// The entry existed in A, but not in B, C, nor M.
{
	// Set path from A because that is the last place we saw it.
	SG_ERR_CHECK(  _item__set_path_from(pCtx, pItem, "A")  );

	SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, SG_WC_STATUS_FLAGS__S__DELETED, "Removed (B,C,M)")  );

	// No __C__ bits possible because no final version.

fail:
	return;
}

DCL(_fn_case__A__M)		// The entry existed in A and M, but not B nor C.
{
	// This should not be possible.
	SG_ERR_THROW2_RETURN(  SG_ERR_NOTIMPLEMENTED,
						   (pCtx, "MSTATUS case 9")  );
	SG_UNUSED( pItem );
}

DCL(_fn_case__A_C_)		// The entry existed in A and C, but not B nor M.
{
	// set path from C because that is the last place we saw it.
	SG_ERR_CHECK(  _item__set_path_from(pCtx, pItem, "C")  );

	SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, SG_WC_STATUS_FLAGS__S__DELETED, "Removed (B,M)")  );

	// No __C__ bits possible because no final version.

fail:
	return;
}

DCL(_fn_case__A_CM)		// The entry existed in A, C and M, but not in B.
{
	SG_ERR_CHECK(  _item__set_path_from(pCtx, pItem, "M")  );

	// We *ONLY* set an 'existence' bit when there is a delete/non-delete
	// between B and C and we can't report it as ADDED or REMOVED because
	// A and M are in agreement.
	// 
	// This is to indicate that somebody had to deal with whether or not
	// the item should exist (whether manually or automatically).
	SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, SG_WC_STATUS_FLAGS__M__AbCM, "Existence (A,!B,C,M)")  );

	// Set __S__/__C__ bits between A,C,M.

	SG_ERR_CHECK(  TestAndSet_AxM(pCtx, pItem,
								  NDX_VEC__A_C, NDX_VEC__C_M, "C",
								  SG_WC_STATUS_FLAGS__S__RENAMED, "Renamed")  );
	SG_ERR_CHECK(  TestAndSet_AxM(pCtx, pItem,
								  NDX_VEC__A_C, NDX_VEC__C_M, "C",
								  SG_WC_STATUS_FLAGS__S__MOVED,   "Moved")  );
	SG_ERR_CHECK(  TestAndSet_AxM(pCtx, pItem,
								  NDX_VEC__A_C, NDX_VEC__C_M, "C",
								  SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED, "Attributes")  );
	if ((pItem->sfR & SG_WC_STATUS_FLAGS__T__DIRECTORY) == 0)
		SG_ERR_CHECK(  TestAndSet_AxM(pCtx, pItem, NDX_VEC__A_C, NDX_VEC__C_M, "C",
									  SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED, "Modified")  );

fail:
	return;
}

DCL(_fn_case__AB__)		// The entry existed in A and B, but not C nor M.
{
	// set path from B because that is the last place we saw it.
	SG_ERR_CHECK(  _item__set_path_from(pCtx, pItem, "B")  );

	// We say that the item was removed from C and from M.
	// This implies that it was present in A and B.
	SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, SG_WC_STATUS_FLAGS__S__DELETED, "Removed (C,M)")  );

	// No __C__ bits possible because no final version.

fail:
	return;
}

DCL(_fn_case__AB_M)		// The entry existed in A, B and M, but not in C.
{
	SG_ERR_CHECK(  _item__set_path_from(pCtx, pItem, "M")  );

	SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, SG_WC_STATUS_FLAGS__M__ABcM, "Existence (A,B,!C,M)")  );

	// Set __S__/__C__ bits between A,B,M.

	SG_ERR_CHECK(  TestAndSet_AxM(pCtx, pItem,
								  NDX_VEC__A_B, NDX_VEC__B_M, "B",
								  SG_WC_STATUS_FLAGS__S__RENAMED, "Renamed")  );
	SG_ERR_CHECK(  TestAndSet_AxM(pCtx, pItem,
								  NDX_VEC__A_B, NDX_VEC__B_M, "B",
								  SG_WC_STATUS_FLAGS__S__MOVED,   "Moved")  );
	SG_ERR_CHECK(  TestAndSet_AxM(pCtx, pItem,
								  NDX_VEC__A_B, NDX_VEC__B_M, "B",
								  SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED, "Attributes")  );
	if ((pItem->sfR & SG_WC_STATUS_FLAGS__T__DIRECTORY) == 0)
		SG_ERR_CHECK(  TestAndSet_AxM(pCtx, pItem, NDX_VEC__A_B, NDX_VEC__B_M, "B",
									  SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED, "Modified")  );

fail:
	return;
}

DCL(_fn_case__ABC_)		// The entry existed in A, B, and C, but not in M.
{
	// either B or C's path will do here.
	SG_ERR_CHECK(  _item__set_path_from(pCtx, pItem, "B")  );

	SG_ERR_CHECK(  _item__set_sf_bit(pCtx, pItem, SG_WC_STATUS_FLAGS__S__DELETED, "Removed (M)")  );

	// No __C__ bits possible because no final version.

fail:
	return;
}

DCL(_fn_case__ABCM)		// The entry existed in A, B, C, and M.
{
	SG_ERR_CHECK(  _item__set_path_from(pCtx, pItem, "M")  );

	// No Added/Removed/Existence bits need to be set.

	// Set __S__/__C__ bits between A,B,C,M.

	SG_ERR_CHECK(  TestAndSet_ABCM(pCtx, pItem, SG_WC_STATUS_FLAGS__S__RENAMED, "Renamed")  );
	SG_ERR_CHECK(  TestAndSet_ABCM(pCtx, pItem, SG_WC_STATUS_FLAGS__S__MOVED,   "Moved")  );
	SG_ERR_CHECK(  TestAndSet_ABCM(pCtx, pItem, SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED, "Attributes")  );
	if ((pItem->sfR & SG_WC_STATUS_FLAGS__T__DIRECTORY) == 0)
		SG_ERR_CHECK(  TestAndSet_ABCM(pCtx, pItem, SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED, "Modified")  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

static _fn_case * saCaseTable[] = { _fn_case__zero,
									 _fn_case_____M,
									 _fn_case____C_,
									 _fn_case____CM,
									 _fn_case___B__,
									 _fn_case___B_M,
									 _fn_case___BC_,
									 _fn_case___BCM,
									 _fn_case__A___,
									 _fn_case__A__M,
									 _fn_case__A_C_,
									 _fn_case__A_CM,
									 _fn_case__AB__,
									 _fn_case__AB_M,
									 _fn_case__ABC_,
									 _fn_case__ABCM,
};

//////////////////////////////////////////////////////////////////

static SG_rbtree_foreach_callback _item__examine_item__cb;

static void _item__examine_item__cb(SG_context * pCtx,
									const char * pszGid,
									void * pVoidAssoc,
									void * pVoidData)
{
	struct _vv2_mstatus_data * pData = (struct _vv2_mstatus_data *)pVoidData;
	struct _item * pItem = (struct _item *)pVoidAssoc;

	SG_UNUSED( pData );
	SG_UNUSED( pszGid );

	// Complete the construction of the result vhash for this item.
	// We've already partially build the vhash and stuffed in the
	// result varray.
	//
	// pItem->pvhResult already contains:
	// 
	// <status_k> ::= { "gid"      : "<gid>",
	//                  "headings" : [ ],                  -- empty at first
	//                  "A"        : <sub_section_A>,      -- if entry existed in A
	//                  "B"        : <sub_section_B>,      -- if entry existed in B
	//                  "C"        : <sub_section_C>,      -- if entry existed in C
	//                  "M"        : <sub_section_M>,      -- if entry existed in M
	//                };
	//
	// We need to be sure to add:
	//
	//            "path"      : "<path>",
	//            "status"    : <properties>,       -- vhash with flag bits expanded into JS-friendly fields

	SG_ERR_CHECK(  (*(saCaseTable[pItem->existBits]))(pCtx, pItem)  );
	SG_ERR_CHECK(  _item__insert_prop(pCtx, pItem)  );
	
#if TRACE_VV2_STATUS
	SG_ERR_IGNORE(  SG_vhash_debug__dump_to_console__named(pCtx, pItem->pvhResult, "MSTATUS Examine")  );
#endif

fail:
	return;
}

//////////////////////////////////////////////////////////////////

static void _vv2__mstatus__do_pairwise(SG_context * pCtx,
									   struct _vv2_mstatus_data * pData,
									   char chDomain_l,
									   char chDomain_r,
									   const char * pszLabel_l,
									   const char * pszLabel_r,
									   const char * pszWasLabel_l,
									   const char * pszWasLabel_r,
									   SG_uint32 bit_l,
									   SG_uint32 bit_r,
									   SG_uint32 ndx_vec__requested)
{
	const char * pszHid_l;
	const char * pszHid_r;
	SG_varray * pvaStatusAllocated = NULL;
	SG_varray * pvaStatus;
	SG_uint32 k, nrItems;
	SG_uint32 ndx_vec__obtained;

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pData->pvhLegend, pszLabel_l, &pszHid_l)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pData->pvhLegend, pszLabel_r, &pszHid_r)  );

	// Get pairwise historical status beteen these 2 CSETs.
	// Note that this is sparse and only lists the items that
	// have changed between the 2 CSETs.
	// 
	// This returns a VARRAY of VHASH that looks like this:
	// See also sg_wc__status__append.c and sg_vv2__status__summarize.c
	// for info on this format.
	//
	// status ::= [ <item_k>, ... ];
	// 
	// <item_k> ::= { "status"    : <properties>,       -- vhash with flag bits expanded into JS-friendly fields
	//                "gid"       : "<gid>",
	//                "path"      : "<path>",
	//                "headings"  : [ "<heading_k>" ],  -- list of headers for classic_formatter
	//                "<label_l>" : <sub_section_l>,    -- for left cset, if present
	//                "<label_r>" : <sub_section_r>,    -- for right cset, if present
	//              };
	// 
	// <sub_section_x> ::= { "path"           : "<path_j>",
	//                       "name"           : "<name_j>",
	//                       "gid_parent"     : "<gid_parent_j>",
	//                       "hid"            : "<hid_j>",           -- when not a directory
	//                       "attributes"     : <attrbits_j>,
	//                       "was_label"      : "<was_label_j>"
	//                     };

	SG_ERR_CHECK(  SG_vv2__status__repo(pCtx, pData->pRepo,
										pData->prbTreenodeCache,
										pszHid_l,   pszHid_r,
										chDomain_l, chDomain_r,
										pszLabel_l, pszLabel_r,
										pszWasLabel_l, pszWasLabel_r,
										SG_TRUE,				// DO NOT bother sorting the individual pairwise diffs.
										&pvaStatusAllocated, NULL)  );
	SG_ERR_CHECK(  SG_vector__append(pCtx, pData->pvecPairwiseStatus, pvaStatusAllocated, &ndx_vec__obtained)  );
	pvaStatus = pvaStatusAllocated;
	pvaStatusAllocated = NULL;

	if (ndx_vec__requested < COUNT_NDX_KNOWN)
	{
		SG_ASSERT(  (ndx_vec__obtained == ndx_vec__requested)  );
	}

	// For each dirty item, begin synthesizing a new <status_k>
	// for it in the result that reflects the complete {A,B,C,M}
	// state.  Since we don't have enough information to build it
	// completely, we use a 'struct _item *' to accumulate the
	// parts.  Here we stuff in the sub-sections for the 2 that
	// we know about (if they are not already present).

	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaStatus, &nrItems)  );
	for (k=0; k<nrItems; k++)
	{
		/*const*/ SG_vhash * pvhItem_k;
		const char * pszGid_k;
		struct _item * pItem_k;
		/*const*/ SG_vhash * pvhSubSection_l_k;
		/*const*/ SG_vhash * pvhSubSection_r_k;
		/*const*/ SG_vhash * pvhStatus_k;
		SG_int64 i64;
		SG_wc_status_flags sf;
		SG_bool bFound_k;
		SG_bool bHas_l_k;
		SG_bool bHas_r_k;

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaStatus, k, &pvhItem_k)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem_k, "gid", &pszGid_k)  );
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, pData->prbItems, pszGid_k, &bFound_k, (void **)&pItem_k)  );

		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem_k, "status", &pvhStatus_k)  );
		SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhStatus_k, "flags", &i64)  );
		sf = ((SG_wc_status_flags)i64);

		if (!bFound_k)
			SG_ERR_CHECK(  _item__alloc(pCtx, pData, pszGid_k, sf, &pItem_k)  );

		if (ndx_vec__requested < COUNT_NDX_KNOWN)
		{
			// for pairs in the fixed set, we preload the statusFlags
			// into the item.asf[].
			pItem_k->asf[ndx_vec__requested] = sf;
		}

		SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvhItem_k, pszLabel_l, &pvhSubSection_l_k)  );
		if (pvhSubSection_l_k)
		{
			pItem_k->existBits |= bit_l;
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pItem_k->pvhResult, pszLabel_l, &bHas_l_k)  );
			if (!bHas_l_k)
				SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pItem_k->pvhResult, pszLabel_l, pvhSubSection_l_k)  );
		}
		
		SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pvhItem_k, pszLabel_r, &pvhSubSection_r_k)  );
		if (pvhSubSection_r_k)
		{
			pItem_k->existBits |= bit_r;
			SG_ERR_CHECK(  SG_vhash__has(pCtx, pItem_k->pvhResult, pszLabel_r, &bHas_r_k)  );
			if (!bHas_r_k)
				SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pItem_k->pvhResult, pszLabel_r, pvhSubSection_r_k)  );
		}
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaStatusAllocated);
}

//////////////////////////////////////////////////////////////////

static void _vv2__mstatus__main2(SG_context * pCtx,
								 struct _vv2_mstatus_data * pData,
								 const char * pszHid_M)
{
	const char * pszWasLabel_A = "Ancestor (A)";
	const char * pszWasLabel_B = "Parent 0 (B)";
	const char * pszWasLabel_C = "Parent 1 (C)";
	const char * pszWasLabel_M = "Merged (M)";

	SG_ERR_CHECK(  _vv2__mstatus__build_legend(pCtx, pData, pszHid_M)  );

	// We need to do several cset-vs-cset pair-wise diffs, so
	// cache the treenodes across all of them.  Each individual
	// diff automatically short-circuits and skips equal parts
	// of the tree, but this helps even more.
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pData->prbTreenodeCache)  );

	// we want to get the per-cset sub-sections for each entry
	// and bind them all together so that we know everything
	// about the entry.
	// 
	// since each pair-wise status is sparse (only contains rows
	// for items that changed between the 2 csets), we need to 
	// diff each pair so that we are sure to get them all.
	//
	// for example, if an entry only changed in M (by the user
	// after the auto-merge and before the commit), then we won't
	// get any data for it in A-vs-B, A-vs-C, or B-vs-C; we will
	// get columns B and M in B-vs-M and columns C and M in C-vs-M;
	// we can disregard the duplicate M column, but we won't get
	// the A column until we do A-vs-M.  Granted, we can infer
	// most of the fields for A (from B and/or C in this example),
	// but we can't necessarily infer the A-specific extended-prefix
	// repo-path for the item.
	//
	// So we do multiple pair-wise diff and accumulate.
	//
	// We DO NOT sort the individual pair-wise diffs.  A later step
	// will aggregate all of these in to a single MSTATUS result that
	// should be sorted.

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pData->pvaStatusResult)  );
	SG_ERR_CHECK(  SG_VECTOR__ALLOC(pCtx, &pData->pvecPairwiseStatus, 6)  );
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pData->prbItems)  );

	SG_ERR_CHECK(  _vv2__mstatus__do_pairwise(pCtx, pData,
											  SG_WC__REPO_PATH_DOMAIN__A, SG_WC__REPO_PATH_DOMAIN__B,
											  SG_WC__STATUS_SUBSECTION__A, SG_WC__STATUS_SUBSECTION__B,
											  pszWasLabel_A, pszWasLabel_B,
											  BIT_A, BIT_B,
											  NDX_VEC__A_B)  );
	SG_ERR_CHECK(  _vv2__mstatus__do_pairwise(pCtx, pData,
											  SG_WC__REPO_PATH_DOMAIN__A, SG_WC__REPO_PATH_DOMAIN__C,
											  SG_WC__STATUS_SUBSECTION__A, SG_WC__STATUS_SUBSECTION__C,
											  pszWasLabel_A, pszWasLabel_C,
											  BIT_A, BIT_C,
											  NDX_VEC__A_C)  );
	SG_ERR_CHECK(  _vv2__mstatus__do_pairwise(pCtx, pData,
											  SG_WC__REPO_PATH_DOMAIN__B, SG_WC__REPO_PATH_DOMAIN__C,
											  SG_WC__STATUS_SUBSECTION__B, SG_WC__STATUS_SUBSECTION__C,
											  pszWasLabel_B, pszWasLabel_C,
											  BIT_B, BIT_C,
											  NDX_VEC__B_C)  );

	SG_ERR_CHECK(  _vv2__mstatus__do_pairwise(pCtx, pData,
											  SG_WC__REPO_PATH_DOMAIN__B, SG_WC__REPO_PATH_DOMAIN__M,
											  SG_WC__STATUS_SUBSECTION__B, SG_WC__STATUS_SUBSECTION__M,
											  pszWasLabel_B, pszWasLabel_M,
											  BIT_B, BIT_M,
											  NDX_VEC__B_M)  );
	SG_ERR_CHECK(  _vv2__mstatus__do_pairwise(pCtx, pData,
											  SG_WC__REPO_PATH_DOMAIN__C, SG_WC__REPO_PATH_DOMAIN__M,
											  SG_WC__STATUS_SUBSECTION__C, SG_WC__STATUS_SUBSECTION__M,
											  pszWasLabel_C, pszWasLabel_M,
											  BIT_C, BIT_M,
											  NDX_VEC__C_M)  );
	SG_ERR_CHECK(  _vv2__mstatus__do_pairwise(pCtx, pData,
											  SG_WC__REPO_PATH_DOMAIN__A, SG_WC__REPO_PATH_DOMAIN__M,
											  SG_WC__STATUS_SUBSECTION__A, SG_WC__STATUS_SUBSECTION__M,
											  pszWasLabel_A, pszWasLabel_M,
											  BIT_A, BIT_M,
											  NDX_VEC__A_M)  );
	
	// Now that we have all possible columns for each row, inspect
	// each row and convert it into a real status item.

	SG_ERR_CHECK(  SG_rbtree__foreach(pCtx, pData->prbItems, _item__examine_item__cb, pData)  );
	
fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Look at the given cset "M".
 *
 * If M has 2 parents, find the csets involved in
 * the merge that created M:
 * 
 *     A
 *    / \.
 *   B   C
 *    \ /
 *     M
 *
 * If M has 1 parent (and fallback is allowed),
 * find it:
 *
 *     A
 *     |
 *     M
 *
 * And populate the LEGEND with them. 
 *
 */
void sg_vv2__mstatus__main(SG_context * pCtx,
						   SG_repo * pRepo,
						   const char * pszHid_M,
						   SG_bool bNoFallback,
						   SG_bool bNoSort,
						   SG_varray ** ppvaStatus,
						   SG_vhash ** ppvhLegend)
{
	struct _vv2_mstatus_data data;
	const char ** paszHidParents;
	SG_uint32 nrParents;

	SG_NULLARGCHECK_RETURN( ppvaStatus );
	// ppvhLegend is optional

	memset(&data, 0, sizeof(data));
	data.pRepo = pRepo;

	SG_ERR_CHECK(  SG_repo__fetch_dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL, pszHid_M, &data.pDagnode_M)  );
	SG_ERR_CHECK(  SG_dagnode__get_parents__ref(pCtx, data.pDagnode_M, &nrParents, &paszHidParents)  );
	if (nrParents == 1)
	{
		const char * pszWasLabel_l = "Ancestor (A)";
		const char * pszWasLabel_r = "Merged (M)";

		if (bNoFallback)
			SG_ERR_THROW( SG_ERR_NOT_IN_A_MERGE );

		// fallback and do regular STATUS (and let it compute the legend).

		SG_ERR_CHECK(  SG_vv2__status__repo(pCtx, pRepo, NULL,
											paszHidParents[0], pszHid_M,
											SG_WC__REPO_PATH_DOMAIN__A, SG_WC__REPO_PATH_DOMAIN__M,
											SG_WC__STATUS_SUBSECTION__A, SG_WC__STATUS_SUBSECTION__M,
											pszWasLabel_l, pszWasLabel_r,
											bNoSort,
											ppvaStatus,
											ppvhLegend)  );
	}
	else	// do actual MSTATUS
	{
		SG_ERR_CHECK(  _vv2__mstatus__main2(pCtx, &data, pszHid_M)  );
		if (!bNoSort)
			SG_ERR_CHECK(  SG_wc__status__sort_by_repopath(pCtx, data.pvaStatusResult)  );

		SG_RETURN_AND_NULL( data.pvaStatusResult, ppvaStatus );
		SG_RETURN_AND_NULL( data.pvhLegend, ppvhLegend );
	}

fail:
	SG_DAGNODE_NULLFREE(pCtx, data.pDagnode_M);
	SG_DAGLCA_NULLFREE(pCtx, data.pDagLca);
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, data.prbItems, (SG_free_callback *)_item__free);
	SG_VECTOR_NULLFREE_WITH_ASSOC(pCtx, data.pvecPairwiseStatus, (SG_free_callback *)SG_varray__free);
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, data.prbTreenodeCache,((SG_free_callback *)SG_treenode__free));
	SG_VARRAY_NULLFREE(pCtx, data.pvaStatusResult);
	SG_VHASH_NULLFREE(pCtx, data.pvhLegend);
}

