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

void SG_wc_tx__branch__attach(SG_context * pCtx,
							  SG_wc_tx * pWcTx,
							  const char * pszBranchName)
{
	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NONEMPTYCHECK_RETURN( pszBranchName );

	// we require that the branch already exist, but we
	// do not force validation on the name (so that they
	// can attach to branches created pre-2.0 (that might
	// have punctuation characters)).

	SG_ERR_CHECK_RETURN(  sg_wc_db__branch__attach(pCtx, pWcTx->pDb, pszBranchName,
												   SG_VC_BRANCHES__CHECK_ATTACH_NAME__MUST_EXIST,
												   SG_FALSE)  );
}

void SG_wc_tx__branch__attach_new(SG_context * pCtx,
								  SG_wc_tx * pWcTx,
								  const char * pszBranchName)
{
	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NONEMPTYCHECK_RETURN( pszBranchName );

	// we require that the branch NOT already exist and
	// we force validation so that new branches created
	// follow the 2.0 restrictions.

	SG_ERR_CHECK_RETURN(  sg_wc_db__branch__attach(pCtx, pWcTx->pDb, pszBranchName,
												   SG_VC_BRANCHES__CHECK_ATTACH_NAME__MUST_NOT_EXIST,
												   SG_TRUE)  );
}

void SG_wc_tx__branch__detach(SG_context * pCtx,
							  SG_wc_tx * pWcTx)
{
	SG_NULLARGCHECK_RETURN( pWcTx );

	SG_ERR_CHECK_RETURN(  sg_wc_db__branch__detach(pCtx, pWcTx->pDb)  );
}

void SG_wc_tx__branch__get(SG_context * pCtx,
						   SG_wc_tx * pWcTx,
						   char ** ppszBranchName)
{
	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( ppszBranchName );

	SG_ERR_CHECK_RETURN(  sg_wc_db__branch__get_branch(pCtx, pWcTx->pDb, ppszBranchName)  );
}

