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
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

#if 1 && defined(DEBUG)
#define DEBUG_INCLUDE_CONFLICT_REASONS
#endif

#if defined(DEBUG_INCLUDE_CONFLICT_REASONS)
/**
 * Build a VARRAY of conflict reason messages.
 * Returns NULL when there are no reasons.
 *
 * TODO do we want this to be debug only?
 */
static void _make_conflict_reasons(SG_context * pCtx, SG_varray ** ppvaConflictReasons, SG_mrg_cset_entry_conflict_flags flagsConflict)
{
	SG_varray * pva = NULL;

	if (flagsConflict)
	{
		SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );

#define M(f,bit,msg)	SG_STATEMENT( if ((f) & (bit)) SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx,pva,(msg))  ); )

		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_MOVE,						"REMOVE-vs-MOVE" );
		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_RENAME,					"REMOVE-vs-RENAME" );
		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_ATTRBITS,					"REMOVE-vs-ATTRBITS_CHANGE" );
		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SYMLINK_EDIT,				"REMOVE-vs-SYMLINK_EDIT" );
		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_FILE_EDIT,					"REMOVE-vs-FILE_EDIT" );
		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_VS_SUBMODULE_EDIT,			"REMOVE-vs-SUBMODULE_EDIT" );

		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DELETE_CAUSED_ORPHAN,				"REMOVE-CAUSED-ORPHAN" );
		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE,				"MOVES-CAUSED-PATH_CYCLE" );

		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE,						"DIVERGENT-MOVE" );
		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME,					"DIVERGENT-RENAME" );
		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_ATTRBITS,					"DIVERGENT-ATTRBITS_CHANGE" );
		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SYMLINK_EDIT,				"DIVERGENT-SYMLINK_EDIT" );
		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SUBMODULE_EDIT,			"DIVERGENT-SUBMODULE_EDIT" );

		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__TBD,			"DIVERGENT-FILE_EDIT_TBD" );
		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__NO_RULE,		"DIVERGENT-FILE_EDIT_NO_RULE" );
		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_CONFLICT,	"DIVERGENT-FILE_EDIT_AUTO_CONFLICT" );
		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_ERROR,		"DIVERGENT-FILE_EDIT_AUTO_ERROR" );
		M(flagsConflict, SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_OK,		"DIVERGENT-FILE_EDIT_AUTO_MERGED" );

#undef M
	}

	*ppvaConflictReasons = pva;
	return;

fail:
	SG_VARRAY_NULLFREE(pCtx, pva);
}
#endif

//////////////////////////////////////////////////////////////////

static void _make_undeletes(SG_context * pCtx,
							SG_varray ** ppvaConflictUndeletes,
							SG_mrg_cset_entry * pMrgCSetEntry_FinalResult)
{
	SG_varray * pva = NULL;
	SG_uint32 k, kLimit;
	const SG_vector * pVec = pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict->pVec_MrgCSet_Deletes;

	if (pVec)
	{
		SG_ERR_CHECK(  SG_vector__length(pCtx, pVec, &kLimit)  );
		if (kLimit > 0)
		{
			SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );

			for (k=0; k<kLimit; k++)
			{
				SG_mrg_cset * pMrgCSet_k;

				SG_ERR_CHECK(  SG_vector__get(pCtx, pVec,k, (void **)&pMrgCSet_k)  );
				SG_ASSERT(  (pMrgCSet_k->bufHid_CSet[0] != 0)  );		// hid is only set for actual branch/leaf csets; it is null for merge-results.
				SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva, pMrgCSet_k->bufHid_CSet)  );
			}
		}
	}

	*ppvaConflictUndeletes = pva;
	return;

fail:
	SG_VARRAY_NULLFREE(pCtx, pva);
}

//////////////////////////////////////////////////////////////////

/**
 * Used to display conflict details when the list of candidates are given
 * by keys in the given rbUnique *and* the associated values are a vector
 * of entries that have that key (whatever it may be).  This type of rbUnique
 * is used to support a many-to-one relationship.  
 *
 * We build a VHASH[<key> --> VARRAY[]]
 *
 * For example, if branch L0 (with hid0) renames "foo" to "foo.0" and the
 * branches L1 (hid1) and L2 (hid2) rename "foo" to "foo.x", we need to
 * create (handwave):
 *     {
 *       "foo.x" : [ "hid1", "hid2" ],
 *       "foo.0" : [ "hid0" ],
 *     }
 */
static void _make_rbUnique(SG_context * pCtx,
						   SG_vhash ** ppvhDivergent,
						   SG_rbtree * prbUnique)
{
	SG_vhash * pvhDivergent = NULL;
	SG_varray * pvaDuplicates_k = NULL;
	SG_rbtree_iterator * pIter = NULL;

	if (prbUnique)
	{
		const char * pszKey_k;
		const SG_vector * pVec_k;
		SG_bool bFound;

		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhDivergent)  );

		SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIter, prbUnique, &bFound, &pszKey_k, (void **)&pVec_k)  );
		while (bFound)
		{
			SG_uint32 nrDuplicates_k;
			const SG_mrg_cset_entry * pMrgCSetEntry_j;
			SG_uint32 j;

			SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaDuplicates_k)  );

			SG_ERR_CHECK(  SG_vector__length(pCtx, pVec_k, &nrDuplicates_k)  );
			for (j=0; j<nrDuplicates_k; j++)
			{
				SG_ERR_CHECK(  SG_vector__get(pCtx, pVec_k, j, (void **)&pMrgCSetEntry_j)  );
				SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pvaDuplicates_k, pMrgCSetEntry_j->pMrgCSet->bufHid_CSet)  );
			}

			SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhDivergent, pszKey_k, &pvaDuplicates_k)  );	// this steals our VARRAY

			SG_ERR_CHECK(  SG_rbtree__iterator__next(pCtx, pIter, &bFound, &pszKey_k, (void **)&pVec_k)  );
		}
	}

	*ppvhDivergent = pvhDivergent;
	pvhDivergent = NULL;

fail:
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);
	SG_VARRAY_NULLFREE(pCtx, pvaDuplicates_k);
	SG_VHASH_NULLFREE(pCtx, pvhDivergent);
}

/**
 * Use to build a varray of GIDs when given a vector of SG_mrg_cset_entry's.
 */
static void _make_gid_array__vec_mrg_cset_entry(SG_context * pCtx,
												SG_varray ** ppva,
												SG_vector * pVec_MrgCSetEntry)
{
	SG_varray * pva = NULL;
	SG_uint32 k, count;

	SG_ERR_CHECK(  SG_vector__length(pCtx, pVec_MrgCSetEntry, &count)  );
	SG_ASSERT(  (count > 0)  );

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pva)  );
	for (k=0; k<count; k++)
	{
		SG_mrg_cset_entry * pMrgCSetEntry_k;

		SG_ERR_CHECK(  SG_vector__get(pCtx, pVec_MrgCSetEntry, k, (void **)&pMrgCSetEntry_k)  );
		SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pva, pMrgCSetEntry_k->bufGid_Entry)  );
	}

	*ppva = pva;
	return;

fail:
	SG_VARRAY_NULLFREE(pCtx, pva);
}

//////////////////////////////////////////////////////////////////

/**
 * Record the given temp-file pathname in the Issue.
 * We convert this absolute path (which points to a
 * file in <sgtemp>) to a "repo-path" because the user
 * could move their entire working directory between
 * the MERGE and RESOLVE and/or COMMIT.
 *
 * TODO 2012/01/25 This temp-file (which is inside
 * TODO            .sgdrawer somewhere) is *NOT* under version control
 * TODO            and so the repo-path is syntax-only.  Consider using
 * TODO            the extended-prefix stuff to make that distinction
 * TODO            more clear.
 * TODO
 * TODO            For example, maybe use '@x/' domain.
 *
 */
static void _store_temp_path(SG_context * pCtx,
							 SG_mrg * pMrg,
							 SG_vhash * pvh,
							 const char * pszKey,
							 const SG_pathname * pPath)
{
	SG_string * pStringRepoPath = NULL;

	SG_ERR_CHECK(  SG_workingdir__wdpath_to_repopath(pCtx,
													 pMrg->pWcTx->pDb->pPathWorkingDirectoryTop,
													 pPath,
													 SG_FALSE,
													 &pStringRepoPath)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, pszKey, SG_string__sz(pStringRepoPath))  );

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
}

//////////////////////////////////////////////////////////////////

/**
 * Document the various fields of an entry AS THEY EXISTED IN A PARTICULAR CSET.
 */
static void _doc_input_version_detail(SG_context * pCtx,
									  SG_mrg * pMrg,
									  const SG_mrg_cset_entry * pMrgCSetEntry,
									  const SG_pathname * pPathTempFile,
									  SG_vhash ** ppvhDetail)
{
	SG_vhash * pvh = NULL;
	SG_vhash * pvhProperties = NULL;

	// Create:
	//    <details> ::=  { "hid"        : "<hid>",
	//                     "entryname"  : "<entryname>",
	//                     "gid_parent" : "<gid_parent>",
	//                     "attrbits"   : "<attrbits>",
	//                     "accept_label" : "<accept_label>",
	//                     "cset_hid"   : "<hid_cset>",			-- when defined (inputs)
	//                     "status"     : <properties>,			-- only defined for __using_wc() branch
	//                     "tempfile"   : "<repo_path_of_merge_temp_file>",	 -- when defined
	//                     }

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "hid",        pMrgCSetEntry->bufHid_Blob)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "entryname",  SG_string__sz(pMrgCSetEntry->pStringEntryname))  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "gid_parent", pMrgCSetEntry->bufGid_Parent)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvh, "attrbits",   pMrgCSetEntry->attrBits)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "accept_label", SG_string__sz(pMrgCSetEntry->pMrgCSet->pStringAcceptLabel))  );

	if (pMrgCSetEntry->pMrgCSet->bufHid_CSet[0])
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "cset_hid", pMrgCSetEntry->pMrgCSet->bufHid_CSet)  );
	if (pMrgCSetEntry->statusFlags__using_wc)
	{
		// We only report the status of the WC/L0 leg of the merge/update.
		// We want to know if the __using_wc() version is clean/dirty WRT
		// the official L0 cset.  I'm not sure if we care what specifically
		// has changed, but rather we want to be able to report that it is
		// dirty (not unlike how editors put a star next to the file name
		// when you have unsaved changes).
		SG_ERR_CHECK(  SG_wc__status__flags_to_properties(pCtx, pMrgCSetEntry->statusFlags__using_wc, &pvhProperties)  );
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh, "status", &pvhProperties)  );
	}

	if (pPathTempFile)
		SG_ERR_CHECK(  _store_temp_path(pCtx, pMrg, pvh, "tempfile",  pPathTempFile)  );

	*ppvhDetail = pvh;
	pvh = NULL;

fail:
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_VHASH_NULLFREE(pCtx, pvhProperties);
}

static void _doc_input_version(SG_context * pCtx,
							   SG_vhash * pvhInputs,
							   SG_mrg * pMrg,
							   const SG_mrg_cset_entry * pMrgCSetEntry,
							   const SG_pathname * pPathTempFile)
{
	SG_vhash * pvhDetail = NULL;

	// Add the following to the caller's vhash:
	//    <input_k> ::= "<mnemonic_cset_k>" : <details_in_cset_k>,

	SG_ERR_CHECK(  _doc_input_version_detail(pCtx, pMrg, pMrgCSetEntry, pPathTempFile, &pvhDetail)  );
	
	SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx,
										pvhInputs,
										pMrgCSetEntry->pMrgCSet->pszMnemonicName,
										&pvhDetail)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pvhDetail);
}

/**
 * Document the various fields of the entry as they existed in the
 * ancestor and all of the leaves (where it wasn't deleted).
 *
 * That is, the value of ancestor/baseline/other* as it WOULD HAVE
 * BEEN SEEEN in that CSET.  This is the historical INPUT data.  It
 * may or may not reflect the state of the entry after MERGE does
 * what it needs to do to the entry.  This info should only be used
 * to print nice headers for the user when they are helping to resolve
 * a conflict.
 * 
 * TODO 2012/09/21 I think this is old: We're doing it the hardway (by looking up the entry in each
 * TODO                                 CSET) rather than just using the info in a "conflict" structure
 * TODO                                 so that we can give RESOLVE the info for collisions too.
 */
static void _sg_mrg__prepare_issue__hardway__document_inputs(SG_context * pCtx,
															 SG_mrg * pMrg,
															 SG_mrg_cset_entry * pMrgCSetEntry_FinalResult,
															 SG_vhash * pvhInputs)
{
	const char * pszGid = pMrgCSetEntry_FinalResult->bufGid_Entry;
	SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict = pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict;
	const SG_mrg_cset_entry * pMrgCSetEntry_k;
	SG_bool bFiles = SG_FALSE;
	SG_bool bFound;

	if (pMrgCSetEntryConflict)
		if (pMrgCSetEntryConflict->flags & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__MASK)
			bFiles = SG_TRUE;

	// Add the following to the caller's vhash:
	//
	// <inputs> ::= <input_ancestor> <input_L0> <input_L1> ...

	// TODO 2012/09/21 Now that we can do per-file ancestors, we need to be
	// TODO            a bit careful in how we refer to it.
	// TODO
	// TODO            I think we should be able to get rid of the pMrg->pMrgCSet_Ancestor
	// TODO            based lookups and always use the fields from the conflict.
	// TODO
	// TODO            Likewise, I think we can get rid of the pMrg->pMrgCSet_Baseline based
	// TODO            lookups and use the fields from the conflict.
	// TODO
	// TODO            But I need to look at the COLLISION stuff before doing this.

	if (pMrgCSetEntryConflict && pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor)
		SG_ERR_CHECK(  _doc_input_version(pCtx, pvhInputs, pMrg, pMrgCSetEntryConflict->pMrgCSetEntry_Ancestor,
										  ((bFiles) ? pMrgCSetEntryConflict->pPathTempFile_Ancestor : NULL))  );
	else if (pMrg->pMrgCSet_LCA && pMrg->pMrgCSet_LCA->prbEntries)
	{
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, pMrg->pMrgCSet_LCA->prbEntries, pszGid, &bFound, (void **)&pMrgCSetEntry_k)  );
		if (bFound)
			SG_ERR_CHECK(  _doc_input_version(pCtx, pvhInputs, pMrg, pMrgCSetEntry_k,
										((bFiles) ? pMrgCSetEntryConflict->pPathTempFile_Ancestor : NULL))  );
	}

	if (pMrg->pMrgCSet_Baseline && pMrg->pMrgCSet_Baseline->prbEntries)
	{
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, pMrg->pMrgCSet_Baseline->prbEntries, pszGid, &bFound, (void **)&pMrgCSetEntry_k)  );
		if (bFound)
			SG_ERR_CHECK(  _doc_input_version(pCtx, pvhInputs, pMrg, pMrgCSetEntry_k,
										((bFiles) ? pMrgCSetEntryConflict->pPathTempFile_Baseline : NULL))  );
	}

	if (pMrg->pMrgCSet_Other && pMrg->pMrgCSet_Other->prbEntries)
	{
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, pMrg->pMrgCSet_Other->prbEntries, pszGid, &bFound, (void **)&pMrgCSetEntry_k)  );
		if (bFound)
			SG_ERR_CHECK(  _doc_input_version(pCtx, pvhInputs, pMrg, pMrgCSetEntry_k,
											  ((bFiles) ? pMrgCSetEntryConflict->pPathTempFile_Other : NULL))  );
	}

	// TODO 2012/01/25 Consider dumping details for SPCA versions of this item too.
	// TODO
	// TODO 2012/09/21 Forget about this (in favor of the new per-file ancestor stuff).

fail:
	return;
}

/**
 * Document the various fields as they existed in all of the (non-deleted)
 * inputs for this entry (without regard for whether or not we have a "conflict"
 * structure for the entry).
 */
static void _sg_mrg__prepare_issue__document_inputs(SG_context * pCtx,
													SG_mrg * pMrg,
													SG_mrg_cset_entry * pMrgCSetEntry_FinalResult,
													SG_vhash * pvhIssue)
{
	SG_vhash * pvhInputs = NULL;

	// Add the following to the issue:
	//
	// "input" : <inputs>

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhInputs)  );
	SG_ERR_CHECK(  _sg_mrg__prepare_issue__hardway__document_inputs(pCtx,
																	pMrg,
																	pMrgCSetEntry_FinalResult,
																	pvhInputs)  );
	SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhIssue, "input", &pvhInputs)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pvhInputs);
}

//////////////////////////////////////////////////////////////////

static void _doc_output_version_detail(SG_context * pCtx,
									   SG_mrg * pMrg,
									   const SG_mrg_cset_entry * pMrgCSetEntry,
									   const SG_pathname * pPathTempFile,
									   SG_vhash ** ppvhDetail)
{
	SG_vhash * pvh = NULL;
	SG_vhash * pvhProperties = NULL;

	// Create:
	//    <details> ::=  { "entryname"    : "<entryname>",
	//                     "accept_label" : "<accept_label>",
	//                     "tempfile"     : "<repo_path_of_merge_temp_file>",	 -- when defined
	//                     }

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "entryname",  SG_string__sz(pMrgCSetEntry->pStringEntryname))  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh, "accept_label", SG_string__sz(pMrgCSetEntry->pMrgCSet->pStringAcceptLabel))  );
	if (pPathTempFile)
		SG_ERR_CHECK(  _store_temp_path(pCtx, pMrg, pvh, "tempfile",  pPathTempFile)  );

	*ppvhDetail = pvh;
	pvh = NULL;

fail:
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_VHASH_NULLFREE(pCtx, pvhProperties);
}

/**
 * Document the various fields as they existed in the entry in the computed
 * final result cset.  This is a summary of what we did to the entry and
 * the (arbitrary) choices that we made to get the WD populated with the
 * merge result.
 */
static void _sg_mrg__prepare_issue__document_result(SG_context * pCtx,
													SG_mrg * pMrg,
													SG_mrg_cset_entry * pMrgCSetEntry_FinalResult,
													SG_vhash * pvhIssue)
{
	SG_vhash * pvhDetail = NULL;
	SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict = pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict;
	SG_bool bFiles = SG_FALSE;

	if (pMrgCSetEntryConflict)
		if (pMrgCSetEntryConflict->flags & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__MASK)
			bFiles = SG_TRUE;

	// Add the following to the issue:
	//
	// "output" : <details_for_result>

	SG_ERR_CHECK(  _doc_output_version_detail(pCtx, pMrg, pMrgCSetEntry_FinalResult,
											  ((bFiles) ? pMrgCSetEntryConflict->pPathTempFile_Result : NULL),
											  &pvhDetail)  );
	SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhIssue, "output", &pvhDetail)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pvhDetail);
}

//////////////////////////////////////////////////////////////////

/**
 * We get called once for each entry in the final-result.
 * We get called in RANDOM GID order.
 *
 * If this entry has any kind of a conflict (conflict, collision
 * or portability), write the details to the ISSUES.
 * 
 */
static SG_rbtree_foreach_callback _sg_mrg__prepare_issues__cb;

static void _sg_mrg__prepare_issues__cb(SG_context * pCtx,
										const char * pszKey_GidEntry,
										void * pVoidValue_MrgCSetEntry,
										void * pVoidData_Mrg)
{
	SG_mrg_cset_entry * pMrgCSetEntry_FinalResult = (SG_mrg_cset_entry *)pVoidValue_MrgCSetEntry;
	SG_mrg * pMrg = (SG_mrg *)pVoidData_Mrg;
	SG_vhash * pvhIssue = NULL;
	SG_varray * pva = NULL;
	SG_vhash * pvh = NULL;
	SG_rbtree_iterator * pIter = NULL;
	SG_mrg_cset_entry_conflict_flags flagsConflict;
	SG_wc_port_flags flagsPortability;
	SG_bool bHaveConflicts, bHaveCollision, bHavePortability, bHaveIssues;
	SG_wc_status_flags statusFlags_x_xr_xu = SG_WC_STATUS_FLAGS__ZERO;
	SG_bool bHaveLVI;
	sg_wc_liveview_item * pLVI = NULL;	// we don't own this

	SG_ASSERT(  (strcmp(pszKey_GidEntry, pMrgCSetEntry_FinalResult->bufGid_Entry) == 0)  );

	flagsConflict = ((pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict)
					 ? pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict->flags
					 : SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__ZERO);
	bHaveConflicts = (flagsConflict != SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__ZERO);

	bHaveCollision = (pMrgCSetEntry_FinalResult->pMrgCSetEntryCollision != NULL);

	flagsPortability = pMrgCSetEntry_FinalResult->portFlagsObserved;
	bHavePortability = (flagsPortability != SG_WC_PORT_FLAGS__NONE);

	bHaveIssues = (bHaveConflicts || bHaveCollision || bHavePortability);
	if (!bHaveIssues)
	{
#if 0 && TRACE_WC_MERGE
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "PrepareIssues: No issues for '%s'\n",
								   pszKey_GidEntry)  );
#endif
		return;
	}

	//////////////////////////////////////////////////////////////////
	// We need to write an ISSUE for this ITEM.
	//////////////////////////////////////////////////////////////////

	// begin an ISSUE:
	// {
	//    "gid"                      : "<gid>",
	//    "tne_type"                 : <tne_type>,
	//
	//    "conflict_flags"           : <conflict_flags>,
	//    "collision_flags"          : <collision_flags>,
	//    "portability_flags"        : <portability_flags>,
	//    "restore_original_name"    : <entryname>,
	//    "reference_original_name"  : <entryname>,

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhIssue)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhIssue, "gid",               pszKey_GidEntry)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhIssue, "tne_type",          pMrgCSetEntry_FinalResult->tneType)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhIssue, "conflict_flags",    flagsConflict)  );	// don't include __AUTO_OK bit
	SG_ERR_CHECK(  SG_vhash__add__bool(      pCtx, pvhIssue, "collision_flags",   bHaveCollision)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(     pCtx, pvhIssue, "portability_flags", flagsPortability)  );

	if (pMrgCSetEntry_FinalResult->pStringEntryname_OriginalBeforeMadeUnique)
	{
		// We gave this item a temporary name "<entryname>~<gid7>" because
		// of a collision or some kind of conflict.
		//
		// Put the reference/unmangled name in "reference_original_name".

		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhIssue, "reference_original_name",
												 SG_string__sz(pMrgCSetEntry_FinalResult->pStringEntryname_OriginalBeforeMadeUnique))  );

		if (flagsConflict & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME)
		{
			// When we have a divergent-rename, RESOLVE will have to ask the
			// user to choose what the right name should be, so we don't request
			// that RESOLVE implicitly restore it back to the unmangled name.
		}
		else
		{
			// Otherwise (we created the temp name as a convenience for us) and
			// RESOLVE should automatically restore/rename the item to this name
			// whenever the other (non-name) conflict(s) are handled.

			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhIssue, "restore_original_name",
													 SG_string__sz(pMrgCSetEntry_FinalResult->pStringEntryname_OriginalBeforeMadeUnique))  );
		}
	}

	// add "inputs" data to the issue.

	SG_ERR_CHECK(  _sg_mrg__prepare_issue__document_inputs(pCtx,
														   pMrg,
														   pMrgCSetEntry_FinalResult,
														   pvhIssue)  );

	// add an "output" vhash with all of the details we have for the final result
	// as MERGE computed and updated set them in the WD.
	// WARNING: these values are ONLY CURRENT AS OF THE END OF THE MERGE.  These
	// WARNING: may be useful for RESOLVE to display where we put something, but
	// WARNING: it doesn't mean that RESOLVE can rely on them because the user could
	// WARNING: have already changed something.

	SG_ERR_CHECK(  _sg_mrg__prepare_issue__document_result(pCtx,
														   pMrg,
														   pMrgCSetEntry_FinalResult,
														   pvhIssue)  );

	if (bHaveConflicts)
	{
#if defined(DEBUG_INCLUDE_CONFLICT_REASONS)
		// add an array of messages for each bit in conflict_flags.
		// this is mostly for display purposes; use conflict_flags in code.
		//
		//    "conflict_reasons" : [ "<name1>", "<name2>", ... ],

		SG_ERR_CHECK(  _make_conflict_reasons(pCtx, &pva, flagsConflict)  );
		SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhIssue, "conflict_reasons", &pva)  );
#endif

		if (flagsConflict & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__UNDELETE__MASK)
		{
			// entry was deleted in one or more branches and modified/needed in other branch(es).
			// we treat all the various way this can happen the same way since we don't really
			// care whether it was a __DELETE_VS_MOVE or a __DELETE_VS_RENAME.  all that really
			// matters (to the RESOLVER) is whether to keep the entry (as modified) or to delete
			// it from the final merge result.
			//
			// as a convenience to the user (and mostly for display purposes), we list the
			// branch(es) in which it was deleted.
			//
			// "conflict_deletes" : [ "<hid_cset_k>", ... ],
			//
			// TODO 2012/05/23 decide if we actually need to create this field (resolve doesn't use it).
			// TODO            if so, put the cset mnemonics in the array instead of the HIDs.

			SG_ERR_CHECK(  _make_undeletes(pCtx, &pva, pMrgCSetEntry_FinalResult)  );
			SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhIssue, "conflict_deletes", &pva)  );

			statusFlags_x_xr_xu |= SG_WC_STATUS_FLAGS__XU__EXISTENCE;
		}

		if (flagsConflict & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__MOVES_CAUSED_PATH_CYCLE)
		{
			// Give some hints about the other dirs involved in the cycle.
			// The HINT string uses the entrynames as they WANTED TO APPEAR
			// in the merge result.  This is info-only (because MERGE had to
			// arbitrarily put each of the dirs somewhere (probably where it
			// was in the baseline).
			//
			// Use the GIDs in the OTHERS array if you want to determine where
			// they actually are on disk after the MERGE.
			//
			// "conflict_path_cycle_hint" : ".../<name_this>/.../<dir_other>/.../<name_this>"
			// "conflict_path_cycle_others" : [ "<gid_other_1>", "<gid_other_2>", ... ],

			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx,
													 pvhIssue,
													 "conflict_path_cycle_hint",
													 SG_string__sz(pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict->pStringPathCycleHint))  );

			SG_ERR_CHECK(  _make_gid_array__vec_mrg_cset_entry(pCtx,
															   &pva,
															   pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict->pVec_MrgCSetEntry_OtherDirsInCycle)  );
			SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhIssue, "conflict_path_cycle_others", &pva)  );	// this steals our varray

			statusFlags_x_xr_xu |= SG_WC_STATUS_FLAGS__XU__LOCATION;
		}

		if (flagsConflict & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_MOVE)
		{
			// we DO NOT bother to list the repo-paths of the various parent directory
			// choices since these may be invalid by the time the user is ready to
			// resolve this issue.  the resolver should use the gid-parent-x and the
			// pendingtree/WD to find the actual location of the parent directory at
			// the time of the resovlve.
			//
			// "conflict_divergent_moves" : { "<gid_parent_1>" : [ "<hid_cset_j>", ... ],
			//                                "<gid_parent_2>" : [ "<hid_cset_k>", ... ],
			//                                ... },
			//
			// TODO 2012/05/23 decide if we actually need to create this field (resolve doesn't use it).
			// TODO            if so, put the cset mnemonics in the array instead of the HIDs.

			SG_ERR_CHECK(  _make_rbUnique(pCtx, &pvh, pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict->prbUnique_GidParent)  );
			SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhIssue, "conflict_divergent_moves", &pvh)  );

			statusFlags_x_xr_xu |= SG_WC_STATUS_FLAGS__XU__LOCATION;
		}

		if (pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict->flags & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_RENAME)
		{
			// "conflict_divergent_renames" : { "<name_1>" : [ "<hid_cset_j>", ... ],
			//                                  "<name_2>" : [ "<hid_cset_k>", ... ],
			//                                  ... },
			//
			// TODO 2012/05/23 decide if we actually need to create this field (resolve doesn't use it).
			// TODO            if so, put the cset mnemonics in the array instead of the HIDs.

			SG_ERR_CHECK(  _make_rbUnique(pCtx, &pvh, pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict->prbUnique_Entryname)  );
			SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhIssue, "conflict_divergent_renames", &pvh)  );

			statusFlags_x_xr_xu |= SG_WC_STATUS_FLAGS__XU__NAME;
		}

		if (flagsConflict & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_ATTRBITS)
		{
			// "conflict_divergent_attrbits" : { "<attrbits_1>" : [ "<hid_cset_j>", ... ],
			//                                   "<attrbits_2>" : [ "<hid_cset_k>", ... ],
			//                                   ... },
			//
			// TODO 2012/05/23 decide if we actually need to create this field (resolve doesn't use it).
			// TODO            if so, put the cset mnemonics in the array instead of the HIDs.

			SG_ERR_CHECK(  _make_rbUnique(pCtx, &pvh, pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict->prbUnique_AttrBits)  );
			SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhIssue, "conflict_divergent_attrbits", &pvh)  );

			statusFlags_x_xr_xu |= SG_WC_STATUS_FLAGS__XU__ATTRIBUTES;
		}

		if (flagsConflict & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SYMLINK_EDIT)
		{
			// "conflict_divergent_hid_symlink" : { "<hid_blob_1>" : [ "<hid_cset_j>", ... ],
			//                                      "<hid_blob_2>" : [ "<hid_cset_k>", ... ],
			//                                      ... },
			//
			// TODO 2010/05/20 I'm putting the HIDs of the symlink content blob.  This is not
			// TODO            very display friendly.  Think about including the link target
			// TODO            in the ISSUE.
			//
			// TODO 2012/05/23 decide if we actually need to create this field (resolve doesn't use it).
			// TODO            if so, put the cset mnemonics in the array instead of the HIDs.

			SG_ERR_CHECK(  _make_rbUnique(pCtx, &pvh, pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict->prbUnique_Symlink_HidBlob)  );
			SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhIssue, "conflict_divergent_hid_symlink", &pvh)  );

			statusFlags_x_xr_xu |= SG_WC_STATUS_FLAGS__XU__CONTENTS;
		}

#if 0 // TODO 2012/01/30 deal with sub-modules aka subrepos
		if (flagsConflict & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_SUBMODULE_EDIT)
		{
			// "conflict_divergent_hid_submodule" : { "<hid_blob_1>" : [ "<hid_cset_j>", ... ],
			//                                        "<hid_blob_2>" : [ "<hid_cset_k>", ... ],
			//                                        ... },
			//
			// TODO 2011/02/14 I'm putting the HIDs of the submodule content blob.  This is not
			// TODO            very display friendly.
			//
			// TODO 2012/05/23 decide if we actually need to create this field (resolve doesn't use it).
			// TODO            if so, put the cset mnemonics in the array instead of the HIDs.

			SG_ERR_CHECK(  _make_rbUnique(pCtx, &pvh, pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict->prbUnique_Submodule_HidBlob)  );
			SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhIssue, "conflict_divergent_hid_submodule", &pvh)  );

			statusFlags_x_xr_xu |= SG_WC_STATUS_FLAGS__XU__CONTENTS;
		}
#endif

		if (flagsConflict & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__MASK)
		{
			// We write the following info for file-content conflicts (any time the
			// file was modified in both branches whether auto-mergeable or not).
			// 
			// "repopath_tempdir"            : "<repo_path_of_temp_dir_for_this_file>",
			// "automerge_mergetool"         : "<merge_tool_name>",  // omitted if auto-merge disabled or no rule.
			// "automerge_generated_hid"     : "<generated_hid>",    // omitted if auto-merge did not generate a result file.
			// "automerge_disposable_hid"    : "<disposable_hid>",   // omitted if auto-merge did not generate a result file.

			SG_mrg_cset_entry_conflict * pMrgCSetEntryConflict = pMrgCSetEntry_FinalResult->pMrgCSetEntryConflict;

			SG_ERR_CHECK(  _store_temp_path(pCtx, pMrg, pvhIssue, "repopath_tempdir",  pMrgCSetEntryConflict->pPathTempDirForFile)  );

			if (pMrgCSetEntryConflict->pMergeTool)
			{
				const char* szName = NULL;

				SG_ERR_CHECK(  SG_filetool__get_name(pCtx, pMrgCSetEntryConflict->pMergeTool, &szName)  );
				SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhIssue, "automerge_mergetool", szName)  );
			}
			if (pMrgCSetEntryConflict->pszHidGenerated)
			{
				// If the auto-merge tool ran and created *ANYTHING* (good or bad) we
				// set this field.
				SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhIssue, "automerge_generated_hid",
														 pMrgCSetEntryConflict->pszHidGenerated)  );
			}
			if (pMrgCSetEntryConflict->pszHidDisposable)
			{
				// If the auto-merge tool ran and created anything (good or bad) *AND*
				// the tool is fully automatic, we also set this field.
				SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhIssue, "automerge_disposable_hid",
														 pMrgCSetEntryConflict->pszHidDisposable)  );
			}

			if (flagsConflict & SG_MRG_CSET_ENTRY_CONFLICT_FLAGS__DIVERGENT_FILE_EDIT__AUTO_OK)
				statusFlags_x_xr_xu |= SG_WC_STATUS_FLAGS__XR__CONTENTS;
			else
				statusFlags_x_xr_xu |= SG_WC_STATUS_FLAGS__XU__CONTENTS;
		}
	}

	if (bHaveCollision)
	{
		// list the set of entries (including this entry) fighting for this entryname in this directory.
		// we only list the GIDs of the entries; we assume that each of them will have their own ISSUE
		// reported (with another copy of this list).  we CANNOT give the CSET HID for the leaf which
		// made the change that caused the collision to happen because we do not know it (and it may be
		// the result of combination of factors).
		//
		//     "collisions" : [ "<gid_1>", "<gid_2>", ... ],

		SG_ASSERT(  (pMrgCSetEntry_FinalResult->pMrgCSetEntryCollision->pVec_MrgCSetEntry)  );
		SG_ERR_CHECK(  _make_gid_array__vec_mrg_cset_entry(pCtx,
														   &pva,
														   pMrgCSetEntry_FinalResult->pMrgCSetEntryCollision->pVec_MrgCSetEntry)  );
		SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhIssue, "collisions", &pva)  );	// this steals our varray
	}

	if (bHavePortability)
	{
		// If present and non-empty, add the entry's portability-log to the ISSUE.
		// I'm thinking that this is primarily for testing/debug/etc, but it may
		// turn out to be useful to RESOLVE.
		//
		//     "portability_log" : <string>
		//
		// TODO 2012/01/25 Think about whether we want to keep this or maybe
		// TODO            make it debug only.

		if (pMrgCSetEntry_FinalResult->pStringPortItemLog)
		{
			const char * psz = SG_string__sz(pMrgCSetEntry_FinalResult->pStringPortItemLog);
			if (psz[0])
				SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhIssue, "portability_log", psz)  );
		}

		// If there were potential collisions with this item, add the
		// list of the GIDs of the other items.
		//
		// TODO 2012/08/23 Do we care if this item is not in the set and/or
		// TODO            that we do put it in for hard collisions above?
		//
		//      "portability_collisions" : [ "<gid_1>", "<gid_2>", ... ],

		if (pMrgCSetEntry_FinalResult->pvaPotentialPortabilityCollisions)
		{
			SG_ERR_CHECK(  SG_VARRAY__ALLOC__COPY(pCtx, &pva, pMrgCSetEntry_FinalResult->pvaPotentialPortabilityCollisions)  );
			SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhIssue, "portability_collisions", &pva)  );	// this steals our varray
		}
	}

	if (bHaveCollision || bHavePortability)
	{
		// We have a hard- or potential-collision.
		// It is not that this item (by itself) has multiple answers, but
		// rather that this item bumps into another item.  We want to try
		// to mark this item with a choice relecting what happened to this
		// item and not blindly set all of the choice bits on it.  For
		// example, if this item was MOVED and as a result it will/would
		// collide with another item that was CREATED or RENAMED and/or MOVED
		// to the same spot, then we only want to set a LOCATION choice
		// on this item.  (When we look at the other item we may set
		// EXISTENCE, NAME, LOCATION, or NAME+LOCATION on it, but that is
		// independent of what we set on this item.)

		SG_mrg_cset_entry * pMrgCSetEntry_Baseline = NULL;
		SG_mrg_cset_entry * pMrgCSetEntry_Other = NULL;
		SG_mrg_cset_entry_neq neq = 0;
		SG_bool bFoundBaseline = SG_FALSE;
		SG_bool bFoundOther = SG_FALSE;
		SG_bool bPeerless;
		if (pMrg->pMrgCSet_Baseline && pMrg->pMrgCSet_Baseline->prbEntries)
			SG_ERR_CHECK(  SG_rbtree__find(pCtx, pMrg->pMrgCSet_Baseline->prbEntries,
										   pszKey_GidEntry, &bFoundBaseline,
										   (void **)&pMrgCSetEntry_Baseline)  );
		if (pMrg->pMrgCSet_Other && pMrg->pMrgCSet_Other->prbEntries)
			SG_ERR_CHECK(  SG_rbtree__find(pCtx, pMrg->pMrgCSet_Other->prbEntries,
										   pszKey_GidEntry, &bFoundOther,
										   (void **)&pMrgCSetEntry_Other)  );
		bPeerless = (!bFoundBaseline || !bFoundOther);
		if (bPeerless)
		{
#if TRACE_WC_RESOLVE
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   ("PrepareIssues:Collision: Peerless/Existence %s [bBase  %d][bOther %d]\n"),
									   pszKey_GidEntry, bFoundBaseline, bFoundOther)  );
#endif
			statusFlags_x_xr_xu |= SG_WC_STATUS_FLAGS__XU__EXISTENCE;
		}
		else
		{
#if TRACE_WC_RESOLVE
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   ("PrepareIssues:Collision: Peers %s\n"
										"\t[base  %s,%s]\n"
										"\t[other %s,%s]\n"),
									   pszKey_GidEntry,
									   pMrgCSetEntry_Baseline->bufGid_Parent,
									   SG_string__sz(pMrgCSetEntry_Baseline->pStringEntryname),
									   pMrgCSetEntry_Other->bufGid_Parent,
									   SG_string__sz(pMrgCSetEntry_Other->pStringEntryname))  );
#endif
			SG_ERR_CHECK(  SG_mrg_cset_entry__equal(pCtx, pMrgCSetEntry_Baseline, pMrgCSetEntry_Other, &neq)  );
			if (neq & SG_MRG_CSET_ENTRY_NEQ__GID_PARENT)
				statusFlags_x_xr_xu |= SG_WC_STATUS_FLAGS__XU__LOCATION;
			if (neq & SG_MRG_CSET_ENTRY_NEQ__ENTRYNAME)
				statusFlags_x_xr_xu |= SG_WC_STATUS_FLAGS__XU__NAME;
		}
	}

	if (statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XU__MASK)
	{
		statusFlags_x_xr_xu |= SG_WC_STATUS_FLAGS__X__UNRESOLVED;
		pMrg->nrUnresolved++;
	}
	else if (statusFlags_x_xr_xu & SG_WC_STATUS_FLAGS__XR__MASK)
	{
		statusFlags_x_xr_xu |= SG_WC_STATUS_FLAGS__X__RESOLVED;
		pMrg->nrResolved++;
	}
	else
	{
		SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
	}

#if TRACE_WC_MERGE || TRACE_WC_RESOLVE
	SG_ERR_CHECK(  SG_vhash_debug__dump_to_console__named(pCtx, pvhIssue, pszKey_GidEntry)  );
	SG_ERR_IGNORE(  SG_wc_debug__status__dump_flags_to_console(pCtx, statusFlags_x_xr_xu, "PrepareIssues _x_xr_xu:")  );
#endif

	SG_ERR_CHECK(  sg_wc_tx__liveview__fetch_random_item(pCtx, pMrg->pWcTx,
														 pMrgCSetEntry_FinalResult->uiAliasGid,
														 &bHaveLVI, &pLVI)  );

	SG_ASSERT( (bHaveLVI) );

	// NOTE 2012/10/17 W4887. See the note in __queue_plan__peer about defering
	// NOTE            some of the outstanding issue checking to here.
	if (pLVI->pvhIssue)
		SG_ERR_CHECK(  SG_mrg_cset_entry__check_for_outstanding_issue(pCtx, pMrg, pLVI,
																	  pMrgCSetEntry_FinalResult)  );

	SG_ERR_CHECK(  sg_wc_tx__queue__insert_issue(pCtx, pMrg->pWcTx, pLVI,
												statusFlags_x_xr_xu, &pvhIssue, NULL)  );

fail:
	SG_VARRAY_NULLFREE(pCtx, pva);
	SG_VHASH_NULLFREE(pCtx, pvh);
	SG_VHASH_NULLFREE(pCtx, pvhIssue);
	SG_RBTREE_ITERATOR_NULLFREE(pCtx, pIter);
}

//////////////////////////////////////////////////////////////////

/**
 * Write the set of ISSUES/conflicts to the WC.DB.
 * These issues are basically a dump of the in-memory
 * state info that we have for each item with a conflict.
 * 
 * This info will be used later by RESOLVE.
 *
 */
void sg_wc_tx__merge__prepare_issues(SG_context * pCtx,
									 SG_mrg * pMrg)
{
	// we only need to look at each entry in the final-result for conflicts.
	// unlike when building wd_plan, we do not need to look at the things
	// present in the baseline that were deleted from the final-result
	// (because in all cases where there is a DELETE-VS-* conflict we keep
	// the item in the candidate final result).
	//
	// That is, final-result-->prbEntries only has data for items that
	// are present in the final result -- not for things that were present
	// in the baseline and deleted in the final result.
	//
	// We're asserting that if we accepted (didn't override) the delete,
	// then the item can't be in conflict, so it won't need an issue.

	SG_ERR_CHECK(  SG_rbtree__foreach(pCtx, pMrg->pMrgCSet_FinalResult->prbEntries,
									  _sg_mrg__prepare_issues__cb, pMrg)  );

fail:
	return;
}


//////////////////////////////////////////////////////////////////

/**
 * create the tbl_issue table if it doesn't already exist.
 * 
 */
void sg_wc_tx__merge__prepare_issue_table(SG_context * pCtx,
										  SG_mrg * pMrg)
{
	SG_ERR_CHECK(  sg_wc_db__issue__create_table(pCtx, pMrg->pWcTx->pDb)  );

fail:
	return;
}
