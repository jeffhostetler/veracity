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
 * @file sg_mergetool__builtin_merge.h
 *
 * @details This is a built-in mergetool that knows how to call the
 * internal diff3 (aka merge) code in sg_diffcore.c
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_MERGETOOL__BUILTIN_MERGE_H
#define H_SG_MERGETOOL__BUILTIN_MERGE_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

static void _do_merge(SG_context * pCtx, SG_textfilediff_options options,
					  const SG_pathname * pPathAncestor, const SG_string * pStrAncestorLabel,
					  const SG_pathname * pPathLeft,     const SG_string * pStrLeftLabel,
					  const SG_pathname * pPathRight,    const SG_string * pStrRightLabel,
					  const SG_pathname * pPathResult,   const SG_string * pStrResultLabel,
					  SG_bool * pbHaveConflicts)
{
	SG_file * pFile_Output = NULL;
	SG_textfilediff_t * pTextfilediff = NULL;

	SG_UNUSED( pStrAncestorLabel );
	SG_UNUSED( pStrResultLabel );

	SG_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathResult, SG_FILE_WRONLY|SG_FILE_OPEN_OR_CREATE|SG_FILE_TRUNC, 0644, &pFile_Output)  );

	SG_ERR_CHECK(  SG_textfilediff3(pCtx,
								 pPathAncestor, pPathLeft, pPathRight,
								 options, &pTextfilediff, pbHaveConflicts)  );
	SG_ERR_CHECK(  SG_textfilediff3__output__file(pCtx, pTextfilediff,pStrLeftLabel, pStrRightLabel,pFile_Output)  );

fail:
	SG_FILE_NULLCLOSE(pCtx, pFile_Output);
	SG_TEXTFILEDIFF_NULLFREE(pCtx, pTextfilediff);
}

static void _call_do_merge(SG_context * pCtx, SG_textfilediff_options options,
						   const SG_pathname * pPathAncestor, const SG_string * pStrAncestorLabel,
						   const SG_pathname * pPathLeft,     const SG_string * pStrLeftLabel,
						   const SG_pathname * pPathRight,    const SG_string * pStrRightLabel,
						   const SG_pathname * pPathResult,   const SG_string * pStrResultLabel,
						   SG_int32 * pStatus)
{
	SG_string * pStrAncestor_Header = NULL;
	SG_string * pStrLeft_Header     = NULL;
	SG_string * pStrRight_Header    = NULL;
	SG_bool bHaveConflicts = SG_TRUE;

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStrAncestor_Header)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStrLeft_Header)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStrRight_Header)  );

	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStrAncestor_Header, "%s: %s", SG_string__sz(pStrAncestorLabel), SG_pathname__sz(pPathAncestor))  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStrLeft_Header,     "%s: %s", SG_string__sz(pStrLeftLabel),     SG_pathname__sz(pPathLeft))  );
	SG_ERR_CHECK(  SG_string__sprintf(pCtx, pStrRight_Header,    "%s: %s", SG_string__sz(pStrRightLabel),    SG_pathname__sz(pPathRight))  );

	// I'm going to deviate from our normal error handling here.
	// If something (anything) within the content merge fails,
	// we trap it and just return an error status.  This prevents
	// us from killing the merge; rather, we just log the files
	// as unmerged/unresolved and let them manually deal with
	// them later.

	SG_ERR_CHECK(  _do_merge(pCtx, options,
			  pPathAncestor, pStrAncestor_Header,
			  pPathLeft,     pStrLeft_Header,
			  pPathRight,    pStrRight_Header,
			  pPathResult,   pStrResultLabel,
			  &bHaveConflicts)  );
	if (bHaveConflicts)
	{
		*pStatus = SG_MERGETOOL__RESULT__CONFLICT;
	}
	else
	{
		*pStatus = SG_FILETOOL__RESULT__SUCCESS;
	}

fail:
	SG_STRING_NULLFREE(pCtx, pStrAncestor_Header);
	SG_STRING_NULLFREE(pCtx, pStrLeft_Header);
	SG_STRING_NULLFREE(pCtx, pStrRight_Header);
}

//////////////////////////////////////////////////////////////////
	
static void _sg_mergetool__builtin_merge__strict(SG_context * pCtx,
												 const SG_pathname * pPathAncestor, const SG_string * pStrAncestorLabel,
												 const SG_pathname * pPathLeft,     const SG_string * pStrLeftLabel,
												 const SG_pathname * pPathRight,    const SG_string * pStrRightLabel,
												 const SG_pathname * pPathResult,   const SG_string * pStrResultLabel,
												 SG_int32 * pStatus)
{
	SG_ERR_CHECK_RETURN(  _call_do_merge(pCtx, SG_TEXTFILEDIFF_OPTION__STRICT_EOL,
										 pPathAncestor, pStrAncestorLabel,
										 pPathLeft,     pStrLeftLabel,
										 pPathRight,    pStrRightLabel,
										 pPathResult,   pStrResultLabel,
										 pStatus)  );
}

//////////////////////////////////////////////////////////////////
	
static void _sg_mergetool__builtin_merge__any_eol(SG_context * pCtx,
												  const SG_pathname * pPathAncestor, const SG_string * pStrAncestorLabel,
												  const SG_pathname * pPathLeft,     const SG_string * pStrLeftLabel,
												  const SG_pathname * pPathRight,    const SG_string * pStrRightLabel,
												  const SG_pathname * pPathResult,   const SG_string * pStrResultLabel,
												  SG_int32 * pStatus)
{
	SG_ERR_CHECK_RETURN(  _call_do_merge(pCtx, SG_TEXTFILEDIFF_OPTION__REGULAR,
										 pPathAncestor, pStrAncestorLabel,
										 pPathLeft,     pStrLeftLabel,
										 pPathRight,    pStrRightLabel,
										 pPathResult,   pStrResultLabel,
										 pStatus)  );
}
								  
//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_MERGETOOL__BUILTIN_MERGE_H
