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
 * @file sg_wc__addremove.c
 *
 * @details Deal with an ADDREMOVE.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * ADDREMOVE: ADD all FOUND and REMOVE all LOST items named by
 * (and optionally contained with) <path> to/from version control
 * in a stand-alone transaction.
 *
 * Each <input> path can be an absolute, relative or repo path.
 *
 * DEPTH refers to whether we just add a directory and/or how
 * deeply we try to add things within it.
 * 
 * We OPTIONALLY allow the user to just test whether we
 * could do the adds.
 *
 * We OPTIONALLY return the journal of the operations.
 *
 */
void SG_wc__addremove(SG_context * pCtx,
					  const SG_pathname* pPathWc,
					  const char * pszInput,
					  SG_uint32 depth,
					  SG_bool bNoIgnores,
					  SG_bool bTest,
					  SG_varray ** ppvaJournal)
{
	SG_WC__WRAPPER_TEMPLATE__tj(
		pPathWc,
		bTest,
		ppvaJournal,
		SG_ERR_CHECK(  SG_wc_tx__addremove(pCtx, pWcTx, pszInput, depth, bNoIgnores)  ));
}

void SG_wc__addremove__stringarray(SG_context * pCtx,
								   const SG_pathname* pPathWc,
								   const SG_stringarray * psaInputs,
								   SG_uint32 depth,
								   SG_bool bNoIgnores,
								   SG_bool bTest,
								   SG_varray ** ppvaJournal)
{
	SG_WC__WRAPPER_TEMPLATE__tj(
		pPathWc,
		bTest,
		ppvaJournal,
		SG_ERR_CHECK(  SG_wc_tx__addremove__stringarray(pCtx, pWcTx, psaInputs, depth, bNoIgnores)  ));
}
