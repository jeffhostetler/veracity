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
 * @file sg_wc_tx__get_wc_info.c
 *
 * @details Return meta-data about the current WD.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Return some meta-data about the given WD.
 *
 * info = { "parents"   : [ "<hid0>", "<hid1>", ... ],
 *          "csets"     : { "L0" : "<hid0>", ... "A" : "<hidA>" },
 *          "path"      : "<wd_top>",
 *          "repo"      : "<repo_name>",
 *          "user"      : "<user_name>",       -- omitted if no whoami
 *          "branch"    : "<branch_name>"      -- omitted if detached
 *          "ignores"   : [ "<pattern0>", "<pattern1>", ... ],
 *          };
 *
 */
void SG_wc_tx__get_wc_info(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   SG_vhash ** ppvhInfo)
{
	SG_vhash * pvhInfo = NULL;
	SG_varray * pvaParents = NULL;
	SG_vhash * pvhCSets = NULL;
	SG_varray * pvaIgnores = NULL;
	char * pszUserName = NULL;
	char * pszTiedBranchName = NULL;
	const char * pszRepoDescriptorName = NULL;	// we do not own this

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhInfo)  );

	//////////////////////////////////////////////////////////////////
	// We include a little redundant info here, but it doesn't
	// take up much space/time.
	// 
	// Create an varray of the parent hids (baseline first).
	// And a vhash of all of the csets involved.  The latter
	// will also contain all of the parent info, but mixed
	// in with the ancestor/spca's (when there is a pending
	// merge).

	SG_ERR_CHECK(  SG_wc_tx__get_wc_parents__varray(pCtx, pWcTx, &pvaParents)  );
	SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhInfo, "parents", &pvaParents)  );
	pvaParents = NULL;

	SG_ERR_CHECK(  SG_wc_tx__get_wc_csets__vhash(pCtx, pWcTx, &pvhCSets)  );
	SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhInfo, "csets", &pvhCSets)  );
	pvhCSets = NULL;

	//////////////////////////////////////////////////////////////////

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhInfo,
											 "path",
											 SG_pathname__sz(pWcTx->pDb->pPathWorkingDirectoryTop))  );

	//////////////////////////////////////////////////////////////////

	SG_ERR_CHECK(  SG_repo__get_descriptor_name(pCtx, pWcTx->pDb->pRepo, &pszRepoDescriptorName)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhInfo, "repo", pszRepoDescriptorName)  );

	//////////////////////////////////////////////////////////////////
	// Don't fail if no whoami.

	SG_user__get_username_for_repo(pCtx,
								   pWcTx->pDb->pRepo,
								   &pszUserName);
	if (SG_context__err_equals(pCtx, SG_ERR_NO_CURRENT_WHOAMI))
		SG_context__err_reset(pCtx);
	else
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhInfo,
												 "user",
												 pszUserName)  );

	//////////////////////////////////////////////////////////////////
	// Don't fail if not attached.

	SG_ERR_CHECK(  SG_wc_tx__branch__get(pCtx, pWcTx, &pszTiedBranchName)  );
	if (pszTiedBranchName)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhInfo,
												 "branch",
												 pszTiedBranchName)  );

	//////////////////////////////////////////////////////////////////

	SG_ERR_CHECK(  sg_wc_db__ignores__get_varray(pCtx, pWcTx->pDb, &pvaIgnores)  );
	SG_ERR_CHECK(  SG_vhash__add__varray(pCtx, pvhInfo, "ignores", &pvaIgnores)  );

	//////////////////////////////////////////////////////////////////

	*ppvhInfo = pvhInfo;
	pvhInfo = NULL;

fail:
	SG_VARRAY_NULLFREE(pCtx, pvaParents);
	SG_VHASH_NULLFREE(pCtx, pvhCSets);
	SG_NULLFREE(pCtx, pszUserName);
	SG_NULLFREE(pCtx, pszTiedBranchName);
	SG_VHASH_NULLFREE(pCtx, pvhInfo);
	SG_VARRAY_NULLFREE(pCtx, pvaIgnores);
}

