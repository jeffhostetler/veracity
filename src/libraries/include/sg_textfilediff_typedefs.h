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
 * @file sg_textfilediff_typedefs.h
 * 
 * @details 
 * 
 */

#ifndef H_SG_TEXTFILEDIFF_TYPEDEFS_H
#define H_SG_TEXTFILEDIFF_TYPEDEFS_H

BEGIN_EXTERN_C;

///////////////////////////////////////////////////////////////////////////////


enum _SG_textfilediff_options
{
	SG_TEXTFILEDIFF_OPTION__REGULAR           = 0x00000000,
	SG_TEXTFILEDIFF_OPTION__IGNORE_WHITESPACE = 0x00000001,
	SG_TEXTFILEDIFF_OPTION__IGNORE_CASE       = 0x00000002,

	// "Strict eol" means that changes to the line terminator on an
	// individual line is considered a change to that line.
	//
	// Alternatively, "Native", "LF", "CRLF", or "CR" means that
	// Only that kind of line terminator is acknowledged, period.
	//
	// When none of these are chosen, the default behavior is that
	// any line terminator ("LF", "CR", or "CRLF) is recognized as
	// an eol, and a change to the line terminator of an individual
	// line is NOT considered a change to that line.
	SG_TEXTFILEDIFF_OPTION__STRICT_EOL = 0x00000100,
	SG_TEXTFILEDIFF_OPTION__NATIVE_EOL = 0x00000110,
	SG_TEXTFILEDIFF_OPTION__LF_EOL     = 0x00000120,
	SG_TEXTFILEDIFF_OPTION__CRLF_EOL   = 0x00000140,
	SG_TEXTFILEDIFF_OPTION__CR_EOL     = 0x00000180,
};
typedef enum _SG_textfilediff_options SG_textfilediff_options;


typedef struct _SG_textfilediff_t SG_textfilediff_t;

typedef struct _SG_textfilediff_iterator SG_textfilediff_iterator;


///////////////////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif
