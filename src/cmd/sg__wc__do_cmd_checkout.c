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
 * @file sg__wc__do_cmd_checkout.c
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_typedefs.h"
#include "sg_prototypes.h"
#include "sg_cmd_util_prototypes.h"

//////////////////////////////////////////////////////////////////

void wc__do_cmd_checkout(SG_context * pCtx,
						 const char * pszRepoName,
						 const char * pszPath,
						 /*const*/ SG_rev_spec * pRevSpec,
						 const char * pszAttach,
						 const char * pszAttachNew,
						 const SG_varray * pvaSparse)
{
	SG_repo * pRepo = NULL;
	SG_vhash * pvh_pile = NULL;

	SG_wc__checkout(pCtx, pszRepoName, pszPath, pRevSpec, pszAttach, pszAttachNew, pvaSparse);

	// TODO 2011/10/17 The following *ALTERS* the given pRevSpec
	// TODO            so that it can print the heads of the branch.
	// TODO            This has a bad feel to it.

	if (SG_CONTEXT__HAS_ERR(pCtx))
	{
		if (SG_context__err_equals(pCtx, SG_ERR_BRANCH_NEEDS_MERGE))
		{
			SG_uint32 branchesSpecified;

			SG_context__err_reset(pCtx);

			SG_ERR_CHECK(  SG_rev_spec__count_branches(pCtx, pRevSpec, &branchesSpecified)  );

			if (!branchesSpecified)
			{
				SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "The master branch has multiple heads.\n\n")  );
				SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Check out using \"--attach=master\" to attach your working folder to it.\n")  );
				SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Use the '--rev' or '--tag' option to pick a specific revision.\n")  );

				SG_ERR_CHECK(  SG_rev_spec__add_branch(pCtx, pRevSpec, "master")  );
			}
			else
			{
				SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "The branch has multiple heads.\n")  );
				SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Check out using the \"--attach\" option rather than \"--branch\" to attach your working folder to it.\n")  );
				SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "You must also use the \"--rev\" or \"--tag\" option to pick a specific revision.\n")  );
			}

			// TODO 2012/02/10 I hate the way we print the output of another command as
			// TODO            part of the error message of this command because the
			// TODO            actual error message above will probably be scrolled off
			// TODO            the screen.
			// TODO
			// TODO            I think it would be better if we printed the above error
			// TODO            message and then printed the text of the command that they
			// TODO            could type to get the following list of heads.
			SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "\nCurrent heads are:\n")  );

			SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRepoName, &pRepo)  );
			SG_ERR_CHECK(  SG_vc_branches__cleanup(pCtx, pRepo, &pvh_pile)  );
			SG_ERR_CHECK(  SG_cmd_util__dump_log__revspec(pCtx, SG_CS_STDERR, pRepo, pRevSpec, pvh_pile, SG_FALSE, SG_FALSE)  );

			SG_ERR_THROW(  SG_ERR_TOO_MANY_HEADS  );
		}

		SG_ERR_RETHROW;
	}

fail:
	SG_VHASH_NULLFREE(pCtx, pvh_pile);
	SG_REPO_NULLFREE(pCtx, pRepo);
}
