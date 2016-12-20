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

#ifndef H_SG_TEXTFILEDIFF_PROTOTYPES_H
#define H_SG_TEXTFILEDIFF_PROTOTYPES_H

BEGIN_EXTERN_C;

///////////////////////////////////////////////////////////////////////////////


/**
 * Compute differences between two files.
 * Output is an opaque SG_textfilediff_t, which stores the diff purely in terms of line numbers.
 */
void SG_textfilediff(
	SG_context * pCtx,
	const SG_pathname * pPathnameOriginal, const SG_pathname * pPathnameModified,
	SG_textfilediff_options options,
	SG_textfilediff_t ** ppDiff);

/**
 * Output a "unified" style diff based on the result from SG_textfilediff().
 */
void SG_textfilediff__output_unified__string(
	SG_context * pCtx,
	SG_textfilediff_t * pDiff,
	const SG_string * pHeaderOriginal, const SG_string * pHeaderModified,
	int context,
	SG_string ** ppOutput);

/**
 * Compute the merge of two files based on a common ancestor.
 * Output is an opaque SG_textfilediff_t, which stores the merge purely in terms of line numbers.
 */
void SG_textfilediff3(
	SG_context * pCtx,
	const SG_pathname * pPathnameOriginal, const SG_pathname * pPathnameModified, const SG_pathname * pPathnameLatest,
	SG_textfilediff_options options,
	SG_textfilediff_t ** ppDiff,
	SG_bool * pHadConflicts);

/**
 * Output the merge result from SG_textfilediff3() as a string.
 * Note: As an SG_string, the result will be in utf-8 regardless of the original encoding(s).
 */
void SG_textfilediff3__output__string(
	SG_context * pCtx,                //< [in] [out] Error and context info.
	SG_textfilediff_t * pDiff,        //< [in] The merge result computation from SG_textfilediff3().
	const SG_string * pLabelModified, //< [in] Label to print after "<<<<<<< " in conflicts.
	const SG_string * pLabelLatest,   //< [in] Label to print after ">>>>>>> " in conflicts.
	SG_string ** ppOutput             //< [out] The result of the merge as an SG_string.
	);

/**
 * Output the merge result from SG_textfilediff3() to a file.
 * Note: The file will be encoded using the same encoding as the original (ancestor) file
 *       (unless the encoding was changed in one of the modified files).
 */
void SG_textfilediff3__output__file(
	SG_context * pCtx,                //< [in] [out] Error and context info.
	SG_textfilediff_t * pDiff,        //< [in] The merge result computation from SG_textfilediff3().
	const SG_string * pLabelModified, //< [in] Label to print after "<<<<<<< " in conflicts.
	const SG_string * pLabelLatest,   //< [in] Label to print after ">>>>>>> " in conflicts.
	SG_file *pOutputFile              //< [in] File handle to ready to be written to with the result of the merge.
	);

/**
 * Begin iterating over the diffs.
 */
void SG_textfilediff__iterator__first(
	SG_context * pCtx,							//[in] [out] Error and context info.
	SG_textfilediff_t * pDiff,					//[in] The textfilediff which 
	SG_textfilediff_iterator ** ppDiffIterator, //[out] The iterator, which you must free.
	SG_bool * pbOk);							//[out] If true, then the iterator points at the first diff entry

/**
 * Continue iterating over the diffs.
 */
void SG_textfilediff__iterator__next(
	SG_context * pCtx,							//[in] [out] Error and context info.
	SG_textfilediff_iterator * pDiffIterator, //[out] The iterator.
	SG_bool * pbOk);							//[out] If true, then the iterator points at the next diff entry

/**
 * Continue iterating over the diffs.
 */
void SG_textfilediff__iterator__details(
	SG_context * pCtx,							//[in] [out] Error and context info.
	SG_textfilediff_iterator * pDiffIterator, //[out] The iterator.
	SG_diff_type * pDiffType,
	SG_int32 * pStart1,
	SG_int32 * pStart2,
	SG_int32 * pStart3,
	SG_int32 * pLen1,
	SG_int32 * pLen2,
	SG_int32 * pLen3);							//[out] If true, then the iterator points at the next diff entry
/**
 * Free the iterator
 */
void SG_textfilediff__iterator__free(
	SG_context * pCtx,							 //[in] [out] Error and context info.
	SG_textfilediff_iterator * pDiffIterator); //[out] The iterator.

/**
 * Free memory used by SG_textfilediff_t.
 */
void SG_textfilediff__free(SG_context * pCtx, SG_textfilediff_t * pDiff);


///////////////////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif
