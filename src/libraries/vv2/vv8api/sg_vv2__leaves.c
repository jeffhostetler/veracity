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

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>
#include <sg_wc__public_prototypes.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Return a VARRAY of the HID of all LEAVES in this DAG.
 *
 */
void SG_vv2__leaves(SG_context * pCtx,
					const char * pszRepoName,
					SG_uint64 ui64DagNum,
					SG_varray ** ppvaHidLeaves)
{
	SG_repo * pRepo = NULL;
	SG_rbtree * prbLeaves = NULL;
	SG_bool bRepoNamed = SG_FALSE;

	// pszRepoName is optional
	SG_NULLARGCHECK_RETURN( ppvaHidLeaves );

	// open either the named repo or the repo of the WD.
	SG_ERR_CHECK(  sg_vv2__util__get_repo(pCtx, pszRepoName, &pRepo, &bRepoNamed)  );

	SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo, ui64DagNum, &prbLeaves)  );
	SG_ERR_CHECK(  SG_rbtree__to_varray__keys_only(pCtx, prbLeaves, ppvaHidLeaves)  );

fail:
	SG_REPO_NULLFREE(pCtx, pRepo);
	SG_RBTREE_NULLFREE(pCtx, prbLeaves);
}
