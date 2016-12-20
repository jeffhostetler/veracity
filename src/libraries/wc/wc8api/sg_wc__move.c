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
 * @file sg_wc__move.c
 *
 * @details Deal with a MOVE.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * MOVE <path> to another <directory> as a stand-alone
 * transaction.  This is a convenience wrapper for callers
 * that don't need to do multiple operations and/or don't
 * want to bother with setting up their own transaction.
 * 
 * This API artificially restricts the destination to be
 * the TARGET DIRECTORY and so it disallows renames at
 * the same time.  This corresponds to the 'vv move' usage.
 *
 * We OPTIONALLY allow/disallow after-the-fact renames.
 *
 * We OPTIONALLY allow the user to just test whether we
 * could do the moves.
 *
 * We OPTIONALLY return the journal of the operations.
 *
 */
void SG_wc__move(SG_context * pCtx,
				 const SG_pathname* pPathWc,
				 const char * pszInput_Src,
				 const char * pszInput_DestDir,
				 SG_bool bNoAllowAfterTheFact,
				 SG_bool bTest,
				 SG_varray ** ppvaJournal)
{
	SG_WC__WRAPPER_TEMPLATE__tj(
		pPathWc,
		bTest,
		ppvaJournal,
		SG_ERR_CHECK(  SG_wc_tx__move(pCtx, pWcTx, pszInput_Src, pszInput_DestDir, bNoAllowAfterTheFact)  ));
}

//////////////////////////////////////////////////////////////////

/**
 * MOVE n <paths> into a destination <directory> as a
 * stand-alone transaction.  (All n moves are done in
 * a single transaction.)
 *
 */
void SG_wc__move__stringarray(SG_context * pCtx,
							  const SG_pathname* pPathWc,
							  const SG_stringarray * psaInputs,
							  const char * pszInput_DestDir,
							  SG_bool bNoAllowAfterTheFact,
							  SG_bool bTest,
							  SG_varray ** ppvaJournal)
{
	SG_WC__WRAPPER_TEMPLATE__tj(
		pPathWc,
		bTest,
		ppvaJournal,
		SG_ERR_CHECK(  SG_wc_tx__move__stringarray(pCtx, pWcTx, psaInputs, pszInput_DestDir, bNoAllowAfterTheFact)  ));
}
