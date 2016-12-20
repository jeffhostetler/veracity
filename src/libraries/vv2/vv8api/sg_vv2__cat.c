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

/**
 * Use the given "Input" to find the GID of the item.
 * Use the GID to then find the TNE for the item IN THE GIVEN CSET.
 * Append row of CAT-data to VARRAY of results.
 *
 */
static void _sg_vv2__cat__lookup_by_tne(SG_context * pCtx,
										SG_repo * pRepo,
										const char * pszHidCSet,
										SG_treenode * pTreenodeRoot,
										const char * pszInput_k,
										SG_varray * pvaResults)
{
	char * pszGid_k = NULL;
	SG_treenode_entry * ptne_k = NULL;
	SG_vhash * pvh_k = NULL;			// we do not own this
	const char * pszHidBlob_k;			// we do not own this
	SG_treenode_entry_type tneType_k;
    SG_uint64 len = 0;
	SG_string * pStringRepoPathInCSet_k = NULL;

	if ((pszInput_k[0] == '@') && (pszInput_k[1] == '/'))
	{
		SG_ERR_CHECK( SG_treenode__find_treenodeentry_by_path(pCtx, pRepo, pTreenodeRoot, pszInput_k,
															  &pszGid_k, &ptne_k) );
	}
	else if ((pszInput_k[0] == '@') && (pszInput_k[1] == 'g'))
	{
		SG_bool bValid;
		SG_ERR_CHECK(  SG_gid__verify_format(pCtx, &pszInput_k[1], &bValid)  );
		if (!bValid)
			SG_ERR_THROW2(  SG_ERR_INVALID_REPO_PATH,
							(pCtx, "The input '%s' is not a valid gid-domain input.", pszInput_k)  );
		// copy gid without the '@' prefix into our buffer
		SG_ERR_CHECK(  SG_STRDUP(pCtx, &pszInput_k[1], &pszGid_k)  );
		SG_ERR_CHECK(  SG_repo__treendx__get_path_in_dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL,
															 pszGid_k, pszHidCSet,
															 &pStringRepoPathInCSet_k, &ptne_k)  );
	}
	else
	{
		SG_ERR_THROW2(  SG_ERR_INVALID_REPO_PATH,
						(pCtx, "Argument '%s' must be full repo-path.", pszInput_k)  );
	}
	
	if (!pszGid_k || !ptne_k)
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx, "'%s' is not present in changeset '%s'.", pszInput_k, pszHidCSet)  );

	SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, ptne_k, &tneType_k)  );
	if (tneType_k != SG_TREENODEENTRY_TYPE_REGULAR_FILE)
		SG_ERR_THROW2(  SG_ERR_NOTAFILE,
						(pCtx, "'%s' is not a file.", pszInput_k)  );

	SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pvaResults, &pvh_k)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_k, "gid", pszGid_k)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_k, "path",
											 ((pStringRepoPathInCSet_k)
											  ? SG_string__sz(pStringRepoPathInCSet_k)
											  : pszInput_k))  );

	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, ptne_k, &pszHidBlob_k)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_k, "hid", pszHidBlob_k)  );
		
	SG_ERR_CHECK(  SG_repo__fetch_blob__begin(pCtx, pRepo, pszHidBlob_k,
											  SG_TRUE, NULL, NULL, NULL, &len, NULL)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvh_k, "len", (SG_int64)len)  );

fail:
	SG_NULLFREE(pCtx, pszGid_k);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne_k);
	SG_STRING_NULLFREE(pCtx, pStringRepoPathInCSet_k);
}
	
/**
 * Lookup data for CAT where we have a named repo and CANNOT
 * USE the WD (to help parse the input paths).  This restricts
 * the inputs to be full repo-paths.
 *
 * For a purely historical CAT, the full-repo-paths describe
 * where a specific item was *IN THAT CSET* (which may or may
 * not have anything to do with where the item currently is in
 * the tree (or it it even still exists in the tree)).  The point
 * is that we DO NOT USE THE WD FOR ANYTHING.
 *
 */
static void _sg_vv2__cat__repo(SG_context * pCtx,
							   const char * pszRepoName,
							   const SG_rev_spec * pRevSpec,
							   const SG_stringarray * psaInput,
							   SG_varray ** ppvaResults,
							   SG_repo ** ppRepo)
{
	char * pszHidCSet = NULL;
	SG_repo * pRepo = NULL;
	char * pszHidForRoot = NULL;
	SG_treenode * pTreenodeRoot = NULL;
	SG_varray * pvaResults = NULL;
	SG_uint32 k, count;

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaResults)  );

	SG_ERR_CHECK(  SG_vv2__find_cset(pCtx, pszRepoName, pRevSpec, &pszHidCSet, NULL)  );
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, &pRepo)  );
	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, pszHidCSet, &pszHidForRoot)  );
	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pszHidForRoot, &pTreenodeRoot)  );

	SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaInput, &count)  );
	for (k=0; k<count; k++)
	{
		const char * pszInput_k;			// we do not own this

		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaInput, k, &pszInput_k)  );
		if ((pszInput_k[0] == '@') && ((pszInput_k[1] == '/') || (pszInput_k[1] == 'g')))
			SG_ERR_CHECK(  _sg_vv2__cat__lookup_by_tne(pCtx, pRepo, pszHidCSet, pTreenodeRoot, pszInput_k, pvaResults)  );
		else
			SG_ERR_THROW2(  SG_ERR_INVALID_REPO_PATH,
							(pCtx,
							 "Argument '%s' must be full repo-path when the '--repo' option is used.",
							 pszInput_k)  );
	}

	*ppvaResults = pvaResults;
	pvaResults = NULL;

	if (ppRepo)
	{
		*ppRepo = pRepo;
		pRepo = NULL;
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaResults);
	SG_NULLFREE(pCtx, pszHidCSet);
	SG_NULLFREE(pCtx, pszHidForRoot);
	SG_TREENODE_NULLFREE(pCtx, pTreenodeRoot);
	SG_REPO_NULLFREE(pCtx, pRepo);
}

/**
 * Lookup data for CAT where we DID NOT have a named repo given,
 * but we were given a rev-spec.
 * 
 * We try to use the WD (if present) to backfill omitted args.
 *
 */
static void _sg_vv2__cat__revspec(SG_context * pCtx,
								  const SG_rev_spec * pRevSpec,
								  const SG_stringarray * psaInput,
								  SG_varray ** ppvaResults,
								  SG_repo ** ppRepo)
{
	SG_varray * pvaResults = NULL;
	SG_wc_tx * pWcTx = NULL;
	SG_repo * pRepo = NULL;
	char * pszHidCSet = NULL;
	char * pszHidForRoot = NULL;
	SG_treenode * pTreenodeRoot = NULL;
	char * pszGidPath_k = NULL;
	SG_uint32 k, count;

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaResults)  );

	SG_ERR_CHECK(  SG_vv2__find_cset(pCtx, NULL, pRevSpec, &pszHidCSet, NULL)  );

	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, NULL, SG_TRUE)  );
	SG_ERR_CHECK(  SG_wc_tx__get_repo_and_wd_top(pCtx, pWcTx, &pRepo, NULL)  );
	// Prepare to make GID-->HID lookups relative to the CSET given in the rev-spec.
	// This will throw if the rev-spec maps to multiple csets (such as when the
	// branch has multiple heads).
	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, pszHidCSet, &pszHidForRoot)  );
	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pszHidForRoot, &pTreenodeRoot)  );

	SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaInput, &count)  );
	for (k=0; k<count; k++)
	{
		const char * pszInput_k;			// we do not own this

		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaInput, k, &pszInput_k)  );
		if ((pszInput_k[0] == '@') && ((pszInput_k[1] == '/') || (pszInput_k[1] == 'g')))
		{
			// use normal historical lookup (and interpret the repo-path as it
			// was in the historical cset).
			SG_ERR_CHECK(  _sg_vv2__cat__lookup_by_tne(pCtx, pRepo, pszHidCSet, pTreenodeRoot, pszInput_k, pvaResults)  );
		}
		else
		{
			// We use the WC to find the GID.  That lets the user use
			// relative- and absolute-pathnames on the command line.
			//
			// (I'm not sure how important this is, but) We also allow
			// (get for free) extended-prefix repo-paths to help name
			// the item.  (This is primarily for "@b/" for pending
			// deleted items.)
			//
			// This does introduce a little ambiguity (or the potential for
			// some) since "@/foo" is well defined against either the "-r r1"
			// or baseline csets, but "@b/foo" with "-r r1" is confusing.
			SG_ERR_CHECK(  SG_wc_tx__get_item_gid_path(pCtx, pWcTx, pszInput_k, &pszGidPath_k)  );
			SG_ERR_CHECK(  _sg_vv2__cat__lookup_by_tne(pCtx, pRepo, pszHidCSet, pTreenodeRoot, pszGidPath_k, pvaResults)  );
		}
	}

	*ppvaResults = pvaResults;
	pvaResults = NULL;

	if (ppRepo)
	{
		*ppRepo = pRepo;
		pRepo = NULL;
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaResults);
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_NULLFREE(pCtx, pszHidCSet);
	SG_NULLFREE(pCtx, pszHidForRoot);
	SG_TREENODE_NULLFREE(pCtx, pTreenodeRoot);
	SG_NULLFREE(pCtx, pszGidPath_k);
}

//////////////////////////////////////////////////////////////////

/**
 * Lookup data for CAT where we DID NOT have a named repo *NOR* a rev-spec given.
 * We use the WD to interpret everything.
 *
 */
static void _sg_vv2__cat__working_folder(SG_context * pCtx,
										 const SG_stringarray * psaInput,
										 SG_varray ** ppvaResults,
										 SG_repo ** ppRepo)
{
	SG_varray * pvaResults = NULL;
	SG_wc_tx * pWcTx = NULL;
	SG_repo * pRepo = NULL;
	SG_vhash * pvhCSets = NULL;
	struct _x
	{
		const char * pszHidCSet;
		char * pszHidForRoot;
		SG_treenode * pTreenodeRoot;
	};
	struct _x A = { NULL, NULL, NULL };
	struct _x B = { NULL, NULL, NULL };
	struct _x C = { NULL, NULL, NULL };
	SG_string * pStringRepoPath_k = NULL;
	char * pszGidPath_k = NULL;
	SG_uint32 nrParents = 0;
	SG_uint32 k, count;

	SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaResults)  );

	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, NULL, SG_TRUE)  );
	SG_ERR_CHECK(  SG_wc_tx__get_repo_and_wd_top(pCtx, pWcTx, &pRepo, NULL)  );
	SG_ERR_CHECK(  SG_wc_tx__get_wc_csets__vhash(pCtx, pWcTx, &pvhCSets)  );

	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhCSets, "A", &A.pszHidCSet)  );
	if (A.pszHidCSet)
	{
		SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, A.pszHidCSet, &A.pszHidForRoot)  );
		SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, A.pszHidForRoot, &A.pTreenodeRoot)  );
	}

	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhCSets, "L0", &B.pszHidCSet)  );
	if (B.pszHidCSet)
	{
		SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, B.pszHidCSet, &B.pszHidForRoot)  );
		SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, B.pszHidForRoot, &B.pTreenodeRoot)  );
		nrParents++;
	}
	SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pvhCSets, "L1", &C.pszHidCSet)  );
	if (C.pszHidCSet)
	{
		SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, C.pszHidCSet, &C.pszHidForRoot)  );
		SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, C.pszHidForRoot, &C.pTreenodeRoot)  );
		nrParents++;
	}
	
	SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaInput, &count)  );
	for (k=0; k<count; k++)
	{
		const char * pszInput_k;			// we do not own this

		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaInput, k, &pszInput_k)  );

		// If they qualified the input ("@a/..." or "@b/..." or "@c/...") to bind it
		// to a particular parent, we respect that and get the HID from
		// that parent.  This will throw if the file wasn't present in
		// that parent, but that's what they asked for.
		if ((pszInput_k[0] == '@') && (pszInput_k[1] == 'a'))
		{
			if (nrParents < 2)
				SG_ERR_THROW2(  SG_ERR_INVALIDARG,
								(pCtx, "Extended repo-path prefix '@a/' not valid when not in a merge.")  );

			// Convert "@a/foo" to "@/foo".
			SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringRepoPath_k)  );
			SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStringRepoPath_k, "@%s", &pszInput_k[2])  );
			SG_ERR_CHECK(  _sg_vv2__cat__lookup_by_tne(pCtx, pRepo, A.pszHidCSet, A.pTreenodeRoot,
													   SG_string__sz(pStringRepoPath_k), pvaResults)  );
			SG_STRING_NULLFREE(pCtx, pStringRepoPath_k);
		}
		else if ((pszInput_k[0] == '@') && (pszInput_k[1] == 'b'))
		{
			// Convert "@b/foo" to "@/foo".
			SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringRepoPath_k)  );
			SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStringRepoPath_k, "@%s", &pszInput_k[2])  );
			SG_ERR_CHECK(  _sg_vv2__cat__lookup_by_tne(pCtx, pRepo, B.pszHidCSet, B.pTreenodeRoot,
													   SG_string__sz(pStringRepoPath_k), pvaResults)  );
			SG_STRING_NULLFREE(pCtx, pStringRepoPath_k);
		}
		else if ((pszInput_k[0] == '@') && (pszInput_k[1] == 'c'))
		{
			if (nrParents < 2)
				SG_ERR_THROW2(  SG_ERR_INVALIDARG,
								(pCtx, "Extended repo-path prefix '@c/' not valid when not in a merge.")  );

			// Convert "@c/foo" to "@/foo".
			SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStringRepoPath_k)  );
			SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStringRepoPath_k, "@%s", &pszInput_k[2])  );
			SG_ERR_CHECK(  _sg_vv2__cat__lookup_by_tne(pCtx, pRepo, C.pszHidCSet, C.pTreenodeRoot,
													   SG_string__sz(pStringRepoPath_k), pvaResults)  );
			SG_STRING_NULLFREE(pCtx, pStringRepoPath_k);
		}
		else if (nrParents == 1)
		{
			// Look up the item in the single parent and throw if
			// it is not present in it (possible in the case of an
			// ADD or ADD-SPECIAL).  use the GID-repo-path to avoid
			// ambiguities.
			SG_ERR_CHECK(  SG_wc_tx__get_item_gid_path(pCtx, pWcTx, pszInput_k, &pszGidPath_k)  );
			SG_ERR_CHECK(  _sg_vv2__cat__lookup_by_tne(pCtx, pRepo, B.pszHidCSet, B.pTreenodeRoot,
													   pszGidPath_k, pvaResults)  );
			SG_NULLFREE(pCtx, pszGidPath_k);
		}
		else // nrParents == 2
		{
			SG_uint32 nrFound = 0;
			SG_uint32 j;
			SG_uint32 nrResultsAtStart;

			// Look it up in both parents.
			SG_wc_tx__get_item_gid_path(pCtx, pWcTx, pszInput_k, &pszGidPath_k);

			SG_ERR_CHECK(  SG_varray__count(pCtx, pvaResults, &nrResultsAtStart)  );
			for (j=0; j<2; j++)
			{
				// TODO 2013/01/07 This is a tad expensive.  If this routine were
				// TODO            inside WC, we could get the HIDs from the wc.db.
				// TODO            And we could have gotten the result-info from
				// TODO            STATUS or MSTATUS.
				if (j == 0)
					SG_ERR_CHECK(  _sg_vv2__cat__lookup_by_tne(pCtx, pRepo, B.pszHidCSet, B.pTreenodeRoot,
															   pszGidPath_k, pvaResults)  );
				else
					SG_ERR_CHECK(  _sg_vv2__cat__lookup_by_tne(pCtx, pRepo, C.pszHidCSet, C.pTreenodeRoot,
															   pszGidPath_k, pvaResults)  );
				if (SG_context__has_err(pCtx))
				{
					if (SG_context__err_equals(pCtx, SG_ERR_NOT_FOUND))
						SG_context__err_reset(pCtx);
					else
						SG_ERR_RETHROW;
				}
				else
				{
					nrFound++;
				}
			}
			SG_NULLFREE(pCtx, pszGidPath_k);

			if (nrFound == 0)
			{
				SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
								(pCtx, "'%s' is not present in either parent changeset.", pszInput_k)  );
			}
			else if (nrFound == 1)
			{
				// we've already inserted the 1 match from whichever parent into the pvaResults.
			}
			else // nrFound == 2
			{
				// we've already inserted 2 rows for this item into the results.
				// compare them and complain if they are different or remove 1 if
				// the same.
				/*const*/ SG_vhash * pvhResult_0;
				/*const*/ SG_vhash * pvhResult_1;
				const char * pszHid_0;
				const char * pszHid_1;

				SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaResults, nrResultsAtStart,   &pvhResult_0)  );
				SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaResults, nrResultsAtStart+1, &pvhResult_1)  );
				SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhResult_0, "hid", &pszHid_0)  );
				SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhResult_1, "hid", &pszHid_1)  );

				if (strcmp(pszHid_0, pszHid_1) == 0)
				{
					SG_ERR_CHECK(  SG_varray__remove(pCtx, pvaResults, nrResultsAtStart+1)  );
				}
				else
				{
					SG_ERR_THROW2(  SG_ERR_WC_HAS_MULTIPLE_PARENTS,
									(pCtx, ("The contents of file '%s' are different in the 2 parents;"
											" consider using the '@b/...' or '@c/...' prefix to qualify."),
									 pszInput_k)  );
				}
			}
		}
	}
	
	*ppvaResults = pvaResults;
	pvaResults = NULL;

	if (ppRepo)
	{
		*ppRepo = pRepo;
		pRepo = NULL;
	}

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaResults);
	SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
	SG_WC_TX__NULLFREE(pCtx, pWcTx);
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_NULLFREE(pCtx, A.pszHidForRoot);
	SG_NULLFREE(pCtx, B.pszHidForRoot);
	SG_NULLFREE(pCtx, C.pszHidForRoot);
	SG_TREENODE_NULLFREE(pCtx, A.pTreenodeRoot);
	SG_TREENODE_NULLFREE(pCtx, B.pTreenodeRoot);
	SG_TREENODE_NULLFREE(pCtx, C.pTreenodeRoot);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath_k);
	SG_NULLFREE(pCtx, pszGidPath_k);
	SG_VHASH_NULLFREE(pCtx, pvhCSets);
}

//////////////////////////////////////////////////////////////////

/**
 * First part of CAT.
 *
 * Use the (<reponame>, <<rev-spec>>) to get info for one or more files.
 * We return a VARRAY of VHASH of details and let the caller decide if/when/how to
 * print/output the content.
 *
 * We return:  [ { "gid" : "<gid>", "path" : "<repo-path>", "hid" : "<hid>", "len" : <len> },
 *               ... ]
 *
 * That is, for each item in psaInput, we return the repo-path and the content-hid
 * that it had in the requested CSET.  The caller can then use the hid to fetch and
 * print/output the content as they see fit using a regular repo lookup routine (that
 * is, we don't need to broker the chunking of the content to the stream of their
 * choice).
 *
 * The caller can use the (optionally) returned pRepo to then fetch the individual
 * file blobs.
 *
 * You own the returned pvaResults and pRepo.
 *
 */
void SG_vv2__cat(SG_context * pCtx,
				 const char * pszRepoName,
				 const SG_rev_spec * pRevSpec,
				 const SG_stringarray * psaInput,
				 SG_varray ** ppvaResults,
				 SG_repo ** ppRepo)
{
	SG_uint32 countRevSpec = 0;

	// pszRepoName is optional (defaults to WD if present)
	// pRevSpec is optional (defaults to WD baseline if present)
	SG_NULLARGCHECK_RETURN( psaInput );	// we require at least one item
	SG_NULLARGCHECK_RETURN( ppvaResults );
	// ppRepo is optional

	if (pRevSpec)
	{
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &countRevSpec)  );
		if (countRevSpec > 1)
			SG_ERR_THROW2(  SG_ERR_INVALIDARG,
							(pCtx, "Use only one rev-spec (--rev, --tag, or --branch).")  );
		if (countRevSpec == 0)
			pRevSpec = NULL;
	}

	if (pszRepoName && *pszRepoName)
	{
		if (countRevSpec == 1)
			SG_ERR_CHECK(  _sg_vv2__cat__repo(pCtx, pszRepoName, pRevSpec, psaInput, ppvaResults, ppRepo)  );
		else
			SG_ERR_THROW2(  SG_ERR_INVALIDARG,
							(pCtx, "Missing rev-spec (--rev, --tag, or --branch) must be used with --repo.")  );
	}
	else if (pRevSpec)
	{
		SG_ERR_CHECK(  _sg_vv2__cat__revspec(pCtx, pRevSpec, psaInput, ppvaResults, ppRepo)  );
	}
	else
	{
		SG_ERR_CHECK(  _sg_vv2__cat__working_folder(pCtx, psaInput, ppvaResults, ppRepo)  );
	}

#if TRACE_VV2_CAT
	SG_ERR_IGNORE(  SG_varray_debug__dump_varray_of_vhashes_to_console(pCtx, *ppvaResults, "cat data")  );
#endif

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * This routine takes both an SG_file* and a SG_console_binary_handle*, but only one is written to.
 * Leave one NULL;
 */
static void _cat_one__to_stream(SG_context * pCtx,
								SG_repo * pRepo,
								SG_vhash * pvh_k,
								SG_file * pFile)
{
	SG_uint64 lenFile64;
	const char * pszHidBlob;
	SG_repo_fetch_blob_handle* pfh = NULL;
	SG_byte* pBuf = NULL;
	SG_uint32 lenBuf32;
	SG_uint32 lenGot32;
	SG_bool bDone = SG_FALSE;

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh_k, "hid", &pszHidBlob)  );

	SG_ERR_CHECK(  SG_repo__fetch_blob__begin(pCtx, pRepo, pszHidBlob, SG_TRUE,
											  NULL, NULL, NULL, &lenFile64, &pfh)  );

	// Make cat of an empty file a no-op, rather than a malloc error.
	if (lenFile64)
	{
		// We DO NOT pad the buffer with a null byte
		// because we DO NOT want to use a print
		// routine that needs it (because the file
		// may contain binary data so it is bogus to
		// assume that we need one).

		if (lenFile64 > 0x10000)
			lenBuf32 = 0x10000;
		else
			lenBuf32 = (SG_uint32)lenFile64;
		SG_ERR_CHECK(  SG_allocN(pCtx, lenBuf32, pBuf)  );

		while (!bDone)
		{
			SG_ERR_CHECK(  SG_repo__fetch_blob__chunk(pCtx, pRepo, pfh, lenBuf32, pBuf, &lenGot32, &bDone)  );
			if (pFile)
				SG_ERR_CHECK(  SG_file__write(pCtx, pFile, lenGot32, pBuf, NULL)  );
			else
				SG_ERR_CHECK(  SG_console__binary__stdout(pCtx, pBuf, lenGot32)  );
		}
	}

	SG_ERR_CHECK(  SG_repo__fetch_blob__end(pCtx, pRepo, &pfh)  );

fail:
	if (pfh)
		SG_ERR_IGNORE (  SG_repo__fetch_blob__abort(pCtx, pRepo, &pfh)  );
	SG_NULLFREE(pCtx, pBuf);
}

/**
 * If pFile is set, we write to it.
 * Otherwise, we write to the console.
 *
 */
static void _cat_all__to_stream(SG_context * pCtx,
								SG_repo * pRepo,
								SG_varray * pvaResults,
								SG_file * pFile)
{
	SG_uint32 k, count;

	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaResults, &count)  );
	for (k=0; k<count; k++)
	{
		SG_vhash * pvh_k;	// we do not own this

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaResults, k, &pvh_k)  );
		SG_ERR_CHECK(  _cat_one__to_stream(pCtx, pRepo, pvh_k, pFile)  );
	}

fail:
	;
}
	
/**
 * Wrapper around SG_vv2__cat() that does a normal/raw output
 * of the content of the file(s) to the console.
 *
 */
void SG_vv2__cat__to_console(SG_context * pCtx,
							 const char * pszRepoName,
							 const SG_rev_spec * pRevSpec,
							 const SG_stringarray * psaInput)
{
	SG_varray * pvaResults = NULL;
	SG_repo * pRepo = NULL;

	SG_ERR_CHECK(  SG_vv2__cat(pCtx, pszRepoName, pRevSpec, psaInput, &pvaResults, &pRepo)  );
	SG_ERR_CHECK(  _cat_all__to_stream(pCtx, pRepo, pvaResults, NULL)  );

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaResults);
	SG_REPO_NULLFREE(pCtx, pRepo);
}

/**
 * Wrapper around SG_vv2__cat() that does a normal/raw output
 * of the content of the file(s) to the given pathname.
 *
 */
void SG_vv2__cat__to_pathname(SG_context * pCtx,
							  const char * pszRepoName,
							  const SG_rev_spec * pRevSpec,
							  const SG_stringarray * psaInput,
							  const SG_pathname * pPathOutput)
{
	SG_varray * pvaResults = NULL;
	SG_repo * pRepo = NULL;
	SG_file * pFile = NULL;

	SG_ERR_CHECK(  SG_vv2__cat(pCtx, pszRepoName, pRevSpec, psaInput, &pvaResults, &pRepo)  );

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathOutput,
										   SG_FILE_WRONLY|SG_FILE_OPEN_OR_CREATE|SG_FILE_TRUNC,
										   0644,
										   &pFile)  );

	SG_ERR_CHECK(  _cat_all__to_stream(pCtx, pRepo, pvaResults, pFile)  );

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_VARRAY_NULLFREE(pCtx, pvaResults);
	SG_REPO_NULLFREE(pCtx, pRepo);
}
