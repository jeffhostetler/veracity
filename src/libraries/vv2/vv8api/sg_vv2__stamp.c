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
 * @file sg_vv2__stamp.c
 *
 * @details Routines to handle the STAMP command and its sub-commands.
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

void SG_vv2__stamp__add(SG_context * pCtx,
						const char * pszRepoName,
						const SG_rev_spec * pRevSpec,
						const char * pszStampName,
						SG_varray ** ppvaInfo)
{
	SG_repo * pRepo = NULL;
	SG_stringarray * psaHidChangesets = NULL;
	SG_varray * pvaInfo = NULL;
	SG_uint32 countRevSpecs = 0;
	SG_uint32 countChangesets = 0;
	SG_uint32 k;
	SG_audit q;
	SG_bool bRepoNamed = SG_FALSE;

	// pszRepoName is optional (defaults to WD if present)
	// pRevSpec is optional (defaults to parents of WD if present)
	SG_NONEMPTYCHECK_RETURN( pszStampName );

	// open either the named repo or the repo of the WD.

	SG_ERR_CHECK(  sg_vv2__util__get_repo(pCtx, pszRepoName, &pRepo, &bRepoNamed)  );
	SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );

	// determine the list of CSETs referenced by rev-spec or
	// default to the current parent(s).

	if (pRevSpec)
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &countRevSpecs)  );
	if (countRevSpecs > 0)
	{
		SG_ERR_CHECK(  SG_rev_spec__get_all__repo(pCtx, pRepo, pRevSpec, SG_TRUE, &psaHidChangesets, NULL)  );
	}
	else if (bRepoNamed)
	{
		// normally the (count==0) case means to use
		// the parents of the current WD, but we have to disallow it here
		// because when they give us a named repo, we do not want to assume
		// anything about the current WD (if present).

		SG_ERR_THROW2(  SG_ERR_USAGE,
						(pCtx, "Must specify which changeset(s) to stamp in repo '%s'.", pszRepoName)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_wc__get_wc_parents__stringarray(pCtx, NULL, &psaHidChangesets)  );
	}

	// associate the given stamp with each identified CSET.

	if (ppvaInfo)
		SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaInfo)  );
	if (psaHidChangesets)
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaHidChangesets, &countChangesets)  );
	for (k=0; k<countChangesets; k++)
	{
		SG_vhash * pvhInfo_k;	// we do not own this
		const char * pszHid_k;	// we do not own this
		SG_bool bRedundant;

		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaHidChangesets, k, &pszHid_k)  );
		SG_ERR_CHECK(  SG_vc_stamps__add(pCtx, pRepo, pszHid_k, pszStampName, &q, &bRedundant)  );

		if (ppvaInfo)
		{
			SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pvaInfo, &pvhInfo_k)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhInfo_k, "hid", pszHid_k)  );
			SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvhInfo_k, "redundant", bRedundant)  );
		}
	}
	
	if (ppvaInfo)
	{
		*ppvaInfo = pvaInfo;
		pvaInfo = NULL;
	}

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_STRINGARRAY_NULLFREE(pCtx, psaHidChangesets);
	SG_VARRAY_NULLFREE(pCtx, pvaInfo);
}

//////////////////////////////////////////////////////////////////

/**
 * Remove the named STAMP from the requested CSET(s).
 * It is an error if the stamp is not present in all
 * of the csets.
 *
 */
void SG_vv2__stamp__remove_stamp_from_cset(SG_context * pCtx,
										   const char * pszRepoName,
										   const SG_rev_spec * pRevSpec,
										   const char * pszStampName,
										   SG_varray ** ppvaInfo)
{
	SG_repo * pRepo = NULL;
	SG_stringarray * psaHidChangesets = NULL;
	SG_varray * pvaInfo = NULL;
	SG_uint32 countRevSpecs = 0;
	SG_uint32 countChangesets = 0;
	SG_uint32 k;
	SG_audit q;
	SG_bool bRepoNamed = SG_FALSE;

	// pszRepoName is optional (defaults to WD if present)
	// pRevSpec is optional (defaults to parents of WD if present)
	SG_NONEMPTYCHECK_RETURN( pszStampName );

	// open either the named repo or the repo of the WD.

	SG_ERR_CHECK(  sg_vv2__util__get_repo(pCtx, pszRepoName, &pRepo, &bRepoNamed)  );
	SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );

	// determine the list of CSETs referenced by rev-spec or
	// default to the current parents(s).

	if (pRevSpec)
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &countRevSpecs)  );
	if (countRevSpecs > 0)
	{
		SG_ERR_CHECK(  SG_rev_spec__get_all__repo(pCtx, pRepo, pRevSpec, SG_TRUE, &psaHidChangesets, NULL)  );
	}
	else if (bRepoNamed)
	{
		// normally the (count==0) case means to use
		// the parents of the current WD, but we have to disallow it here
		// because when they give us a named repo, we do not want to assume
		// anything about the current WD (if present).

		SG_ERR_THROW2(  SG_ERR_USAGE,
						(pCtx, "Must specify which changeset(s) to unstamp in repo '%s'.", pszRepoName)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_wc__get_wc_parents__stringarray(pCtx, NULL, &psaHidChangesets)  );
	}

	// ensure that the named STAMP is present in EACH CSET.

	SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaHidChangesets, &countChangesets)  );
	for (k=0; k<countChangesets; k++)
	{
		const char* pszHid_k;
		SG_bool bStampIsThere_k = SG_FALSE;

		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaHidChangesets, k, &pszHid_k)  );
		SG_ERR_CHECK(  SG_vc_stamps__is_stamp_already_applied(pCtx, pRepo, pszHid_k, pszStampName, &bStampIsThere_k)  );
		if(!bStampIsThere_k)
			SG_ERR_THROW2(  SG_ERR_STAMP_NOT_FOUND,
							(pCtx, "%s in changeset %s", pszStampName, pszHid_k)  );
	}

	// remove the stamp from EACH CSET and optionally return info
	// to the caller for each cset that we touched.

	if (ppvaInfo)
		SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaInfo)  );
	for (k=0; k<countChangesets; k++)
	{
		SG_vhash * pvhInfo_k;	// we do not own this
		const char * pszHid_k;	// we do not own this

		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaHidChangesets, k, &pszHid_k)  );
		SG_ERR_CHECK(  SG_vc_stamps__remove(pCtx, pRepo, &q, pszHid_k, 1, &pszStampName)  );

		if (ppvaInfo)
		{
			SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pvaInfo, &pvhInfo_k)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhInfo_k, "hid", pszHid_k)  );
		}
	}

	if (ppvaInfo)
	{
		*ppvaInfo = pvaInfo;
		pvaInfo = NULL;
	}

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_STRINGARRAY_NULLFREE(pCtx, psaHidChangesets);
	SG_VARRAY_NULLFREE(pCtx, pvaInfo);
}

/**
 * Remove the named STAMP from *ALL* CSETS where it occurs.
 * We DO NOT take a rev-spec.  It is an error if the stamp
 * is not present in any cset.
 *
 */
void SG_vv2__stamp__remove_stamp_from_all_csets(SG_context * pCtx,
												const char * pszRepoName,
												const char * pszStampName,
												SG_varray ** ppvaInfo)
{
	SG_repo * pRepo = NULL;
	SG_stringarray * psaHidChangesets = NULL;
	SG_varray * pvaInfo = NULL;
	SG_uint32 countChangesets = 0;
	SG_uint32 k;
	SG_audit q;
	SG_bool bRepoNamed = SG_FALSE;

	// pszRepoName is optional (defaults to WD if present)
	if (!pszStampName || !*pszStampName)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Must specify a stamp name with 'all' option.")  );

	// open either the named repo or the repo of the WD.

	SG_ERR_CHECK(  sg_vv2__util__get_repo(pCtx, pszRepoName, &pRepo, &bRepoNamed)  );
	SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );

	// find all csets where this stamp occurs.
	
	SG_ERR_CHECK(  SG_vc_stamps__lookup_by_stamp(pCtx, pRepo, &q, pszStampName, &psaHidChangesets)  );

	if (psaHidChangesets)
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaHidChangesets, &countChangesets)  );
	if (countChangesets == 0)
		SG_ERR_THROW2(  SG_ERR_STAMP_NOT_FOUND,
						(pCtx, "%s", pszStampName)  );
	
	// since we got the list of CSETs using a query-by-stamp,
	// we don't need to verify that they each have the stamp.
	// we already know that.

	// remove the stamp from EACH CSET and optionally return info
	// to the caller for each cset that we touched.

	if (ppvaInfo)
		SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaInfo)  );
	for (k=0; k<countChangesets; k++)
	{
		SG_vhash * pvhInfo_k;	// we do not own this
		const char * pszHid_k;	// we do not own this

		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaHidChangesets, k, &pszHid_k)  );
		SG_ERR_CHECK(  SG_vc_stamps__remove(pCtx, pRepo, &q, pszHid_k, 1, &pszStampName)  );

		if (ppvaInfo)
		{
			SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pvaInfo, &pvhInfo_k)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhInfo_k, "hid", pszHid_k)  );
		}
	}
	
	if (ppvaInfo)
	{
		*ppvaInfo = pvaInfo;
		pvaInfo = NULL;
	}

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_STRINGARRAY_NULLFREE(pCtx, psaHidChangesets);
	SG_VARRAY_NULLFREE(pCtx, pvaInfo);
}

/**
 * Remove *ALL* STAMPS from a CSET.
 *
 */
void SG_vv2__stamp__remove_all_stamps_from_cset(SG_context * pCtx,
												const char * pszRepoName,
												const SG_rev_spec * pRevSpec,
												SG_varray ** ppvaInfo)
{
	SG_repo * pRepo = NULL;
	SG_stringarray * psaHidChangesets = NULL;
	SG_varray * pvaInfo = NULL;
	SG_uint32 countRevSpecs = 0;
	SG_uint32 countChangesets = 0;
	SG_uint32 k;
	SG_audit q;
	SG_bool bRepoNamed = SG_FALSE;

	// pszRepoName is optional (defaults to WD if present)
	// pRevSpec is optional (defaults to parents of WD if present)

	// open either the named repo or the repo of the WD.

	SG_ERR_CHECK(  sg_vv2__util__get_repo(pCtx, pszRepoName, &pRepo, &bRepoNamed)  );
	SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );

	// determine the list of CSETs referenced by rev-spec or
	// default to the current parent(s).

	if (pRevSpec)
		SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &countRevSpecs)  );
	if (countRevSpecs > 0)
	{
		SG_ERR_CHECK(  SG_rev_spec__get_all__repo(pCtx, pRepo, pRevSpec, SG_TRUE, &psaHidChangesets, NULL)  );
	}
	else if (bRepoNamed)
	{
		// normally the (count==0) case means to use
		// the parents of the current WD, but we have to disallow it here
		// because when they give us a named repo, we do not want to assume
		// anything about the current WD (if present).

		SG_ERR_THROW2(  SG_ERR_USAGE,
						(pCtx, "Must specify which changeset(s) to unstamp in repo '%s'.", pszRepoName)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_wc__get_wc_parents__stringarray(pCtx, NULL, &psaHidChangesets)  );
	}

	// We are not given a named STAMP.  We just iterate on each of
	// the identified CSETs and for each remove any stamps we find
	// (if any).  So we don't need to verify that a specific stamp
	// is present first.

	// remove the stamps on each identified CSET and optionally return
	// info to the caller for each cset that we touched.

	if (ppvaInfo)
		SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaInfo)  );

	if (psaHidChangesets)
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psaHidChangesets, &countChangesets)  );
	for (k=0; k<countChangesets; k++)
	{
		SG_vhash * pvhInfo_k;	// we do not own this
		const char * pszHid_k;	// we do not own this

		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psaHidChangesets, k, &pszHid_k)  );
		SG_ERR_CHECK(  SG_vc_stamps__remove(pCtx, pRepo, &q, pszHid_k, 0, NULL)  );

		if (ppvaInfo)
		{
			SG_ERR_CHECK(  SG_varray__appendnew__vhash(pCtx, pvaInfo, &pvhInfo_k)  );
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhInfo_k, "hid", pszHid_k)  );
		}
	}

	if (ppvaInfo)
	{
		*ppvaInfo = pvaInfo;
		pvaInfo = NULL;
	}

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_STRINGARRAY_NULLFREE(pCtx, psaHidChangesets);
	SG_VARRAY_NULLFREE(pCtx, pvaInfo);
}

/**
 * Main interface for 'vv stamp remove' which handles the
 * various default/override cases.
 *
 */
void SG_vv2__stamp__remove(SG_context * pCtx,
						   const char * pszRepoName,
						   const SG_rev_spec * pRevSpec,
						   const char * pszStampName,
						   SG_bool bAll,
						   SG_varray ** ppvaInfo)
{
	if (bAll)
	{
		if (pRevSpec)
		{
			SG_uint32 countRevSpecs = 0;
			
			SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &countRevSpecs)  );
			if (countRevSpecs > 0)
				SG_ERR_THROW2(  SG_ERR_USAGE,
								(pCtx, "Cannot mix 'rev-spec' and 'all' options.")  );
		}

		SG_ERR_CHECK(  SG_vv2__stamp__remove_stamp_from_all_csets(pCtx, pszRepoName, pszStampName, ppvaInfo)  );
	}
	else
	{
		if (pszStampName && *pszStampName)
		{
			SG_ERR_CHECK(  SG_vv2__stamp__remove_stamp_from_cset(pCtx, pszRepoName, pRevSpec, pszStampName, ppvaInfo)  );
		}
		else
		{
			SG_ERR_CHECK(  SG_vv2__stamp__remove_all_stamps_from_cset(pCtx, pszRepoName, pRevSpec, ppvaInfo)  );
		}
	}

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * Return a list of the CSets having this stamp.
 *
 */
void SG_vv2__stamp__list(SG_context * pCtx,
						 const char * pszRepoName,
						 const char * pszStampName,
						 SG_stringarray ** ppsaHidChangesets)
{
	SG_repo * pRepo = NULL;
	SG_audit q;
	SG_bool bRepoNamed = SG_FALSE;

	// pszRepoName is optional
	SG_NONEMPTYCHECK_RETURN( pszStampName );
	SG_NULLARGCHECK_RETURN( ppsaHidChangesets );

	// open either the named repo or the repo of the WD.

	SG_ERR_CHECK(  sg_vv2__util__get_repo(pCtx, pszRepoName, &pRepo, &bRepoNamed)  );

	SG_ERR_CHECK(  SG_audit__init(pCtx,&q,pRepo,SG_AUDIT__WHEN__NOW,SG_AUDIT__WHO__FROM_SETTINGS)  );

	SG_ERR_CHECK(  SG_vc_stamps__lookup_by_stamp(pCtx, pRepo, &q, pszStampName, ppsaHidChangesets)  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
}

//////////////////////////////////////////////////////////////////

/**
 * Return a list of all stamps and their frequency.
 *
 */
void SG_vv2__stamps(SG_context * pCtx,
					const char * pszRepoName,
					SG_varray ** ppva_results)
{
	SG_repo * pRepo = NULL;
	SG_bool bRepoNamed = SG_FALSE;

	// pszRepoName is optional
	SG_NULLARGCHECK_RETURN( ppva_results );

	// open either the named repo or the repo of the WD.

	SG_ERR_CHECK(  sg_vv2__util__get_repo(pCtx, pszRepoName, &pRepo, &bRepoNamed)  );

	SG_ERR_CHECK(  SG_vc_stamps__list_all_stamps(pCtx, pRepo, ppva_results)  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
}
