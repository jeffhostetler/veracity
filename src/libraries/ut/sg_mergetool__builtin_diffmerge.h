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
 * @file sg_mergetool__builtin_diffmerge.h
 *
 * @details This is a built-in mergetool that knows how to exec
 * SourceGear DiffMerge to attempt the merge of a set of files.
 *
 * The user can configure many different custom mergetools (and
 * set both the program path and the command line args) using
 * config/localsettings, so this built-in routine is not really
 * necessary.  However, I'm keeping this here as a default fall-back.
 */

//////////////////////////////////////////////////////////////////

#ifndef H_SG_MERGETOOL__BUILTIN_DIFFMERGE_H
#define H_SG_MERGETOOL__BUILTIN_DIFFMERGE_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

static void _sg_mergetool__builtin_diffmerge(SG_context * pCtx,
											 const SG_pathname * pPathAncestor, const SG_string * pStrAncestorLabel,
											 const SG_pathname * pPathLeft,     const SG_string * pStrLeftLabel,
											 const SG_pathname * pPathRight,    const SG_string * pStrRightLabel,
											 const SG_pathname * pPathResult,   const SG_string * pStrResultLabel,
											 SG_int32 * pStatus)
{
	SG_pathname * pPathCmd = NULL;
	SG_exec_argvec * pArgVec = NULL;
	SG_exit_status childExitStatus;

	SG_ERR_CHECK(  SG_filetool__get_builtin_diffmerge_exe(pCtx, &pPathCmd)  );

	SG_ERR_CHECK(  SG_exec_argvec__alloc(pCtx,&pArgVec)  );

	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, "-r")  );
	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, SG_pathname__sz(pPathResult  ))  );

	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, "-t1")  );
	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, SG_string__sz(pStrLeftLabel))  );

	// In DiffMerge the center panel is initially the ancestor and becomes
	// the result, so it is a little arbitrary which we set it to.  I think
	// using the result is less confusing.
	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, "-t2")  );
	SG_UNUSED( pStrAncestorLabel );
	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, SG_string__sz(pStrResultLabel))  );

	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, "-t3")  );
	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, SG_string__sz(pStrRightLabel))  );

	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, SG_pathname__sz(pPathLeft    ))  );
	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, SG_pathname__sz(pPathAncestor))  );
	SG_ERR_CHECK(  SG_exec_argvec__append__sz(pCtx, pArgVec, SG_pathname__sz(pPathRight   ))  );

	// exec diffmerge and wait for exit status.

	SG_ERR_CHECK(  SG_exec__exec_sync__files(pCtx,
							  SG_pathname__sz(pPathCmd),
							  pArgVec,
							  NULL, NULL, NULL,
							  &childExitStatus)  );
	// DiffMerge returns:
	// 0 --> auto-merge successful
	// 1 --> auto-merge found conflicts
	// 2 --> hard errors of some kind

	switch (childExitStatus)
	{
	case 0:
		*pStatus = SG_FILETOOL__RESULT__SUCCESS;
		break;

	case 1:
		*pStatus = SG_MERGETOOL__RESULT__CONFLICT;
		break;

//	case 2:
	default:
		*pStatus = SG_FILETOOL__RESULT__FAILURE;
		break;
	}

fail:
	SG_EXEC_ARGVEC_NULLFREE(pCtx, pArgVec);
	SG_PATHNAME_NULLFREE(pCtx, pPathCmd);
}

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_MERGETOOL__BUILTIN_DIFFMERGE_H
