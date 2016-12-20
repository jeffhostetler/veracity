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
 * @file sg_vv2__status__work_queue.c
 *
 * @details Private routines within VV2-STATUS to manipulate
 * the work-queue as we compute the comparison of 2 csets.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include <sg_wc__public_typedefs.h>

#include "sg_vv2__public_typedefs.h"
#include "sg_vv2__public_prototypes.h"
#include "sg_vv2__private.h"

//////////////////////////////////////////////////////////////////

/**
 * create (depth,ObjectGID) key and add entry to work-queue.
 */
void sg_vv2__status__add_to_work_queue(SG_context * pCtx, sg_vv2status * pST, sg_vv2status_od * pOD)
{
	char buf[SG_GID_BUFFER_LENGTH + 20];

	SG_ERR_CHECK_RETURN(  SG_sprintf(pCtx,
									 buf,SG_NrElements(buf),
									 "%08d.%s",
									 pOD->minDepthInTree,pOD->bufGidObject)  );

#if TRACE_VV2_STATUS
	SG_console(pCtx,
			   SG_CS_STDERR,
			   "TD_ADQUE [GID %s][minDepth %d] type[%d,%d] depth[%d,%d]\n",
			   pOD->bufGidObject,pOD->minDepthInTree,
			   (int)((pOD->apInst[SG_VV2__OD_NDX_ORIG]) ? (int)pOD->apInst[SG_VV2__OD_NDX_ORIG]->typeInst : -1),
			   (int)((pOD->apInst[SG_VV2__OD_NDX_DEST]) ? (int)pOD->apInst[SG_VV2__OD_NDX_DEST]->typeInst : -1),
			   ((pOD->apInst[SG_VV2__OD_NDX_ORIG]) ? pOD->apInst[SG_VV2__OD_NDX_ORIG]->depthInTree : -1),
			   ((pOD->apInst[SG_VV2__OD_NDX_DEST]) ? pOD->apInst[SG_VV2__OD_NDX_DEST]->depthInTree : -1));
	SG_ERR_DISCARD;
#endif

	SG_ERR_CHECK_RETURN(  SG_rbtree__add__with_assoc(pCtx,pST->prbWorkQueue,buf,pOD)  );
}

/**
 * Is this object a candidate for short-circuit evaluation?  That is, can we
 * avoid diving into this pair of folders and recursively comparing everything
 * within.
 *
 * We must have a pair of unscanned folders with equal content HIDs.  If one has
 * already been scanned, we must scan the other so that the children don't look
 * like peerless objects.  (We either scan neither or both.)
 *
 * If we have pendingtree data, we don't know how much of the tree it populated.
 * If the dsFlags in the pendingtree version indicate a change *on* the folder
 * (such as a rename/move), we can still allow the short-circuit; only if it
 * indicates a change in the stuff *within* the folder do we force it to continue.
 * This is another instance of the accidental peerless problem.
 */
void sg_vv2__status__can_short_circuit_from_work_queue(SG_context * pCtx,
													   const sg_vv2status_od * pOD,
													   SG_bool * pbCanShortCircuit)
{
	SG_bool bEqual;
	const char * pszHid_orig;
	const char * pszHid_dest;

	SG_UNUSED( pCtx );
	if (!pOD->apInst[SG_VV2__OD_NDX_ORIG])
		goto no;
	if (!pOD->apInst[SG_VV2__OD_NDX_DEST])
		goto no;

	if (pOD->apInst[SG_VV2__OD_NDX_ORIG]->typeInst != SG_VV2__OD_TYPE_UNSCANNED_FOLDER)
		goto no;
	if (pOD->apInst[SG_VV2__OD_NDX_DEST]->typeInst != SG_VV2__OD_TYPE_UNSCANNED_FOLDER)
		goto no;

	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pOD->apInst[SG_VV2__OD_NDX_ORIG]->pTNE, &pszHid_orig)  );
	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pOD->apInst[SG_VV2__OD_NDX_DEST]->pTNE, &pszHid_dest)  );
	
	bEqual = (strcmp(pszHid_orig, pszHid_dest) == 0);
	if (!bEqual)
		goto no;

	*pbCanShortCircuit = SG_TRUE;
	return;

no:
	*pbCanShortCircuit = SG_FALSE;

fail:
	return;
}

/**
 *
 */
void sg_vv2__status__remove_from_work_queue(SG_context * pCtx,
											sg_vv2status * pST,
											sg_vv2status_od * pOD,
											SG_uint32 depthInQueue)
{
	char buf[SG_GID_BUFFER_LENGTH + 20];

	SG_ERR_CHECK_RETURN(  SG_sprintf(pCtx,
									 buf,SG_NrElements(buf),
									 "%08d.%s",
									 depthInQueue,pOD->bufGidObject)  );

#if TRACE_VV2_STATUS
	SG_console(pCtx,
			   SG_CS_STDERR,
			   "TD_RMQUE [GID %s][minDepth %d] (short circuit)\n",
			   pOD->bufGidObject,depthInQueue);
	SG_ERR_DISCARD;
#endif

	SG_ERR_CHECK_RETURN(  SG_rbtree__remove(pCtx,pST->prbWorkQueue,buf)  );
}

/**
 *
 */
void sg_vv2__status__assert_in_work_queue(SG_context * pCtx,
										  sg_vv2status * pST,
										  sg_vv2status_od * pOD,
										  SG_uint32 depthInQueue)
{
	char buf[SG_GID_BUFFER_LENGTH + 20];
	SG_bool bFound;
	sg_vv2status_od * pOD_test;

	SG_ERR_CHECK_RETURN(  SG_sprintf(pCtx,
									 buf,SG_NrElements(buf),
									 "%08d.%s",
									 depthInQueue,pOD->bufGidObject)  );

	SG_ERR_CHECK_RETURN(  SG_rbtree__find(pCtx, pST->prbWorkQueue,buf,&bFound,(void **)&pOD_test)  );
	SG_ASSERT_RELEASE_RETURN2(  (bFound),
								(pCtx,
								 "Object [GID %s][depth %d] should have been in work-queue.",
								 pOD->bufGidObject,
								 depthInQueue)  );
}

/**
 * remove the first item in the work-queue and return it for processing.
 *
 * You DO NOT own the returned sg_vv2status_od pointer.
 */
void sg_vv2__status__remove_first_from_work_queue(SG_context * pCtx,
												  sg_vv2status * pST,
												  SG_bool * pbFound,
												  sg_vv2status_od ** ppOD)
{
	const char * szKey;
	sg_vv2status_od * pOD;
	SG_bool bFound;

	SG_ERR_CHECK_RETURN(  SG_rbtree__iterator__first(pCtx,
													 NULL,pST->prbWorkQueue,
													 &bFound,
													 &szKey,(void **)&pOD)  );
	if (bFound)
	{
		SG_ERR_CHECK_RETURN(  SG_rbtree__remove(pCtx, pST->prbWorkQueue,szKey)  );

#if TRACE_VV2_STATUS
		SG_console(pCtx,SG_CS_STDERR,"TD_RMQUE %s (head)\n",szKey);
		SG_ERR_DISCARD;
#endif
	}

	*pbFound = bFound;
	*ppOD = ((bFound) ? pOD : NULL);
}
