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
 * @file sg_mergetool__builtin_baseline_other.h
 *
 * @details This file contains built-in mergetools to copy one of
 * the branches to the final result.
 *
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_MERGETOOL__BUILTIN_BASELINE_OTHER_H
#define H_SG_MERGETOOL__BUILTIN_BASELINE_OTHER_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

/**
 * Copy the BASELINE version to the RESULT file.
 */
static void _sg_mergetool__builtin_baseline(SG_context * pCtx,
											const SG_pathname * pPathAncestor, const SG_string * pStrAncestorLabel,
											const SG_pathname * pPathLeft,     const SG_string * pStrLeftLabel,
											const SG_pathname * pPathRight,    const SG_string * pStrRightLabel,
											const SG_pathname * pPathResult,   const SG_string * pStrResultLabel,
											SG_int32 * pStatus)
{
	SG_UNUSED( pPathAncestor );
	SG_UNUSED( pPathRight );
	SG_UNUSED( pStrAncestorLabel );
	SG_UNUSED( pStrLeftLabel );
	SG_UNUSED( pStrRightLabel );
	SG_UNUSED( pStrResultLabel );

	SG_ERR_CHECK_RETURN(  SG_fsobj__copy_file(pCtx, pPathLeft, pPathResult, 0644)  );

	*pStatus = SG_FILETOOL__RESULT__SUCCESS;
}

/**
 * Copy the OTHER version to the RESULT file.
 */
static void _sg_mergetool__builtin_other(SG_context * pCtx,
										 const SG_pathname * pPathAncestor, const SG_string * pStrAncestorLabel,
										 const SG_pathname * pPathLeft,     const SG_string * pStrLeftLabel,
										 const SG_pathname * pPathRight,    const SG_string * pStrRightLabel,
										 const SG_pathname * pPathResult,   const SG_string * pStrResultLabel,
										 SG_int32 * pStatus)
{
	SG_UNUSED( pPathAncestor );
	SG_UNUSED( pPathLeft );
	SG_UNUSED( pStrAncestorLabel );
	SG_UNUSED( pStrLeftLabel );
	SG_UNUSED( pStrRightLabel );
	SG_UNUSED( pStrResultLabel );

	SG_ERR_CHECK_RETURN(  SG_fsobj__copy_file(pCtx, pPathRight, pPathResult, 0644)  );

	*pStatus = SG_FILETOOL__RESULT__SUCCESS;
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_MERGETOOL__BUILTIN_BASELINE_OTHER_H
