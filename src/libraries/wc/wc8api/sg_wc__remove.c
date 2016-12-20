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
 * @file sg_wc__remove.c
 *
 * @details Deal with an REMOVE.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * REMOVE <path> from version control in a stand-alone transaction.
 *
 * Each <input> path can be an absolute, relative or repo path.
 *
 *
 * Normally, we refuse to delete something that is dirty.
 * If bForce is set, we will.
 *
 * 
 * If a file was dirty and a delete was forced, normally we back
 * it up to a temp file rather than actually deleting it from the
 * disk.  If bNoBackup is set, we won't do that.  (Either way we
 * do remove the item from version control.)
 *
 * 
 * Normally, we both delete the file from disk and remove it from
 * version control in tandem.  When bKeep, we will try to keep
 * the file in the WD (so that it would appear as FOUND later).
 *
 * 
 * We OPTIONALLY allow the user to just test whether we
 * could do the removes.
 *
 * 
 * We OPTIONALLY return the journal of the operations.
 *
 */
void SG_wc__remove(SG_context * pCtx,
				   const SG_pathname* pPathWc,
				   const char * pszInput,
				   SG_bool bNonRecursive,
				   SG_bool bForce,
				   SG_bool bNoBackups,
				   SG_bool bKeep,
				   SG_bool bTest,
				   SG_varray ** ppvaJournal)
{
	SG_WC__WRAPPER_TEMPLATE__tj(
		pPathWc,
		bTest,
		ppvaJournal,
		SG_ERR_CHECK(  SG_wc_tx__remove(pCtx, pWcTx, pszInput,
										bNonRecursive, bForce, bNoBackups, bKeep)  ));
}

void SG_wc__remove__stringarray(SG_context * pCtx,
								const SG_pathname* pPathWc,
								const SG_stringarray * psaInputs,
								SG_bool bNonRecursive,
								SG_bool bForce,
								SG_bool bNoBackups,
								SG_bool bKeep,
								SG_bool bTest,
								SG_varray ** ppvaJournal)
{
	SG_WC__WRAPPER_TEMPLATE__tj(
		pPathWc,
		bTest,
		ppvaJournal,
		SG_ERR_CHECK(  SG_wc_tx__remove__stringarray(pCtx, pWcTx, psaInputs,
													 bNonRecursive, bForce, bNoBackups, bKeep)  ));
}
