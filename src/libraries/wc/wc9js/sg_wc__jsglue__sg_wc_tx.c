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
 * @file sg_wc__jsglue__sg_wc_tx.h
 *
 * @details JS glue for the wc7txapi layer.  This layer presents
 * a Transaction interface to allow more than one operation to be
 * QUEUED and then APPLIED.  The caller must manage the TX handle.
 *
 * Usage should look something like:
 * 
 * var tx = new sg_wc_tx( { ... } );
 * tx.move( { ... } );       // Queue move
 * tx.rename( { ... } );     // Queue rename
 * tx.apply( { ... } );      // Apply all queued changes
 *
 * NOTE that sg_wc_tx.apply() only commits this transaction
 * (in the SQL sense) causing the queued changes to be applied
 * and OFFICIALLY PENDED in the working directory.  It *does not*
 * COMMIT (in the 'vv commit' sense) the changes; that is it
 * *does not* create a new cset.
 *
 * 
 * NOTE: In the following syntax diagrams, define:
 *     <<path-or-array-of-paths>>  ::= "<path>"
 * or
 *     <<path-or-array-of-paths>>  ::= [ "<path1>", "<path2>", ... ]
 * 
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * tx.apply( {} );
 *
 * or
 *
 * tx.apply( );
 * 
 *
 * Apply all of the QUEUED changes in this transaction.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, apply)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_js_safeptr * pSafePtr_Owned = NULL;
	SG_wc_tx * pWcTx = NULL;		// we don't own this
	SG_wc_tx * pWcTx_Owned = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	// Since APPLY is an all or nothing affair, we probably
	// don't need any args here.  (Or at least I haven't
	// thought of any yet.)
	//
	// Since we standardized on named parameters, we expect
	// to see something like:
	//       tx.apply( {} );
	// but also allow:
	//       tx.apply( );

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.apply", pvh_args, pvh_got)  );
	}

	// We are ready to attempt the APPLY.  Whatever happens, we want to remove
	// the safeptr from the object so that they can't do it again *and* so that
	// if we take a hard error and throw, we don't want the JS GC code to
	// auto-attempt a CANCEL.
	//
	// The downside of this is that if the caller wants the journal
	// (to do verbose-style reporting), they'll need to get a copy
	// of it before the APPLY/CANCEL.

	JS_SetPrivate(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)), NULL);
	pSafePtr_Owned = pSafePtr;
	pWcTx_Owned = pWcTx;

	SG_ERR_CHECK(  SG_wc_tx__apply(pCtx, pWcTx)  );

	SG_WC_TX__NULLFREE(pCtx, pWcTx_Owned);
	SG_JS_SAFEPTR_NULLFREE(pCtx, pSafePtr_Owned);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
	SG_WC_TX__NULLFREE(pCtx, pWcTx_Owned);
	SG_JS_SAFEPTR_NULLFREE(pCtx, pSafePtr_Owned);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * tx.cancel( { } );
 *
 * or
 *
 * tx.cancel( );
 * 
 *
 * Discard all of the QUEUED changes in this transaction.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, cancel)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_js_safeptr * pSafePtr_Owned = NULL;
	SG_wc_tx * pWcTx = NULL;		// we do not own this
	SG_wc_tx * pWcTx_Owned = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	// Since CANCEL is an all or nothing affair, we probably
	// don't need any args here.  (Or at least I haven't
	// thought of any yet.)
	//
	// Since we standardized on named parameters, we expect
	// to see something like:
	//       tx.cancel( {} );
	// but also allow:
	//       tx.cancel( );

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.cancel", pvh_args, pvh_got)  );
	}
	
	// Most of CANCEL is just ignoring the in-memory stuff
	// that we have QUEUED, but there is a little database work.
	// Remove the pWcTx and SafePtr from the 'cx' so that the
	// GC/finalize code won't be able/need to re-attempt an
	// auto-cancel.

	JS_SetPrivate(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)), NULL);
	pSafePtr_Owned = pSafePtr;
	pWcTx_Owned = pWcTx;

	SG_ERR_CHECK(  SG_wc_tx__cancel(pCtx, pWcTx)  );

	SG_WC_TX__NULLFREE(pCtx, pWcTx_Owned);
	SG_JS_SAFEPTR_NULLFREE(pCtx, pSafePtr_Owned);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;

fail:
	SG_WC_TX__NULLFREE(pCtx, pWcTx_Owned);
	SG_JS_SAFEPTR_NULLFREE(pCtx, pSafePtr_Owned);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * journal = tx.journal( { } );
 *
 * or
 *
 * journal = tx.journal( );
 * 
 *
 * Return the sequence of QUEUED opeations.
 *
 * During the QUEUE phase, each tx-based operation appends
 * 1 or more operations to the JOURNAL (after doing any
 * required data validation).  This journal will be used
 * in the APPLY phase to alter the working directory.
 *
 * NOTE: If the caller wants to do verbose logging, call
 * this to get a copy of the journal *AFTER* queueing all
 * of the operations you want to do and *BEFORE* calling
 * either .apply() or .cancel().
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, journal)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;
	const SG_varray * pvaJournal;	// we do not own this

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	// Journal doesn't need an arg, but since we standardized
	// on named parameters, we allow:
	//        tx.journal( {} );
	// or
	//        tx.journal( );

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.journal", pvh_args, pvh_got)  );
	}

	SG_ERR_CHECK(  SG_wc_tx__journal(pCtx, pWcTx, &pvaJournal)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, (SG_varray *)pvaJournal, jso)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * tx.add( { "src"        : <<path-or-array-of-paths>>,
 *           "depth"      : <int>,
 *           "no_ignores" : <bool> } );
 *
 *           
 * ADD 1 or more item to version control.
 *
 * Each <path> can be an absolute, relative, or repo path.
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
 * WARNING: Adding an item that whose parent directory
 * is not under version control might cause the parent
 * directory to also be added.
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, add)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_stringarray * psa_src = NULL;
	SG_uint32 depth = SG_INT32_MAX;
	SG_bool bNoIgnores = SG_FALSE;

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "src", &psa_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__uint32(            pCtx, pvh_args, pvh_got, "depth",      SG_INT32_MAX, &depth)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_ignores", SG_FALSE,     &bNoIgnores)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.add", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_wc_tx__add__stringarray(pCtx, pWcTx, psa_src, depth, bNoIgnores)  );

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

//////////////////////////////////////////////////////////////////

/**
 * tx.addremove( { "src"        : <<path-or-array-of-paths>>,
 *                 "depth"      : <int>,
 *                 "no_ignores" : <bool> } );
 *
 *           
 * Do ADDREMOVE:  ADD any FOUND items; REMOVE any LOST items.
 *
 * Each <path> can be an absolute, relative, or repo path.
 *
 * The OPTIONAL "src" refers to one or more items.
 * If omitted, we default to "@/".
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
 * WARNING: Adding an item that whose parent directory
 * is not under version control might cause the parent
 * directory to also be added.
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, addremove)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_stringarray * psa_src = NULL;
	SG_uint32 depth = SG_INT32_MAX;
	SG_bool bNoIgnores = SG_FALSE;

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	// Since all of the named-parameters are optional, we can allow
	// them to skip giving us an empty arg.

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "src", &psa_src)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__uint32(            pCtx, pvh_args, pvh_got, "depth",      SG_INT32_MAX, &depth)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_ignores", SG_FALSE,     &bNoIgnores)  );
		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.add", pvh_args, pvh_got)  );
	}
	
	SG_ERR_CHECK(  SG_wc_tx__addremove__stringarray(pCtx, pWcTx, psa_src, depth, bNoIgnores)  );

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

//////////////////////////////////////////////////////////////////

/**
 * tx.move( { "src"      : <<path-or-array-of-paths>>,
 *            "dest_dir" : "<path>",
 *            "no_allow_after_the_fact"    : <bool> } );
 *
 *            
 * Move 1 or more source items into the destination directory.
 * 
 * Each <path> can be an absolute, relative, or repo path.
 *
 * OPTIONAL "no_allow_after_the_fact" specifies whether to
 * disallow after-the-fact operations.  We default to ALLOW.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, move)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_stringarray * psa_src = NULL;
	const char * pszDestDir = NULL;	// we do not own this
	SG_bool bNoAllowAfterTheFact = SG_FALSE;

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "src", &psa_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "dest_dir",       &pszDestDir)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_allow_after_the_fact", SG_FALSE, &bNoAllowAfterTheFact)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.move", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_wc_tx__move__stringarray(pCtx, pWcTx, psa_src, pszDestDir, bNoAllowAfterTheFact)  );

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
 * tx.move_rename( { "src"   : "<path>",
 *                   "dest"  : "<path>",
 *                   "no_allow_after_the_fact" : <bool> } );
 *
 * Move and/or Rename an item in a single step.
 * 
 * Each <path> can be an absolute, relative, or repo path.
 *
 * OPTIONAL "no_allow_after_the_fact" specifies whether to
 * disallow after-the-fact operations.  We default to ALLOW.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, move_rename)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	const char * psz_src = NULL;	// we do not own this
	const char * pszDest = NULL;	// we do not own this
	SG_bool bNoAllowAfterTheFact = SG_FALSE;

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "src",            &psz_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "dest",           &pszDest)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_allow_after_the_fact", SG_FALSE, &bNoAllowAfterTheFact)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.move_rename", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_wc_tx__move_rename(pCtx, pWcTx, psz_src, pszDest, bNoAllowAfterTheFact)  );

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

/**
 * tx.rename( { "src"       : "<path>",
 *              "entryname" : "<entryname>",
 *              "no_allow_after_the_fact"     : <bool> } );
 *
 * Rename an item to have the new entryname.
 * 
 * <path> can be an absolute, relative, or repo path.
 *
 * OPTIONAL "no_allow_after_the_fact" specifies whether to
 * disallow after-the-fact operations.  We default to ALLOW.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, rename)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	const char * psz_src = NULL;		// we do not own this
	const char * pszEntryname = NULL;	// we do not own this
	SG_bool bNoAllowAfterTheFact = SG_FALSE;

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "src",            &psz_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "entryname",      &pszEntryname)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_allow_after_the_fact", SG_FALSE, &bNoAllowAfterTheFact)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.rename", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_wc_tx__rename(pCtx, pWcTx, psz_src, pszEntryname, bNoAllowAfterTheFact)  );

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

/**
 * tx.remove( { "src"          : <<path-or-array-of-paths>>,
 *              "nonrecursive" : <bool>,
 *              "force"        : <bool>,
 *              "no_backups"   : <bool>,
 *              "keep"         : <bool> } );
 *
 * Remove/Delete one or more items.
 *
 * Each <path> can be an absolute, relative, or repo path.
 *
 *
 * OPTIONAL "nonrecursive" specifies to complain if a named
 * directory is not empty or contains controlled items.
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
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, remove)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_stringarray * psa_src = NULL;
	SG_bool bNonRecursive = SG_FALSE;
	SG_bool bForce = SG_FALSE;
	SG_bool bNoBackups = SG_FALSE;
	SG_bool bKeep = SG_FALSE;

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "src", &psa_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "nonrecursive", SG_FALSE, &bNonRecursive)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "force",        SG_FALSE, &bForce)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_backups",   SG_FALSE, &bNoBackups)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "keep",         SG_FALSE, &bKeep)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.remove", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_wc_tx__remove__stringarray(pCtx, pWcTx, psa_src, bNonRecursive, bForce, bNoBackups, bKeep)  );

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
 * varray = tx.status( { "src"            : <<path-or-array-of-paths>>,
 *                       "depth"          : <int>,
 *                       "rev"            : "<revision_number_or_csid_prefix>",
 *                       "tag"            : "<tag>",
 *                       "branch"         : "<branch_name>",
 *                       "list_unchanged" : <bool>,
 *                       "no_ignores"     : <bool>,
 *                       "no_tsc"         : <bool>,
 *                       "list_sparse"    : <bool>,
 *                       "list_reserved"  : <bool>,
 *                       "no_sort"        : <bool>  } );
 *
 * or
 *
 * varray = tx.status( );
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
 * The OPTIONAL "no_tsc" valud indicates whether we should
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
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, status)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
	SG_varray * pvaStatus = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;
	SG_stringarray * psa_src = NULL;
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

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	// Since all of the named parameters are optional, we
	// don't force them to give us an empty {}.

	SG_JS_BOOL_CHECK( (argc < 2)  );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz_or_varray_of_sz__stringarray(pCtx, pvh_args, pvh_got, "src",   &psa_src)  );
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
		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.status", pvh_args, pvh_got)  );
	}

	if (pszRev || pszTag || pszBranch)
	{
		SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec)  );
		if (pszRev)
			SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec, pszRev)  );
		if (pszTag)
			SG_ERR_CHECK(  SG_rev_spec__add_tag(pCtx, pRevSpec, pszTag)  );
		if (pszBranch)
			SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, pszBranch)  );

		SG_ERR_CHECK(  SG_wc_tx__status1__stringarray(pCtx, pWcTx, pRevSpec,
													  psa_src, depth,
													  bListUnchanged, bNoIgnores, bNoTSC, bListSparse, bListReserved, bNoSort,
													  &pvaStatus)  );
	}
	else
	{
		SG_ERR_CHECK(  SG_wc_tx__status__stringarray(pCtx, pWcTx,
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
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);

    return JS_TRUE;

fail:
    SG_VARRAY_NULLFREE(pCtx, pvaStatus);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * varray = tx.mstatus( { "no_ignores"  : <bool>,
 *                        "no_fallback" : <bool>,
 *                        "no_sort"     : <bool>  } );
 *
 * or
 *
 * varray = tx.status( );
 *
 * 
 * Compute "canonical" MERGE-STATUS (aka MSTATUS) for the WD.
 * This is the net-result after a merge.
 * We return a varray of vhashes with one row per item
 * (mostly compatible with regular STATUS output).
 *
 *
 * If not in a merge, we fallback to a regular status,
 * unless disallowed.
 *
 * TODO 2012/02/17 Currently, we do not take any of the
 * TODO            arguments to limit and/or pre-filter
 * TODO            the result.  Not sure I want to at this
 * TODO            point.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, mstatus)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
	SG_varray * pvaStatus = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;
	SG_bool bNoIgnores = SG_FALSE;
	SG_bool bNoFallback = SG_FALSE;
	SG_bool bNoSort = SG_FALSE;

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	// Since all of the named parameters are optional, we
	// don't force them to give us an empty {}.

	SG_JS_BOOL_CHECK( (argc < 2)  );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_ignores",     SG_FALSE,     &bNoIgnores)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_fallback",    SG_FALSE,     &bNoFallback)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_sort",        SG_FALSE,     &bNoSort)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.mstatus", pvh_args, pvh_got)  );
	}

	SG_ERR_CHECK(  SG_wc_tx__mstatus(pCtx, pWcTx, bNoIgnores, bNoFallback, bNoSort, &pvaStatus, NULL)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaStatus, jso)  );
    SG_VARRAY_NULLFREE(pCtx, pvaStatus);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
    return JS_TRUE;

fail:
    SG_VARRAY_NULLFREE(pCtx, pvaStatus);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * vhash = tx.get_item_status_flags( { "src"        : "<path>",
 *                                     "no_ignores" : <bool>,
 *                                     "no_tsc"     : <bool> } );
 *
 * Compute the status flags for an item.  This is a
 * cheaper version of "sg_wc_tx.status()"
 * when you don't care about all of the details.
 *
 * <path> can be an absolute, relative, or repo path.
 * 
 * The OPTIONAL "no_ignores" value indicates whether 
 * we should disregard the settings in .vvignores.
 * This defaults to FALSE.
 *
 * The OPTIONAL "no_tsc" valud indicates whether we should
 * bypass the TimeStampCache.  (This is mainly a debug option.)
 *
 * Note that we DO NOT take a DEPTH.
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, get_item_status_flags)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
	SG_wc_status_flags flags;
	SG_vhash * pvhProperties = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;
	const char * psz_src = NULL;	// we do not own this
	SG_bool bNoIgnores = SG_FALSE;
	SG_bool bNoTSC = SG_FALSE;

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "src",                  &psz_src)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_ignores", SG_FALSE, &bNoIgnores)  );
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(              pCtx, pvh_args, pvh_got, "no_tsc",     SG_FALSE, &bNoTSC)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.get_item_status_flags", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_wc_tx__get_item_status_flags(pCtx, pWcTx, psz_src, bNoIgnores, bNoTSC, &flags, &pvhProperties)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhProperties, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvhProperties);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvhProperties);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * vhash = tx.get_item_dirstatus_flags( { "src" : "<path>" } );
 *
 * Compute an status flags for a DIRECTORY.
 * Also determine if anything *WITHIN* the
 * directory has changed.
 *
 * <path> can be an absolute, relative, or repo path.
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, get_item_dirstatus_flags)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
	SG_wc_status_flags sf;
	SG_vhash * pvhResult = NULL;
	SG_vhash * pvhProperties = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;
	const char * psz_src = NULL;	// we do not own this
	SG_bool bDirContainsChanges = SG_FALSE;

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "src", &psz_src)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.get_item_dirstatus_flags", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_wc_tx__get_item_dirstatus_flags(pCtx, pWcTx, psz_src, &sf, &pvhProperties, &bDirContainsChanges)  );

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

    return JS_TRUE;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhResult);
    SG_VHASH_NULLFREE(pCtx, pvhProperties);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * varray = tx.parents( { } );
 *
 * or
 *
 * varray = tx.parents();
 * 
 *
 * Return an array of the HIDs of the parents of the WD.
 *
 * 
 * WARNING: We DO NOT take any arguments.  The set of parents
 * is essentially a constant and based upon the last CHECKOUT,
 * UPDATE, MERGE, or REVERT.  The original sg.pending_tree.parents()
 * did take arguments and did 3 different things (2 of them were
 * history-related).  This routine does not do that.
 *
 * By convention, parent[0] is the BASELINE.
 *
 * If varry.length > 1, then there is an active MERGE.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, parents)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_varray * pvaParents = NULL;
	JSObject* jso = NULL;

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	SG_JS_BOOL_CHECK( (argc < 2)  );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.parents", pvh_args, pvh_got)  );
	}

	SG_ERR_CHECK(  SG_wc_tx__get_wc_parents__varray(pCtx, pWcTx, &pvaParents)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewArrayObject(cx, 0, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_varray_into_jsobject(pCtx, cx, pvaParents, jso)  );
    SG_VARRAY_NULLFREE(pCtx, pvaParents);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    return JS_TRUE;

fail:
    SG_VARRAY_NULLFREE(pCtx, pvaParents);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * vhash = tx.get_wc_info( { } );
 *
 * or
 *
 * vhash = tx.get_wc_info( );
 *
 * 
 * Return a vhash of meta-data for the WD.
 *
 * 
 * NOTE: sg_wc_tx.get_wc_info() also returns the parents and a bunch
 *       other meta-data.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, get_wc_info)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
	SG_vhash * pvhInfo = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	// Since there are no named-parameters we allow
	// them to skip giving us an empty arg.

	SG_JS_BOOL_CHECK( (argc < 2)  );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.get_wc_info", pvh_args, pvh_got)  );
	}

	SG_ERR_CHECK(  SG_wc_tx__get_wc_info(pCtx, pWcTx, &pvhInfo)  );

    SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
    SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhInfo, jso)  );
    SG_VHASH_NULLFREE(pCtx, pvhInfo);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvhInfo);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * path = tx.get_wc_top( { } );
 *
 * or
 *
 * path = tx.get_wc_top( );
 *
 * 
 * Return the pathname of the top of the WD.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, get_wc_top)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
	SG_pathname * pPath = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSString * pjs = NULL;

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	// Since there are no named-parameters we allow
	// them to skip giving us an empty arg.

	SG_JS_BOOL_CHECK( (argc < 2)  );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.get_wc_top", pvh_args, pvh_got)  );
	}

	SG_ERR_CHECK(  SG_wc_tx__get_wc_top(pCtx, pWcTx, &pPath)  );

	SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, SG_pathname__sz(pPath)))  );
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

	SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    return JS_TRUE;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPath);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * tx.flush_timestamp_cache( { } );
 *
 * or
 * 
 * tx.flush_timestamp_cache( );
 *
 * Flush the timestamp cache.
 *
 * TODO 2012/06/14 This really doesn't need to be in the public API.
 * 
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, flush_timestamp_cache)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	SG_JS_BOOL_CHECK( (argc < 2)  );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.flush_timestamp_cache", pvh_args, pvh_got)  );
	}

	SG_ERR_CHECK(  SG_wc_tx__flush_timestamp_cache(pCtx, pWcTx)  );

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
 * vhash = tx.revert_all( { "no_backups" : <bool> }  );
 *
 * or
 *
 * vhash = tx.revert_all( );
 *
 * 
 * REVERT everything in the WD.
 *
 *
 * We return a vhash of summary stats.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, revert_all)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_vhash * pvhStats = NULL;
	JSObject* jso = NULL;
	SG_bool bNoBackups = SG_FALSE;

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	// Since there are no required named-parameters we allow
	// them to skip giving us an empty arg.

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "no_backups", SG_FALSE, &bNoBackups)  );

		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.revert_all", pvh_args, pvh_got)  );
	}

	SG_ERR_CHECK(  SG_wc_tx__revert_all(pCtx, pWcTx, bNoBackups, &pvhStats)  );

	SG_JS_NULL_CHECK(  (jso = JS_NewObject(cx, NULL, NULL, NULL))  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	SG_ERR_CHECK(  sg_jsglue__copy_vhash_into_jsobject(pCtx, cx, pvhStats, jso)  );

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_VHASH_NULLFREE(pCtx, pvhStats);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * tx.revert_items( {    "src"        : <<path-or-array-of-paths>>,
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
 *                       "no_backups" : <bool> }  );
 *
 *                 
 * REVERT one or more items in the WD.
 *
 * The various "revert-*" indicate that we should do partial
 * reverts (either by file type and/or by types of changes).
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, revert_items)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
	SG_stringarray * psa_src = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	SG_uint32 depth = SG_INT32_MAX;
	SG_bool bNoBackups = SG_FALSE;
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

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	SG_JS_BOOL_CHECK( (argc == 2) );
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
	SG_ERR_CHECK(  SG_jsglue__np__optional__bool(  pCtx, pvh_args, pvh_got, "no_backups", SG_FALSE, &bNoBackups)  );

	SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.revert_items", pvh_args, pvh_got)  );

	if (flagMask_type == SG_WC_STATUS_FLAGS__ZERO)
		flagMask_type = flagMask_type_default;
	if (flagMask_dirt == SG_WC_STATUS_FLAGS__ZERO)
		flagMask_dirt = flagMask_dirt_default;
	flagMask = (flagMask_type | flagMask_dirt);

	SG_ERR_CHECK(  SG_wc_tx__revert_items__stringarray(pCtx, pWcTx, psa_src, depth, flagMask, bNoBackups)  );

	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;

fail:
	SG_STRINGARRAY_NULLFREE(pCtx, psa_src);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * var itemIssues = tx.get_item_issues( { "src" : "<path>" } );
 *
 * TODO 2012/04/18 describe the shape of the returned vhash value.
 *
 *
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, get_item_issues)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
	SG_vhash * pvhItemIssues = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;
	const char * psz_src = NULL;	// we do not own this

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "src",                  &psz_src)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.get_item_issues", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_wc_tx__get_item_issues(pCtx, pWcTx, psz_src, &pvhItemIssues)  );
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

    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvhItemIssues);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * var itemResolveInfo = tx.get_item_resolve_info( { "src" : "<path>" } );
 *
 * TODO 2012/04/18 describe the shape of the returned vhash value.
 *
 *
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, get_item_resolve_info)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
	SG_vhash * pvhResolveInfo = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSObject* jso = NULL;
	const char * psz_src = NULL;	// we do not own this

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "src",                  &psz_src)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.get_item_resolve_info", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_wc_tx__get_item_resolve_info(pCtx, pWcTx, psz_src, &pvhResolveInfo)  );
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

    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvhResolveInfo);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

/**
 * var szGid = tx.get_item_gid( { "src" : "<path>" } );
 *
 * Returns the GID of the item.
 *
 * WARNING: UNCONTROLLED items have a TEMPORARY GID that is only
 * WARNING: good for the life of this TX.  They have a 't' prefix
 * WARNING: rather than 'g' prefix.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, get_item_gid)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
	char * pszGid = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSString * pjs = NULL;
	const char * psz_src = NULL;	// we do not own this

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "src",                  &psz_src)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.get_item_gid", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_wc_tx__get_item_gid(pCtx, pWcTx, psz_src, &pszGid)  );
	SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, pszGid))  );
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

	SG_NULLFREE(pCtx, pszGid);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    return JS_TRUE;

fail:
	SG_NULLFREE(pCtx, pszGid);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

/**
 * var szGidPath = tx.get_item_gid_path( { "src" : "<path>" } );
 *
 * Returns the GID-domain repo-path of the item.  "@<gid>"
 *
 * WARNING: UNCONTROLLED items have a TEMPORARY GID that is only
 * WARNING: good for the life of this TX.  They have a 't' prefix
 * WARNING: rather than 'g' prefix.
 *
 */
SG_JSGLUE_METHOD_PROTOTYPE(sg_wc_tx, get_item_gid_path)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, JSVAL_TO_OBJECT(JS_THIS(cx, vp)));
	SG_wc_tx * pWcTx = NULL;		// we do not own this
	char * pszGidPath = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	JSString * pjs = NULL;
	const char * psz_src = NULL;	// we do not own this

	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );

	SG_JS_BOOL_CHECK( (argc == 1) );
	SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
    SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
    SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

    SG_ERR_CHECK(  SG_jsglue__np__required__sz(                pCtx, pvh_args, pvh_got, "src",                  &psz_src)  );
    SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx.get_item_gid_path", pvh_args, pvh_got)  );

	SG_ERR_CHECK(  SG_wc_tx__get_item_gid_path(pCtx, pWcTx, psz_src, &pszGidPath)  );
	SG_JS_NULL_CHECK(  (pjs = JS_NewStringCopyZ(cx, pszGidPath))  );
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(pjs));

	SG_NULLFREE(pCtx, pszGidPath);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    return JS_TRUE;

fail:
	SG_NULLFREE(pCtx, pszGidPath);
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
    return JS_FALSE;
}

//////////////////////////////////////////////////////////////////

static void sg_wc_tx__finalize(JSContext * cx, JSObject * obj);

static JSClass sg_wc_tx__class = {
    "sg_wc_tx",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
	sg_wc_tx__finalize /*JS_FinalizeStub*/,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

//////////////////////////////////////////////////////////////////

/**
 * The constructor for the sg_wc_tx class.
 * 
 * tx = new sg_wc_tx( { "wc_path" : "<path>",
 *                      "read_only" : <bool> } );
 *
 * or
 *
 * tx = new sg_wc_tx( );
 * 
 *
 * Begin a transaction to view and/or alter the working
 * directory (aka working copy).
 *
 * The OPTIONAL <path> specifies the "Working Directory Top".
 * If omitted, we use the CWD to find it.
 *
 * We DO NOT take a repo-descriptor since it can be fetched
 * using the wc_path.
 *
 * The OPTIONAL "read_only" indicates that we don't intend to
 * make any changes (e.g. a STATUS).  This is advisory only
 * (and might let the DB cut some corners).  It DOES NOT mean
 * that we won't write in the DB (because we want to be able
 * to update the timestamp cache, for example, at any time).
 * This defaults to FALSE.
 *
 * We return a TX handle in a SafePtr.  ALL SUBSEQUENT WC
 * OPERATIONS SHOULD USE THIS HANDLE AND THE sg_wc_tx METHODS.
 * DO NOT TRY TO MIX TX-based AND non-TX-based OPERATIONS.
 * The caller should eventually do a sg_wc_tx.apply() or
 * sg_wc_tx.cancel() to clean up.
 * 
 */
static JSBool sg_wc_tx__constructor(JSContext *cx, uintN argc, jsval *vp)
{
	SG_context * pCtx = SG_jsglue__get_clean_sg_context(cx);
	jsval * argv = JS_ARGV(cx, vp);
	JSObject* jso = NULL;
    SG_vhash* pvh_args = NULL;
    SG_vhash* pvh_got = NULL;
	const char * pszWcPath = NULL;	// we do not own this
	SG_pathname * pPath = NULL;
	SG_wc_tx * pWcTx = NULL;
	SG_js_safeptr * pSafePtr = NULL;
	SG_bool bReadOnly = SG_FALSE;

	// Since all of the named-parameters are optional, we can allow
	// them to skip giving us an empty arg.  that is, they can do:
	//      tx = new sg_wc_tx();
	// rather than:
	//      tx = new sg_wc_tx( {} );

	SG_JS_BOOL_CHECK( (argc < 2) );
	if (argc == 1)
	{
		SG_JS_BOOL_CHECK( (JSVAL_IS_OBJECT(argv[0])) );
		SG_ERR_CHECK(  sg_jsglue__jsobject_to_vhash(pCtx, cx, JSVAL_TO_OBJECT(argv[0]), &pvh_args)  );
		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvh_got)  );

		SG_ERR_CHECK(  SG_jsglue__np__optional__sz(  pCtx, pvh_args, pvh_got, "wc_path",       NULL,     &pszWcPath)  );
		SG_ERR_CHECK(  SG_jsglue__np__optional__bool(pCtx, pvh_args, pvh_got, "read_only", SG_FALSE, &bReadOnly)  );
		SG_ERR_CHECK(  SG_jsglue__np__anything_not_got_is_invalid( pCtx, "sg_wc_tx__construtor", pvh_args, pvh_got)  );

		if (pszWcPath)
			SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPath, pszWcPath)  );
	}

	SG_ERR_CHECK(  SG_WC_TX__ALLOC__BEGIN(pCtx, &pWcTx, pPath, bReadOnly)  );
	SG_ERR_CHECK(  sg_wc__jsglue__safeptr__wrap__wc_tx(pCtx, pWcTx, &pSafePtr)  );
	pWcTx = NULL;		// safe-ptr owns it now
	
	SG_JS_NULL_CHECK(  jso = JS_NewObject(cx, &sg_wc_tx__class, NULL, NULL)  );
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(jso));
	JS_SetPrivate(cx, jso, pSafePtr);
	pSafePtr = NULL;	// JS owns it now

    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);

    return JS_TRUE;

fail:
    SG_VHASH_NULLFREE(pCtx, pvh_args);
    SG_VHASH_NULLFREE(pCtx, pvh_got);
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	// TODO do we need to destroy pSafePtr, pWcTx ?
    return JS_FALSE;
}

/**
 * From what I can tell from testing, the "finalize" method
 * gets called once for each instance *and* once for the
 * class.
 *
 * The "safe ptr" is allocated in our constructor and freed
 * in either sg_wc_tx.apply() or sg_wc_tx.cancel().  So after
 * either of those calls, the sg_wc_tx is basically a zombie
 * waiting for GC (from our point of view).
 *
 * The important point is that if an object is GC'd and
 * still has valid safe ptr, then the caller didn't do a
 * APPLY or CANCEL on the TX and we're going to leak
 * and more importantly, we're not going to do any of the
 * things we QUEUED.
 *
 */
static void sg_wc_tx__finalize(JSContext * cx, JSObject * obj)
{
	SG_js_safeptr * pSafePtr = SG_jsglue__get_object_private(cx, obj);

	if (pSafePtr)
	{
		SG_context * pCtx = (SG_context *)JS_GetContextPrivate(cx);
		SG_wc_tx * pWcTx = NULL;

		SG_ERR_IGNORE(  sg_wc__jsglue__safeptr__unwrap__wc_tx(pCtx, pSafePtr, &pWcTx)  );
		if (pWcTx)
		{
#if defined(DEBUG)
			SG_uint32 sizeJournal = 0;

			SG_ERR_IGNORE(  SG_wc_tx__journal_length(pCtx, pWcTx, &sizeJournal)  );
			if (sizeJournal > 0)
			{
				SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
										   "sg_wc_tx__finalize: implicit sg_wc_tx.cancel() on journal containing %d items.\n",
										   sizeJournal)  );
			}
			else
			{
				// If the TX was clean, don't bother complaining.
			}
#endif		

			SG_ERR_IGNORE(  SG_wc_tx__cancel(pCtx, pWcTx)  );
			SG_WC_TX__NULLFREE(pCtx, pWcTx);
		}
		SG_JS_SAFEPTR_NULLFREE(pCtx, pSafePtr);
		JS_SetPrivate(cx, obj, NULL);
	}

	JS_FinalizeStub(cx, obj);
}

static JSPropertySpec sg_wc_tx__properties[] = {
    {NULL,0,0,NULL,NULL}
};

// All sg_wc_tx.* methods use the __np__ "Named Parameters" calling
// method (which gets passed in as a single jsobject/vhash), so
// all of the methods declare here that they take exactly 1 argument.
static JSFunctionSpec sg_wc_tx__methods[] = {
	{ "apply",            SG_JSGLUE_METHOD_NAME(sg_wc_tx, apply),            1,0},
	{ "cancel",           SG_JSGLUE_METHOD_NAME(sg_wc_tx, cancel),           1,0},
	{ "journal",          SG_JSGLUE_METHOD_NAME(sg_wc_tx, journal),          1,0},

	// NOTE 2011/10/21 We DO NOT have (sg_wc_tx, initialize) nor
	// NOTE            (sg_wc_tx, init_new_repo) methods in this API.
	// NOTE            These do not belong here (since the primary
	// NOTE            task of 'init' is to create a new repo).

	// NOTE 2011/10/21 We DO NOT have a (sg_wc_tx, checkout) method at
	// NOTE            this API layer (since a checkout may require
	// NOTE            more than one TX to get everything populated).

	// NOTE 2011/10/21 We DO NOT have a (sg_wc_tx, export) method in
	// NOTE            this API.  Export has nothing to do with a WD.

	{ "add",              SG_JSGLUE_METHOD_NAME(sg_wc_tx, add),              1,0},
	{ "addremove",        SG_JSGLUE_METHOD_NAME(sg_wc_tx, addremove),        1,0},
	{ "move",             SG_JSGLUE_METHOD_NAME(sg_wc_tx, move),             1,0},
	{ "move_rename",      SG_JSGLUE_METHOD_NAME(sg_wc_tx, move_rename),      1,0},
	{ "rename",           SG_JSGLUE_METHOD_NAME(sg_wc_tx, rename),           1,0},
	{ "remove",           SG_JSGLUE_METHOD_NAME(sg_wc_tx, remove),           1,0},

	{ "get_item_status_flags", SG_JSGLUE_METHOD_NAME(sg_wc_tx, get_item_status_flags), 1,0},
	{ "get_item_dirstatus_flags", SG_JSGLUE_METHOD_NAME(sg_wc_tx, get_item_dirstatus_flags), 1,0},
	{ "status",           SG_JSGLUE_METHOD_NAME(sg_wc_tx, status),           1,0},
	{ "mstatus",          SG_JSGLUE_METHOD_NAME(sg_wc_tx, mstatus),          1,0},

	{ "parents",          SG_JSGLUE_METHOD_NAME(sg_wc_tx, parents),          1,0},
	{ "get_wc_info",      SG_JSGLUE_METHOD_NAME(sg_wc_tx, get_wc_info),      1,0},
	{ "get_wc_top",       SG_JSGLUE_METHOD_NAME(sg_wc_tx, get_wc_top),       1,0},

	{ "flush_timestamp_cache",   SG_JSGLUE_METHOD_NAME(sg_wc_tx, flush_timestamp_cache),   1,0},

	// NOTE 2011/11/29 We DO NOT have a (sg_wc_tx, commit) method in
	// NOTE            this API.  Because of the side-effects to the
	// NOTE            REPO after the new changeset is created, COMMIT
	// NOTE            needs to have complete control of the TX.

	// NOTE 2012/01/17 We DO NOT have a (sg_wc_tx, update) method in
	// NOTE            this API.  Because of the side-effects to the
	// NOTE            WD after we switch baselines, the 'attached'
	// NOTE            branch name is not stored in the wd.sql so
	// NOTE            we need to atomically change both it and the
	// NOTE            branches.json.  And with the fake-merge stuff
	// NOTE            there are other reasons too.

	// NOTE 2012/09/11 We DO NOT have a (sg_wc_tx, merge) method in
	// NOTE            this API.  Now that MERGE can do a fast-forward
	// NOTE            merge (which alters stuff outside of the TX),
	// NOTE            we need this to be self-contained like UPDATE.

	// TODO 2012/07/27 Not sure if I want to expose revert_all in layer 7.
	{ "revert_all",       SG_JSGLUE_METHOD_NAME(sg_wc_tx, revert_all),       1,0},
	{ "revert_items",     SG_JSGLUE_METHOD_NAME(sg_wc_tx, revert_items),     1,0},

	{ "get_item_gid",          SG_JSGLUE_METHOD_NAME(sg_wc_tx, get_item_gid),          1,0},
	{ "get_item_gid_path",     SG_JSGLUE_METHOD_NAME(sg_wc_tx, get_item_gid_path),     1,0},
	{ "get_item_issues",       SG_JSGLUE_METHOD_NAME(sg_wc_tx, get_item_issues),       1,0},
	{ "get_item_resolve_info", SG_JSGLUE_METHOD_NAME(sg_wc_tx, get_item_resolve_info), 1,0},

	{ NULL, NULL, 0, 0}
};

//////////////////////////////////////////////////////////////////

/**
 * Install the 'sg_wc_tx' class into the namespace.
 * Scripts must use
 *       tx = new sg_wc_tx( ... );
 * to create an instance of this class before they
 * can use any of the methods.  These methods are
 * NOT directly available in the 'sg.' namespace.
 *
 */
void sg_wc__jsglue__install_scripting_api__wc_tx(SG_context * pCtx,
												 JSContext * cx,
												 JSObject * glob,
												 JSObject * pjsobj_sg)
{
	SG_UNUSED( pjsobj_sg );

    SG_JS_NULL_CHECK(  JS_InitClass(
            cx,
            glob,
            NULL, /* parent proto */
            &sg_wc_tx__class,
			sg_wc_tx__constructor,
            0, /* nargs */
            sg_wc_tx__properties,
            sg_wc_tx__methods,
            NULL,  /* static properties */
            NULL   /* static methods */
            )  );

	return;

fail:
	SG_jsglue__report_sg_error(pCtx,cx);	// DO NOT SG_ERR_IGNORE() THIS
	return;
}

