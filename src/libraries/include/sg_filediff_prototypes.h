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
 * @file sg_filediff_prototypes.h
 * 
 * @details 
 * 
 */

#ifndef H_SG_FILEDIFF_PROTOTYPES_H
#define H_SG_FILEDIFF_PROTOTYPES_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////
// The Main Events
//////////////////////////////////////////////////////////////////

void SG_filediff(SG_context * pCtx, const SG_filediff_vtable * pVtable, void * pDiffBaton, SG_filediff_t ** ppDiff);
void SG_filediff3(SG_context * pCtx, const SG_filediff_vtable * pVtable, void * pDiffBaton, SG_filediff_t ** ppDiff);

void SG_filediff__free(SG_context * pCtx, SG_filediff_t * pDiff);


//////////////////////////////////////////////////////////////////
// Utility Functions
//////////////////////////////////////////////////////////////////

SG_bool SG_filediff__contains_conflicts(SG_filediff_t * pDiff);
SG_bool SG_filediff__contains_diffs(SG_filediff_t * pDiff);


//////////////////////////////////////////////////////////////////
// Displaying Diffs
//////////////////////////////////////////////////////////////////

void SG_filediff__get_next(SG_context * pCtx, SG_filediff_t * pDiff, SG_filediff_t ** ppNextDiff);

void SG_filediff__get_details(SG_context * pCtx, SG_filediff_t * pDiff, SG_diff_type * pDiffType,
	SG_int32 * pStart1,
	SG_int32 * pStart2,
	SG_int32 * pStart3,
	SG_int32 * pLen1,
	SG_int32 * pLen2,
	SG_int32 * pLen3);

void SG_filediff__output(SG_context * pCtx, const SG_filediff_output_functions * pVtable, void * pOutputBaton, SG_filediff_t * pDiff);


END_EXTERN_C;

#endif
