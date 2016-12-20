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

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * Create a per-tx temp-dir if we don't already have one.
 *
 */
void sg_wc_tx__create_session_temp_dir(SG_context * pCtx,
									   SG_wc_tx * pWcTx)
{
	char bufTidSession[SG_TID_MAX_BUFFER_LENGTH];
	SG_uint32 nrDigits = 10;

	if (pWcTx->pPathSessionTempDir)
		return;

	// pick a space in /tmp for exporting temporary copies of the files
	// so that internal and/or external tools can compare them.
	//
	// TODO 2012/05/02 Investigate the use of SG_workingdir__get_temp_path() (which
	// TODO            creates things in .sgdrawer rather than /tmp).
	// TODO            (see also sg_mrg__private_file_mrg.h)
	// TODO            See also sg_vv2__diff__create_session_temp_dir().

	SG_ERR_CHECK(  SG_PATHNAME__ALLOC__USER_TEMP_DIRECTORY(pCtx, &pWcTx->pPathSessionTempDir)  );
	SG_ERR_CHECK(  SG_tid__generate2(pCtx, bufTidSession, sizeof(bufTidSession), nrDigits)  );
	SG_ERR_CHECK(  SG_pathname__append__from_sz(pCtx, pWcTx->pPathSessionTempDir, bufTidSession)  );

	SG_ERR_TRY(  SG_fsobj__mkdir_recursive__pathname(pCtx, pWcTx->pPathSessionTempDir)  );
	SG_ERR_CATCH_IGNORE(  SG_ERR_DIR_ALREADY_EXISTS  );
	SG_ERR_CATCH_END;

#if 0 && defined(DEBUG)
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "CreateSessionTempDir: %s\n",
							   SG_pathname__sz(pWcTx->pPathSessionTempDir))  );
#endif

fail:
	return;
}

