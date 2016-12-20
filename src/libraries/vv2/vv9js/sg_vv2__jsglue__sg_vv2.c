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
 * @file sg_vv2__jsglue__sg_vv2.c
 *
 * @details JS glue for the "vv2" API.  This layer presents the "sg.vv2"
 * routines.
 *
 *
 * NOTE: In the following syntax diagrams, define:
 *
 *     <<rev-spec>> ::=   "rev"    : "<revision_number_or_csid_prefix>"
 *                      | "tag"    : "<tag>"
 *                      | "branch" : "<branch_name>"
 *
 *
 *     <<sz-or-array-of-sz>> ::= "<sz>" | [ "<sz1>", "<sz2>", ... ]
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

/**
 * sg.vv2.init_new_repo( { "repo"         : "<repo_name>",
 *                         "storage"      : "<storage_method>",
 *                         "hash"         : "<hash_method>",
 *                         "shared-users" : "<repo_instance>",
 *                         "no-wc"        : <bool>,
 *                         "path"         : "<path>",
 *                         "from-user-master : <bool> } )
 *
 * Create and initialize a new repository in the closet
 * with the given (repo name, storage technology, hash).
 *
 * 
 * NOTE: 2011/10/24 Currently we only have 1 storage implementation
 * NOTE:            defined: "fs3".  We'll have others later.  The
 * NOTE:            'vv' command currently has the --storage option
 * NOTE:            commented out because of this.  I'm going to leave
 * NOTE:            it enabled here for testing purposes.
 * 
 * 
 * The OPTIONAL, "shared-users" indicates that the new
 * repo should share user accounts with the given existing
 * repo instance.
 * 
 * 
 * The OPTIONAL, "no-wc" indicates that we SHOULD NOT
 * create a WD.  (Useful for WIT-only repos, for example.)
 *
 * 
 * The OPTIONAL <path> is an absolute or relative path to the
 * desired root directory (what will be mapped to "@/" after
 * we're done).  If omitted, we assume the CWD.
 *
 */

SG_JSGLUE_METHOD_PROTOTYPE(vv2, init_new_repo)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	const char * pszRepoName = NULL;	// we do not own this
	const char * pszStorage = NULL;		// we do not own this
	const char * pszHash = NULL;		// we do not own this
	const char * pszSharedUsers = NULL;	// we do not own this
	const char * pszPath = NULL;		// we do not own this
	SG_bool bNoWC = SG_FALSE;
	SG_bool bFromUserMaster = SG_FALSE;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

	SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "repo",                   &pszRepoName)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "storage",      NULL,     &pszStorage)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "hash",         NULL,     &pszHash)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "shared-users", NULL,     &pszSharedUsers)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "path",         NULL,     &pszPath)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no-wc",        SG_FALSE, &bNoWC)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "from-user-master", SG_FALSE, &bFromUserMaster)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.init_new_repo", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_vv2__init_new_repo(pCtx,
										 pszRepoName,
										 pszPath,
										 pszStorage,
										 pszHash,
										 bNoWC,
										 pszSharedUsers,
										 bFromUserMaster,
										 NULL,
										 NULL)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * sg.vv2.export( { "repo"         : "<repo_name>",
 *                  "path"         : "<path">,
 *                  "rev"          : "<revision_number_or_csid_prefix>",
 *                  "tag"          : "<tag>",
 *                  "branch"       : "<branch_name>",
 *                  "sparse"       : <<sparse-pattern-or-array-of-sparse-patterns>> } )
 *
 *                  
 * Do an EXPORT.  Create a non-working view of a changeset.
 *
 * The OPTIONAL <path> is an absolute or relative path to the
 * desired root directory (what will be mapped to "@/" after
 * we're done).  If omitted, we assume the CWD.
 * 
 *
 * The OPTIONAL "rev", "tag", and "branch" args can be used to
 * specify the changeset that we should checkout.  At most only
 * one can be given.
 *
 * 
 * TODO 2011/10/17 Document the possible values and the default
 * TODO            values of "rev", "tag", "branch".
 *
 * 
 * The OPTIONAL "sparse" when present indicates that the user
 * wants to create a "sparse" view where some portions of the VC
 * tree are not populated.  This provided list of patterns
 * define things that will be EXCLUDED from the directory.
 * These may be of the following form:
 * [1] a standard repo-path -- "@/foo/bar" -- will exclude 'bar'
 *     and anything under it.
 * [2] a standard relative or absolute pathname -- we will
 *     load the contents of this file and assume it contains
 *     a list of patterns.
 * [3] a special double-@ repo-path -- "@@/foo/bar.txt" -- we will
 *     look inside the CSET and fetch the contents of the file
 *     with this repo-path (*before* we populate) and use it as
 *     a file of patterns to be loaded.
 *
 *     [Think of this like a ".vvignores" only we can't assume
 *     the user will always want it.  You might use this to
 *     predefine (and version control) a custom list of excusions
 *     for developers vs media artists.]
 *
 * In a SPARSE EXPORT, we preserve the overall tree
 * structure/layout, so we will populate any parent directories
 * required to connect the requested items to the root directory.)
 * 
 * 
 * NOTE: We *ASSUME* that we are to do a fresh EXPORT into
 * a new/empty directory.  We *DO NOT* take a bNewOrEmpty flag.
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, export)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_rev_spec * pRevSpec = NULL;
	SG_varray * pvaSparseAllocated = NULL;
	const char * pszRepoName = NULL;	// we do not own this
	const char * pszPath = NULL;	// we do not own this
	const char * pszRev = NULL;		// we do not own this
	const char * pszTag = NULL;		// we do not own this
	const char * pszBranch = NULL;	// we do not own this
	const char * pszSparse = NULL;	// we do not own this
	SG_varray * pvaSparse = NULL;	// we do not own this

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

	SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "repo",                   &pszRepoName)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "path",         NULL,     &pszPath)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "rev",          NULL,     &pszRev)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "tag",          NULL,     &pszTag)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "branch",       NULL,     &pszBranch)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz(pCtx, pvh_args, pvh_got, "sparse",                 &pszSparse, &pvaSparse)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.export", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	if (pszRev)
		SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, pszRev)  );
	if (pszTag)
		SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pRevSpec, pszTag)  );
	if (pszBranch)
		SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, pszBranch)  );

	if (pszSparse)
	{
		SG_ERR_CHECK(  SG_VARRAY__ALLOC(pCtx, &pvaSparseAllocated)  );
		SG_ERR_CHECK(  SG_varray__append__string__sz(pCtx, pvaSparseAllocated, pszSparse)  );
		pvaSparse = pvaSparseAllocated;
	}

	SG_ERR_CHECK(  SG_vv2__export(pCtx,
								  pszRepoName,
								  pszPath,
								  pRevSpec,
								  pvaSparse)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_VARRAY_NULLFREE(pCtx, pvaSparseAllocated);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_VARRAY_NULLFREE(pCtx, pvaSparseAllocated);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * sg.vv2.comment( { "repo"    : "<repo_name>",
 *                   "rev"     : "<revision_number_or_csid_prefix>",
 *                   "tag"     : "<tag>",
 *                   "branch"  : "<branch_name>",
 *                   "message" : "<message>",
 *                   "user"    : "<user_name_or_id>"
 *                   "when"    : "<when>" } );
 *
 * Add a COMMENT to an existing CHANGESET.  This is an immediate
 * operation.  It has NOTHING to do with any WD.
 *
 *
 * TODO 2011/11/07 The method sg.repo.add_vc_comment() also does
 * TODO            what this routine does.  I added this during the
 * TODO            PendingTree==>WC conversion to mirror what I did
 * TODO            in sg.c and for testing purposes.  This version
 * TODO            uses named-parameters; the one in sg.repo is old-style.
 * TODO            I'd like to deprecate one of these and/or convert the
 * TODO            one in sg.repo to use named-parameters.  That can't
 * TODO            happen today.
 * 
 *
 * The OPTIONAL <repo_name>.  If omitted, we will try to get the
 * repo-name from the WD, if one is present.
 *
 * We REQUIRE one of the --rev/--tag/--branch.
 *
 * We REQUIRE a non-empty message.
 *
 * The OPTIONAL "user" may specify the user-name or user-id.
 * These are defined RELATIVE to the indicated REPO.  If omitted,
 * we will try to get the default WHOAMI for the indicated REPO.
 *
 * The OPTIONAL "when" can be a formatted date string of the form
 * "YYYY-MM-DD" or "YYYY-MM-DD hh:mm:ss" or a string containing
 * the time-in-ms-since-the-epoch.  If omitted, we default to the
 * current time.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, comment)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_rev_spec * pRevSpec = NULL;
	const char * pszRepoName = NULL;	// we do not own this
	const char * pszRev = NULL;			// we do not own this
	const char * pszTag = NULL;			// we do not own this
	const char * pszBranch = NULL;		// we do not own this
	const char * pszMessage = NULL;		// we do not own this
	const char * pszUser = NULL;		// we do not own this
	const char * pszWhen = NULL;		// we do not own this
	SG_uint32 count_rev_spec = 0;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "repo",    NULL, &pszRepoName)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "rev",     NULL, &pszRev)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "tag",     NULL, &pszTag)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "branch",  NULL, &pszBranch)  );
	SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "message",       &pszMessage)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "user",    NULL, &pszUser)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "when",    NULL, &pszWhen)  );

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.comment", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	if (pszRev)
		SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, pszRev)  );
	if (pszTag)
		SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pRevSpec, pszTag)  );
	if (pszBranch)
		SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, pszBranch)  );
	SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &count_rev_spec)  );
	if (count_rev_spec != 1)
        SG_ERR_THROW2(  SG_ERR_JS_NAMED_PARAM_REQUIRED,
						(pCtx, "Exactly one instance of 'rev', 'tag', or 'branch' is required")  );

	SG_ERR_CHECK(  SG_vv2__comment(pCtx, pszRepoName, pRevSpec, pszUser, pszWhen, pszMessage, NULL)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * result = sg.vv2.status( { "repo"        : "<repo_name>",
 *                           "revs"        : [ <<rev-spec-0>>,
 *                                             <<rev-spec-1>> ],
 *                           "src"         : <<path-or-array-of-paths>>,
 *                           "depth"       : <int>,
 *                           "no-sort"     : <bool>,
 *                           "legend"      : <bool> } );
 *
 * Compute "canonical" historical STATUS between 2 CSETs.
 * Like 'vv status -r r0 -r r1'
 *
 * We REQUIRE 2 CSETs to be specified.  Any combination of
 * rev/tag/branch may be used and accumulated in a revspec
 * as usual.  BECAUSE OF LIMITATIONS IN JSON SYNTAX, we need
 * those terms to be in an array and cannot just iterate
 * over keys like getopt does.
 * 
 *
 * We DO NOT know anything about the WD nor assume any
 * current baseline.  However, if <repo_name> is omitted,
 * we will try to get it from the WD if we can find one.
 *
 *
 * The OPTIONAL "legend" indicates that the caller wants
 * a legend to be returned also.  When "legend" is false
 * (the default), we return:
 *
 * result := <<array_of_status>>
 *
 * When "legend" is true, we return:
 *
 * result := { "status" : <<array_of_status>>,
 *             "legend"  : <<vhash_legend>> }
 *
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, status)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_varray * pvaStatus = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
    SG_vhash* pvh_got_k = NULL;
	JSObject* jso = NULL;
	SG_rev_spec * pRevSpec = NULL;
	const char * pszRepoName = NULL;	// we do not own this
	SG_stringarray * psa_src = NULL;
	SG_uint32 depth = SG_INT32_MAX;
	SG_bool bLegend = SG_FALSE;
	SG_bool bNoSort = SG_FALSE;
	SG_vhash * pvhLegend = NULL;
	SG_vhash * pvhResult = NULL;
	SG_varray * pvaRevs = NULL;			// we do not own this
	SG_uint32 nrRevs;
	SG_uint32 k;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(    pCtx, pvh_args, pvh_got, "repo",        NULL, &pszRepoName)  );
	SG_ERR_CHECK(  SG_jsglue__np__required__varray(pCtx, pvh_args, pvh_got, "revs",      &pvaRevs)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "src", &psa_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__uint32(pCtx, pvh_args, pvh_got, "depth",     SG_INT32_MAX, &depth)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "legend",      SG_FALSE, &bLegend)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "no-sort",     SG_FALSE, &bNoSort)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.status", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );

	SG_ERR_CHECK(  SG_varray__count(pCtx, pvaRevs, &nrRevs)  );
	if (nrRevs != 2)
		SG_ERR_THROW2(  SG_ERR_JS_NAMED_PARAM_REQUIRED,
						(pCtx, "The 'revs' field must have 2 values")  );
	for (k=0; k<nrRevs; k++)
	{
		SG_vhash* pvh_args_k = NULL;		// we do not own this
		const char * pszRev = NULL;			// we do not own this
		const char * pszTag = NULL;			// we do not own this
		const char * pszBranch = NULL;		// we do not own this
		SG_uint32 count_rev_spec = 0;

		SG_ERR_CHECK(  SG_varray__get__vhash(pCtx, pvaRevs, k, &pvh_args_k)  );
		SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got_k)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args_k, pvh_got_k, "rev",    NULL, &pszRev)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args_k, pvh_got_k, "tag",    NULL, &pszTag)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args_k, pvh_got_k, "branch", NULL, &pszBranch)  );
		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.status", pvh_args_k, pvh_got_k)  );

		if (pszRev)
		{
			SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, pszRev)  );
			count_rev_spec++;
		}
		if (pszTag)
		{
			SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pRevSpec, pszTag)  );
			count_rev_spec++;
		}
		if (pszBranch)
		{
			SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, pszBranch)  );
			count_rev_spec++;
		}
		if (count_rev_spec != 1)
			SG_ERR_THROW2(  SG_ERR_JS_NAMED_PARAM_REQUIRED,
							(pCtx,
							 "Exactly one instance of 'rev', 'tag', or 'branch' is required in 'revs[%d]'",
							 k)  );

		SG_VHASH_NULLFREE(pCtx, pvh_got_k);
	}

	SG_ERR_CHECK(  SG_vv2__status(pCtx, pszRepoName, pRevSpec,
								  psa_src, depth,
								  bNoSort,
								  &pvaStatus,
								  ((bLegend) ? &pvhLegend : NULL))  );

	if (bLegend)
	{
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhResult)  );
		SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhResult, "changes", &pvaStatus)  );	// TODO 2012/04/26 decide on the name of this field and/or fix the documentation at the top of this function.
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhResult, "legend", &pvhLegend)  );

		SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
		SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhResult, jso)  );
		SG_VHASH_NULLFREE(pCtx, pvhResult);
	}
	else
	{
		SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
		SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaStatus, jso)  );
		SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	}

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);

    return JS_TRUE;

fail:
    SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_VHASH_NULLFREE(pCtx, pvhLegend);
	SG_VHASH_NULLFREE(pCtx, pvhResult);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    SG_VHASH_NULLFREE(pCtx, pvh_got_k);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * result = sg.vv2.mstatus( { "repo"        : "<repo_name>",
 *                            "rev"         : "<revision_number_or_csid_prefix>",
 *                            "tag"         : "<tag>",
 *                            "branch"      : "<branch_name>",
 *                            "no-fallback" : <bool>,
 *                            "no-sort"     : <bool>,
 *                            "legend"      : <bool> } );
 *
 * Compute "canonical" MERGE-STATUS (aka MSTATUS) for a CSET.
 * That is, return the MSTATUS of the given cset and its parents
 * and the implied ancestor.
 *
 * We REQUIRE one of the --rev/--tag/--branch.
 * We only take 1 rev-spec because we treat it as the
 * merge-result and compare that cset with its parent(s).
 * If the given cset is a merge result, the returned MSTATUS
 * will refer to:
 *      A
 *     / \.
 *    B   C
 *     \ /
 *      M
 *
 * If the given cset is not a merge result (and fallback is
 * allowed), we will return a regular STATUS comparing the
 * single parent with the given cset:
 *      A
 *      |
 *      M
 *
 * 
 * The OPTIONAL <repo_name>.  If omitted, we will try to get the
 * repo-name from the WD, if one is present.
 *
 *
 * The OPTIONAL "no-fallback" indicates that we should throw
 * if the given cset is not a merge node.
 *
 *
 * The OPTIONAL "legend" indicates that the caller wants
 * a legend to be returned also.  When "legend" is false
 * (the default), we return:
 *
 * result := <<array_of_mstatus>>
 *
 * When "legend" is true, we return:
 *
 * result := { "mstatus" : <<array_of_mstatus>>,
 *             "legend"  : <<vhash_legend>> }
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, mstatus)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_varray * pvaStatus = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;
	SG_rev_spec * pRevSpec = NULL;
	const char * pszRepoName = NULL;	// we do not own this
	const char * pszRev = NULL;			// we do not own this
	const char * pszTag = NULL;			// we do not own this
	const char * pszBranch = NULL;		// we do not own this
	SG_bool bNoFallback = SG_FALSE;
	SG_bool bLegend = SG_FALSE;
	SG_bool bNoSort = SG_FALSE;
	SG_uint32 count_rev_spec = 0;
	SG_vhash * pvhLegend = NULL;
	SG_vhash * pvhResult = NULL;

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "repo",        NULL, &pszRepoName)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "rev",         NULL, &pszRev)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "tag",         NULL, &pszTag)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "branch",      NULL, &pszBranch)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "no-fallback", SG_FALSE, &bNoFallback)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "legend",      SG_FALSE, &bLegend)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "no-sort",     SG_FALSE, &bNoSort)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.mstatus", pvh_args, pvh_got)  );
	}

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	if (pszRev)
		SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, pszRev)  );
	if (pszTag)
		SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pRevSpec, pszTag)  );
	if (pszBranch)
		SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, pszBranch)  );
	SG_ERR_CHECK(  SG_rev_spec__count(pCtx, pRevSpec, &count_rev_spec)  );
	if (count_rev_spec != 1)
        SG_ERR_THROW2(  SG_ERR_JS_NAMED_PARAM_REQUIRED,
						(pCtx, "Exactly one instance of 'rev', 'tag', or 'branch' is required")  );

	SG_ERR_CHECK(  SG_vv2__mstatus(pCtx, pszRepoName, pRevSpec, bNoFallback, bNoSort,
								   &pvaStatus,
								   ((bLegend) ? &pvhLegend : NULL))  );

	if (bLegend)
	{
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhResult)  );
		SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhResult, "changes", &pvaStatus)  );	// TODO 2012/04/26 decide on the name of this field and/or fix the documentation at the top of this function.
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhResult, "legend", &pvhLegend)  );

		SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
		SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhResult, jso)  );
		SG_VHASH_NULLFREE(pCtx, pvhResult);
	}
	else
	{
		SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
		SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaStatus, jso)  );
		SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	}

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);

    return JS_TRUE;

fail:
    SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_VHASH_NULLFREE(pCtx, pvhLegend);
	SG_VHASH_NULLFREE(pCtx, pvhResult);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * result = sg.vv2.stamps( { "repo" : "<repo_name>"              -- optional
 *                         } );
 * result = sg.vv2.stamps( );
 *
 * Return a summary of the stamps in use and their frequency
 * in this repo.
 * 
 * 
 * The OPTIONAL <repo_name>.  If omitted, we will try to get the
 * repo-name from the WD, if one is present.
 *
 *
 * result := [ { "stamp" : "<stampname_k>" , "count" : <int_k> },
 *             ... ];
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, stamps)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_varray * pvaStamps = NULL;
	const char * pszRepoName = NULL;	// we do not own this
	JSObject* jso = NULL;

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz( pCtx, pvh_args, pvh_got, "repo", NULL, &pszRepoName)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.stamps", pvh_args, pvh_got)  );
	}

	SG_ERR_CHECK(  SG_vv2__stamps(pCtx, pszRepoName, &pvaStamps)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaStamps, jso)  );

    SG_VARRAY_NULLFREE(pCtx, pvaStamps);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    return JS_TRUE;

fail:
    SG_VARRAY_NULLFREE(pCtx, pvaStamps);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * result = sg.vv2.add_stamp( { "repo"   : "<repo_name>",
 *                              "stamp"  : "<stamp_name>",
 *                              "rev"    : <<sz-or-array-of-sz>>, -- optional
 *                              "tag"    : <<sz-or-array-of-sz>>, -- optional
 *                              "branch" : <<sz-or-array-of-sz>>, -- optional
 *                            } );
 *
 * Add a STAMP to one or more changesets.
 *
 * 
 * NOTE THAT WE DEVIATE FROM THE NORMAL MODEL AND ALLOW
 * "rev", "tag", AND "branch" TO HAVE AN ARRAY OF VALUES.
 * Here they indicate are used to indicate the set of CSets
 * that we want to stamp -- rather than the
 * traditional meaning of picking a specific CSet for some
 * purpose.  And order is not important so we don't need
 * the "revs" syntax like in sg.vv2.status().
 *
 * 
 * The OPTIONAL <repo_name>.  If omitted, we will try to get the
 * repo-name from the WD, if one is present.
 *                              
 * The OPTIONAL "rev", "tag", and "branch" args can be used to specify the
 * changeset(s) that we should stamp.  If omitted and we got the repo from
 * the WD, we assume the current parent(s) of the WD.
 *
 * We return an array containing the list of the changesets changed
 * and whether it already had the stamp.
 *
 * result := [ { "hid" : "<hid_k>", "redundant" : <bool> },
 *             ...
 *           ];
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, add_stamp)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_stringarray * psa_rev = NULL;
	SG_stringarray * psa_tag = NULL;
	SG_stringarray * psa_branch = NULL;
	SG_rev_spec * pRevSpec = NULL;
	const char * pszRepoName = NULL;	// we do not own this
	const char * pszStampName = NULL;	// we do not own this
	SG_varray * pvaInfo = NULL;
	JSObject* jso = NULL;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "repo",    NULL, &pszRepoName)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "stamp",         &pszStampName)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "rev", &psa_rev)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "tag", &psa_tag)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "branch", &psa_branch)  );

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.add_stamp", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	if (psa_rev)
	{
		SG_uint32 k, count;
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_rev, &count)  );
		for (k=0; k<count; k++)
		{
			const char * psz_k;
			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa_rev, k, &psz_k)  );
			SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, psz_k)  );
		}
	}
	if (psa_tag)
	{
		SG_uint32 k, count;
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_tag, &count)  );
		for (k=0; k<count; k++)
		{
			const char * psz_k;
			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa_tag, k, &psz_k)  );
			SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pRevSpec, psz_k)  );
		}
	}
	if (psa_branch)
	{
		SG_uint32 k, count;
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_branch, &count)  );
		for (k=0; k<count; k++)
		{
			const char * psz_k;
			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa_branch, k, &psz_k)  );
			SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, psz_k)  );
		}
	}

	SG_ERR_CHECK(  SG_vv2__stamp__add(pCtx, pszRepoName, pRevSpec, pszStampName, &pvaInfo)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaInfo, jso)  );

    SG_VARRAY_NULLFREE(pCtx, pvaInfo);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_rev);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_tag);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_branch);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
    return JS_TRUE;

fail:
    SG_VARRAY_NULLFREE(pCtx, pvaInfo);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_rev);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_tag);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_branch);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * result = sg.vv2.remove_stamp( { "repo"   : "<repo_name>",
 *                                 "stamp"  : "<stamp_name>",
 *                                 "all"    : <bool>,
 *                                 "rev"    : <<sz-or-array-of-sz>>, -- optional
 *                                 "tag"    : <<sz-or-array-of-sz>>, -- optional
 *                                 "branch" : <<sz-or-array-of-sz>>, -- optional
 *                               } );
 *
 * Remove a STAMP on one or more changesets.
 *
 *
 * NOTE THAT WE DEVIATE FROM THE NORMAL MODEL AND ALLOW
 * "rev", "tag", AND "branch" TO HAVE AN ARRAY OF VALUES.
 * Here they indicate are used to indicate the set of CSets
 * that we want to stamp -- rather than the
 * traditional meaning of picking a specific CSet for some
 * purpose.  And order is not important so we don't need
 * the "revs" syntax like in sg.vv2.status().
 *
 * 
 * The OPTIONAL <repo_name>.  If omitted, we will try to get the
 * repo-name from the WD, if one is present.
 *                              
 * The OPTIONAL "rev", "tag", and "branch" args can be used to
 * specify the changeset(s) that we should "un-stamp".  If omitted
 * and we got the repo from the WD,
 * we assume the current parent(s) of the WD.
 *
 * 
 * The OPTIONAL "all" indicates that all instances of the stamp should
 * be removed across all changesets.  If "all", no <rev-specs> are allowed.
 *
 * The OPTIONAL <stamp_name>.  If omitted (and we are given specific
 * csets), we will remove all of the stamps on that/those cset(s).
 * 
 *
 * We return an array each containing the HID of a changeset that was
 * changed.
 *
 * result := [ { "hid" : "<hid_k>" },
 *             ...
 *           ];
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, remove_stamp)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_stringarray * psa_rev = NULL;
	SG_stringarray * psa_tag = NULL;
	SG_stringarray * psa_branch = NULL;
	SG_rev_spec * pRevSpec = NULL;
	const char * pszRepoName = NULL;	// we do not own this
	const char * pszStampName = NULL;	// we do not own this
	SG_varray * pvaInfo = NULL;
	JSObject* jso = NULL;
	SG_bool bAll = SG_FALSE;

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "repo",    NULL,     &pszRepoName)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "stamp",   NULL,     &pszStampName)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "rev", &psa_rev)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "tag", &psa_tag)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "branch", &psa_branch)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "all",     SG_FALSE, &bAll)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.remove_stamp", pvh_args, pvh_got)  );
	}

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	if (psa_rev)
	{
		SG_uint32 k, count;
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_rev, &count)  );
		for (k=0; k<count; k++)
		{
			const char * psz_k;
			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa_rev, k, &psz_k)  );
			SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, psz_k)  );
		}
	}
	if (psa_tag)
	{
		SG_uint32 k, count;
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_tag, &count)  );
		for (k=0; k<count; k++)
		{
			const char * psz_k;
			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa_tag, k, &psz_k)  );
			SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pRevSpec, psz_k)  );
		}
	}
	if (psa_branch)
	{
		SG_uint32 k, count;
		SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa_branch, &count)  );
		for (k=0; k<count; k++)
		{
			const char * psz_k;
			SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa_branch, k, &psz_k)  );
			SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, psz_k)  );
		}
	}

	SG_ERR_CHECK(  SG_vv2__stamp__remove(pCtx, pszRepoName, pRevSpec, pszStampName, bAll, &pvaInfo)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaInfo, jso)  );

    SG_VARRAY_NULLFREE(pCtx, pvaInfo);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_rev);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_tag);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_branch);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
    return JS_TRUE;

fail:
    SG_VARRAY_NULLFREE(pCtx, pvaInfo);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_rev);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_tag);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_branch);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * result = sg.vv2.list_stamp( { "repo"   : "<repo_name>",
 *                               "stamp"  : "<stamp_name>"
 *                             } );
 *
 * Return the set of CSETs that have this STAMP.
 *
 * 
 * The OPTIONAL <repo_name>.  If omitted, we will try to get the
 * repo-name from the WD, if one is present.
 *                              
 * We return an array containing the list of the changesets having
 * this stamp.
 *
 * result := [ "<hid_k>", ... ];
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, list_stamp)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	const char * pszRepoName = NULL;	// we do not own this
	const char * pszStampName = NULL;	// we do not own this
	SG_stringarray * psa_hids = NULL;
	JSObject* jso = NULL;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "repo",    NULL, &pszRepoName)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "stamp",         &pszStampName)  );

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.list_stamp", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_vv2__stamp__list(pCtx, pszRepoName, pszStampName, &psa_hids)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_stringarray_into_jsobject(pCtx, cx, psa_hids, jso)  );

	SG_STRINGARRAY_NULLFREE(pCtx, psa_hids);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    return JS_TRUE;

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psa_hids);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * result = sg.vv2.tags( { "repo"  : "<repo_name>",         -- optional
 *                         "revno" : <bool>,                -- optional
 *                       } );
 *
 * Returns a list of the active tags.
 *
 * If OPTIONAL "revno" is true, we also get the revno's of each cset.
 * (This is not on by default because it is more expensive.)
 *
 * If "revno" is requested and a cset is not present in the local repo,
 * we omit the revno from the result (and the caller can report that it
 * is missing from the local repo).
 *
 *
 * result := [ { "tag" : "<tag_name_k>", "csid" : "<hid_cset_k>" },
 *             ... ];
 *
 * or
 * 
 * result := [ { "tag" : "<tag_name_k>", "csid" : "<hid_cset_k>", "revno" : <int_k> },
 *             ... ];
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, tags)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_varray * pvaTags = NULL;
	const char * pszRepoName = NULL;	// we do not own this
	SG_bool bGetRevno = SG_FALSE;
	JSObject* jso = NULL;

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "repo",  NULL,     &pszRepoName)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "revno", SG_FALSE, &bGetRevno)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.tags", pvh_args, pvh_got)  );
	}

	SG_ERR_CHECK(  SG_vv2__tags(pCtx, pszRepoName, bGetRevno, &pvaTags)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaTags, jso)  );

    SG_VARRAY_NULLFREE(pCtx, pvaTags);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    return JS_TRUE;

fail:
    SG_VARRAY_NULLFREE(pCtx, pvaTags);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * sg.vv2.add_tag( { "repo"  : "<repo_name>",
 *                   "value" : "<tag_name_to_add>",  -- note key name is 'value' because of collision with <<rev-spec>>
 *                   <<rev-spec>>,
 *                   "force" : <bool>                -- force move if it already exists
 *                 } );
 *
 * Add a TAG to a changeset identified by <<rev-spec>>.
 *
 * 
 * The OPTIONAL <repo_name>.  If omitted, we will try to get the
 * repo-name from the WD, if one is present.
 *                              
 * The OPTIONAL "rev", "tag", and "branch" args can be used to
 * specify the changeset that we should tag.  At most only
 * one can be given.  If omitted and we got the repo from the WD,
 * we assume the current parent of the WD.
 *
 * The OPTIONAL "force" indicates that we should force a 'move-tag'
 * if the tag already exists.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, add_tag)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_rev_spec * pRevSpec = NULL;
	const char * pszRepoName = NULL;	// we do not own this
	const char * pszRev = NULL;			// we do not own this
	const char * pszTag = NULL;			// we do not own this
	const char * pszBranch = NULL;		// we do not own this
	const char * pszTagName = NULL;	// we do not own this
	SG_bool bForceMove = SG_FALSE;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "repo",    NULL, &pszRepoName)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "value",         &pszTagName)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "rev",     NULL, &pszRev)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "tag",     NULL, &pszTag)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "branch",  NULL, &pszBranch)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "force", SG_FALSE, &bForceMove)  );

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.add_tag", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	if (pszRev)
		SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, pszRev)  );
	if (pszTag)
		SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pRevSpec, pszTag)  );
	if (pszBranch)
		SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, pszBranch)  );

	SG_ERR_CHECK(  SG_vv2__tag__add(pCtx, pszRepoName, pRevSpec, pszTagName, bForceMove)  );

    JS_SET_RVAL(cx, vp, JSVAL_VOID);

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * sg.vv2.move_tag( { "repo"  : "<repo_name>",
 *                    "value" : "<tag_name_to_move>",  -- note key name is 'value' because of collision with <<rev-spec>>
 *                    <<rev-spec>>
 *                  } );
 *
 * Move a TAG to a changeset identified by <<rev-spec>>
 *
 * 
 * The OPTIONAL <repo_name>.  If omitted, we will try to get the
 * repo-name from the WD, if one is present.
 *                              
 * The OPTIONAL "rev", "tag", and "branch" args can be used to
 * specify the changeset that we should tag.  At most only
 * one can be given.  If omitted and we got the repo from the WD,
 * we assume the current parent of the WD.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, move_tag)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_rev_spec * pRevSpec = NULL;
	const char * pszRepoName = NULL;	// we do not own this
	const char * pszRev = NULL;			// we do not own this
	const char * pszTag = NULL;			// we do not own this
	const char * pszBranch = NULL;		// we do not own this
	const char * pszTagName = NULL;	// we do not own this

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "repo",    NULL, &pszRepoName)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "value",         &pszTagName)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "rev",     NULL, &pszRev)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "tag",     NULL, &pszTag)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "branch",  NULL, &pszBranch)  );

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.move_tag", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	if (pszRev)
		SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, pszRev)  );
	if (pszTag)
		SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pRevSpec, pszTag)  );
	if (pszBranch)
		SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, pszBranch)  );

	SG_ERR_CHECK(  SG_vv2__tag__move(pCtx, pszRepoName, pRevSpec, pszTagName)  );

    JS_SET_RVAL(cx, vp, JSVAL_VOID);

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * result = sg.vv2.remove_tag( { "repo"  : "<repo_name>",
 *                               "value" : "<tag_name>"   -- note name is 'value' to avoid confusion with <<rev-spec>>
 *                               } );
 *
 * Remove a TAG if it exists.
 *
 * 
 * The OPTIONAL <repo_name>.  If omitted, we will try to get the
 * repo-name from the WD, if one is present.
 *                              
 *
 * Note that (unlike a STAMP) a TAG is unique.  Therefore, we
 * DO NOT need a REVSPEC to uniquely identify if.
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, remove_tag)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	const char * pszRepoName = NULL;	// we do not own this
	const char * pszTagName = NULL;	// we do not own this

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "repo",  NULL, &pszRepoName)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__sz(  pCtx, pvh_args, pvh_got, "value",       &pszTagName)  );

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.remove_tag", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_vv2__tag__remove(pCtx, pszRepoName, pszTagName)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * result = sg.vv2.leaves( { "repo"   : "<repo_name>",       -- optional
 *                           "dagnum" : "<hex_sz>"     } );  -- optional (mainly for debugging) defaults to VC
 *
 * Returns a varray of the hids of the leaves.
 *
 * 
 * The OPTIONAL <repo_name>.  If omitted, we will try to get the
 * repo-name from the WD, if one is present.
 *
 * 
 * result := [ "<hid_k>", ... ];
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, leaves)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	const char * pszRepoName = NULL;	// we do not own this
	const char * pszHexDagNum = NULL;	// we do not own this
	SG_uint64 ui64DagNum = SG_DAGNUM__VERSION_CONTROL;
	SG_varray * pvaHidLeaves = NULL;
	JSObject* jso = NULL;

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "repo",   NULL, &pszRepoName)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "dagnum", NULL, &pszHexDagNum)  );
		if (pszHexDagNum && *pszHexDagNum)
			SG_ERR_CHECK(  SG_dagnum__from_sz__hex(pCtx, pszHexDagNum, &ui64DagNum)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.leaves", pvh_args, pvh_got)  );
	}

	SG_ERR_CHECK(  SG_vv2__leaves(pCtx, pszRepoName, ui64DagNum, &pvaHidLeaves)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaHidLeaves, jso)  );

	SG_VARRAY_NULLFREE(pCtx, pvaHidLeaves);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    return JS_TRUE;

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaHidLeaves);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

static void _import_sa_into_revspec(SG_context * pCtx,
									SG_rev_spec * pRevSpec,
									SG_rev_spec_type t,
									const SG_stringarray * psa)
{
	SG_uint32 k, count;

	if (!psa)
		return;
	
	SG_ERR_CHECK(  SG_stringarray__count(pCtx, psa, &count)  );
	for (k=0; k<count; k++)
	{
		const char * psz_k;
		SG_ERR_CHECK(  SG_stringarray__get_nth(pCtx, psa, k, &psz_k)  );
		SG_ERR_CHECK(  SG_rev_spec__add(pCtx, pRevSpec, t, psz_k)  );
	}

fail:
	return;
}

/**
 * history = sg.vv2.history( { "repo"        : "<repo_name>",         -- optional
 *                             "leaves"      : <bool>,                -- optional
 *                             "from"        : "<from_date>",         -- optional
 *                             "to"          : "<to_date>",           -- optional
 *                             "rev"         : <<sz-or-array-of-sz>>, -- optional
 *                             "tag"         : <<sz-or-array-of-sz>>, -- optional
 *                             "branch"      : <<sz-or-array-of-sz>>, -- optional
 *                             "stamp"       : "<stamp_name>",        -- optional
 *                             "max"         : <int>,                 -- optional
 *                             "user"        : "<user_name>",         -- optional
 *                             "hide-merges" : <bool>,                -- optional
 *                             "reverse"     : <bool>,                -- optional
 *                             "list-all"    : <bool>,                -- optional
 *                             "src"         : <<sz-or-array-of-sz>>  -- optional (the files-or-folders arg)
 *                           } );
 *
 * NOTE THAT WE DEVIATE FROM THE NORMAL MODEL AND ALLOW
 * "rev", "tag", AND "branch" TO HAVE AN ARRAY OF VALUES.
 * Here they indicate are used to indicate the set of CSets
 * that we are including in the filter -- rather than the
 * traditional meaning of picking a specific CSet for some
 * purpose.  And order is not important so we don't need
 * the "revs" syntax like in sg.vv2.status().
 *
 *
 * The OPTIONAL "src" is the files-or-folders list that
 * we want the history on.  When "repo" is given, src paths
 * must be full repo-paths.  When "repo" is not given (and
 * we have to look for a WD in the CWD), they can be full
 * repo-paths and/or relative/absolute paths.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, history)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;
	SG_vv2_history_args historyArgs;
	SG_stringarray * psa_rev = NULL;
	SG_stringarray * psa_tag = NULL;
	SG_stringarray * psa_branch = NULL;
	SG_stringarray * psa_src = NULL;
	SG_rev_spec * pRevSpec = NULL;
	SG_rev_spec * pRevSpec_single_revisions = NULL;
	SG_history_result* pHistoryResult = NULL;
	SG_varray * pvaRef = NULL;	// we do not own this

	SG_ERR_CHECK(  memset(&historyArgs, 0, sizeof(historyArgs))  );
	historyArgs.nToDate = SG_INT64_MAX;
	historyArgs.nResultLimit = SG_UINT32_MAX;

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "repo",       NULL,                  &historyArgs.pszRepoDescriptorName)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "leaves",     SG_FALSE,              &historyArgs.bLeaves)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__when(pCtx, pvh_args, pvh_got, "from",       0,                     &historyArgs.nFromDate)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__when(pCtx, pvh_args, pvh_got, "to",         SG_INT64_MAX,          &historyArgs.nToDate  )  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "rev",    &psa_rev)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "tag",    &psa_tag)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "branch", &psa_branch)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "stamp",       NULL,                 &historyArgs.pszStamp)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__uint32(pCtx, pvh_args, pvh_got, "max",       SG_UINT32_MAX,        &historyArgs.nResultLimit)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "user",        NULL,                 &historyArgs.pszUser)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "hide-merges", SG_FALSE,             &historyArgs.bHideObjectMerges)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "reverse",     SG_FALSE,             &historyArgs.bReverse)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "list-all",    SG_FALSE,             &historyArgs.bListAll)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "src",    &psa_src)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "sg.vv2.history", pvh_args, pvh_got)  );
	}
	historyArgs.psaSrcArgs = psa_src;

	// if bListAll we keep the --rev and --tag with --branch in the main pRevSpec.
	// Otherwise, we split them out separately.

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	if (historyArgs.bListAll)
	{
		if (psa_rev)
			SG_ERR_CHECK(  _import_sa_into_revspec(pCtx, pRevSpec, SPECTYPE_REV, psa_rev)  );
		if (psa_tag)
			SG_ERR_CHECK(  _import_sa_into_revspec(pCtx, pRevSpec, SPECTYPE_TAG, psa_tag)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec_single_revisions)  );
		if (psa_rev)
			SG_ERR_CHECK(  _import_sa_into_revspec(pCtx, pRevSpec_single_revisions, SPECTYPE_REV, psa_rev)  );
		if (psa_tag)
			SG_ERR_CHECK(  _import_sa_into_revspec(pCtx, pRevSpec_single_revisions, SPECTYPE_TAG, psa_tag)  );
	}
	if (psa_branch)
		SG_ERR_CHECK(  _import_sa_into_revspec(pCtx, pRevSpec, SPECTYPE_BRANCH, psa_branch)  );

	historyArgs.pRevSpec = pRevSpec;
	historyArgs.pRevSpec_single_revisions = pRevSpec_single_revisions;

	SG_ERR_CHECK(  SG_vv2__history(pCtx, &historyArgs, NULL, NULL, &pHistoryResult)  );

	if (pHistoryResult)
	{
		SG_ERR_CHECK(  SG_history_result__get_root(pCtx, pHistoryResult, &pvaRef)  );

        SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
        SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaRef, jso)  );

		SG_HISTORY_RESULT_NULLFREE(pCtx, pHistoryResult);
	}
	else
    {
		JS_SET_RVAL(cx, vp, JSVAL_VOID);
    }
		
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_rev);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_tag);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_branch);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec_single_revisions);
	SG_HISTORY_RESULT_NULLFREE(pCtx, pHistoryResult);
	return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_rev);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_tag);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_branch);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec_single_revisions);
	SG_HISTORY_RESULT_NULLFREE(pCtx, pHistoryResult);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * vhash = sg.vv2.locks( { "repo"        : "<repo_name>",         -- optional
 *                         "branch"      : "<branch_name>",       -- optional
 *                         "server"      : "<server_name>",       -- optional
 *                         "user"        : "<user_name>",         -- optional
 *                         "password"    : "<password>",          -- optional
 *                         "pull"        : <bool>,                -- optional
 *                        } );
 *
 * TODO 2012/07/06 Should we build a stringarray of branch names?
 *
 *
 * TODO describe returned vhash format.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, locks)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;
	const char * pszRepoName = NULL;
	const char * pszBranchName = NULL;
	const char * pszServerName = NULL;
	const char * pszUserName = NULL;
	const char * pszPassword = NULL;
	SG_bool bPull = SG_FALSE;

	SG_vhash * pvh_result = NULL;
	char *     psz_tied_branch_name = NULL;
	SG_vhash * pvh_locks_by_branch = NULL;
	SG_vhash * pvh_violated_locks = NULL;
	SG_vhash * pvh_duplicate_locks = NULL;

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "repo",     NULL,     &pszRepoName)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "branch",   NULL,     &pszBranchName)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "server",   NULL,     &pszServerName)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "user",     NULL,     &pszUserName)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "password", NULL,     &pszPassword)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "pull",     SG_FALSE, &bPull)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid(pCtx, "sg.vv2.locks", pvh_args, pvh_got)  );
	}

	SG_ERR_CHECK(  SG_vv2__locks(pCtx, pszRepoName, pszBranchName, pszServerName,
								 pszUserName, pszPassword,
								 bPull, SG_TRUE,
								 &psz_tied_branch_name, &pvh_locks_by_branch, &pvh_violated_locks, &pvh_duplicate_locks)  );

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_result)  );
	// we could probably ASSERT that all 4 return variables are set
	// rather than just guarding against null, but this is kinder.
	if (psz_tied_branch_name && *psz_tied_branch_name)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvh_result, "branch", psz_tied_branch_name)  );
	if (pvh_locks_by_branch)
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_result, "locks", &pvh_locks_by_branch)  );
    if (pvh_duplicate_locks)
        SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_result, "duplicate", &pvh_duplicate_locks)  );
    if (pvh_violated_locks)
        SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvh_result, "violated", &pvh_violated_locks)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvh_result, jso)  );

    SG_VHASH_NULLFREE(pCtx, pvh_result);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_NULLFREE(pCtx, psz_tied_branch_name);

	return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_result);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_NULLFREE(pCtx, psz_tied_branch_name);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * data = sg.vv2.cat( { "repo" : "<repo_name>",         -- optional
 *                      <<rev-spec>>,                   -- optional
 *                      "src"  : <<sz-or-array-of-sz>>, -- require at least 1
 *                      "output"  : "<pathname>"        -- optional
 *                      "console" : <bool>              -- optional
 *                    } );
 *
 * When "output" is defined, we write the contents of the requested
 * file(s) as of the requested cset to this pathname.  Think of this
 * as like "/bin/cat src0 src1 ... > pathname".  And we return nothing.
 *
 * When "console" is true, we write the contents of the requested
 * files as of the requested cset to the console.  Think of this as
 * like "/bin/cat src0 src1 ..." to the console.
 * And we return nothing.
 *
 * Otherwise, we return enough info to let you do your own 'cat'.
 * We don't actually return the contents of the individual files,
 * but rather the HIDs and sizes that the files had in the indicated
 * cset.  You can then use this info with the various sg.repo.fetch_blob
 * routines to write the contents where you want.  In this case, we return
 * an array with one row for each source item:
 *
 * data :=  [ { "gid" : "<gid>", "path" : "<repo-path>", "hid" : "<hid>", "len" : <len> },
 *            ... ]
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, cat)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_rev_spec * pRevSpec = NULL;
	const char * pszRepoName = NULL;	// we do not own this
	const char * pszRev = NULL;			// we do not own this
	const char * pszTag = NULL;			// we do not own this
	const char * pszBranch = NULL;		// we do not own this
	const char * pszOutput = NULL;		// we do not own this
	SG_bool bConsole = SG_FALSE;
	JSObject* jso = NULL;
	SG_stringarray * psa_src = NULL;
	SG_varray * pvaResults = NULL;
	SG_pathname * pPathOutput = NULL;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "repo",    NULL,     &pszRepoName)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "rev",     NULL,     &pszRev)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "tag",     NULL,     &pszTag)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "branch",  NULL,     &pszBranch)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "output",  NULL,     &pszOutput)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "console", SG_FALSE, &bConsole)  );
	SG_ERR_CHECK(  SG_jsglue__np__required__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "src", &psa_src)  );

	SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.cat", pvh_args, pvh_got)  );

	if (pszOutput && bConsole)
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Cannot use both 'output' and 'console'.")  );

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
	if (pszRev)
		SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, pszRev)  );
	if (pszTag)
		SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pRevSpec, pszTag)  );
	if (pszBranch)
		SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, pszBranch)  );

	if (pszOutput)
	{
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathOutput, pszOutput)  );
		SG_ERR_CHECK(  SG_vv2__cat__to_pathname(pCtx, pszRepoName, pRevSpec, psa_src, pPathOutput)  );

		JS_SET_RVAL(cx, vp, JSVAL_VOID);
	}
	else if (bConsole)
	{
		SG_ERR_CHECK(  SG_vv2__cat__to_console(pCtx, pszRepoName, pRevSpec, psa_src)  );

		JS_SET_RVAL(cx, vp, JSVAL_VOID);
	}
	else
	{
		SG_ERR_CHECK(  SG_vv2__cat(pCtx, pszRepoName, pRevSpec, psa_src, &pvaResults, NULL)  );

		SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
		SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaResults, jso)  );
	}

    SG_VARRAY_NULLFREE(pCtx, pvaResults);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_PATHNAME_NULLFREE(pCtx, pPathOutput);

    return JS_TRUE;

fail:
    SG_VARRAY_NULLFREE(pCtx, pvaResults);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_PATHNAME_NULLFREE(pCtx, pPathOutput);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * hid = sg.vv2.hid( { "repo" : "<repo_name>",          -- optional
 *                     "path" : "<pathname>" } );       -- required, absolute or relative path (not repo-path)
 *
 * Compute the HID of a file using the hash algorithm associated
 * with the given named repo; if repo name is omitted, default to
 * the repo associated with the current WD, if present.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, hid)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	const char * pszRepoName = NULL;	// we do not own this
	const char * pszPath = NULL;		// we do not own this
    SG_pathname* pPath = NULL;
	char * pszHid = NULL;
	JSString * pjs = NULL;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

	SG_ERR_CHECK(  SG_jsglue__np__optional__sz( pCtx, pvh_args, pvh_got, "repo", NULL, &pszRepoName)  );
	SG_ERR_CHECK(  SG_jsglue__np__required__sz( pCtx, pvh_args, pvh_got, "path",       &pszPath)  );

    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.hid", pvh_args, pvh_got)  );

    SG_PATHNAME__ALLOC__SZ(pCtx, &pPath, pszPath);
	SG_ERR_CHECK(  SG_vv2__compute_file_hid(pCtx, pszRepoName, pPath, &pszHid)  );
	SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, pszHid))  );
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

    SG_NULLFREE(pCtx, pszHid);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    return JS_TRUE;

fail:
    SG_NULLFREE(pCtx, pszHid);
    SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}


/**
 * whoami = sg.vv2.whoami( { "repo" : "<repo_name>",          -- optional
 *							"username" : "<string>",       -- optional, the username to set
 *                          "create"   : "<bool>" });  --optional, create the user if it doesn't already exist.
 *
 * Set the current user identity.  This will apply to all repositories that share a commin admin_id.
 * The current username (after any username changes from the username parameter) is always returned from whoami.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(vv2, whoami)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	const char * pszRepoName = NULL;	// we do not own this
	const char * pszUsername = NULL;		// we do not own this
	char * pszNewestUsername = NULL;
	SG_bool bCreate = SG_FALSE;
	JSString * pjs = NULL;

	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (argc == 1) );
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_vhash__alloc(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz( pCtx, pvh_args, pvh_got, "repo", NULL, &pszRepoName)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz( pCtx, pvh_args, pvh_got, "username", NULL, &pszUsername)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool( pCtx, pvh_args, pvh_got, "create", SG_FALSE, &bCreate)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.vv2.whoami", pvh_args, pvh_got)  );
	}

    SG_ERR_CHECK(  SG_vv2__whoami(pCtx, pszRepoName, pszUsername, bCreate,&pszNewestUsername)  );
	SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, pszNewestUsername))  );
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

	SG_NULLFREE(pCtx, pszNewestUsername);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    return JS_TRUE;

fail:
	SG_NULLFREE(pCtx, pszNewestUsername);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

static JSClass sg_vv2__class = {
    "sg_vv2",
    0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

// All sg.vv2.* methods use the __np__ "Named Parameters" calling
// method (which gets passed in as a single jsobject/vhash), so
// all of the methods declare here that they take exactly 1 argument.
static JSFunctionSpec sg_vv2__methods[] = {

	{ "init_new_repo",    SG_JSGLUE_METHOD_NAME(vv2, init_new_repo),   1,0},
	{ "export",           SG_JSGLUE_METHOD_NAME(vv2, export),          1,0},
	{ "comment",          SG_JSGLUE_METHOD_NAME(vv2, comment),         1,0},
	{ "status",           SG_JSGLUE_METHOD_NAME(vv2, status),          1,0},
	{ "mstatus",          SG_JSGLUE_METHOD_NAME(vv2, mstatus),         1,0},

	{ "stamps",           SG_JSGLUE_METHOD_NAME(vv2, stamps),          1,0},
	{ "add_stamp",        SG_JSGLUE_METHOD_NAME(vv2, add_stamp),       1,0},
	{ "remove_stamp",     SG_JSGLUE_METHOD_NAME(vv2, remove_stamp),    1,0},
	{ "list_stamp",       SG_JSGLUE_METHOD_NAME(vv2, list_stamp),      1,0},

	{ "tags",             SG_JSGLUE_METHOD_NAME(vv2, tags),            1,0},
	{ "add_tag",          SG_JSGLUE_METHOD_NAME(vv2, add_tag),         1,0},
	{ "move_tag",         SG_JSGLUE_METHOD_NAME(vv2, move_tag),        1,0},
	{ "remove_tag",       SG_JSGLUE_METHOD_NAME(vv2, remove_tag),      1,0},

	{ "leaves",           SG_JSGLUE_METHOD_NAME(vv2, leaves),          1,0},

	{ "history",          SG_JSGLUE_METHOD_NAME(vv2, history),         1,0},

	{ "locks",            SG_JSGLUE_METHOD_NAME(vv2, locks),           1,0},

	{ "cat",              SG_JSGLUE_METHOD_NAME(vv2, cat),             1,0},

	{ "hid",              SG_JSGLUE_METHOD_NAME(vv2, hid),             1,0},

	{ "whoami",           SG_JSGLUE_METHOD_NAME(vv2, whoami),          0,0},

	{ NULL, NULL, 0, 0}
};

//////////////////////////////////////////////////////////////////

/**
 * Install the 'vv2' api into the 'sg' namespace.
 *
 * The main 'vv2' class is publicly available in 'sg.vv2'
 * which makes things like 'sg.vv2.export()' globally available.
 *
 */
void sg_vv2__jsglue__install_scripting_api__vv2(SG_context * pCtx,
												JSContext * cx,
												JSObject * glob,
												JSObject * pjsobj_sg)
{
	JSObject* pjsobj_vv2 = NULL;

	SG_UNUSED( glob );

	SG_JS_NULL_CHECK(  pjsobj_vv2 = JS_DefineObject(cx, pjsobj_sg, "vv2", &sg_vv2__class, NULL, 0)  );
	SG_JS_BOOL_CHECK(  JS_DefineFunctions(cx, pjsobj_vv2, sg_vv2__methods)  );
	return;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return;
}
