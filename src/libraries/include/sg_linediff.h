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
 * @file sg_textfilediff_prototypes.h
 * 
 * @details 
 * 
 */

#ifndef H_SG_LINEDIFF_H
#define H_SG_LINEDIFF_H

BEGIN_EXTERN_C;

///////////////////////////////////////////////////////////////////////////////

/**
 * Compare two disk files, and report if the given range of lines was edited between them.
 */
void SG_linediff(SG_context * pCtx,
				   SG_pathname * pPathLocalFileParent,
				   SG_pathname * pPathLocalFileChild,
				   SG_int32 nStartLine,
				   SG_int32 nNumLinesToSearch,
				   SG_bool * pbWasChanged,
				   SG_int32 * pnLineNumInParent,
				   SG_vhash ** ppvh_ResultDetails);

///////////////////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif
