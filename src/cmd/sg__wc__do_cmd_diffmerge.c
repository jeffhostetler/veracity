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
 * @file sg__wc__do_cmd_diffmerge.c
 *
 * @details Handle the 'vv diffmerge' command.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include <sg_vv2__public_typedefs.h>
#include <sg_vv2__public_prototypes.h>

#include "sg_typedefs.h"
#include "sg_prototypes.h"
#include "sg_cmd_util_prototypes.h"

//////////////////////////////////////////////////////////////////

void wc__do_cmd_diffmerge(SG_context * pCtx,
						  SG_option_state * pOptSt)
{
	SG_string * pStringRepoName = NULL;
	SG_pathname * pPathCwd = NULL;
	SG_rev_spec * pRevSpec_From = NULL;
	SG_rev_spec * pRevSpec_To = NULL;
	char * pszRev1 = NULL;
	char * pszRev2 = NULL;
	char * pszHidBaseline = NULL;
	const char * pszHidFrom;
	SG_pathname * pPathTemp1 = NULL;
	SG_pathname * pPathTemp2 = NULL;
	SG_string * pStringLabel1 = NULL;
	SG_string * pStringLabel2 = NULL;
	SG_bool bDeletePath2 = SG_FALSE;
	SG_bool bRightSideIsWritable = SG_FALSE;
	SG_int32 result_code = SG_FILETOOL__RESULT__SUCCESS;

	// if --repo <repo_name> : return (<repo_name>, NULL).
	// otherwise if in a WD  : return (<repo_name>, <cwd>).
	// otherwise             : throw.
	SG_ERR_CHECK(  SG_cmd_util__get_descriptor_name_from_options_or_cwd(pCtx,
																		pOptSt,
																		&pStringRepoName,
																		&pPathCwd)  );
	SG_ERR_CHECK(  _get_zero_one_or_two_revision_specifiers(pCtx,
															SG_string__sz(pStringRepoName),
															pOptSt->pRevSpec,
															&pszRev1,
															&pszRev2)  );
	if (pOptSt->psz_repo)
	{
		SG_ASSERT( (pPathCwd == NULL) );
		if (!pszRev1 || !pszRev2)
			SG_ERR_THROW2(  SG_ERR_USAGE, (pCtx, "Two rev-specs are required with --repo")  );
	}

	// build an independent rev-spec for the FROM side.
	// since the rev-specs were optional on the command line,
	// we fill in the baseline as necessary.

	SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec_From)  );
	if (pszRev1 == NULL)
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStringLabel1, "Baseline")  );

		SG_ERR_CHECK(  SG_wc__get_wc_baseline(pCtx, NULL, &pszHidBaseline, NULL)  );
		SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec_From, pszHidBaseline)  );
		pszHidFrom = pszHidBaseline;
	}
	else
	{
		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStringLabel1, pszRev1)  );
		SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec_From, pszRev1)  );
		pszHidFrom = pszRev1;
	}

	// EXPORT the FROM version into a TEMP directory.

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__USER_TEMP_DIRECTORY(pCtx, &pPathTemp1)  );
	SG_ERR_CHECK(  SG_pathname__append__force_nonexistant(pCtx, pPathTemp1, pszHidFrom)  );
	SG_ERR_CHECK(  SG_vv2__export(pCtx,
								  SG_string__sz(pStringRepoName),
								  SG_pathname__sz(pPathTemp1),
								  pRevSpec_From,
								  NULL)  );		// TODO 2011/11/01 pvaSparse

	// build an independent rev-spec for the TO side.
	// optionally use the CWD, if available.

	if (pszRev2 == NULL)
	{
		// They want to compare something to the current (possibly dirty)
		// working directory.  Find the actual working-directory-top.

		SG_ERR_CHECK(  SG_workingdir__find_mapping(pCtx, pPathCwd, &pPathTemp2, NULL, NULL)  );
		bDeletePath2 = SG_FALSE;
		bRightSideIsWritable = SG_TRUE;		// right-side refers to the WD so they can edit the contents.

		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStringLabel2, "Working Copy")  );
	}
	else
	{
		// They want the TO side to be a specific changeset.
		// EXPORT the TO version into a TEMP directory.

		SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, &pStringLabel2, pszRev2)  );

		SG_ERR_CHECK(  SG_REV_SPEC__ALLOC(pCtx, &pRevSpec_To)  );
		SG_ERR_CHECK(  SG_rev_spec__add_rev(pCtx, pRevSpec_To, pszRev2)  );

		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__USER_TEMP_DIRECTORY(pCtx, &pPathTemp2)  );
		SG_ERR_CHECK(  SG_pathname__append__force_nonexistant(pCtx, pPathTemp2, pszRev2)  );
		SG_ERR_CHECK(  SG_vv2__export(pCtx,
									  SG_string__sz(pStringRepoName),
									  SG_pathname__sz(pPathTemp2),
									  pRevSpec_To,
									  NULL)  );	// TODO 2011/11/01 pvaSparse
		bDeletePath2 = SG_TRUE;
		bRightSideIsWritable = SG_FALSE;
	}
	
	// Launch DiffMerge

	SG_ERR_CHECK(  SG_difftool__built_in_tool__exec_diffmerge(pCtx,
															  pPathTemp1,
															  pPathTemp2,
															  pStringLabel1,
															  pStringLabel2,
															  bRightSideIsWritable,
															  NULL, &result_code)  );

	if (bDeletePath2)
		SG_ERR_CHECK( SG_fsobj__rmdir_recursive__pathname(pCtx, pPathTemp2)  );
	SG_ERR_CHECK(  SG_fsobj__rmdir_recursive__pathname(pCtx, pPathTemp1)  );
	
fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoName);
	SG_PATHNAME_NULLFREE(pCtx, pPathCwd);
	SG_NULLFREE(pCtx, pszRev1);
	SG_NULLFREE(pCtx, pszRev2);
	SG_NULLFREE(pCtx, pszHidBaseline);
	SG_PATHNAME_NULLFREE(pCtx, pPathTemp1);
	SG_PATHNAME_NULLFREE(pCtx, pPathTemp2);
	SG_STRING_NULLFREE(pCtx, pStringLabel1);
	SG_STRING_NULLFREE(pCtx, pStringLabel2);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec_From);
	SG_REV_SPEC_NULLFREE(pCtx, pRevSpec_To);
}
