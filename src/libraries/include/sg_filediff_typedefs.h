/**
 * @copyright
 * 
 * Copyright (C) 2003, 2010-2013 SourceGear LLC. All rights reserved.
 * This file contains file-diffing-related source from subversion's
 * libsvn_delta, forked at version 0.20.0, which contained the
 * following copyright notice:
 *
 *      Copyright (c) 2000-2003 CollabNet.  All rights reserved.
 *      
 *      This software is licensed as described in the file COPYING, which
 *      you should have received as part of this distribution.  The terms
 *      are also available at http://subversion.tigris.org/license-1.html.
 *      If newer versions of this license are posted there, you may use a
 *      newer version instead, at your option.
 *      
 *      This software consists of voluntary contributions made by many
 *      individuals.  For exact contribution history, see the revision
 *      history and logs, available at http://subversion.tigris.org/.
 * 
 * @endcopyright
 * 
 * @file sg_filediff_typedefs.h
 * 
 * @details 
 * 
 */

#ifndef H_SG_FILEDIFF_TYPEDEFS_H
#define H_SG_FILEDIFF_TYPEDEFS_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

enum __sg_diff_type
{
    SG_DIFF_TYPE__COMMON        = 0,
    SG_DIFF_TYPE__DIFF_MODIFIED = 1,
    SG_DIFF_TYPE__DIFF_LATEST   = 2,
    SG_DIFF_TYPE__DIFF_COMMON   = 3,
    SG_DIFF_TYPE__CONFLICT      = 4,
    SG_DIFF_TYPES_COUNT
};

typedef enum __sg_diff_type SG_diff_type;


/// Opaque type diffs between datasources.
typedef struct _sg_filediff_t SG_filediff_t; // Formerly "svn_diff_t"


enum _SG_filediff_datasource
{
    SG_FILEDIFF_DATASOURCE__ORIGINAL = 0, // The oldest form of the data.
    SG_FILEDIFF_DATASOURCE__MODIFIED = 1, // The same data, but potentially changed by the user.
    SG_FILEDIFF_DATASOURCE__LATEST   = 2, // The latest version of the data, possibly different than the user's modified version.
    SG_FILEDIFF_DATASOURCE__ANCESTOR = 3  // The common ancestor of original and modified.
};
typedef enum _SG_filediff_datasource SG_filediff_datasource; // Formerly "svn_diff_datasource_e"


/// A vtable for reading data from the datasources for a diff or merge.
struct _SG_filediff_vtable
{
    void (*datasource_open)(SG_context * pCtx, void * pDiffBaton, SG_filediff_datasource datasource);

    void (*datasource_close)(SG_context * pCtx, void * pDiffBaton, SG_filediff_datasource datasource);

    void (*datasource_get_next_token)(SG_context * pCtx, void * pDiffBaton, SG_filediff_datasource datasource, void **ppToken);

    SG_int32 (*token_compare)(void * pDiffBaton, void * pToken1, void * pToken2);

    // This will only ever get call on the most recent token fetched by datasource_get_next_token (if the tokens were equal according to token_compare).
    void (*token_discard)(void * pDiffBaton, void * pToken);

    void (*token_discard_all)(void * pDiffBaton);
};
typedef struct _SG_filediff_vtable SG_filediff_vtable; // Formerly "svn_diff_fns_t"


typedef void (*SG_filediff_output_function)(
    SG_context * pCtx,
    void * pOutputBaton,
    SG_int32 originalStart, SG_int32 originalLength,
    SG_int32 modifiedStart, SG_int32 modifiedLength,
    SG_int32 latestStart, SG_int32 latestLength);

/// A vtable for displaying (or consuming) differences between datasources.
struct _SG_filediff_output_functions
{
    SG_filediff_output_function output_common;
    SG_filediff_output_function output_diff_modified;
    SG_filediff_output_function output_diff_latest;
    SG_filediff_output_function output_diff_common;
    SG_filediff_output_function output_conflict;
};
typedef struct _SG_filediff_output_functions SG_filediff_output_functions; // Formerly "svn_diff_output_fns_t"


enum _SG_filediff_doc_ndx
{
    SG_FILEDIFF_DOC_NDX__UNSPECIFIED = -1,
    SG_FILEDIFF_DOC_NDX__ORIGINAL    =  0,
    SG_FILEDIFF_DOC_NDX__MODIFIED    =  1,
    SG_FILEDIFF_DOC_NDX__LATEST      =  2
};
typedef enum _SG_filediff_doc_ndx SG_filediff_doc_ndx;


//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif
