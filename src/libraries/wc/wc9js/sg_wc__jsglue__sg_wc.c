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
 * @file sg_wc__jsglue__sg_wc.c
 *
 * @details JS glue for the wc8api layer.  This layer presents the "sg.wc"
 * self-contained/convenience routines to operate on the working directory.
 *
 * NOTE: Some of the methods in the JS glue for this layer take
 * a "test" parameter.  This allows the caller to still use the
 * convenience layer and do a "test rename", for example.  The
 * rename routine in the wc8api layer also takes the "bTest" arg
 * and does all of the data validation and QUEUEING, but either
 * APPLIES or CANCELS the self-contained transaction.  If there
 * is a rename conflict/collision/problem, the wc8api layer will
 * throw.
 *
 *
 * NOTE: In the following syntax diagrams, define:
 *     <<path-or-array-of-paths>>  ::= "<path>"
 * or
 *     <<path-or-array-of-paths>>  ::= [ "<path1>", "<path2>", ... ]
 * 
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * sg.wc.checkout( { "repo"         : "<repo_name>",
 *                   "path"         : "<path">,
 *                   "rev"          : "<revision_number_or_csid_prefix>",
 *                   "tag"          : "<tag>",
 *                   "branch"       : "<branch_name>",
 *                   "attach"       : "<branch_name>",
 *                   "attach_new"   : "<branch_name>",
 *                   "sparse"       : <<sparse-pattern-or-array-of-sparse-patterns>> } )
 *
 * Do a CHECKOUT (Create a new working copy.)
 *
 * The OPTIONAL <path> is an absolute or relative path to the
 * desired root directory (what will be mapped to "@/" after
 * we're done).  If omitted, we assume the CWD.
 * 
 * NOTE: This *should* be the only routine in sg.wc that needs
 * to specify the location of "@/"; all other routines should
 * implicitly be CWD based.
 *
 *
 * The OPTIONAL "rev", "tag", and "branch" args can be used to
 * specify the changeset that we should checkout.  At most only
 * one can be given.
 *
 * The OPTIONAL "attach" arg specifies the branch name to be
 * set on the resulting WD (and requires the use of "rev" or "tag").
 *
 * The OPTIONAL "attach_new" arg specifies a new branch to be
 * attached to after the checkout.
 *
 * TODO 2011/10/17 Document the possible values, the default
 * TODO            values, and the weird interaction of
 * TODO            "rev", "tag", "branch", "attach", and "attach-new".
 *
 * 
 * The OPTIONAL "sparse" when present indicates that the user
 * wants to create a "sparse" WD where some portions of the VC
 * tree are not populated.  This provided list of patterns
 * define things that will be EXCLUDED from the WD.  These may
 * be of the following form:
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
 * In a SPARSE CHECKOUT, we preserve the overall tree
 * structure/layout, so we will populate any parent directories
 * required to connect the requested items to the root directory.)
 * 
 * 
 * NOTE: We *ASSUME* that we are to do a fresh checkout into
 * a new/empty directory.  We *DO NOT* take a bNewOrEmpty flag.
 * That flag was only needed by 'vv init' in the original
 * PendingTree version to set up a WD in an existing directory.
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, checkout)
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
	const char * pszAttach = NULL;	// we do not own this
	const char * pszAttachNew = NULL;	// we do not own this
	const char * pszSparse = NULL;	// we do not own this
	SG_varray * pvaSparse = NULL;	// we do not own this

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

	SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "repo",                   &pszRepoName)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "path",         NULL,     &pszPath)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "rev",          NULL,     &pszRev)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "tag",          NULL,     &pszTag)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "branch",       NULL,     &pszBranch)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "attach",       NULL,     &pszAttach)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "attach_new",   NULL,     &pszAttachNew)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz(pCtx, pvh_args, pvh_got, "sparse",                 &pszSparse, &pvaSparse)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.checkout", pvh_args, pvh_got)  );

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

	SG_ERR_CHECK(  SG_wc__checkout(pCtx,
								   pszRepoName,
								   pszPath,
								   pRevSpec,
								   pszAttach,
								   pszAttachNew,
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
 * sg.wc.add( { "src"        : <<path-or-array-of-paths>>,
 *              "wc_path"    : "<absolute disk path>",
 *              "depth"      : <int>,
 *              "no_ignores" : <bool>,
 *              "test"       : <bool> } );
 *
 *           
 * ADD 1 or more item to version control.
 *
 * Each <path> in <<path-or-array-of-paths>> can be an
 * absolute, relative, or repo path.
 * 
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 * The OPTIONAL "no_ignores" value indicates whether 
 * we should disregard the settings in .vvignores.
 * This defaults to FALSE.
 * 
 * The OPTIONAL "depth" value indicates how many levels
 * of sub-directories that we search for each item.
 * A value of 0 is equivalent to non-recursive.
 * A value of 1 will look inside a directory, but
 * no deeper.
 * This defaults to MAX_INT for a full recursive status.
 *
 * OPTIONAL "test" specifies whether we should just look for
 * problems and not actually APPLY the change.
 *
 *
 * Note: We DO NOT return a JOURNAL; if you want that
 * Note: use the sg_wc_tx layer.
 * 
 *
 * WARNING: Adding an item that whose parent directory
 * is not under version control might cause the parent
 * directory to also be added.
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, add)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_stringarray * psa_src = NULL;
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;
	SG_uint32 depth = SG_INT32_MAX;
	SG_bool bNoIgnores = SG_FALSE;
	SG_bool bTest = SG_FALSE;


	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "src", &psa_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "wc_path",    NULL,         &pszWc)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__uint32(            pCtx, pvh_args, pvh_got, "depth",      SG_INT32_MAX, &depth)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_ignores", SG_FALSE,     &bNoIgnores)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "test",       SG_FALSE,     &bTest)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.add", pvh_args, pvh_got)  );

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__add__stringarray(pCtx, pPathWc, psa_src, depth, bNoIgnores, bTest, NULL)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * sg.wc.addremove( { "src"        : <<path-or-array-of-paths>>,
 *                    "wc_path"    : "<absolute disk path>",
 *                    "depth"      : <int>,
 *                    "no_ignores" : <bool>,
 *                    "test"       : <bool> } );
 *
 *           
 * Do ADDREMOVE:  ADD any FOUND items; REMOVE any LOST items.
 *
 * Each <path> in <<path-or-array-of-paths>> can be an
 * absolute, relative, or repo path.
 *
 * The OPTIONAL "src" refers to one or more items.
 * If omitted, we default to "@/".
 * 
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 * The OPTIONAL "no_ignores" value indicates whether 
 * we should disregard the settings in .vvignores.
 * This defaults to FALSE.
 * 
 * The OPTIONAL "depth" value indicates how many levels
 * of sub-directories that we search for each item.
 * A value of 0 is equivalent to non-recursive.
 * A value of 1 will look inside a directory, but
 * no deeper.
 * This defaults to MAX_INT for a full recursive status.
 *
 * NOTE: "depth" is a little confusing in the context of
 * an "addremove".  I'm going to say that it means to do
 * a STATUS of depth-n and for anything in the result set:
 * [] ADD (with depth 0) anything FOUND;
 * [] REMOVE (no depth arg allowed) anything LOST.
 *
 * 
 * OPTIONAL "test" specifies whether we should just look for
 * problems and not actually APPLY the change.
 *
 * 
 * Note: We DO NOT return a JOURNAL; if you want that
 * Note: use the sg_wc_tx layer.
 * 
 *
 * WARNING: Adding an item that whose parent directory
 * is not under version control might cause the parent
 * directory to also be added.
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, addremove)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_stringarray * psa_src = NULL;
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;
	SG_uint32 depth = SG_INT32_MAX;
	SG_bool bNoIgnores = SG_FALSE;
	SG_bool bTest = SG_FALSE;

	// Since all of the named-parameters are optional, we can allow
	// them to skip giving us an empty arg.

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "src", &psa_src)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "wc_path",    NULL,         &pszWc)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__uint32(            pCtx, pvh_args, pvh_got, "depth",      SG_INT32_MAX, &depth)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_ignores", SG_FALSE,     &bNoIgnores)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "test",       SG_FALSE,     &bTest)  );
		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.addremove", pvh_args, pvh_got)  );
	}

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__addremove__stringarray(pCtx, pPathWc, psa_src, depth, bNoIgnores, bTest, NULL)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * sg.wc.move( { "src"         : <<path-or-array-of-paths>>,
 *               "wc_path"     : "<absolute disk path>",
 *               "dest_dir"    : "<path>",
 *               "no_allow_after_the_fact" : <bool>,
 *               "test"        : <bool>    } );
 *
 *
 * Move 1 or more source items into the destination directory.
 * 
 * Each <path> can be an absolute, relative, or repo path.
 *
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 * OPTIONAL "no_allow_after_the_fact" specifies whether we
 * disallow after-the-fact operations.  We default to ALLOW.
 *
 * OPTIONAL "test" specifies whether we should just look for
 * problems and not actually APPLY the change.
 *
 * 
 * Note: We DO NOT return a JOURNAL; if you want that
 * Note: use the sg_wc_tx layer.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, move)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_stringarray * psa_src = NULL;
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;
	const char * pszDestDir = NULL;	// we do not own this
	SG_bool bNoAllowAfterTheFact = SG_FALSE;
	SG_bool bTest = SG_FALSE;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "src", &psa_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "wc_path",    NULL,         &pszWc)  );
	SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "dest_dir",             &pszDestDir)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_allow_after_the_fact", SG_FALSE, &bNoAllowAfterTheFact)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "test",        SG_FALSE, &bTest)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.move", pvh_args, pvh_got)  );

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__move__stringarray(pCtx, pPathWc, psa_src, pszDestDir, bNoAllowAfterTheFact, bTest, NULL)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * sg.wc.move_rename( { "src"         : "<path>",
 *                      "dest"        : "<path>",
 *                      "wc_path"     : "<absolute disk path>",
 *                      "no_allow_after_the_fact" : <bool>,
 *                      "test"        : <bool>    } );
 *
 * Move and/or Rename an item in a single step.
 * 
 * Each <path> can be an absolute, relative, or repo path.
 *
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 * OPTIONAL "no_allow_after_the_fact" specifies whether to
 * disallow after-the-fact operations.  We default to ALLOW.
 *
 * OPTIONAL "test" specifies whether we should just look for
 * problems and not actually APPLY the change.
 *
 * Note: We DO NOT return a JOURNAL; if you want that
 * Note: use the sg_wc_tx layer.
 *
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, move_rename)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	const char * psz_src = NULL;	// we do not own this
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;
	const char * pszDest = NULL;	// we do not own this
	SG_bool bNoAllowAfterTheFact = SG_FALSE;
	SG_bool bTest = SG_FALSE;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "src",                  &psz_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "dest",                 &pszDest)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "wc_path",    NULL,         &pszWc)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_allow_after_the_fact", SG_FALSE, &bNoAllowAfterTheFact)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "test",        SG_FALSE, &bTest)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.move_rename", pvh_args, pvh_got)  );

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__move_rename(pCtx, pPathWc, psz_src, pszDest, bNoAllowAfterTheFact, bTest, NULL)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * sg.wc.rename( { "src"         : "<path>",
 *                 "entryname"   : "<entryname>",
 *                 "wc_path"     : "<absolute disk path>",
 *                 "no_allow_after_the_fact" : <bool>,
 *                 "test"        : <bool>    } );
 *
 * Rename an item to have the new entryname.
 * 
 * <path> can be an absolute, relative, or repo path.
 *
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 * OPTIONAL "no_allow_after_the_fact" specifies whether to
 * disallow after-the-fact operations.  We default to ALLOW.
 *
 * OPTIONAL "test" specifies whether we should just look for
 * problems and not actually APPLY the change.
 *
 * 
 * Note: We DO NOT return a JOURNAL; if you want that
 * Note: use the sg_wc_tx layer.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, rename)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	const char * psz_src = NULL;		// we do not own this
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;
	const char * pszEntryname = NULL;	// we do not own this
	SG_bool bNoAllowAfterTheFact = SG_FALSE;
	SG_bool bTest = SG_FALSE;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "src",                  &psz_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "entryname",            &pszEntryname)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "wc_path",    NULL,         &pszWc)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_allow_after_the_fact", SG_FALSE, &bNoAllowAfterTheFact)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "test",        SG_FALSE, &bTest)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.rename", pvh_args, pvh_got)  );

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__rename(pCtx, pPathWc, psz_src, pszEntryname, bNoAllowAfterTheFact, bTest, NULL)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * sg.wc.remove( { "src"          : <<path-or-array-of-paths>>,
 *                 "wc_path"      : "<absolute disk path>",
 *                 "nonrecursive" : <bool>,
 *                 "force"        : <bool>,
 *                 "no_backups"   : <bool>,
 *                 "keep"         : <bool>,
 *                 "test"         : <bool> } );
 *
 * Remove/Delete one or more items.
 *
 * Each <path> can be an absolute, relative, or repo path.
 *
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 * 
 *
 * OPTIONAL "force" specifies to force delete things that
 * we would normally object to and/or override the "safe
 * delete" concept.
 * 
 *
 * OPTIONAL "no_backups" specifies to not make backup copies
 * of modified files.  (Requires "force".)
 *
 * 
 * OPTIONAL "keep" specifies whether we should leave the items
 * in the directory and only remove them from version control.
 * (Any kept items will appear as FOUND afterwards.)
 *
 * 
 * OPTIONAL "test" specifies whether we should just look for
 * problems and not actually APPLY the change.
 *
 * 
 * Note: We DO NOT return a JOURNAL; if you want that
 * Note: use the sg_wc_tx layer.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, remove)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_stringarray * psa_src = NULL;
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;
	SG_bool bNonRecursive = SG_FALSE;
	SG_bool bForce = SG_FALSE;
	SG_bool bNoBackups = SG_FALSE;
	SG_bool bKeep = SG_FALSE;
	SG_bool bTest = SG_FALSE;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "src", &psa_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "wc_path",    NULL,         &pszWc)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "nonrecursive", SG_FALSE, &bNonRecursive)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "force",        SG_FALSE, &bForce)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_backups",   SG_FALSE, &bNoBackups)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "keep",         SG_FALSE, &bKeep)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "test",         SG_FALSE, &bTest)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.remove", pvh_args, pvh_got)  );

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__remove__stringarray(pCtx, pPathWc, psa_src, bNonRecursive, bForce, bNoBackups, bKeep, bTest, NULL)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * sg.wc.lock( { "src" : <<path-or-array-of-paths>>,
 *               "user" : <user-name-or-id>,
 *               "password" : "<password>",
 *               "server" : "<server>" } );
 *
 * Create a LOCK on one or more files.  At least 1 file
 * is required.
 *
 * Each <path> can be an absolute, relative, or repo-path.
 *
 *
 * OPTIONAL "user".  Defaults to the current user.
 * Only needed if the destination server requires
 * authorization.
 *
 * OPTIONAL "password".
 * Only needed if the destination server requires
 * authorization.
 *
 * OPTIONAL "server" is the destination server/repo
 * to whom we broadcast the lock.  Defaults to the
 * default path for the repo.
 *
 *
 * Note: we DO NOT take a "test" argument because we
 * Note: have to talk to the server.
 *
 *
 * Note: We DO NOT return a JOURNAL.
 * 
 *
 * WARNING: This routine only exists at the level-8
 * WARNING: layer and so we only have a sg.wc method.
 * WARNING: We DO NOT have a sg_wc_tx version.
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, lock)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_stringarray * psa_src = NULL;
	const char * pszUser = NULL;			// we do not own this
	const char * pszPassword = NULL;		// we do not own this
	const char * pszServer = NULL;			// we do not own this

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "src", &psa_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                             pCtx, pvh_args, pvh_got, "user",     NULL, &pszUser)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                             pCtx, pvh_args, pvh_got, "password", NULL, &pszPassword)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                             pCtx, pvh_args, pvh_got, "server",   NULL, &pszServer)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.lock", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_wc__lock(pCtx, NULL, psa_src, pszUser, pszPassword, pszServer)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * sg.wc.unlock( { "src"      : <<path-or-array-of-paths>>,
 *                 "wc_path"  : "<absolute disk path>",
 *                 "force"    : <bool>,
 *                 "user"     : <user-name-or-id>,
 *                 "password" : "<password>",
 *                 "server"   : "<server>" } );
 *
 * UNLOCK one or more files.  At least 1 file
 * is required.
 *
 * Each <path> can be an absolute, relative, or repo-path.
 *
 *
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 * 
 * OPTIONAL "force" specifies to force the unlock.
 *
 * OPTIONAL "user".  Defaults to the current user.
 * Only needed if the destination server requires
 * authorization.
 *
 * OPTIONAL "password".
 * Only needed if the destination server requires
 * authorization.
 *
 * OPTIONAL "server" is the destination server/repo
 * to whom we broadcast the lock.  Defaults to the
 * default path for the repo.
 *
 * 
 * Note: we DO NOT take a "test" argument because we
 * Note: have to talk to the server.
 * 
 *
 * Note: We DO NOT return a JOURNAL.
 * 
 *
 * WARNING: This routine only exists at the level-8
 * WARNING: layer and so we only have a sg.wc method.
 * WARNING: We DO NOT have a sg_wc_tx version.
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, unlock)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_stringarray * psa_src = NULL;
	const char* pszWc = NULL;				// we do not own this
	SG_pathname* pPathWc = NULL;
	const char * pszUser = NULL;			// we do not own this
	const char * pszPassword = NULL;		// we do not own this
	const char * pszServer = NULL;			// we do not own this
	SG_bool bForce = SG_FALSE;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "src", &psa_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(								pCtx, pvh_args, pvh_got, "wc_path",  NULL, &pszWc)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                             pCtx, pvh_args, pvh_got, "user",     NULL, &pszUser)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                             pCtx, pvh_args, pvh_got, "password", NULL, &pszPassword)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                             pCtx, pvh_args, pvh_got, "server",   NULL, &pszServer)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(                           pCtx, pvh_args, pvh_got, "force",    SG_FALSE, &bForce)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.unlock", pvh_args, pvh_got)  );

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__unlock(pCtx, pPathWc, psa_src, bForce, pszUser, pszPassword, pszServer)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * varray = sg.wc.status( { "src"            : <<path-or-array-of-paths>>,
 *                          "wc_path"        : "<absolute disk path>",
 *                          "depth"          : <int>,
 *                          "rev"            : "<revision_number_or_csid_prefix>",
 *                          "tag"            : "<tag>",
 *                          "branch"         : "<branch_name>",
 *                          "list_unchanged" : <bool>,
 *                          "no_ignores"     : <bool>,
 *                          "no_tsc"         : <bool>,
 *                          "list_sparse"    : <bool>,
 *                          "list_reserved"  : <bool>,
 *                          "no_sort"        : <bool>  } );
 *
 * or
 *
 * varray = sg.wc.status( );
 *
 * 
 * Compute "canonical" status one or more items.
 * We return a varray of vhashes with one row per item.
 *
 * Each <path> can be an absolute, relative, or repo path.
 *
 * The OPTIONAL "src" refers to one or more items.
 * If omitted, we default to "@/".
 * 
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 * The OPTIONAL "no_ignores" value indicates whether 
 * we should disregard the settings in .vvignores.
 * This defaults to FALSE.
 * 
 * The OPTIONAL "depth" value indicates how many levels
 * of sub-directories that we search for each item.
 * A value of 0 is equivalent to non-recursive.
 * A value of 1 will look inside a directory, but
 * no deeper.
 * This defaults to MAX_INT for a full recursive status.
 *
 * The OPTIONAL "list_unchanged" value indicates whether
 * we should include status info for clean items.
 * Normally, we don't report on clean items, but I thought
 * something like Tortoise might like to use this to get
 * a complete view of a directory (clean and dirty) so
 * that it doesn't have to infer the former.  This defaults
 * to FALSE.
 * 
 * The OPTIONAL "no_tsc" value indicates whether we should
 * bypass the TimeStampCache.  (This is mainly a debug option.)
 *
 * The OPTIONAL "list_sparse" value indicates whether we
 * should print info for any "sparse" items.
 *
 * The OPTIONAL "list_reserved" value indicates whether we
 * should print info for any "reserved" items (such as .sgdrawer).
 *
 * The OPTIONAL "no_sort" value requests that we not bother
 * sorting the returned array by repo-path.
 *
 * If a rev-spec (rev/tag/branch) is given, we use that for
 * the left side rather than the baseline.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, status)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_varray * pvaStatus = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;
	SG_stringarray * psa_src = NULL;
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;
	SG_uint32 depth = SG_INT32_MAX;
	SG_bool bListUnchanged = SG_FALSE;
	SG_bool bNoIgnores = SG_FALSE;
	SG_bool bNoTSC = SG_FALSE;
	SG_bool bListSparse = SG_TRUE;		// See P1579
	SG_bool bListReserved = SG_FALSE;
	SG_bool bNoSort = SG_FALSE;
	SG_rev_spec * pRevSpec = NULL;
	const char * pszRev = NULL;			// we do not own this
	const char * pszTag = NULL;			// we do not own this
	const char * pszBranch = NULL;		// we do not own this

	// Since all of the named-parameters are optional, we can allow
	// them to skip giving us an empty arg.  that is, they can do:
	//      s = sg.wc.status();
	// rather than:
	//      s = sg.wc.status( {} );

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "src",   &psa_src)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(    pCtx, pvh_args, pvh_got, "wc_path",        NULL,           &pszWc)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__uint32(pCtx, pvh_args, pvh_got, "depth",          depth,          &depth)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(  pCtx, pvh_args, pvh_got, "list_unchanged", bListUnchanged, &bListUnchanged)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(  pCtx, pvh_args, pvh_got, "no_ignores",     bNoIgnores,     &bNoIgnores)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(  pCtx, pvh_args, pvh_got, "no_tsc",         bNoTSC,         &bNoTSC)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(  pCtx, pvh_args, pvh_got, "list_sparse",    bListSparse,    &bListSparse)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(  pCtx, pvh_args, pvh_got, "list_reserved",  bListReserved,  &bListReserved)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(  pCtx, pvh_args, pvh_got, "no_sort",        bNoSort,        &bNoSort)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(    pCtx, pvh_args, pvh_got, "rev",            NULL,           &pszRev)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(    pCtx, pvh_args, pvh_got, "tag",            NULL,           &pszTag)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(    pCtx, pvh_args, pvh_got, "branch",         NULL,           &pszBranch)  );
		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.status", pvh_args, pvh_got)  );
	}

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	if (pszRev || pszTag || pszBranch)
	{
		SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
		if (pszRev)
			SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, pszRev)  );
		if (pszTag)
			SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pRevSpec, pszTag)  );
		if (pszBranch)
			SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, pszBranch)  );

		SG_ERR_CHECK(  SG_wc__status1__stringarray(pCtx, pPathWc, pRevSpec,
												   psa_src, depth,
												   bListUnchanged, bNoIgnores, bNoTSC, bListSparse, bListReserved, bNoSort,
												   &pvaStatus)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_wc__status__stringarray(pCtx, pPathWc,
												  psa_src, depth, 
												  bListUnchanged, bNoIgnores, bNoTSC, bListSparse, bListReserved, bNoSort,
												  &pvaStatus, NULL)  );
	}

    SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaStatus, jso)  );
    SG_VARRAY_NULLFREE(pCtx, pvaStatus);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);

    return JS_TRUE;

fail:
    SG_VARRAY_NULLFREE(pCtx, pvaStatus);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * varray = sg.wc.mstatus( { "wc_path"     : "<absolute disk path>",
 *                           "no_ignores"  : <bool>,
 *                           "no_fallback" : <bool>,
 *                           "no_sort"     : <bool>  } );
 *
 * or
 *
 * varray = sg.wc.mstatus( );
 *
 * 
 * Compute "canonical" MERGE-STATUS (aka MSTATUS) for the WD.
 * This is the net-result after a merge.
 * We return a varray of vhashes with one row per item
 * (mostly compatible with regular STATUS output).
 *
 *
 * If not in a merge, we fallback to a regular status, unless
 * disallowed.
 *
 * 
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 * TODO 2012/02/17 Currently, we do not take any of the
 * TODO            arguments to limit and/or pre-filter
 * TODO            the result.  Not sure I want to at this
 * TODO            point.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, mstatus)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_varray * pvaStatus = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;
	SG_bool bNoIgnores = SG_FALSE;
	SG_bool bNoFallback = SG_FALSE;
	SG_bool bNoSort = SG_FALSE;

	// Since all of the named-parameters are optional, we can allow
	// them to skip giving us an empty arg.  that is, they can do:
	//      s = sg.wc.mstatus();
	// rather than:
	//      s = sg.wc.mstatus( {} );

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(   pCtx, pvh_args, pvh_got, "wc_path",        NULL,         &pszWc)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool( pCtx, pvh_args, pvh_got, "no_ignores",     SG_FALSE,     &bNoIgnores)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool( pCtx, pvh_args, pvh_got, "no_fallback",    SG_FALSE,     &bNoFallback)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool( pCtx, pvh_args, pvh_got, "no_sort",        SG_FALSE,     &bNoSort)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.mstatus", pvh_args, pvh_got)  );
	}

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__mstatus(pCtx, pPathWc, bNoIgnores, bNoFallback, bNoSort, &pvaStatus, NULL)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaStatus, jso)  );
    SG_VARRAY_NULLFREE(pCtx, pvaStatus);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    return JS_TRUE;

fail:
    SG_VARRAY_NULLFREE(pCtx, pvaStatus);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * vhash = sg.wc.get_item_status_flags( { "src"        : "<path>",
 *                                        "wc_path"    : "<absolute disk path>",
 *                                        "no_ignores" : <bool>,
 *                                        "no_tsc"     : <bool> } );
 *
 * Compute the status flags for an item.  This is a
 * cheaper version of "sg.wc.status()" when
 * you don't care about all of the details.
 *
 * <path> can be an absolute, relative, or repo path.
 * 
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 * The OPTIONAL "no_ignores" value indicates whether 
 * we should disregard the settings in .vvignores.
 * This defaults to FALSE.
 *
 * The OPTIONAL "no_tsc" valud indicates whether we should
 * bypass the TimeStampCache.  (This is mainly a debug option.)
 *
 * NOTE that we DO NOT take a DEPTH.
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, get_item_status_flags)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_wc_status_flags flags;
	SG_vhash * pvhProperties = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;
	const char * psz_src = NULL;	// we do not own this
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;
	SG_bool bNoIgnores = SG_FALSE;
	SG_bool bNoTSC = SG_FALSE;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "src",                  &psz_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "wc_path",    NULL,     &pszWc)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_ignores", SG_FALSE, &bNoIgnores)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_tsc",     SG_FALSE, &bNoTSC)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.get_item_status_flags", pvh_args, pvh_got)  );

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__get_item_status_flags(pCtx, pPathWc, psz_src, bNoIgnores, bNoTSC, &flags, &pvhProperties)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhProperties, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvhProperties);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvhProperties);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * vhash = sg.wc.get_item_dirstatus_flags( { "src"        : "<path>",
 *                                           "wc_path"    : "<absolute disk path>" } );
 *
 * Compute an status flags for a DIRECTORY.
 * Also determine if anything *WITHIN* the
 * directory has changed.
 *
 * <path> can be an absolute, relative, or repo path.
 * 
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, get_item_dirstatus_flags)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_wc_status_flags sf;
	SG_vhash * pvhResult = NULL;
	SG_vhash * pvhProperties = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;
	const char * psz_src = NULL;	// we do not own this
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;
	SG_bool bDirContainsChanges = SG_FALSE;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "src",                  &psz_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "wc_path",    NULL,     &pszWc)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.get_item_dirstatus_flags", pvh_args, pvh_got)  );

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__get_item_dirstatus_flags(pCtx, pPathWc, psz_src, &sf, &pvhProperties, &bDirContainsChanges)  );

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhResult)  );
	SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvhResult, "changes", bDirContainsChanges)  );
	SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhResult, "status", &pvhProperties)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhResult, jso)  );
	SG_VHASH_NULLFREE(pCtx, pvhResult);
    SG_VHASH_NULLFREE(pCtx, pvhProperties);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    return JS_TRUE;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhResult);
    SG_VHASH_NULLFREE(pCtx, pvhProperties);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * varray = sg.wc.parents( { "wc_path" : "<absolute disk path>" } );
 *
 * or
 *
 * varray = sg.wc.parents();
 * 
 *
 * Return an array of the HIDs of the parents of the WD.
 *
 * 
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 * WARNING: We DO NOT take any arguments beyond the option working 
 * copy path.  The set of parents is essentially a constant and based 
 * upon the last CHECKOUT, UPDATE, MERGE, or REVERT.  The original
 * sg.pending_tree.parents() did take arguments and did 3 different
 * things (2 of them were history-related).  This routine does not do
 *  that.
 *
 * By convention, parent[0] is the BASELINE.
 *
 * If varry.length > 1, then there is an active MERGE.
 *
 *
 * NOTE: sg.wc.get_wc_info() also returns the parents and a bunch
 *       other meta-data.
 *       
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, parents)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;
	SG_varray * pvaParents = NULL;
	JSObject* jso = NULL;

	// Since there are no required named-parameters we allow
	// them to skip giving us an empty arg.

	SG_JS_BOOL_CHECK( (argc < 2)  );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "wc_path", NULL, &pszWc)  );
		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.parents", pvh_args, pvh_got)  );
	}

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__get_wc_parents__varray(pCtx, pPathWc, &pvaParents)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaParents, jso)  );
    SG_VARRAY_NULLFREE(pCtx, pvaParents);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    return JS_TRUE;

fail:
    SG_VARRAY_NULLFREE(pCtx, pvaParents);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * vhash = sg.wc.get_wc_info( { "wc_path" : "<absolute disk path>" } );
 *
 * or
 *
 * vhash = sg.wc.get_wc_info( );
 * 
 * 
 * Return a vhash of meta-data for the WD.
 *
 * 
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, get_wc_info)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_vhash * pvhInfo = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;

	// Since there are no required named-parameters we allow
	// them to skip giving us an empty arg.

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(pCtx, pvh_args, pvh_got, "wc_path", NULL, &pszWc)  );
		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.get_wc_info", pvh_args, pvh_got)  );
	}

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__get_wc_info(pCtx, pPathWc, &pvhInfo)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhInfo, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvhInfo);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvhInfo);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * path = sg.wc.get_wc_top( { "wc_path" : "<absolute disk path>" } );
 *
 * or
 *
 * path = sg.wc.get_wc_top( );
 *
 * 
 * Return the pathname of the top of the WD.
 *
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
*/
SG_JSGLUE_METHOD_PROTOTYPE(wc, get_wc_top)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_pathname * pPath = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSString * pjs = NULL;
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;

	// Since there are no required named-parameters we allow
	// them to skip giving us an empty arg.

	SG_JS_BOOL_CHECK( (argc < 2)  );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "wc_path",    NULL,         &pszWc)  );
		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.get_wc_top", pvh_args, pvh_got)  );
	}

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__get_wc_top(pCtx, pPathWc, &pPath)  );

	SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, SG_pathname__sz(pPath)))  );
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

	SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    return JS_TRUE;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * sg.wc.flush_timestamp_cache( { "wc_path"    : "<absolute disk path>" } );
 *
 * or
 * 
 * sg.wc.flush_timestamp_cache( );
 *
 * Flush the timestamp cache.
 *
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 * TODO 2012/06/14 This really doesn't need to be in the public API.
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, flush_timestamp_cache)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;

	// Since there are no required named-parameters we allow
	// them to skip giving us an empty arg.

	SG_JS_BOOL_CHECK( (argc < 2)  );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "wc_path",    NULL,         &pszWc)  );
		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.flush_timestamp_cache", pvh_args, pvh_got)  );
	}

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__flush_timestamp_cache(pCtx, pPathWc)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * r = sg.wc.commit( { "src"        : <<path-or-array-of-paths>>,
 *                     "wc_path"    : "<absolute disk path>",
 *                     "message"    : "<message>",
 *                     "detached"   : <bool>,
 *                     "allow_lost" : <bool>,
 *                     "stamp"      : <<stamp-or-array-of-stamps>>,
 *                     "assoc"      : <<assoc-or-array-of-assoc>>,
 *                     "depth"      : <int>,
 *                     "user"       : <user-name-or-id>,
 *                     "when"       : <when>,
 *                     "test"       : <bool>,
 *                     "verbose"    : <bool> } );
 *
 *
 * COMMITS some or all of the changes in the WD and
 * creates a new changeset.
 *
 * Each <path> can be absolute, relative, or repo path.
 *
 * The OPTIONAL "src" refers to one or more items in the tree.
 * If omitted, we assume the entire tree ("@/").  If one
 * or more <paths> are given, we *try* to do a partial
 * commit of on that subset of the tree.  If one or more
 * of the <paths> refers to directories, we consider the
 * sub-trees of depth <depth> from each of them.
 *
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 * The REQUIRED "message" argument gives the commit message.
 * 
 * The OPTIONAL "depth" (which is only valid with "src")
 * defines how deep we visit each directory in the src
 * list.
 *
 * The OPTIONAL "detached" allows the commit even though the
 * WD is not attached to a branch and the resulting changeset
 * will not be attached to any branch.
 *
 * The OPTIONAL "stamp" refers to one or more STAMPS to
 * apply to the new changeset.  If omitted, no stamps
 * are attached to the changeset.
 *
 * The OPTIONAL "assoc" refers to one or more WIT
 * associations.  If omitted, no associations are made.
 *
 * The OPTIONAL "user" allows you to attribute the changeset
 * to a different user.  This defaults to the current user.
 * This is primarily to help IMPORT-like tools preserve
 * meta-data.
 *
 * The OPTIONAL "when" allows you to specify the date/time
 * of the changeset.  This defaults to the current date/time.
 * Again, this is primarily to help IMPORT-like tools.
 * Note that in a DVCS there is NO central clock, so we
 * don't rely on this field for anything.
 *
 * The OPTIONAL "allow_lost" allows you to force the
 * commit to continue when there are LOST items in the WD.
 * Normally, COMMIT will stop and complain.  When this
 * option is given, we will assume that lost items have
 * clean content/attrbits and try to continue.
 * There are cases where we cannot continue (ADDED+LOST)
 * and will still throw.
 *
 * The OPTIONAL "test" requests that validate the args and stop.
 *
 * The OPTIONAL "verbose" requests the journal to be returned.
 *
 *
 * If (test || verbose), we return:
 *
 * r := { "hid"      : "<hid_of_new_changeset>",        -- when !test
 *        "journal"  : <<array_of_queued_operations>>,  -- when verbose
 *      }
 *
 * Otherwise, we return:
 *
 * r := "<hid_of_new_changeset>"  (unless we throw)
 *      
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, commit)
{
	SG_wc_commit_args ca;
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_varray * pvaJournal = NULL;
	SG_vhash * pvhResult = NULL;
	SG_stringarray * psa_src = NULL;
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;
	SG_stringarray * psa_stamp = NULL;
	SG_stringarray * psa_assoc = NULL;
	char * pszHidNewCSet = NULL;
	JSObject* jso = NULL;

	const char * psz_message = NULL;
	const char * psz_user = NULL;
	const char * psz_when = NULL;

	SG_uint32 depth = SG_INT32_MAX;
	SG_bool bDetached = SG_FALSE;
	SG_bool bAllowLost = SG_FALSE;
	SG_bool bTest = SG_FALSE;
	SG_bool bVerbose = SG_FALSE;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
	SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

	SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "src", &psa_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                             pCtx, pvh_args, pvh_got, "wc_path",    NULL,         &pszWc)  );
	SG_ERR_CHECK(  SG_jsglue__np__required__sz(                             pCtx, pvh_args, pvh_got, "message",                  &psz_message)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(                           pCtx, pvh_args, pvh_got, "detached",   SG_FALSE,     &bDetached)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "stamp",                    &psa_stamp)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "assoc",                    &psa_assoc)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__uint32(                         pCtx, pvh_args, pvh_got, "depth",      SG_INT32_MAX, &depth)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                             pCtx, pvh_args, pvh_got, "user",       NULL,         &psz_user)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                             pCtx, pvh_args, pvh_got, "when",       NULL,         &psz_when)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(                           pCtx, pvh_args, pvh_got, "allow_lost", SG_FALSE,     &bAllowLost)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(                           pCtx, pvh_args, pvh_got, "test",       SG_FALSE,     &bTest)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(                           pCtx, pvh_args, pvh_got, "verbose",    SG_FALSE,     &bVerbose)  );

	SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.commit", pvh_args, pvh_got)  );

	memset(&ca, 0, sizeof(ca));
	ca.bDetached      = bDetached;
	ca.pszUser        = psz_user;
	ca.pszWhen        = psz_when;
	ca.pszMessage     = psz_message;
	ca.pfnPrompt      = NULL;	// not interactive
	ca.psaInputs      = psa_src;
	ca.depth          = depth;
	ca.psaAssocs      = psa_assoc;
	ca.bAllowLost     = bAllowLost;
	ca.psaStamps      = psa_stamp;

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__commit(pCtx,
								 pPathWc,
								 &ca,
								 bTest,
								 ((bVerbose) ? &pvaJournal : NULL),
								 &pszHidNewCSet)  );
	// pszHidNewCSet is only set when !bTest

	if (bTest || bVerbose)
	{
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhResult)  );
		if (pszHidNewCSet)
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhResult, "hid", pszHidNewCSet)  );
		if (pvaJournal)
			SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhResult, "journal", &pvaJournal)  );

		SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
		SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhResult, jso)  );
		SG_VHASH_NULLFREE(pCtx, pvhResult);
	}
	else
	{
		jsval tmp;
		JSVAL_FROM_SZ(tmp, pszHidNewCSet);
		JS_SET_RVAL(cx, vp, tmp);
	}

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_VARRAY_NULLFREE(pCtx, pvaJournal);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_stamp);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_assoc);
	SG_NULLFREE(pCtx, pszHidNewCSet);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_VARRAY_NULLFREE(pCtx, pvaJournal);
	SG_VHASH_NULLFREE(pCtx, pvhResult);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_stamp);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_assoc);
	SG_NULLFREE(pCtx, pszHidNewCSet);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * r = sg.wc.update( { "rev"         : "<revision_number_or_csid_prefix>",
 *                     "tag"         : "<tag>",
 *                     "branch"      : "<branch_name>",
 *                     "detached"    : <bool>,
 *                     "attach"      : "<branch_name>",
 *                     "attach_new"  : "<branch_name>",
 *                     "attach_current" : <bool>,
 *                     "allow_dirty" : <bool>,
 *                     "test"        : <bool>,
 *                     "verbose"     : <bool>,
 *                     "status"      : <bool> } );
 * TODO 2012/01/17 deal with any clean/keep/force/backup/no-backup args.
  *
 * UPDATE the WD to be relative to a new changeset.
 * 
 * Optionally, we "carry-forward" any current dirt in the WD.
 *
 * We allow at most 1 rev/tag/branch argument to try to name the
 * destination/target changeset.  If omitted, we try to find a
 * single unique descendant head as the target.
 * 
 * If the WD is untied and you update with a 'branch' argument,
 * we will auto-tie the WD (unless overridden with one of the
 * detached/attached/attached-new arguments).
 *
 * If the WD is tied and you update to a non-head changeset,
 * we required one of the detached/attached/attached-new arguments
 * (we don't auto-detach).
 * 
 *
 * We allow at most 1 of the detached/attach/attach-new/attach-current arguments.
 *
 * The optional "detached" means that we will update the baseline
 * and untie it.
 *
 * The optional "attach" or "attach-new" means that we will update
 * the baseline and then tie to the named branch.
 *
 * The optional "attach-current" bool means that we preserve the current
 * attachment. This is mostly useful when updating away from a head where
 * we require explicit attach/detach instructions.
 *
 * The OPTIONAL "test" requests that validate the args and stop.
 *
 * The OPTIONAL "verbose" requests the journal to be returned.
 *
 * The OPTIONAL "status" requests that we compute a sg.wc.status({})
 * on the state of the WD at the end of the UPDATE computation (inside
 * the same TX).  This can be used with "test" to do a full what-if.
 *
 *
 * We return: 
 *
 * r := {
 *        "hid"     : "<hid_of_chosen_target_baseline>",
 *        "stats"   : <<vhash_of_summary_stats>>,
 *        "journal" : <<array_of_queued_operations>>,	-- when verbose=true
 *        "status"  : <<array>>							-- when status=true
 *      };
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, update)
{
	SG_wc_update_args ua;
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_rev_spec * pRevSpec = NULL;
	SG_vhash * pvhResult = NULL;
	SG_varray * pvaJournal = NULL;
	SG_vhash * pvhStats = NULL;
	SG_varray * pvaStatus = NULL;
	char * pszHidChosen = NULL;
	JSObject* jso = NULL;

	const char * pszRev = NULL;		// we do not own this
	const char * pszTag = NULL;		// we do not own this
	const char * pszBranch = NULL;	// we do not own this
	const char * pszAttach = NULL;
	const char * pszAttachNew = NULL;
	SG_bool bDetached = SG_FALSE;
	SG_bool bAttachCurrent = SG_FALSE;
	SG_bool bTest = SG_FALSE;
	SG_bool bVerbose = SG_FALSE;
	SG_bool bStatus = SG_FALSE;
	SG_bool bAllowDirty = SG_TRUE;	// TODO 2013/01/04 decide the default for this

	// Since there are no required named-parameters we allow
	// them to skip giving us an empty arg.

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "rev",          NULL,     &pszRev)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "tag",          NULL,     &pszTag)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "branch",       NULL,     &pszBranch)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "detached",     SG_FALSE, &bDetached)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "attach",       NULL,     &pszAttach)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "attach_new",   NULL,     &pszAttachNew)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "attach_current", SG_FALSE, &bAttachCurrent)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "allow_dirty",  bAllowDirty, &bAllowDirty)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "test",         SG_FALSE, &bTest)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "verbose",      SG_FALSE, &bVerbose)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "status",       SG_FALSE, &bStatus)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.update", pvh_args, pvh_got)  );

		SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
		if (pszRev)
			SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, pszRev)  );
		if (pszTag)
			SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pRevSpec, pszTag)  );
		if (pszBranch)
			SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, pszBranch)  );
	}

	memset(&ua, 0, sizeof(ua));
	ua.pRevSpec     = pRevSpec;
	ua.pszAttach    = pszAttach;
	ua.pszAttachNew = pszAttachNew;
	ua.bDetached    = bDetached;
	ua.bAttachCurrent = bAttachCurrent;
	ua.bDisallowWhenDirty = !bAllowDirty;

	SG_ERR_CHECK(  SG_wc__update(pCtx, NULL, &ua,
								 bTest,
								 ((bVerbose) ? &pvaJournal : NULL),
								 &pvhStats,
								 ((bStatus) ? &pvaStatus : NULL),
								 &pszHidChosen)  );

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhResult)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhResult, "hid", pszHidChosen)  );
	SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhResult, "stats", &pvhStats)  );
	if (pvaJournal)
		SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhResult, "journal", &pvaJournal)  );
	if (pvaStatus)
		SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhResult, "status", &pvaStatus)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhResult, jso)  );
	SG_VHASH_NULLFREE(pCtx, pvhResult);

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_NULLFREE(pCtx, pszHidChosen);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	SG_VARRAY_NULLFREE(pCtx, pvaJournal);
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_VHASH_NULLFREE(pCtx, pvhResult);
	SG_NULLFREE(pCtx, pszHidChosen);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	SG_VARRAY_NULLFREE(pCtx, pvaJournal);
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * r = sg.wc.merge( { "rev"           : "<revision_number_or_csid_prefix>",
 *                    "tag"           : "<tag>",
 *                    "branch"        : "<branch_name>",
 *                    "wc_path"       : "<absolute disk path>",
 *                    "no_ff"         : <bool>,
 *                    "no_auto_merge" : <bool>,
 *                    "allow_dirty"   : <bool>,
 *                    "test"          : <bool>,
 *                    "verbose"       : <bool>,
 *                    "status"        : <bool> } );
 * TODO 2012/01/17 deal with any keep/backup/no-backup args.
 *
 * MERGE another changeset into the baseline in the WD.
 * 
 *
 * We allow at most 1 rev/tag/branch argument to try to name the
 * destination/target changeset.  If omitted, we try to find a
 * single unique descendant head as the target.
 *
 *
 * The OPTIONAL "no_ff" disallows fast-forward merges.
 *
 * The OPTIONAL "allow_dirty" lets us try to do a merge
 * when the is dirt in the WD; the dirt will be carried
 * forward.  This is useful for testing, but is considered
 * bad-practice (in the working without a net sense) in
 * that you should try to commit and then merge rather
 * than merging another changeset in the mess contained
 * in current WD.  If you don't like the result and need
 * to revert, you'll revert back to the original baseline
 * and not the dirty state as of the beginning of this
 * merge.  Using this option does not guarantee that we
 * can actually do the merge -- some types of dirt just
 * need to be fixed before merge can do anything.
 * 
 *
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 * The OPTIONAL "test" requests that validate the args and stop.
 *
 * The OPTIONAL "verbose" requests the journal to be returned.
 *
 * The OPTIONAL "status" requests that we compute a sg.wc.status({})
 * on the state of the WD at the end of the MERGE computation (inside
 * the same TX).  This can be used with "test" to do a full what-if.
 * 
 * We return:
 *
 * r := {
 *        "stats"      : <<vhash_of_summary_stats>>,
 *        "journal"    : <<array_of_queued_operations>>,	-- when verbose=true
 *        "hid_target" : "<hid_of_computed_target_cset>",
 *        "status"     : <<array>>							-- when status=true
 *      };
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, merge)
{
	SG_wc_merge_args mergeArgs;
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_rev_spec * pRevSpec = NULL;
	SG_varray * pvaJournal = NULL;
	SG_vhash * pvhResult = NULL;
	SG_vhash * pvhStats = NULL;
	SG_varray * pvaStatus = NULL;
	char * pszHidTarget = NULL;
	JSObject* jso = NULL;

	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;
	const char * pszRev = NULL;		// we do not own this
	const char * pszTag = NULL;		// we do not own this
	const char * pszBranch = NULL;	// we do not own this
	SG_bool bNoAutoMerge = SG_FALSE;
	SG_bool bTest = SG_FALSE;
	SG_bool bVerbose = SG_FALSE;
	SG_bool bStatus = SG_FALSE;
	SG_bool bNoFFMerge = SG_FALSE;
	SG_bool bAllowDirty = SG_FALSE;

	// Since there are no required named-parameters we allow
	// them to skip giving us an empty arg.

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "wc_path",       NULL,     &pszWc)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "rev",           NULL,     &pszRev)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "tag",           NULL,     &pszTag)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "branch",        NULL,     &pszBranch)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_ff",         SG_FALSE, &bNoFFMerge)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_auto_merge", SG_FALSE, &bNoAutoMerge)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "allow_dirty",   SG_FALSE, &bAllowDirty)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "test",          SG_FALSE, &bTest)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "verbose",       SG_FALSE, &bVerbose)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "status",        SG_FALSE, &bStatus)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.merge", pvh_args, pvh_got)  );

		SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
		if (pszRev)
			SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, pszRev)  );
		if (pszTag)
			SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pRevSpec, pszTag)  );
		if (pszBranch)
			SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, pszBranch)  );
	}

	memset(&mergeArgs, 0, sizeof(mergeArgs));
	mergeArgs.pRevSpec = pRevSpec;
	mergeArgs.bNoFFMerge = bNoFFMerge;
	mergeArgs.bNoAutoMergeFiles = bNoAutoMerge;
	mergeArgs.bComplainIfBaselineNotLeaf = SG_FALSE;	// TODO 2012/01/20 see note in SG_wc_tx__merge().
	mergeArgs.bDisallowWhenDirty = !bAllowDirty;

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__merge(pCtx, pPathWc, &mergeArgs,
								bTest,
								((bVerbose) ? &pvaJournal : NULL),
								&pvhStats,
								&pszHidTarget,
								((bStatus) ? &pvaStatus : NULL))  );

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhResult)  );
	SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhResult, "stats", &pvhStats)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhResult, "hid_target", pszHidTarget)  );
	if (pvaJournal)
		SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhResult, "journal", &pvaJournal)  );
	if (pvaStatus)
		SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhResult, "status", &pvaStatus)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhResult, jso)  );
	SG_VHASH_NULLFREE(pCtx, pvhResult);

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	SG_VARRAY_NULLFREE(pCtx, pvaJournal);
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_NULLFREE(pCtx, pszHidTarget);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_VHASH_NULLFREE(pCtx, pvhResult);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	SG_VARRAY_NULLFREE(pCtx, pvaJournal);
	SG_VARRAY_NULLFREE(pCtx, pvaStatus);
	SG_NULLFREE(pCtx, pszHidTarget);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * sg.wc.revert_all( { "no_backups" : <bool>,
 *                     "wc_path"    : "<absolute disk path>",
 *                     "test"       : <bool>,
 *                     "verbose"    : <bool> } );
 *
 * REVERT everything in the WD.
 *
 *
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 * The OPTIONAL "test" requests that validate the args and stop.
 *
 * The OPTIONAL "verbose" requests the journal to be returned.
 *
 *
 * We return:
 *
 * r := {
 *        "stats"   : <<vhash_of_summary_stats>>,
 *        "journal" : <<array_of_queued_operations>>,    -- when verbose=true
 *      };
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, revert_all)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_varray * pvaJournal = NULL;
	SG_vhash * pvhResult = NULL;
	SG_vhash * pvhStats = NULL;
	JSObject* jso = NULL;
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;
	SG_bool bNoBackups = SG_FALSE;
	SG_bool bTest = SG_FALSE;
	SG_bool bVerbose = SG_FALSE;

	// Since there are no required named-parameters we allow
	// them to skip giving us an empty arg.

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "wc_path",    NULL,     &pszWc)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "no_backups", SG_FALSE, &bNoBackups)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "test",       SG_FALSE, &bTest)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "verbose",    SG_FALSE, &bVerbose)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.revert_all", pvh_args, pvh_got)  );
	}

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__revert_all(pCtx, pPathWc,
									 bNoBackups,
									 bTest,
									 ((bVerbose) ? &pvaJournal : NULL),
									 &pvhStats)  );

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhResult)  );
	SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhResult, "stats", &pvhStats)  );
	if (pvaJournal)
		SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhResult, "journal", &pvaJournal)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhResult, jso)  );
	SG_VHASH_NULLFREE(pCtx, pvhResult);

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	SG_VARRAY_NULLFREE(pCtx, pvaJournal);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_VHASH_NULLFREE(pCtx, pvhResult);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	SG_VARRAY_NULLFREE(pCtx, pvaJournal);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * sg.wc.revert_items( { "src"        : <<path-or-array-of-paths>>,
 *                       "wc_path"    : "<absolute disk path>",
 *                       "depth"      : <int>,
 *                       "revert-files"        : <bool>,
 *                       "revert-symlinks"     : <bool>,
 *                       "revert-directories"  : <bool>,
 *                       "revert-lost"         : <bool>,
 *                       "revert-added"        : <bool>,
 *                       "revert-removed"      : <bool>,
 *                       "revert-renamed"      : <bool>,
 *                       "revert-moved"        : <bool>,
 *                       "revert-merge-added"  : <bool>,
 *                       "revert-update-added" : <bool>,
 *                       "revert-attributes"   : <bool>,
 *                       "revert-content"      : <bool>,
 *                       "no_backups" : <bool>,
 *                       "test"       : <bool>,
 *                       "verbose"    : <bool> } );
 *
 * REVERT one or more items in the WD.
 *
 *
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 * The OPTIONAL "test" requests that validate the args and stop.
 *
 * The OPTIONAL "verbose" requests the journal to be returned.
 *
 * The various "revert-*" indicate that we should do partial
 * reverts (either by file type and/or by types of changes).
 *
 * If (test || verbose), we return:  (I'm going to keep the extra
 * layer of vhash in case I want to put stats in there too.)
 *
 * r := {
 *        "journal" : <<array_of_queued_operations>>,
 *      };
 *
 * Otherwise, we return nothing.
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, revert_items)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_varray * pvaJournal = NULL;
	SG_vhash * pvhResult = NULL;
	SG_stringarray * psa_src = NULL;
	JSObject* jso = NULL;
	SG_uint32 depth = SG_INT32_MAX;
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;
	SG_bool bNoBackups = SG_FALSE;
	SG_bool bTest = SG_FALSE;
	SG_bool bVerbose = SG_FALSE;
	SG_wc_status_flags flagMask_type_default = SG_WC_STATUS_FLAGS__T__MASK;
	SG_wc_status_flags flagMask_dirt_default = (SG_WC_STATUS_FLAGS__U__MASK
												|SG_WC_STATUS_FLAGS__S__MASK
												|SG_WC_STATUS_FLAGS__C__MASK);
	SG_wc_status_flags flagMask_type = SG_WC_STATUS_FLAGS__ZERO;
	SG_wc_status_flags flagMask_dirt = SG_WC_STATUS_FLAGS__ZERO;
	SG_wc_status_flags flagMask;
	SG_uint32 kBit;

	struct _bits
	{
		const char * pszKey;
		SG_wc_status_flags flagBit;
	};
	struct _bits bit_type[] =
		{	{ "revert-files",        SG_WC_STATUS_FLAGS__T__FILE },
			{ "revert-symlinks",     SG_WC_STATUS_FLAGS__T__SYMLINK },
			{ "revert-directories",  SG_WC_STATUS_FLAGS__T__DIRECTORY }
		};
	struct _bits bit_dirt[] =
		{	{"revert-lost",          SG_WC_STATUS_FLAGS__U__LOST},
			{"revert-added",         SG_WC_STATUS_FLAGS__S__ADDED},
			{"revert-removed",       SG_WC_STATUS_FLAGS__S__DELETED},
			{"revert-renamed",       SG_WC_STATUS_FLAGS__S__RENAMED},
			{"revert-moved",         SG_WC_STATUS_FLAGS__S__MOVED},
			{"revert-merge-added",   SG_WC_STATUS_FLAGS__S__MERGE_CREATED},
			{"revert-update-added",  SG_WC_STATUS_FLAGS__S__UPDATE_CREATED},
			{"revert-attributes",    SG_WC_STATUS_FLAGS__C__ATTRBITS_CHANGED},
			{"revert-content",       SG_WC_STATUS_FLAGS__C__NON_DIR_MODIFIED},
		};
	
	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
	SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

	SG_ERR_CHECK(  SG_jsglue__np__required__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "src", &psa_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__uint32(pCtx, pvh_args, pvh_got, "depth",      SG_INT32_MAX, &depth)  );
	for (kBit=0; kBit<SG_NrElements(bit_type); kBit++)
	{
		SG_bool bBit = SG_FALSE;
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(  pCtx, pvh_args, pvh_got, bit_type[kBit].pszKey, SG_FALSE, &bBit)  );
		if (bBit)
			flagMask_type |= bit_type[kBit].flagBit;
	}
	for (kBit=0; kBit<SG_NrElements(bit_dirt); kBit++)
	{
		SG_bool bBit = SG_FALSE;
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(  pCtx, pvh_args, pvh_got, bit_dirt[kBit].pszKey, SG_FALSE, &bBit)  );
		if (bBit)
			flagMask_dirt |= bit_dirt[kBit].flagBit;
	}
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(    pCtx, pvh_args, pvh_got, "wc_path",    NULL,     &pszWc)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(  pCtx, pvh_args, pvh_got, "no_backups", SG_FALSE, &bNoBackups)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(  pCtx, pvh_args, pvh_got, "test",       SG_FALSE, &bTest)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(  pCtx, pvh_args, pvh_got, "verbose",    SG_FALSE, &bVerbose)  );

	SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.revert_items", pvh_args, pvh_got)  );

	if (flagMask_type == SG_WC_STATUS_FLAGS__ZERO)
		flagMask_type = flagMask_type_default;
	if (flagMask_dirt == SG_WC_STATUS_FLAGS__ZERO)
		flagMask_dirt = flagMask_dirt_default;
	flagMask = (flagMask_type | flagMask_dirt);

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__revert_items__stringarray(pCtx, pPathWc, psa_src, depth, flagMask, bNoBackups, bTest,
													((bVerbose) ? &pvaJournal : NULL))  );

	if (pvaJournal)
	{
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhResult)  );
		SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhResult, "journal", &pvaJournal)  );
		SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
		SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhResult, jso)  );
	}
	else
	{
		JS_SET_RVAL(cx, vp, JSVAL_VOID);
	}

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_VARRAY_NULLFREE(pCtx, pvaJournal);
	SG_VHASH_NULLFREE(pCtx, pvhResult);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_VHASH_NULLFREE(pCtx, pvhResult);
	SG_VHASH_NULLFREE(pCtx, pvhResult);
	SG_VARRAY_NULLFREE(pCtx, pvaJournal);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * tx.line_history( { "src"        : <path>,
 *           "start_line"      : <int>,
 *           "length" : <int> } );
 *
 *           
 * Perform a line history on a single file in the working folder.
 *
 * Each <path> can be an absolute, relative, or repo path.
 *
 * The REQUIRED "start_line" parameter is the line that starts
 * the Area of Interest. If this line is outside the length of 
 * the file, an error will be reported.
 * 
 * The REQUIRED "length" parameter is the number of lines to include
 * in the Area of Interest. If the length is zero, an error will be
 * reported.
 *
 * In the WC version of line history, an error will be reported if
 * any lines in the Area of Interest have changed in the working folder
 * relative to the baseline.  If you need to avoid this, call the vv2 
 * version of line_history.
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, line_history)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	const char * psz_src = NULL;
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;
	SG_uint32 start_line = 0;
	SG_uint32 length = 0;
	SG_varray * pva_results = NULL;
	JSObject* jso = NULL;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(    pCtx, pvh_args, pvh_got, "wc_path",    NULL,     &pszWc)  );
    SG_ERR_CHECK(  SG_jsglue__np__required__sz(pCtx, pvh_args, pvh_got, "src", &psz_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__required__uint32(            pCtx, pvh_args, pvh_got, "start_line", &start_line)  );
	SG_ERR_CHECK(  SG_jsglue__np__required__uint32(              pCtx, pvh_args, pvh_got, "length", &length)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.line_history", pvh_args, pvh_got)  );

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__line_history(pCtx, pPathWc, psz_src, start_line, length, &pva_results)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pva_results, jso)  );
    SG_VARRAY_NULLFREE(pCtx, pva_results);

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_VARRAY_NULLFREE(pCtx, pva_results);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}


//////////////////////////////////////////////////////////////////

/**
 * var itemIssues = sg.wc.get_item_issues( { "src"     : "<path>",
 *                                           "wc_path" : "<absolute disk path>" } );
 *
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 * TODO 2012/04/18 describe the shape of the returned vhash value.
 *
 *
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, get_item_issues)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_vhash * pvhItemIssues = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;
	const char * psz_src = NULL;	// we do not own this
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "src",                  &psz_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "wc_path",    NULL,     &pszWc)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.get_item_issues", pvh_args, pvh_got)  );

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__get_item_issues(pCtx, pPathWc, psz_src, &pvhItemIssues)  );
	if (pvhItemIssues)
	{
		SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
		SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhItemIssues, jso)  );
	}
	else
	{
		JS_SET_RVAL(cx, vp, JSVAL_VOID);
	}
	SG_VHASH_NULLFREE(pCtx, pvhItemIssues);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvhItemIssues);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * var itemResolveInfo = sg.wc.get_item_resolve_info( { "src"     : "<path>",
 *                                                      "wc_path" : "<absolute disk path>" } );
 *
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 * TODO 2012/04/18 describe the shape of the returned vhash value.
 *
 *
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, get_item_resolve_info)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_vhash * pvhResolveInfo = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;
	const char * psz_src = NULL;	// we do not own this
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "src",                &psz_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "wc_path",  NULL,     &pszWc)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.get_item_resolve_info", pvh_args, pvh_got)  );

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__get_item_resolve_info(pCtx, pPathWc, psz_src, &pvhResolveInfo)  );
	if (pvhResolveInfo)
	{
		SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
		SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhResolveInfo, jso)  );
	}
	else
	{
		JS_SET_RVAL(cx, vp, JSVAL_VOID);
	}
	SG_VHASH_NULLFREE(pCtx, pvhResolveInfo);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvhResolveInfo);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * var szGid = sg.wc.get_item_gid( { "src"     : "<path>",
 *                                   "wc_path" : "<absolute disk path>"  } );
 *
 * Returns the GID of the item.  "<gid>"
 *
 * You probably want to use sg.wc.get_item_gid_path() rather than
 * this so that the result can be passed back in to a subsequent
 * command.
 *
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 * 
 * WARNING: UNCONTROLLED items have a TEMPORARY GID that is only
 * WARNING: good for the life of this TX.  They have a 't' prefix
 * WARNING: rather than 'g' prefix.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, get_item_gid)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char * pszGid = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSString * pjs = NULL;
	const char * psz_src = NULL;	// we do not own this
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "src",                  &psz_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "wc_path",   NULL,      &pszWc)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.get_item_gid", pvh_args, pvh_got)  );

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__get_item_gid(pCtx, pPathWc, psz_src, &pszGid)  );
	SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, pszGid))  );
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

	SG_NULLFREE(pCtx, pszGid);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    return JS_TRUE;

fail:
	SG_NULLFREE(pCtx, pszGid);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * var szGidPath = sg.wc.get_item_gid_path( { "src"     : "<path>",
 *                                            "wc_path" : "<absolute disk path>" } );
 *
 * Returns the GID-domain repo-path of the item.  "@<gid>"
 *
 * The OPTIONAL "wc_path" value identifies the working copy
 * with an absolute disk path anywhere inside it. If 
 * unspecified, the current working directory is used.
 *
 * WARNING: UNCONTROLLED items have a TEMPORARY GID that is only
 * WARNING: good for the life of this TX.  They have a 't' prefix
 * WARNING: rather than 'g' prefix.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(wc, get_item_gid_path)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	char * pszGidPath = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSString * pjs = NULL;
	const char * psz_src = NULL;	// we do not own this
	const char* pszWc = NULL;   	// we do not own this
	SG_pathname* pPathWc = NULL;

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "src",                  &psz_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__sz(                pCtx, pvh_args, pvh_got, "wc_path",  NULL,       &pszWc)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg.wc.get_item_gid_path", pvh_args, pvh_got)  );

	if (pszWc)
		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathWc, pszWc)  );

	SG_ERR_CHECK(  SG_wc__get_item_gid_path(pCtx, pPathWc, psz_src, &pszGidPath)  );
	SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, pszGidPath))  );
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

	SG_NULLFREE(pCtx, pszGidPath);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);

    return JS_TRUE;

fail:
	SG_NULLFREE(pCtx, pszGidPath);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_PATHNAME_NULLFREE(pCtx, pPathWc);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

static JSClass sg_wc__class = {
    "sg_wc",
    0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

// All sg.wc.* methods use the __np__ "Named Parameters" calling
// method (which gets passed in as a single jsobject/vhash), so
// all of the methods declare here that they take exactly 1 argument.
static JSFunctionSpec sg_wc__methods[] = {

	// NOTE 2011/10/21 (wc, apply), (wc, cancel), and (wc, journal)
	// NOTE            are NOT present in this API (because all
	// NOTE            methods in this API are self-contained.

	// NOTE 2011/10/21 We DO NOT have (wc, initialize) nor
	// NOTE            (wc, init_new_repo) methods.  These do
	// NOTE            not belong here (since their primary
	// NOTE            task is creating a new repo.

	{ "checkout",         SG_JSGLUE_METHOD_NAME(wc, checkout),         1,0},

	// NOTE 2011/10/21 We DO NOT have a (wc, export) method in
	// NOTE            this API.  Export has nothing to do with
	// NOTE            a WD.

	{ "add",              SG_JSGLUE_METHOD_NAME(wc, add),              1,0},
	{ "addremove",        SG_JSGLUE_METHOD_NAME(wc, addremove),        1,0},
	{ "move",             SG_JSGLUE_METHOD_NAME(wc, move),             1,0},
	{ "move_rename",      SG_JSGLUE_METHOD_NAME(wc, move_rename),      1,0},
	{ "rename",           SG_JSGLUE_METHOD_NAME(wc, rename),           1,0},
	{ "remove",           SG_JSGLUE_METHOD_NAME(wc, remove),           1,0},

	{ "lock",             SG_JSGLUE_METHOD_NAME(wc, lock),             1,0},
	{ "unlock",           SG_JSGLUE_METHOD_NAME(wc, unlock),           1,0},

	{ "get_item_status_flags", SG_JSGLUE_METHOD_NAME(wc, get_item_status_flags), 1,0},
	{ "get_item_dirstatus_flags", SG_JSGLUE_METHOD_NAME(wc, get_item_dirstatus_flags), 1,0},
	{ "status",           SG_JSGLUE_METHOD_NAME(wc, status),           1,0},
	{ "mstatus",          SG_JSGLUE_METHOD_NAME(wc, mstatus),          1,0},

	{ "parents",          SG_JSGLUE_METHOD_NAME(wc, parents),          1,0},
	{ "line_history",     SG_JSGLUE_METHOD_NAME(wc, line_history),     1,0},
	{ "get_wc_info",      SG_JSGLUE_METHOD_NAME(wc, get_wc_info),      1,0},
	{ "get_wc_top",       SG_JSGLUE_METHOD_NAME(wc, get_wc_top),       1,0},

	{ "flush_timestamp_cache",   SG_JSGLUE_METHOD_NAME(wc, flush_timestamp_cache),   1,0},

	{ "commit",           SG_JSGLUE_METHOD_NAME(wc, commit),           1,0},
	{ "update",           SG_JSGLUE_METHOD_NAME(wc, update),           1,0},
	{ "merge",            SG_JSGLUE_METHOD_NAME(wc, merge),            1,0},
	{ "revert_all",       SG_JSGLUE_METHOD_NAME(wc, revert_all),       1,0},
	{ "revert_items",     SG_JSGLUE_METHOD_NAME(wc, revert_items),     1,0},

	{ "get_item_gid",          SG_JSGLUE_METHOD_NAME(wc, get_item_gid),          1,0},
	{ "get_item_gid_path",     SG_JSGLUE_METHOD_NAME(wc, get_item_gid_path),     1,0},
	{ "get_item_issues",       SG_JSGLUE_METHOD_NAME(wc, get_item_issues),       1,0},
	{ "get_item_resolve_info", SG_JSGLUE_METHOD_NAME(wc, get_item_resolve_info), 1,0},

	{ NULL, NULL, 0, 0}
};

//////////////////////////////////////////////////////////////////

/**
 * Install the 'wc' api into the 'sg' namespace.
 *
 * The main 'wc' class is publicly available in 'sg.wc'
 * which makes things like 'sg.wc.move()' globally available.
 *
 */
void sg_wc__jsglue__install_scripting_api__wc(SG_context * pCtx,
											  JSContext * cx,
											  JSObject * glob,
											  JSObject * pjsobj_sg)
{
	JSObject* pjsobj_wc = NULL;

	SG_UNUSED( glob );

	SG_JS_NULL_CHECK(  pjsobj_wc = JS_DefineObject(cx, pjsobj_sg, "wc", &sg_wc__class, NULL, 0)  );
	SG_JS_BOOL_CHECK(  JS_DefineFunctions(cx, pjsobj_wc, sg_wc__methods)  );
	return;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return;
}

