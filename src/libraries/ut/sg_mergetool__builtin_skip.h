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
 * @file sg_mergetool__builtin_skip.h
 *
 * @details This is a built-in mergetool that does nothing.
 * It always returns "not attempted".  You can bind binary
 * (image) file types to this to prevent us from doing stupid
 * things like merging 2 JPGs.
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_MERGETOOL__BUILTIN_SKIP_H
#define H_SG_MERGETOOL__BUILTIN_SKIP_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

static void _sg_mergetool__builtin_skip(SG_context * pCtx,
										const SG_pathname * pPathAncestor, const SG_string * pStrAncestorLabel,
										const SG_pathname * pPathLeft,     const SG_string * pStrLeftLabel,
										const SG_pathname * pPathRight,    const SG_string * pStrRightLabel,
										const SG_pathname * pPathResult,   const SG_string * pStrResultLabel,
										SG_int32 * pStatus)
{
	SG_UNUSED( pCtx );
	SG_UNUSED( pPathAncestor );
	SG_UNUSED( pPathLeft );
	SG_UNUSED( pPathRight );
	SG_UNUSED( pPathResult );
	SG_UNUSED( pStrAncestorLabel );
	SG_UNUSED( pStrLeftLabel );
	SG_UNUSED( pStrRightLabel );
	SG_UNUSED( pStrResultLabel );

	*pStatus = SG_MERGETOOL__RESULT__CANCEL;
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_MERGETOOL__BUILTIN_SKIP_H
