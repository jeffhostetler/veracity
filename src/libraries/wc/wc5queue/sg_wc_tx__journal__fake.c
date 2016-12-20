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
 * @file sg_wc_tx__journal__fake.c
 *
 * @details Custom JOURNAL routines for __fake_merge.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

/**
 * QUEUE a SQL statment to insert a row into the named "pc" table.
 * Note that this DOES NOT alter the associated LVI or even refer
 * to it.  This is important since the main-line MERGE has probably
 * already done that.  And we are being called to build an alternate
 * "pc" table after the fact.
 *
 */
void sg_wc_tx__journal__append__insert_fake_pc_stmt(SG_context * pCtx,
													SG_wc_tx * pWcTx,
													const sg_wc_db__cset_row * pCSetRow,
													const sg_wc_db__pc_row * pPcRow)
{
	sqlite3_stmt * pStmt = NULL;

	SG_ERR_CHECK(  sg_wc_db__pc__prepare_insert_stmt(pCtx, pWcTx->pDb, pCSetRow, pPcRow, &pStmt)  );
	SG_ERR_CHECK(  SG_vector__append(pCtx, pWcTx->pvecJournalStmts, (void *)pStmt, NULL)  );
	return;

fail:
	SG_ERR_IGNORE(  sg_sqlite__finalize(pCtx, pStmt)  );
}


