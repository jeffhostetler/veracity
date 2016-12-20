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

void sg_vv2__util__translate_input_to_gid(SG_context * pCtx,
							SG_repo * pRepo,
							const char * pszHidCSet,
							const char * pszInput,
							SG_bool bRequireFile,
							char ** ppszGid)
{
	char * pszGid = NULL;
	SG_treenode_entry * ptne = NULL;
	SG_treenode_entry_type tneType;
	SG_string * pStringRepoPathInCSet = NULL;
	SG_treenode * pTreenodeRoot = NULL;
	char * pszHidForRoot = NULL;

	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, pszHidCSet, &pszHidForRoot)  );
	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pszHidForRoot, &pTreenodeRoot)  );
	if ((pszInput[0] == '@') && (pszInput[1] == '/'))
	{
		SG_ERR_CHECK( SG_treenode__find_treenodeentry_by_path(pCtx, pRepo, pTreenodeRoot, pszInput,
															  &pszGid, &ptne) );
	}
	else if ((pszInput[0] == '@') && (pszInput[1] == 'g'))
	{
		SG_bool bValid;
		SG_ERR_CHECK(  SG_gid__verify_format(pCtx, &pszInput[1], &bValid)  );
		if (!bValid)
			SG_ERR_THROW2(  SG_ERR_INVALID_REPO_PATH,
							(pCtx, "The input '%s' is not a valid gid-domain input.", pszInput)  );
		// copy gid without the '@' prefix into our buffer
		SG_ERR_CHECK(  SG_STRDUP(pCtx, &pszInput[1], &pszGid)  );
		if (bRequireFile)
			SG_ERR_CHECK(  SG_repo__treendx__get_path_in_dagnode(pCtx, pRepo, SG_DAGNUM__VERSION_CONTROL,
															 pszGid, pszHidCSet,
															 &pStringRepoPathInCSet, &ptne)  );
	}
	else
	{
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Argument '%s' must be full repo-path.", pszInput)  );
	}
	
	if (!pszGid || !ptne)
		SG_ERR_THROW2(  SG_ERR_NOT_FOUND,
						(pCtx, "'%s' is not present in changeset '%s'.", pszInput, pszHidCSet)  );

	if (bRequireFile)
	{
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, ptne, &tneType)  );
		if (tneType != SG_TREENODEENTRY_TYPE_REGULAR_FILE)
			SG_ERR_THROW2(  SG_ERR_NOTAFILE,
						(pCtx, "'%s' is not a file.", pszInput)  );
	}

	SG_RETURN_AND_NULL(pszGid, ppszGid);

fail:
	SG_NULLFREE(pCtx, pszGid);
	SG_NULLFREE(pCtx, pszHidForRoot);
	SG_TREENODE_ENTRY_NULLFREE(pCtx, ptne);
	SG_TREENODE_NULLFREE(pCtx, pTreenodeRoot);
	SG_STRING_NULLFREE(pCtx, pStringRepoPathInCSet);
}

