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
 * @file sg_wc_tx__journal.c
 *
 * @details 
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void SG_wc_tx__journal(SG_context * pCtx,
					   SG_wc_tx * pWcTx,
					   const SG_varray ** ppvaJournal)
{
	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( ppvaJournal );

#if TRACE_WC_TX_JRNL
	SG_ERR_IGNORE(  SG_varray_debug__dump_varray_of_vhashes_to_console(pCtx, pWcTx->pvaJournal, "SG_wc_tx__journal")  );
#endif

	*ppvaJournal = pWcTx->pvaJournal;
}

void SG_wc_tx__journal_length(SG_context * pCtx,
							  SG_wc_tx * pWcTx,
							  SG_uint32 * pCount)
{
	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pCount );

	SG_ERR_CHECK_RETURN(  SG_varray__count(pCtx,
										   pWcTx->pvaJournal,
										   pCount)  );
}

//////////////////////////////////////////////////////////////////

void SG_wc_tx__journal__alloc_copy(SG_context * pCtx,
								   SG_wc_tx * pWcTx,
								   SG_varray ** ppvaJournal)
{
	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( ppvaJournal );

	SG_ERR_CHECK_RETURN(  SG_varray__alloc__copy(pCtx, ppvaJournal, pWcTx->pvaJournal)  );
}
