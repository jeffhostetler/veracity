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
 * @file u0021_workingdir.c
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "unittests.h"
#include "unittests_pendingtree.h"

void u0021_test_version(SG_context * pCtx)
{
	/* This test pokes around in workingdir internals in ways normal callers shouldn't. */

	char bufTopDir[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathTopWorkingDir = NULL;
	char buf_repo_name[SG_TID_MAX_BUFFER_LENGTH];

	SG_pathname* pPathDrawer = NULL;
	SG_pathname* pPathDrawerVersion = NULL;
	SG_pathname* pPathTmp = NULL;
	SG_file* pFile = NULL;
	SG_vhash* pvh = NULL;
	SG_uint32 current_version;

	SG_byte bufBytes[100];

	memset(bufBytes, 0, sizeof(bufBytes));
	
	// create repo and WD such that the drawer will have the current version

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufTopDir, sizeof(bufTopDir), 32)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx,&pPathTopWorkingDir,bufTopDir)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx,pPathTopWorkingDir)  );
	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, buf_repo_name, sizeof(buf_repo_name), 32)  );
	VERIFY_ERR_CHECK(  _ut_pt__new_repo2(pCtx, buf_repo_name, pPathTopWorkingDir, NULL)  );

	VERIFY_ERR_CHECK(  SG_workingdir__get_drawer_version(pCtx, pPathTopWorkingDir, &current_version)  );

	// alter the .sgdrawer/version file (effectively causing us to think
	// that the drawer has a different/unsupported version).

	VERIFY_ERR_CHECK(  SG_workingdir__get_drawer_path(pCtx, pPathTopWorkingDir, &pPathDrawer)  );

	VERIFY_ERR_CHECK(  SG_pathname__alloc__pathname_sz(pCtx, &pPathDrawerVersion, pPathDrawer, "version")  );

	/* Deliberately using SG_FILE_OPEN_EXISTING: this should fail if the version file's not already there. */
	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathDrawerVersion, SG_FILE_OPEN_EXISTING|SG_FILE_WRONLY|SG_FILE_TRUNC, 0644, &pFile)  );
	VERIFY_ERR_CHECK(  SG_file__write(pCtx, pFile, current_version+1, bufBytes, NULL)  ); 
	VERIFY_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

	SG_workingdir__get_drawer_path(pCtx, pPathTopWorkingDir, &pPathTmp);
	VERIFY_COND("", SG_context__err_equals(pCtx, SG_ERR_UNSUPPORTED_DRAWER_VERSION));
	SG_ERR_DISCARD;

	/* Common cleanup */
fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathTopWorkingDir);
	SG_PATHNAME_NULLFREE(pCtx, pPathDrawer);
	SG_PATHNAME_NULLFREE(pCtx, pPathDrawerVersion);
	SG_PATHNAME_NULLFREE(pCtx, pPathTmp);
	SG_FILE_NULLCLOSE(pCtx, pFile);
	SG_VHASH_NULLFREE(pCtx, pvh);
}

TEST_MAIN(u0021_workingdir)
{
	TEMPLATE_MAIN_START;

	BEGIN_TEST(  u0021_test_version(pCtx)  );

	TEMPLATE_MAIN_END;
}
