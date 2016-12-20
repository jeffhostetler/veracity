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
 * @file sg_vv2__status__alloc.c
 *
 * @details Private routines to alloc/free the intermediate
 * data structures that we use when computing a VV2-STATUS.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

void sg_vv2__status__free(SG_context * pCtx, sg_vv2status * pST)
{
	if (!pST)
		return;

	// we do not own pST->prbTreenodesAllocated
	SG_RBTREE_NULLFREE_WITH_ASSOC(pCtx, pST->prbObjectDataAllocated,((SG_free_callback *)sg_vv2__status__od__free)  );
	SG_RBTREE_NULLFREE(pCtx, pST->prbWorkQueue);
	SG_REPO_NULLFREE(pCtx, pST->pRepo);
	SG_NULLFREE(pCtx, pST->pszLabel_0);
	SG_NULLFREE(pCtx, pST->pszLabel_1);
	SG_NULLFREE(pCtx, pST->pszWasLabel_0);
	SG_NULLFREE(pCtx, pST->pszWasLabel_1);

	SG_NULLFREE(pCtx, pST);
}

void sg_vv2__status__alloc(SG_context * pCtx,
						   SG_repo * pRepo,
						   SG_rbtree * prbTreenodeCache,
						   const char chDomain_0,
						   const char chDomain_1,
						   const char * pszLabel_0,
						   const char * pszLabel_1,
						   const char * pszWasLabel_0,
						   const char * pszWasLabel_1,
						   sg_vv2status ** ppST)
{
	sg_vv2status * pST = NULL;

	SG_NULLARGCHECK_RETURN(pRepo);
	SG_NULLARGCHECK_RETURN(prbTreenodeCache);
	SG_ARGCHECK_RETURN( (strchr(("abcdef"        // 'g' is reserved
								 "hijklmnopqrs"  // 't' is reserved
								 "uvwxyz"        // '/' is reserved
								 "0123456789"),
								chDomain_0)), chDomain_0 );
	SG_ARGCHECK_RETURN( (strchr(("abcdef"        // 'g' is reserved
								 "hijklmnopqrs"  // 't' is reserved
								 "uvwxyz"        // '/' is reserved
								 "0123456789"),
								chDomain_1)), chDomain_1 );
	SG_NONEMPTYCHECK_RETURN(pszLabel_0);
	SG_NONEMPTYCHECK_RETURN(pszLabel_1);
	SG_NONEMPTYCHECK_RETURN(pszWasLabel_0);
	SG_NONEMPTYCHECK_RETURN(pszWasLabel_1);
	SG_NULLARGCHECK_RETURN(ppST);

	SG_ERR_CHECK(  SG_alloc(pCtx, 1,sizeof(sg_vv2status),&pST)  );

	pST->prbTreenodesAllocated = prbTreenodeCache;

	SG_ERR_CHECK(  SG_repo__open_repo_instance__copy(pCtx, pRepo, &pST->pRepo)  );

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pST->prbObjectDataAllocated)  );
	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pST->prbWorkQueue)  );

	pST->chDomain_0 = chDomain_0;
	pST->chDomain_1 = chDomain_1;
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszLabel_0, &pST->pszLabel_0)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszLabel_1, &pST->pszLabel_1)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszWasLabel_0, &pST->pszWasLabel_0)  );
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszWasLabel_1, &pST->pszWasLabel_1)  );
	
	*ppST = pST;
	return;

fail:
	SG_VV2__STATUS__NULLFREE(pCtx, pST);
}

