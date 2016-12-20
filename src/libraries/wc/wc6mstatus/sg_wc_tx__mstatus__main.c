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

#include "sg_wc__public_typedefs.h"
#include <sg_vv2__public_typedefs.h>
#include "sg_wc__public_prototypes.h"
#include <sg_vv2__public_prototypes.h>
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

#define BIT_A 0x8
#define BIT_B 0x4
#define BIT_C 0x2
#define BIT_M 0x1	// aka WC

struct _folded_item
{
	SG_wc_status_flags sfW;
	SG_wc_status_flags sf__a_b;
	SG_wc_status_flags sf__a_c;
	SG_wc_status_flags sfR;

	SG_uint32 existBits;				// see BIT_* -- mask where item exists.
	SG_uint32 knownBits;				// see BIT_* -- mask where we know value in existBits is final answer.

	SG_uint32 nrBits;					// for setting __A__MULTIPLE_CHANGE

	SG_vhash * pvhItemResult;			// we do not own this
	SG_varray * pvaResultHeadings;		// we do not own this
};

struct _mstatus_data
{
	SG_wc_tx * pWcTx;				// we do not own this
	SG_bool bNoIgnores;
	SG_varray * pvaStatusResult;	// we do not own this

	SG_vhash * pvhCSets;			// we own this
	SG_vhash * pvhLegend;			// we own this

	SG_varray * pvaStatus_B_vs_WC;	// we own these
	SG_varray * pvaStatus_A_vs_B;
	SG_varray * pvaStatus_A_vs_C;

	const char * pszHid_B;			// we do not own these
	const char * pszHid_C;
	const char * pszHid_A;

	const char * pszWasLabel_A;
	const char * pszWasLabel_B;
	const char * pszWasLabel_C;
	const char * pszWasLabel_WC;

	sg_wc_db__cset_row * pCSetRow_C;	// not loaded until needed
	
	SG_rbtree * prbSync;			// map[<gid> ==> <folded_item>]  we own this and the <folded_item>
};

//////////////////////////////////////////////////////////////////

static void _folded_item__free(SG_context * pCtx, struct _folded_item * pFold)
{
	if (!pFold)
		return;

	pFold->pvhItemResult = NULL;
	pFold->pvaResultHeadings = NULL;

	SG_NULLFREE(pCtx, pFold);
}

#define _FOLDED_ITEM__NULLFREE(pCtx, p) _sg_generic_nullfree(pCtx, p, _folded_item__free)

static void _folded_item__alloc(SG_context * pCtx,
								struct _mstatus_data * pData,
								const char * pszGid,
								SG_wc_status_flags statusFlags,	// for __T__ bits onlly
								struct _folded_item **ppFold)	// you do not own this.
{
	struct _folded_item * pFoldAllocated = NULL;
	struct _folded_item * pFold = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pFoldAllocated)  );
	SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pData->prbSync, pszGid, pFoldAllocated)  );
	pFold = pFoldAllocated;
	pFoldAllocated = NULL;

	// seed the result with just the __T__ type.
	pFold->sfR = (statusFlags & SG_WC_STATUS_FLAGS__T__MASK);

	// The final item we build needs to look like this:
	// See also sg_wc__status__append.c and sg_vv2__status__summarize.c for
	// the format of <item> that we need to model.
	// 
	// <item> ::= { "status"   : <properties>,		-- vhash with flag bits expanded into JS-friendly fields
	//              "gid"      : "<gid>",
	//              "path"     : "<path>",
	//              "headings" : [ "<heading_k>" ],	-- list of headers for classic_formatter.
	//              "A"        : <sub-section>,		-- if present in cset A
	//              "B"        : <sub-section>,		-- if present in cset B
	//              "C"        : <sub-section>,		-- if present in cset C
	//              "WC"       : <sub-section>		-- if present in the WD (live)
	//            };
	// 
	// All fields in a <sub-section> refer to the item relative to that state.
	// 
	// <sub-section> ::= { "path"           : "<path_j>",
	//                     "name"           : "<name_j>",
	//                     "gid_parent"     : "<gid_parent_j>",
	//                     "hid"            : "<hid_j>",			-- when not a directory
	//                     "attributes"     : <attrbits_j>
	//                   };
	//

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pData->pvaStatusResult, &pFold->pvhItemResult)  );
	// defer setting item.status (__insert_prop())
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pFold->pvhItemResult, "gid", pszGid)  );
	// defer setting item.path (__insert_path())
	SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pFold->pvhItemResult, "headings", &pFold->pvaResultHeadings)  );
	// defer setting item.A, item.B, item.C, and item.M.

	*ppFold = pFold;

fail:
	_FOLDED_ITEM__NULLFREE(pCtx, pFoldAllocated);
}

#if defined(DEBUG)
#define _FOLDED_ITEM__ALLOC(pCtx,pData,pszGid,sf,ppFold)				\
	SG_STATEMENT(														\
		struct _folded_item * _pNew = NULL;								\
		_folded_item__alloc(pCtx, (pData), (pszGid), (sf), &_pNew);		\
		_sg_mem__set_caller_data(_pNew,__FILE__,__LINE__,"_folded_item"); \
		*(ppFold) = _pNew;												\
		)
#else
#define _FOLDED_ITEM__ALLOC(pCtx,pData,pszGid,sf,ppFold)				\
	_folded_item__alloc(pCtx,(pData),(pszGid),(sf),ppFold)
#endif

//////////////////////////////////////////////////////////////////

/**
 * Copy the requested sub-section from item k into the result
 * item, IF IT EXISTS in item k and doesn't already exist in
 * the result.
 *
 */
static void _folded_item__insert_sub_if_exists(SG_context * pCtx,
											   struct _folded_item * pFold,
											   const SG_vhash * pvhItem_k,
											   const char * pszSubSectionName,
											   SG_uint32 bit)
{
	/*const*/ SG_vhash * pvhSub_j = NULL;
	SG_bool bHasSrc = SG_FALSE;
	SG_bool bHasDest = SG_FALSE;

	SG_ERR_CHECK(  SG_vhash__has(pCtx, pvhItem_k, pszSubSectionName, &bHasSrc)  );
	if (bHasSrc)
	{
		SG_ERR_CHECK(  SG_vhash__has(pCtx, pFold->pvhItemResult, pszSubSectionName, &bHasDest)  );
		if (!bHasDest)
		{
			SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem_k, pszSubSectionName, &pvhSub_j)  );
			SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pFold->pvhItemResult, pszSubSectionName, pvhSub_j)  );
		}

		// Set the 'exists in this cset' bit.
		pFold->existBits |= bit;
	}

	// We have dealt with the 'bit-x' (whether it exists or not).
	// That is, we know for a fact whether or not the entry existed
	// in this CSET.
	pFold->knownBits |= bit;

fail:
	return;
}


/**
 * Get the "path" field from the requested sub-section
 * and make it the official "path" of the item.
 *
 */
static void _folded_item__set_path_from(SG_context * pCtx,
										struct _folded_item * pFold,
										const char * pszLabel)
{
	/*const*/ SG_vhash * pvhSub;
	const char * pszPath;

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pFold->pvhItemResult, pszLabel, &pvhSub)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhSub, "path", &pszPath)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pFold->pvhItemResult, "path", pszPath)  );

fail:
	return;
}

/**
 * Use this to add status bits to the result statusFlags.
 * This lets us count them for setting __A__MULTIPLE_CHANGE
 * later.
 *
 */
static void _folded_item__set_sf_bit(SG_context * pCtx,
									 struct _folded_item * pFold,
									 SG_wc_status_flags sfBit,
									 const char * pszHeading)
{
	if ((pFold->sfR & sfBit) != sfBit)
	{
		pFold->sfR |= sfBit;
		pFold->nrBits++;
	}

	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx,
												 pFold->pvaResultHeadings,
												 pszHeading)  );
#if TRACE_WC_MSTATUS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "MSTATUS:Heading: %s\n",
							   pszHeading)  );
#endif	

fail:
	return;
}

static void _folded_item__set_sf_bit__uncounted(SG_context * pCtx,
												struct _folded_item * pFold,
												SG_wc_status_flags sfBit,
												const char * pszHeading)
{
	if ((pFold->sfR & sfBit) != sfBit)
	{
		pFold->sfR |= sfBit;
		// We do not want to count it: pFold->nrBits++;
	}

	SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx,
												 pFold->pvaResultHeadings,
												 pszHeading)  );
#if TRACE_WC_MSTATUS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "MSTATUS:Heading: %s\n",
							   pszHeading)  );
#endif	

fail:
	return;
}

/**
 * Check for __X__ bits in the WC status, if B-vs-WC is present.
 *
 * We group all of the __X__ bits as one for __A__MULTIPLE_CHANGE
 * purposes.
 *
 */
static void _folded_item__insert_conflict(SG_context * pCtx,
										  struct _folded_item * pFold)
{
	SG_wc_status_flags x_bits;
	SG_wc_status_flags xr_bits;
	SG_wc_status_flags xu_bits;
	
	x_bits = (pFold->sfW & SG_WC_STATUS_FLAGS__X__MASK);
	if (x_bits == 0)
		return;
	
	xr_bits = (pFold->sfW & SG_WC_STATUS_FLAGS__XR__MASK);
	xu_bits = (pFold->sfW & SG_WC_STATUS_FLAGS__XU__MASK);

	if (x_bits & SG_WC_STATUS_FLAGS__X__UNRESOLVED)
	{
		// if we had an issue, the overall state is either resolved or
		// unresolved, so only one of __X__RESOLVED and __X__UNRESOLVED can be set.
		SG_ASSERT( ((x_bits & SG_WC_STATUS_FLAGS__X__RESOLVED) == 0) );
		// xr_bits may or may not have bits set in it.
		SG_ASSERT(  (xu_bits != 0)  );

		SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx, pFold, x_bits, "Unresolved")  );

		if (     xr_bits & SG_WC_STATUS_FLAGS__XR__EXISTENCE)
			SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx, pFold, xr_bits, "Choice Resolved (Existence)")  );
		else if (xu_bits & SG_WC_STATUS_FLAGS__XU__EXISTENCE)
			SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx, pFold, xu_bits, "Choice Unresolved (Existence)")  );

		if (     xr_bits & SG_WC_STATUS_FLAGS__XR__NAME)
			SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx, pFold, xr_bits, "Choice Resolved (Name)")  );
		else if (xu_bits & SG_WC_STATUS_FLAGS__XU__NAME)
			SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx, pFold, xu_bits, "Choice Unresolved (Name)")  );

		if (     xr_bits & SG_WC_STATUS_FLAGS__XR__LOCATION)
			SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx, pFold, xr_bits, "Choice Resolved (Location)")  );
		else if (xu_bits & SG_WC_STATUS_FLAGS__XU__LOCATION)
			SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx, pFold, xu_bits, "Choice Unresolved (Location)")  );

		if (     xr_bits & SG_WC_STATUS_FLAGS__XR__ATTRIBUTES)
			SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx, pFold, xr_bits, "Choice Resolved (Attributes)")  );
		else if (xu_bits & SG_WC_STATUS_FLAGS__XU__ATTRIBUTES)
			SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx, pFold, xu_bits, "Choice Unresolved (Attributes)")  );

		if (     xr_bits & SG_WC_STATUS_FLAGS__XR__CONTENTS)
			SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx, pFold, xr_bits, "Choice Resolved (Contents)")  );
		else if (xu_bits & SG_WC_STATUS_FLAGS__XU__CONTENTS)
			SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx, pFold, xu_bits, "Choice Unresolved (Contents)")  );

	}
	else if (x_bits & SG_WC_STATUS_FLAGS__X__RESOLVED)
	{
		SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx, pFold, x_bits, "Resolved")  );
		SG_ASSERT(  (xr_bits != 0)  );
		SG_ASSERT(  (xu_bits == 0)  );

		if (xr_bits & SG_WC_STATUS_FLAGS__XR__EXISTENCE)
			SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx, pFold, xr_bits, "Choice Resolved (Existence)")  );

		if (xr_bits & SG_WC_STATUS_FLAGS__XR__NAME)
			SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx, pFold, xr_bits, "Choice Resolved (Name)")  );

		if (xr_bits & SG_WC_STATUS_FLAGS__XR__LOCATION)
			SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx, pFold, xr_bits, "Choice Resolved (Location)")  );

		if (xr_bits & SG_WC_STATUS_FLAGS__XR__ATTRIBUTES)
			SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx, pFold, xr_bits, "Choice Resolved (Attributes)")  );

		if (xr_bits & SG_WC_STATUS_FLAGS__XR__CONTENTS)
			SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx, pFold, xr_bits, "Choice Resolved (Contents)")  );
		
	}
	else // unknown __X__ bit ?
	{
		SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
	}

fail:
	return;
}

/**
 * Convert the accumulated statusFlags in the result item
 * into a pvhProperty and insert it into the result item.
 *
 * YOU MUST SET ALL OF THE BITS IN pFold->sfR *BEFORE* YOU
 * CALL THIS.  If you set bits after you call this, they
 * won't be reflected in the item.status properties.
 *
 */
static void _folded_item__insert_prop(SG_context * pCtx,
									  struct _folded_item * pFold)
{
	SG_vhash * pvhProp = NULL;

	if (pFold->nrBits > 1)
		pFold->sfR |= SG_WC_STATUS_FLAGS__A__MULTIPLE_CHANGE;

	SG_ERR_CHECK(  SG_wc__status__flags_to_properties(pCtx, pFold->sfR, &pvhProp)  );
	SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pFold->pvhItemResult, "status", &pvhProp)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pvhProp);
}

//////////////////////////////////////////////////////////////////

/**
 * Compare the named field in given sub-sections.
 *
 */
static void _cmp_ssf__sz(SG_context * pCtx,
						 const SG_vhash * pvhSub_x,
						 const SG_vhash * pvhSub_y,
						 const char * pszFieldName,
						 SG_bool * pbChanged)
{
	const char * psz_x = NULL;
	const char * psz_y = NULL;

	// most sub-section fields are manditory, but the optional fields
	// are only set if needed, so we have to do a little gymnastics
	// here to guard against null pointers.

	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhSub_x, pszFieldName, &psz_x)  );
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhSub_y, pszFieldName, &psz_y)  );

	*pbChanged = (strcmp(((psz_x) ? psz_x : ""),
						 ((psz_y) ? psz_y : "")) != 0);

fail:
	return;
}

static void _cmp_ssf__64(SG_context * pCtx,
						 const SG_vhash * pvhSub_x,
						 const SG_vhash * pvhSub_y,
						 const char * pszFieldName,
						 SG_bool * pbChanged)
{
	SG_int64 i64_x = 0;
	SG_int64 i64_y = 0;

	// Allow these to silently supply 0 when the
	// requested field is not present (which can
	// happen for LOST items).

	SG_ERR_CHECK(  SG_vhash__check__int64(pCtx, pvhSub_x, pszFieldName, &i64_x)  );
	SG_ERR_CHECK(  SG_vhash__check__int64(pCtx, pvhSub_y, pszFieldName, &i64_y)  );
	
	*pbChanged = (i64_x != i64_y);

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Compare 2 subsections and synthesize a set StatusFlags.
 * We only compute values for __S__ and __C__.
 * 
 * We need this to get a summary of the differences on an entry
 * between 2 csets that did not formally get compared.
 *
 */
static void _synthesize_status_flags(SG_context * pCtx,
									 struct _folded_item * pFold,
									 const char * pszSub_L,
									 const char * pszSub_R,
									 SG_wc_status_flags * pSF)
{
	SG_wc_status_flags sf = SG_WC_STATUS_FLAGS__ZERO;
	SG_vhash * pvhSub_L = NULL;		// we do not own this
	SG_vhash * pvhSub_R = NULL;		// we do not own this
	SG_bool bChanged = SG_FALSE;

	SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pFold->pvhItemResult, pszSub_L, &pvhSub_L)  );
	SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pFold->pvhItemResult, pszSub_R, &pvhSub_R)  );

	if (pvhSub_L && pvhSub_R)
	{
		SG_ERR_CHECK(  _cmp_ssf__sz(pCtx, pvhSub_L, pvhSub_R, "name", &bChanged)  );
		if (bChanged)
			sf |= SG_WC_STATUS_FLAGS__S__RENAMED;

		SG_ERR_CHECK(  _cmp_ssf__sz(pCtx, pvhSub_L, pvhSub_R, "gid_parent", &bChanged)  );
		if (bChanged)
			sf |= SG_WC_STATUS_FLAGS__S__MOVED;
		
		if ((pFold->sfR & SG_WC_STATUS_FLAGS__T__DIRECTORY) == 0)
		{
			SG_ERR_CHECK(  _cmp_ssf__sz(pCtx, pvhSub_L, pvhSub_R, "hid", &bChanged)  );
			if (bChanged)
				sf |= SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED;
		}
		
		SG_ERR_CHECK(  _cmp_ssf__64(pCtx, pvhSub_L, pvhSub_R, "attributes", &bChanged)  );
		if (bChanged)
			sf |= SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED;

	}
	else
	{
		// Since we are only called when we know both
		// halves exist, we'll never get here.  So I'm
		// not going to bother filling out the details
		// here and make someone think we might.
		SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
	}

	*pSF = sf;

fail:
	return;
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
			SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx,				\
													pFold,				\
													((sf) & (b)),		\
													(h))  );			\
		)																\

#define TestAndSet_Uncounted(sf, b, h)									\
	SG_STATEMENT(														\
		if ((sf) & (b))													\
			SG_ERR_CHECK(  _folded_item__set_sf_bit__uncounted(pCtx,	\
															   pFold,	\
															   ((sf) & (b)), \
															   (h))  );	\
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
						   struct _folded_item * pFold,
						   SG_wc_status_flags sf__b_c,
						   SG_wc_status_flags sf__b_m,
						   SG_wc_status_flags sf__c_m,
						   const char * pszHeadingPrefix)
{
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
									  pszHeadingPrefix, "(WC!=B,WC!=C,B!=C)")  );
			SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, (sf__b_m|sf__c_m), bufHeader)  );
		}
		else if (sf__b_m)			// B.foo != M.foo *and* C.foo == M.foo
		{
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(WC!=B,WC==C)")  ); // B!=C is implicit
			SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, sf__b_m, bufHeader)  );
		}
		else if (sf__c_m)			// B.foo == M.foo *and* C.foo != M.foo
		{
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(WC==B,WC!=C)")  ); // B!=C is implicit
			SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, sf__c_m, bufHeader)  );
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
									  pszHeadingPrefix, "(WC!=B,WC!=C,B==C)")  );
			SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, sf__b_m, bufHeader)  );
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
						   struct _folded_item * pFold,
						   const char * pszLabel_x,
						   SG_wc_status_flags sf__a_m,
						   SG_wc_status_flags sf__a_x,
						   SG_wc_status_flags sf__x_m,
						   const char * pszHeadingPrefix)
{
	char bufHeader[100];

	if (sf__a_m)				// A.foo != M.foo
	{
		if (sf__a_x && sf__x_m)		// A.foo != x.foo *and* x.foo != M.foo
		{
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s (WC!=A,%s!=A,WC!=%s)",
									  pszHeadingPrefix, pszLabel_x, pszLabel_x)  );
			SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, (sf__a_x|sf__x_m), bufHeader)  );
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
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s (%s!=A,WC==%s)", // WC!=A implicit
									  pszHeadingPrefix, pszLabel_x, pszLabel_x)  );
			SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, sf__a_x, bufHeader)  );
		}
		else if (sf__x_m)			// A.foo == x.foo *and* x.foo != M.foo
		{
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s (%s==A,WC!=%s)", // WC!=A implicit
									  pszHeadingPrefix, pszLabel_x, pszLabel_x)  );
			SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, sf__x_m, bufHeader)  );
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

			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s (WC!=%s,WC==A)", // x!=A implicit
									  pszHeadingPrefix, pszLabel_x)  );
			SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, (sf__a_x|sf__x_m), bufHeader)  );
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
							struct _folded_item * pFold,
							SG_wc_status_flags sf__a_m,
							SG_wc_status_flags sf__b_m,
							SG_wc_status_flags sf__c_m,
							const char * pszHeadingPrefix)
{
	char bufHeader[100];

	if (sf__a_m)				// A.foo != M.foo
	{
		if (sf__b_m && sf__c_m)		// B.foo != M.foo *and* C.foo != M.foo
		{
			// TODO 2012/02/28 Do we want to report on whether B.foo == C.foo also?

			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(WC!=A,WC!=B,WC!=C)")  );
			SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, (sf__a_m|sf__b_m|sf__c_m), bufHeader)  );
		}
		else if (sf__b_m)			// B.foo != M.foo *and* C.foo == M.foo
		{
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(WC!=A,WC!=B,WC==C)")  );
			SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, (sf__a_m|sf__b_m), bufHeader)  );
		}
		else if (sf__c_m)			// B.foo == M.foo *and* C.foo != M.foo
		{
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(WC!=A,WC==B,WC!=C)")  );
			SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, (sf__a_m|sf__c_m), bufHeader)  );
		}
		else						// B.foo == M.foo *and* C.foo == M.foo
		{
			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(WC!=A,WC==B,WC==C)")  );
			SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, sf__a_m, bufHeader)  );
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
									  pszHeadingPrefix, "(WC==A,WC!=B,WC!=C)")  );
			SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, (sf__b_m|sf__c_m), bufHeader)  );
		}
		else if (sf__b_m)			// B.foo != M.foo *and* C.foo == M.foo (implies C.foo == A.foo)
		{
			// If we get here, M ignored the changes in B and
			// carried forward the unchanged C version relative
			// to A.
			//
			// TODO 2012/02/28 Do we want to report it?

			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(WC==A,WC!=B,WC==C)")  );
			SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, sf__b_m, bufHeader)  );
		}
		else if (sf__c_m)			// B.foo == M.foo *and* C.foo != M.foo (implies B.foo == A.foo)
		{
			// If we get here, M ignored the changes in C and
			// carried forward the unchanged B version relative
			// to A.
			//
			// TODO 2012/02/28 Do we want to report it?

			SG_ERR_CHECK(  SG_sprintf(pCtx, bufHeader, sizeof(bufHeader), "%s %s",
									  pszHeadingPrefix, "(WC==A,WC==B,WC!=C)")  );
			SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, sf__c_m, bufHeader)  );
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
						struct _folded_item * pFold);

		
#define DCL(_fn_)											\
	static _fn_case _fn_;									\
	static void _fn_(SG_context * pCtx,						\
					 struct _folded_item * pFold)

DCL(_fn_case__zero)		// The entry did not exist in any of the 4 CSETs.
{
	SG_ERR_THROW2_RETURN(  SG_ERR_NOTIMPLEMENTED,
						   (pCtx, "MSTATUS case 0")  );
	SG_UNUSED( pFold );
}

DCL(_fn_case_____M)		// The entry only exists in M.
{
	SG_ERR_CHECK(  _folded_item__set_path_from(pCtx, pFold, SG_WC__STATUS_SUBSECTION__WC)  );

	if (pFold->sfW & SG_WC_STATUS_FLAGS__U__IGNORED)
	{
		SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__U__IGNORED, "Ignored")  );
		// No __S__ bits possible.
		// No __C__ bits possible.
	}
	else if (pFold->sfW & SG_WC_STATUS_FLAGS__U__FOUND)
	{
		SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__U__FOUND, "Found")  );
		// No __S__ bits possible.
		// No __C__ bits possible.
	}
	else
	{
		SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__S__ADDED, "Added (WC)")  );

		// No __S__ bits possible because no reference version.

		if (pFold->sfW & SG_WC_STATUS_FLAGS__U__LOST)
		{
			SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__U__LOST, "Lost")  );
			// No __C__ bits because of LOST.
		}
		else
		{
			// No __C__ bits possible because no reference version.
		}
	}

fail:
	return;
}

DCL(_fn_case____C_)		// The entry only exists in C.
{
	// So it was added in C and deleted from M.
	// Set path from C because that is the last place we saw it.
	SG_ERR_CHECK(  _folded_item__set_path_from(pCtx, pFold, SG_WC__STATUS_SUBSECTION__C)  );

	SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__S__ADDED,   "Added (C)")  );
	SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__S__DELETED, "Removed (WC)")  );

	// No __S__/__C__ bits possible because no final version.

fail:
	return;
}

DCL(_fn_case____CM)		// The entry exists in both C and M.
{
	SG_wc_status_flags sf__c_m;

	// So it was added in C.
	// It may or may not have changed in M.

	SG_ERR_CHECK(  _folded_item__set_path_from(pCtx, pFold, SG_WC__STATUS_SUBSECTION__WC)  );

	SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__S__ADDED,   "Added (C)")  );

	SG_ERR_CHECK(  _synthesize_status_flags(pCtx, pFold,
											SG_WC__STATUS_SUBSECTION__C,
											SG_WC__STATUS_SUBSECTION__WC,
											&sf__c_m)  );

	// set __S__ bits between C and M.

	TestAndSet( sf__c_m, SG_WC_STATUS_FLAGS__S__RENAMED, "Renamed (WC)" );
	TestAndSet( sf__c_m, SG_WC_STATUS_FLAGS__S__MOVED,   "Moved (WC)" );

	if (pFold->sfW & SG_WC_STATUS_FLAGS__U__LOST)
	{
		SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__U__LOST, "Lost")  );
		// No __C__ bits because of LOST.
	}
	else
	{
		// set __C__ bits between C and M.

		TestAndSet( sf__c_m, SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED, "Attributes (WC)" );
		if ((pFold->sfR & SG_WC_STATUS_FLAGS__T__DIRECTORY) == 0)
			TestAndSet( sf__c_m, SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED, "Modified (WC)" );
	}

fail:
	return;
}

DCL(_fn_case___B__)		// The entry only exists in B.
{
	// So it was added in B and deleted from M.

	SG_ERR_CHECK(  _folded_item__set_path_from(pCtx, pFold, SG_WC__STATUS_SUBSECTION__B)  );

	SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__S__ADDED,   "Added (B)")  );
	SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__S__DELETED, "Removed (WC)")  );

	// No __S__/__C__ bits possible because no final version.

fail:
	return;
}

DCL(_fn_case___B_M)		// The entry exists in both B and M.
{
	SG_wc_status_flags sf__b_m = pFold->sfW;

	// So it was added in B.
	// It may or may not have changed in M.

	SG_ERR_CHECK(  _folded_item__set_path_from(pCtx, pFold, SG_WC__STATUS_SUBSECTION__WC)  );

	SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__S__ADDED,   "Added (B)")  );

	// set __S__ bits between B and M.

	TestAndSet( sf__b_m, SG_WC_STATUS_FLAGS__S__RENAMED, "Renamed (WC)" );
	TestAndSet( sf__b_m, SG_WC_STATUS_FLAGS__S__MOVED,   "Moved (WC)" );

	if (pFold->sfW & SG_WC_STATUS_FLAGS__U__LOST)
	{
		SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__U__LOST, "Lost")  );
		// No __C__ bits because of LOST.
	}
	else
	{
		// set __C__ bits between B and M.

		TestAndSet( sf__b_m, SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED, "Attributes (WC)" );
		if ((pFold->sfR & SG_WC_STATUS_FLAGS__T__DIRECTORY) == 0)
			TestAndSet( sf__b_m, SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED, "Modified (WC)" );
	}

fail:
	return;
}

DCL(_fn_case___BC_)		// The entry exists in B and C, but not in A nor M.
{
	// Therefore it was added in an SPCA between A and {B,C}.
	// And it was deleted in M.

	// "path" could be set using either B or C.  Just pick one.
	SG_ERR_CHECK(  _folded_item__set_path_from(pCtx, pFold, SG_WC__STATUS_SUBSECTION__B)  );

	// Note: reporting it as "ADDED in B *and* C" is a bit of a
	//       lie.  Technically, it first existed in some cset
	//       between A and B which is also between A and C and
	//       because we haven't looked at any of the SPCAs
	//       we are first seeing the item in both B and C.
	//
	//       But from the point of view of A, the item has
	//       been added.
	SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__S__ADDED,   "Added (B,C)")  );
	SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__S__DELETED, "Removed (WC)")  );

	// We DO NOT report __S__/__C__ changes between B and C.

	// No __S__/__C__ bits possible because no final version.

fail:
	return;
}

DCL(_fn_case___BCM)		// The entry exists in B, C and M, but not in A.
{
	SG_wc_status_flags sf__b_c;
	SG_wc_status_flags sf__b_m;
	SG_wc_status_flags sf__c_m;

	SG_ERR_CHECK(  _synthesize_status_flags(pCtx, pFold,
											SG_WC__STATUS_SUBSECTION__B,
											SG_WC__STATUS_SUBSECTION__C,
											&sf__b_c)  );

	sf__b_m = pFold->sfW;
	
	SG_ERR_CHECK(  _synthesize_status_flags(pCtx, pFold,
											SG_WC__STATUS_SUBSECTION__C,
											SG_WC__STATUS_SUBSECTION__WC,
											&sf__c_m)  );

	// Therefore it was added in an SPCA between A and {B,C}.
	// It may or may not have changed in M.

	// "path" could be set using either B or C.  Just pick one.
	SG_ERR_CHECK(  _folded_item__set_path_from(pCtx, pFold, SG_WC__STATUS_SUBSECTION__WC)  );

	SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__S__ADDED,   "Added (B,C)")  );

	// set __S__ bits between {B,C} and M.

	SG_ERR_CHECK(  TestAndSet_BCM(pCtx, pFold,
								  sf__b_c & SG_WC_STATUS_FLAGS__S__RENAMED,
								  sf__b_m & SG_WC_STATUS_FLAGS__S__RENAMED,
								  sf__c_m & SG_WC_STATUS_FLAGS__S__RENAMED,
								  "Renamed")  );
	SG_ERR_CHECK(  TestAndSet_BCM(pCtx, pFold,
								  sf__b_c & SG_WC_STATUS_FLAGS__S__MOVED,
								  sf__b_m & SG_WC_STATUS_FLAGS__S__MOVED,
								  sf__c_m & SG_WC_STATUS_FLAGS__S__MOVED,
								  "Moved")  );

	if (pFold->sfW & SG_WC_STATUS_FLAGS__U__LOST)
	{
		SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__U__LOST, "Lost")  );
		// No __C__ bits because of LOST.
	}
	else
	{
		// set __C__ bits between {B,C} and M.

		SG_ERR_CHECK(  TestAndSet_BCM(pCtx, pFold,
									  sf__b_c & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED,
									  sf__b_m & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED,
									  sf__c_m & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED,
									  "Attributes")  );
		if ((pFold->sfR & SG_WC_STATUS_FLAGS__T__DIRECTORY) == 0)
			SG_ERR_CHECK(  TestAndSet_BCM(pCtx, pFold,
										  sf__b_c & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED,
										  sf__b_m & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED,
										  sf__c_m & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED,
										  "Modified")  );
	}

fail:
	return;
}

DCL(_fn_case__A___)		// The entry existed in A, but not in B, C, nor M.
{
	// Set path from A because that is the last place we saw it.
	SG_ERR_CHECK(  _folded_item__set_path_from(pCtx, pFold, SG_WC__STATUS_SUBSECTION__A)  );

	SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__S__DELETED, "Removed (B,C,WC)")  );

	// No __C__ bits possible because no final version.

fail:
	return;
}

DCL(_fn_case__A__M)		// The entry existed in A and M, but not B nor C.
{
	// This should not be possible.
	SG_ERR_THROW2_RETURN(  SG_ERR_NOTIMPLEMENTED,
						   (pCtx, "MSTATUS case 9")  );
	SG_UNUSED( pFold );
}

DCL(_fn_case__A_C_)		// The entry existed in A and C, but not B nor M.
{
	// set path from C because that is the last place we saw it.
	SG_ERR_CHECK(  _folded_item__set_path_from(pCtx, pFold, SG_WC__STATUS_SUBSECTION__C)  );

	SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__S__DELETED, "Removed (B,WC)")  );

	// No __C__ bits possible because no final version.

fail:
	return;
}

DCL(_fn_case__A_CM)		// The entry existed in A, C and M, but not in B.
{
	SG_wc_status_flags sf__a_m;
	SG_wc_status_flags sf__a_c;
	SG_wc_status_flags sf__c_m;

	SG_ERR_CHECK(  _synthesize_status_flags(pCtx, pFold,
											SG_WC__STATUS_SUBSECTION__A,
											SG_WC__STATUS_SUBSECTION__WC,
											&sf__a_m)  );
	
	sf__a_c = pFold->sf__a_c;

	SG_ERR_CHECK(  _synthesize_status_flags(pCtx, pFold,
											SG_WC__STATUS_SUBSECTION__C,
											SG_WC__STATUS_SUBSECTION__WC,
											&sf__c_m)  );

	SG_ERR_CHECK(  _folded_item__set_path_from(pCtx, pFold, SG_WC__STATUS_SUBSECTION__WC)  );

	// We *ONLY* set an 'existence' bit when there is a delete/non-delete
	// between B and C and we can't report it as ADDED or REMOVED because
	// A and M are in agreement.
	// 
	// This is to indicate that somebody had to deal with whether or not
	// the item should exist (whether manually or automatically).
	SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__M__AbCM, "Existence (A,!B,C,WC)")  );

	// Set __S__ bits between A,C,M.

	SG_ERR_CHECK(  TestAndSet_AxM(pCtx, pFold,
								  SG_WC__STATUS_SUBSECTION__C,
								  sf__a_m & SG_WC_STATUS_FLAGS__S__RENAMED,
								  sf__a_c & SG_WC_STATUS_FLAGS__S__RENAMED,
								  sf__c_m & SG_WC_STATUS_FLAGS__S__RENAMED,
								  "Renamed")  );
	SG_ERR_CHECK(  TestAndSet_AxM(pCtx, pFold,
								  SG_WC__STATUS_SUBSECTION__C,
								  sf__a_m & SG_WC_STATUS_FLAGS__S__MOVED,
								  sf__a_c & SG_WC_STATUS_FLAGS__S__MOVED,
								  sf__c_m & SG_WC_STATUS_FLAGS__S__MOVED,
								  "Moved")  );

	if (pFold->sfW & SG_WC_STATUS_FLAGS__U__LOST)
	{
		SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__U__LOST, "Lost")  );
		// No __C__ bits because of LOST.
	}
	else
	{
		// Set __C__ bits between A,C,M.

		SG_ERR_CHECK(  TestAndSet_AxM(pCtx, pFold,
									  SG_WC__STATUS_SUBSECTION__C,
									  sf__a_m & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED,
									  sf__a_c & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED,
									  sf__c_m & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED,
									  "Attributes")  );
		if ((pFold->sfR & SG_WC_STATUS_FLAGS__T__DIRECTORY) == 0)
			SG_ERR_CHECK(  TestAndSet_AxM(pCtx, pFold,
										  SG_WC__STATUS_SUBSECTION__C,
										  sf__a_m & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED,
										  sf__a_c & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED,
										  sf__c_m & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED,
										  "Modified")  );
	}

fail:
	return;
}

DCL(_fn_case__AB__)		// The entry existed in A and B, but not C nor M.
{
	// set path from B because that is the last place we saw it.
	SG_ERR_CHECK(  _folded_item__set_path_from(pCtx, pFold, SG_WC__STATUS_SUBSECTION__B)  );

	// We say that the item was removed from C and from M.
	// This implies that it was present in A and B.
	SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__S__DELETED, "Removed (C,WC)")  );

	// No __C__ bits possible because no final version.

fail:
	return;
}

DCL(_fn_case__AB_M)		// The entry existed in A, B and M, but not in C.
{
	SG_wc_status_flags sf__a_m;
	SG_wc_status_flags sf__a_b;
	SG_wc_status_flags sf__b_m;

	SG_ERR_CHECK(  _synthesize_status_flags(pCtx, pFold,
											SG_WC__STATUS_SUBSECTION__A,
											SG_WC__STATUS_SUBSECTION__WC,
											&sf__a_m)  );
	sf__a_b = pFold->sf__a_b;
	sf__b_m = pFold->sfW;
	
	SG_ERR_CHECK(  _folded_item__set_path_from(pCtx, pFold, SG_WC__STATUS_SUBSECTION__WC)  );

	SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__M__ABcM, "Existence (A,B,!C,WC)")  );

	// Set __S__ bits between A,B,M.

	SG_ERR_CHECK(  TestAndSet_AxM(pCtx, pFold,
								  SG_WC__STATUS_SUBSECTION__B,
								  sf__a_m & SG_WC_STATUS_FLAGS__S__RENAMED,
								  sf__a_b & SG_WC_STATUS_FLAGS__S__RENAMED,
								  sf__b_m & SG_WC_STATUS_FLAGS__S__RENAMED,
								  "Renamed")  );
	SG_ERR_CHECK(  TestAndSet_AxM(pCtx, pFold,
								  SG_WC__STATUS_SUBSECTION__B,
								  sf__a_m & SG_WC_STATUS_FLAGS__S__MOVED,
								  sf__a_b & SG_WC_STATUS_FLAGS__S__MOVED,
								  sf__b_m & SG_WC_STATUS_FLAGS__S__MOVED,
								  "Moved")  );

	if (pFold->sfW & SG_WC_STATUS_FLAGS__U__LOST)
	{
		SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__U__LOST, "Lost")  );
		// No __C__ bits because of LOST.
	}
	else
	{
		// Set __C__ bits between A,B,M.

		SG_ERR_CHECK(  TestAndSet_AxM(pCtx, pFold,
									  SG_WC__STATUS_SUBSECTION__B,
									  sf__a_m & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED,
									  sf__a_b & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED,
									  sf__b_m & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED,
									  "Attributes")  );
		if ((pFold->sfR & SG_WC_STATUS_FLAGS__T__DIRECTORY) == 0)
			SG_ERR_CHECK(  TestAndSet_AxM(pCtx, pFold,
										  SG_WC__STATUS_SUBSECTION__B,
										  sf__a_m & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED,
										  sf__a_b & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED,
										  sf__b_m & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED,
										  "Modified")  );
	}

fail:
	return;
}

DCL(_fn_case__ABC_)		// The entry existed in A, B, and C, but not in M.
{
	// either B or C's path will do here.
	SG_ERR_CHECK(  _folded_item__set_path_from(pCtx, pFold, SG_WC__STATUS_SUBSECTION__B)  );

	SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__S__DELETED, "Removed (WC)")  );

	// No __C__ bits possible because no final version.

fail:
	return;
}

DCL(_fn_case__ABCM)		// The entry existed in A, B, C, and M.
{
	SG_wc_status_flags sf__a_m;
	SG_wc_status_flags sf__b_m;
	SG_wc_status_flags sf__c_m;

	SG_ERR_CHECK(  _synthesize_status_flags(pCtx, pFold,
											SG_WC__STATUS_SUBSECTION__A,
											SG_WC__STATUS_SUBSECTION__WC,
											&sf__a_m)  );

	sf__b_m = pFold->sfW;

	SG_ERR_CHECK(  _synthesize_status_flags(pCtx, pFold,
											SG_WC__STATUS_SUBSECTION__C,
											SG_WC__STATUS_SUBSECTION__WC,
											&sf__c_m)  );

	SG_ERR_CHECK(  _folded_item__set_path_from(pCtx, pFold, SG_WC__STATUS_SUBSECTION__WC)  );

	// No Added/Removed/Existence bits need to be set.

	// Set __S__ bits between A,B,C,M.

	SG_ERR_CHECK(  TestAndSet_ABCM(pCtx, pFold,
								   sf__a_m & SG_WC_STATUS_FLAGS__S__RENAMED,
								   sf__b_m & SG_WC_STATUS_FLAGS__S__RENAMED,
								   sf__c_m & SG_WC_STATUS_FLAGS__S__RENAMED,
								   "Renamed")  );
	SG_ERR_CHECK(  TestAndSet_ABCM(pCtx, pFold,
								   sf__a_m & SG_WC_STATUS_FLAGS__S__MOVED,
								   sf__b_m & SG_WC_STATUS_FLAGS__S__MOVED,
								   sf__c_m & SG_WC_STATUS_FLAGS__S__MOVED,
								   "Moved")  );

	if (pFold->sfW & SG_WC_STATUS_FLAGS__U__LOST)
	{
		SG_ERR_CHECK(  _folded_item__set_sf_bit(pCtx, pFold, SG_WC_STATUS_FLAGS__U__LOST, "Lost")  );
		// No __C__ bits because of LOST.
	}
	else
	{
		// Set __C__ bits between A,B,C,M.

		SG_ERR_CHECK(  TestAndSet_ABCM(pCtx, pFold,
									   sf__a_m & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED,
									   sf__b_m & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED,
									   sf__c_m & SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED,
									   "Attributes")  );
		if ((pFold->sfR & SG_WC_STATUS_FLAGS__T__DIRECTORY) == 0)
			SG_ERR_CHECK(  TestAndSet_ABCM(pCtx, pFold,
										   sf__a_m & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED,
										   sf__b_m & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED,
										   sf__c_m & SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED,
										   "Modified")  );
		if (pFold->sfR & SG_WC_STATUS_FLAGS__T__FILE)
		{
			TestAndSet_Uncounted( pFold->sfW, SG_WC_STATUS_FLAGS__M__AUTO_MERGED,        "Auto-Merged" );
			TestAndSet_Uncounted( pFold->sfW, SG_WC_STATUS_FLAGS__M__AUTO_MERGED_EDITED, "Auto-Merged (Edited)" );
		}
	}

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

/**
 * Verify that we have a pending merge and get the various parts
 * and compute the pair-wise statuses.
 *
 * We DO NOT need to sort the individual pair-wise statuses
 * at this point.  The caller will/should sort the resulting
 * MSTATUS when we're done.
 *
 */
static void _mstatus__get_csets(SG_context * pCtx, struct _mstatus_data * pData)
{
	SG_uint32 nrCSets;
	SG_bool bListUnchanged_WC = SG_TRUE;
	SG_bool bNoTSC_WC = SG_FALSE;
	SG_bool bListSparse_WC = SG_FALSE;
	SG_bool bListReserved_WC = SG_FALSE;

	SG_ERR_CHECK(  SG_wc_tx__get_wc_csets__vhash(pCtx, pData->pWcTx, &pData->pvhCSets)  );
	SG_ERR_CHECK(  SG_vhash__count(pCtx, pData->pvhCSets, &nrCSets)  );
	SG_ASSERT(  (nrCSets > 0)  );
	if (nrCSets == 1)
		SG_ERR_THROW(  SG_ERR_NOT_IN_A_MERGE  );

	// tne_L0 ==> B
	// tne_L1 ==> C
	// tne_A  ==> A

	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pData->pvhCSets, "L0", &pData->pszHid_B)  );
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pData->pvhCSets, "L1", &pData->pszHid_C)  );
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pData->pvhCSets, "A" , &pData->pszHid_A )  );
	
	if (!pData->pszHid_B || !pData->pszHid_C || !pData->pszHid_A)
		SG_ERR_THROW(  SG_ERR_INVALIDARG  );

	if (pData->pvhLegend)
	{
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pData->pvhLegend, SG_WC__STATUS_SUBSECTION__A, pData->pszHid_A)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pData->pvhLegend, SG_WC__STATUS_SUBSECTION__B, pData->pszHid_B)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pData->pvhLegend, SG_WC__STATUS_SUBSECTION__C, pData->pszHid_C)  );
	}

	// We don't have any way of comparing {A}==>{WC} directly, so we
	// compute a normal {B}==>{WC} status.  We have it report unchanged
	// items so that we can fill in the missing details as necessary.
	// 
	// Items within should have sub-sections:
	//     SG_WC__STATUS_SUBSECTION__B
	//     SG_WC__STATUS_SUBSECTION__WC
	// and domain-relative repo-paths of:
	//     SG_WC__REPO_PATH_DOMAIN__B
	//     SG_WC__REPO_PATH_DOMAIN__WC.
	//
	// TODO 2013/01/11 With the new 1-rev-spec status code added for P4177,
	// TODO            the above comment about {A}==>{WC} is not true anymore.
	// TODO            Revisit this and see if it would be helpful to switch.

	SG_ERR_CHECK(  SG_wc_tx__status(pCtx, pData->pWcTx,
									"@/", SG_INT32_MAX,
									bListUnchanged_WC,
									pData->bNoIgnores,
									bNoTSC_WC,
									bListSparse_WC,
									bListReserved_WC,
									SG_TRUE,			// no sort pairwise status
									&pData->pvaStatus_B_vs_WC,
									NULL)  );

	// Compute historical status for branch B: {A}==>{B}.
	// 
	// Items within should have sub-sections "A" and "B" and
	// domain-relative repo-paths of 'a' and 'b'.

	SG_ERR_CHECK(  SG_vv2__status__repo(pCtx, pData->pWcTx->pDb->pRepo, NULL,
										pData->pszHid_A, pData->pszHid_B,
										SG_WC__REPO_PATH_DOMAIN__A, SG_WC__REPO_PATH_DOMAIN__B,
										SG_WC__STATUS_SUBSECTION__A, SG_WC__STATUS_SUBSECTION__B,
										pData->pszWasLabel_A, pData->pszWasLabel_B,
										SG_TRUE,			// no sort pairwise status
										&pData->pvaStatus_A_vs_B, NULL)  );

	// Compute historical status for branch C: {A}==>{C}.
	// 
	// Items within should have sub-sections "A" and "C" and
	// domain-relative repo-paths of 'a' and 'c'.

	SG_ERR_CHECK(  SG_vv2__status__repo(pCtx, pData->pWcTx->pDb->pRepo, NULL,
										pData->pszHid_A, pData->pszHid_C,
										SG_WC__REPO_PATH_DOMAIN__A, SG_WC__REPO_PATH_DOMAIN__C,
										SG_WC__STATUS_SUBSECTION__A, SG_WC__STATUS_SUBSECTION__C,
										pData->pszWasLabel_A, pData->pszWasLabel_C,
										SG_TRUE,			// no sort pairwise status
										&pData->pvaStatus_A_vs_C, NULL)  );
fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * We know that the entry is unchanged between A and B
 * and we only KNOW ABOUT "B" in our result.
 * 
 * Synthesize the "A" subsection from what we know about
 * the "B" subsection *AND* record that we KNOW ABOUT "A".
 *
 * We have to rephrase the repo-path that we store inside
 * the subsection because it needs to be cset-relative.
 * (In case a parent directory of the entry was moved/renamed
 * between A and B, we want the repo-path in A to be as it
 * existed in cset A.)
 *
 */
static void _synthesize_subsection__a_from_b(SG_context * pCtx,
											 struct _mstatus_data * pData,
											 struct _folded_item * pFold,
											 const char * pszGid)
{
	SG_string * pStringRepoPath = NULL;
	SG_string * pStringRepoPathExtended = NULL;
	const char * psz;
	SG_vhash * pvhSub = NULL;		// we do not own this
	SG_vhash * pvhSubCopy = NULL;	// we do not own this

	SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pFold->pvhItemResult,
										  SG_WC__STATUS_SUBSECTION__B,
										  &pvhSub)  );
	if (pvhSub)
	{
		// TODO 2012/03/06 This call is *extremely* expensive.
		// TODO            I'm thinking of adding a "tne_A" table
		// TODO            to the WC.DB (for other reasons).  This
		// TODO            would let us do a quick lookup for this
		// TODO            using sg_wc_db__tne__get_extended_repo_path_...()

		SG_ERR_CHECK(  SG_repo__treendx__get_path_in_dagnode(pCtx,
															 pData->pWcTx->pDb->pRepo,
															 SG_DAGNUM__VERSION_CONTROL,
															 pszGid,
															 pData->pszHid_A,
															 &pStringRepoPath,
															 NULL)  );
		// The above only returns non-extended repo-paths.
		psz = SG_string__sz(pStringRepoPath);
		SG_ASSERT(  (psz[0]=='@') && (psz[1]=='/')  );

		SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringRepoPathExtended)  );
		SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStringRepoPathExtended, "@%c%s",
										  SG_WC__REPO_PATH_DOMAIN__A,
										  &psz[1])  );

		// Clone the "B" subsection into "A".
		// Then replace the repo-path in "A".
		
		SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pFold->pvhItemResult,
												SG_WC__STATUS_SUBSECTION__A,
												pvhSub)  );
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pFold->pvhItemResult,
											SG_WC__STATUS_SUBSECTION__A,
											&pvhSubCopy)  );
		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pvhSubCopy, "path",
													SG_string__sz(pStringRepoPathExtended))  );

		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pvhSubCopy, "was_label",
													pData->pszWasLabel_A)  );

		pFold->existBits |= BIT_A;
	}
	else
	{
		// This is OK.  It just means that the entry did not exist in B.
		// And since that didn't change between B an A, we leave A empty
		// so that we are saying that it did not exist in A.
	}

	pFold->knownBits |= BIT_A;

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
	SG_STRING_NULLFREE(pCtx, pStringRepoPathExtended);
}

/**
 * We know that the entry is unchanged between A and C and
 * we only KNOW ABOUT "A" in our result.
 *
 * Synthesize the "C" subsection from what we know about
 * the "A" subsection *AND* record that we KNOW ABOUT "C".
 *
 * Again, we have to rephrase the repo-path in the "C"
 * subsection to be C-relative.
 *
 */
static void _synthesize_subsection__c_from_a(SG_context * pCtx,
											 struct _mstatus_data * pData,
											 struct _folded_item * pFold,
											 const char * pszGid)
{
	SG_string * pStringRepoPathExtended = NULL;
	SG_uint64 uiAliasGid;
	SG_vhash * pvhSub = NULL;		// we do not own this
	SG_vhash * pvhSubCopy = NULL;	// we do not own this

	SG_ERR_CHECK(  SG_vhash__check__vhash(pCtx, pFold->pvhItemResult,
										  SG_WC__STATUS_SUBSECTION__A,
										  &pvhSub)  );
	if (pvhSub)
	{
		SG_ERR_CHECK(  sg_wc_db__gid__get_alias_from_gid(pCtx,
														 pData->pWcTx->pDb,
														 pszGid,
														 &uiAliasGid)  );
		if (!pData->pCSetRow_C)
			SG_ERR_CHECK(  sg_wc_db__csets__get_row_by_label(pCtx,
															 pData->pWcTx->pDb,
															 "L1",
															 NULL,
															 &pData->pCSetRow_C)  );

		SG_ERR_CHECK(  sg_wc_db__tne__get_extended_repo_path_from_alias(pCtx,
																		pData->pWcTx->pDb,
																		pData->pCSetRow_C,
																		uiAliasGid,
																		SG_WC__REPO_PATH_DOMAIN__C,
																		&pStringRepoPathExtended)  );

		// Clone the "A" subsection into "C".
		// Then replace the repo-path in "C".
		
		SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pFold->pvhItemResult,
												SG_WC__STATUS_SUBSECTION__C,
												pvhSub)  );
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pFold->pvhItemResult,
											SG_WC__STATUS_SUBSECTION__C,
											&pvhSubCopy)  );
		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pvhSubCopy, "path",
													SG_string__sz(pStringRepoPathExtended))  );

		SG_ERR_CHECK(  SG_vhash__update__string__sz(pCtx, pvhSubCopy, "was_label",
													pData->pszWasLabel_C)  );

		pFold->existBits |= BIT_C;
	}
	else
	{
		// This is OK.  It just means that the entry did not exist in A.
		// And since that didn't change between A and C, we leave C empty
		// so that we are saying that it did not exist in C.
	}

	pFold->knownBits |= BIT_C;

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPathExtended);
}

//////////////////////////////////////////////////////////////////

static SG_rbtree_foreach_callback cbSync;

static void cbSync(SG_context * pCtx,
				   const char * pszGid_k,
				   void * pVoidAssoc,
				   void * pVoidData)
{
	struct _folded_item * pFold = (struct _folded_item *)pVoidAssoc;
//	struct _mstatus_data * pData = (struct _mstatus_data *)pVoidData;

	SG_UNUSED( pVoidData );

#if TRACE_WC_MSTATUS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "MStatus:Fold: [existBits %x]: %s\n",
							   pFold->existBits, pszGid_k)  );
#else
	SG_UNUSED( pszGid_k );
#endif

	SG_ERR_CHECK(  (*(saCaseTable[pFold->existBits]))(pCtx, pFold)  );

	// TODO 2012/02/20 check for  __SPARSE?
	// TODO 2012/11/26 TODO ......

	SG_ERR_CHECK(  _folded_item__insert_conflict(pCtx, pFold)  );
	SG_ERR_CHECK(  _folded_item__insert_prop(pCtx, pFold)  );

#if TRACE_WC_MSTATUS
	SG_ERR_IGNORE(  SG_vhash_debug__dump_to_console__named(pCtx, pFold->pvhItemResult, "MSTATUS Examine")  );
#endif

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Sync-up each of the items in the pair-wise statuses by GID.
 *
 * For A-vs-L*, each of the pair-wise statuses is an array of
 * the (ONLY THE) changed items in that {x}==>{y} set.
 *
 * For B-vs-WC, we get unchanged items too.
 *
 * Build a map[<gid> ==> <folded-item>] where each
 * <folded-item> contains the corresponding ({x}==>{y})[gid].
 *
 */
static void _mstatus__sync_up(SG_context * pCtx, struct _mstatus_data * pData)
{
	SG_uint32 k, kLimit;

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pData->prbSync)  );

	//////////////////////////////////////////////////////////////////
	// fill in "column" A-vs-C.
	//////////////////////////////////////////////////////////////////

	SG_ERR_CHECK(  SG_varray__count(pCtx, pData->pvaStatus_A_vs_C, &kLimit)  );
	for (k=0; k<kLimit; k++)
	{
		SG_vhash * pvhItem_k;			// we do not own this
		SG_vhash * pvhStatus_k;			// we do not own this
		const char * pszGid_k;			// we do not own this
		struct _folded_item * pFold;	// we do not own this
		SG_int64 i64;
		SG_wc_status_flags sf;
		SG_bool bFound;

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pData->pvaStatus_A_vs_C, k, &pvhItem_k)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem_k, "gid", &pszGid_k)  );
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem_k, "status", &pvhStatus_k)  );
		SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhStatus_k, "flags", &i64)  );
		sf = ((SG_wc_status_flags)i64);

		SG_ERR_CHECK(  SG_rbtree__find(pCtx, pData->prbSync, pszGid_k, &bFound, (void **)&pFold)  );
		if (!bFound)
			SG_ERR_CHECK(  _FOLDED_ITEM__ALLOC(pCtx, pData, pszGid_k, sf, &pFold)  );

		pFold->sf__a_c = sf;

		// Go ahead and copy the per-cset subsections into the
		// result item while we know whether they exist.

		SG_ERR_CHECK(  _folded_item__insert_sub_if_exists(pCtx, pFold, pvhItem_k, SG_WC__STATUS_SUBSECTION__A, BIT_A)  );
		SG_ERR_CHECK(  _folded_item__insert_sub_if_exists(pCtx, pFold, pvhItem_k, SG_WC__STATUS_SUBSECTION__C, BIT_C)  );
	}

	//////////////////////////////////////////////////////////////////
	// fill in "column" A-vs-B.
	//////////////////////////////////////////////////////////////////

	SG_ERR_CHECK(  SG_varray__count(pCtx, pData->pvaStatus_A_vs_B, &kLimit)  );
	for (k=0; k<kLimit; k++)
	{
		SG_vhash * pvhItem_k;			// we do not own this
		SG_vhash * pvhStatus_k;			// we do not own this
		const char * pszGid_k;			// we do not own this
		struct _folded_item * pFold;	// we do not own this
		SG_int64 i64;
		SG_wc_status_flags sf;
		SG_bool bFound;

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pData->pvaStatus_A_vs_B, k, &pvhItem_k)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem_k, "gid", &pszGid_k)  );
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem_k, "status", &pvhStatus_k)  );
		SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhStatus_k, "flags", &i64)  );
		sf = ((SG_wc_status_flags)i64);

		SG_ERR_CHECK(  SG_rbtree__find(pCtx, pData->prbSync, pszGid_k, &bFound, (void **)&pFold)  );
		if (!bFound)
			SG_ERR_CHECK(  _FOLDED_ITEM__ALLOC(pCtx, pData, pszGid_k, sf, &pFold)  );

		pFold->sf__a_b = sf;

		SG_ERR_CHECK(  _folded_item__insert_sub_if_exists(pCtx, pFold, pvhItem_k, SG_WC__STATUS_SUBSECTION__A, BIT_A)  );
		SG_ERR_CHECK(  _folded_item__insert_sub_if_exists(pCtx, pFold, pvhItem_k, SG_WC__STATUS_SUBSECTION__B, BIT_B)  );

		if ((pFold->knownBits & BIT_C) == 0)
		{
			// This entry was identical in A and C, so the A-vs-C
			// status didn't have an item for it.  Synthesize a
			// column for C using A as a reference.
			SG_ERR_CHECK(  _synthesize_subsection__c_from_a(pCtx, pData, pFold, pszGid_k)  );
		}
	}

	//////////////////////////////////////////////////////////////////
	// fill in "column" B-vs-WC in the <folded-item>.
	//////////////////////////////////////////////////////////////////

	SG_ERR_CHECK(  SG_varray__count(pCtx, pData->pvaStatus_B_vs_WC, &kLimit)  );
	for (k=0; k<kLimit; k++)
	{
		SG_vhash * pvhItem_k;			// we do not own this
		SG_vhash * pvhStatus_k;			// we do not own this
		const char * pszGid_k;			// we do not own this
		struct _folded_item * pFold;	// we do not own this
		SG_int64 i64;
		SG_wc_status_flags sfW;
		SG_bool bFound;
		SG_bool bChanged;

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pData->pvaStatus_B_vs_WC, k, &pvhItem_k)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem_k, "gid", &pszGid_k)  );
		SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvhItem_k, "status", &pvhStatus_k)  );
		SG_ERR_CHECK(  SG_vhash__get__int64(pCtx, pvhStatus_k, "flags", &i64)  );
		sfW = ((SG_wc_status_flags)i64);

		// since the B-vs-WC status has unchanged items in it,
		// filter them out if we didn't get a peer in either of
		// the A-vs-L* statuses.
		//
		// TODO 2012/03/07 Think about whether we want to also report
		// TODO            on SPARSE using __A__MASK.
		// TODO 2012/11/26 TODO ......

		bChanged = ((sfW & (SG_WC_STATUS_FLAGS__S__MASK
							| SG_WC_STATUS_FLAGS__C__MASK
							| SG_WC_STATUS_FLAGS__U__MASK)) != 0);

		SG_ERR_CHECK(  SG_rbtree__find(pCtx, pData->prbSync, pszGid_k, &bFound, (void **)&pFold)  );
		if (!bFound && !bChanged)		// the item is identical in all 4 parts.
			continue;

		if (!bFound)
			SG_ERR_CHECK(  _FOLDED_ITEM__ALLOC(pCtx, pData, pszGid_k, sfW, &pFold)  );

		pFold->sfW = sfW;

		SG_ERR_CHECK(  _folded_item__insert_sub_if_exists(pCtx, pFold, pvhItem_k, SG_WC__STATUS_SUBSECTION__B, BIT_B)  );
		SG_ERR_CHECK(  _folded_item__insert_sub_if_exists(pCtx, pFold, pvhItem_k, SG_WC__STATUS_SUBSECTION__WC, BIT_M)  );

		if ((pFold->knownBits & BIT_A) == 0)
		{
			// if we did not visit this item in one of the above
			// A-vs-L* loops, we don't have an A subsection in
			// the result.  (Which means that A, B, and C are all
			// identical.)  Synthesize one using B as a model.

			SG_ERR_CHECK(  _synthesize_subsection__a_from_b(pCtx, pData, pFold, pszGid_k)  );
			SG_ERR_CHECK(  _synthesize_subsection__c_from_a(pCtx, pData, pFold, pszGid_k)  );
		}

		SG_ASSERT(  (pFold->knownBits == (BIT_A|BIT_B|BIT_C|BIT_M))  );
	}

	//////////////////////////////////////////////////////////////////
	// Compute the composite status/properties
	// and fill in the remaining details in the
	// result item.
	//////////////////////////////////////////////////////////////////

	SG_ERR_CHECK(  SG_rbtree__foreach(pCtx, pData->prbSync, &cbSync, pData)  );

fail:
	;
}

//////////////////////////////////////////////////////////////////

/**
 * Drive the main task in an MSTATUS.
 *
 */
void sg_wc_tx__mstatus__main(SG_context * pCtx,
							 SG_wc_tx * pWcTx,
							 SG_bool bNoIgnores,
							 SG_varray * pvaStatusResult,
							 SG_vhash ** ppvhLegend)
{
	struct _mstatus_data data;

	// ppvhLegend is optional

	memset(&data, 0, sizeof(data));
	data.pWcTx = pWcTx;
	data.bNoIgnores = bNoIgnores;
	data.pvaStatusResult = pvaStatusResult;
	data.pszWasLabel_A = "Ancestor (A)";
	data.pszWasLabel_B = "Baseline (B)";
	data.pszWasLabel_C = "Other (C)";
	data.pszWasLabel_WC= "Working";

	if (ppvhLegend)
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &data.pvhLegend)  );
	
	SG_ERR_CHECK(  _mstatus__get_csets(pCtx, &data)  );
	SG_ERR_CHECK(  _mstatus__sync_up(pCtx, &data)  );

	if (ppvhLegend)
	{
		*ppvhLegend = data.pvhLegend;
		data.pvhLegend = NULL;
	}

fail:
	SG_VHASH_NULLFREE(pCtx, data.pvhCSets);
	SG_VHASH_NULLFREE(pCtx, data.pvhLegend);
	SG_VARRAY_NULLFREE(pCtx, data.pvaStatus_B_vs_WC);
	SG_VARRAY_NULLFREE(pCtx, data.pvaStatus_A_vs_B);
	SG_VARRAY_NULLFREE(pCtx, data.pvaStatus_A_vs_C);
	SG_WC_DB__CSET_ROW__NULLFREE(pCtx, data.pCSetRow_C);
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, data.prbSync, (SG_free_callback *)_folded_item__free);
}
