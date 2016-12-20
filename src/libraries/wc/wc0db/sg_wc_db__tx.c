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
 * @file sg_wc_db__tx.c
 *
 * @details Routines associated with managing SQL Transactions.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void sg_wc_db__tx__assert(SG_context * pCtx, sg_wc_db * pDb)
{
	if (pDb->txCount == 0)
		SG_ERR_THROW_RETURN(  SG_ERR_NOT_IN_DB_TX  );
}

void sg_wc_db__tx__in_tx(SG_context * pCtx, sg_wc_db * pDb, SG_bool * pbInTx)
{
	SG_UNUSED( pCtx );

	*pbInTx = (pDb->txCount > 0);
}

void sg_wc_db__tx__assert_not(SG_context * pCtx, sg_wc_db * pDb)
{
	if (pDb->txCount != 0)
		SG_ERR_THROW_RETURN(  SG_ERR_CANNOT_NEST_DB_TX  );
}

//////////////////////////////////////////////////////////////////

void sg_wc_db__tx__begin(SG_context * pCtx, sg_wc_db * pDb, SG_bool bExclusive)
{
#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_db__tx__begin: [count %d ==> %d] [bExclusive %d]\n",
							   pDb->txCount, (pDb->txCount+1), bExclusive)  );
#endif

	SG_ERR_CHECK(  sg_wc_db__tx__assert_not(pCtx, pDb)  );

	if (bExclusive)
		SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pDb->psql, "BEGIN EXCLUSIVE TRANSACTION")  );
	else
		SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pDb->psql, "BEGIN TRANSACTION")  );
	pDb->txCount++;

fail:
	SG_ERR_REPLACE(SG_ERR_SQLITE(SQLITE_BUSY), SG_ERR_WC_DB_BUSY);
	return;
}

void sg_wc_db__tx__commit(SG_context * pCtx, sg_wc_db * pDb)
{
#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_db__tx__commit: [count %d ==> %d]\n",
							   pDb->txCount, (pDb->txCount-1))  );
#endif

	SG_ERR_CHECK(  sg_wc_db__tx__assert(pCtx, pDb)  );

	// If we had to create TEMPORARY GIDs (for FOUND/IGNORED items),
	// discard those GIDs/aliases.
	SG_ERR_CHECK(  sg_wc_db__gid__delete_all_tmp(pCtx, pDb)  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pDb->psql, "COMMIT TRANSACTION")  );
	pDb->txCount--;

	// WARNING: 2011/10/13 Currently the timestamp cache is still
	// WARNING:            an rbtreedb beside the WC DB.  So the
	// WARNING:            above COMMIT to the WC DB *does not*
	// WARNING:            save the TSC -- we have to do that in
	// WARNING:            a separate step.
	// WARNING:
	// WARNING:            *BUT* if we put the TSC inside the same
	// WARNING:            WC DB, then we may need 2 parallel TXs
	// WARNING:            so that the TSC changes always get written
	// WARNING:            regardless of whether the WC data gets
	// WARNING:            committed or rolledback.
	
	SG_ERR_CHECK(  sg_wc_db__timestamp_cache__save(pCtx, pDb)  );

fail:
	return;
}

void sg_wc_db__tx__rollback(SG_context * pCtx, sg_wc_db * pDb)
{
#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_db__tx__rollback: [count %d ==> %d]\n",
							   pDb->txCount, (pDb->txCount-1))  );
#endif

	SG_ERR_CHECK(  sg_wc_db__tx__assert(pCtx, pDb)  );

	SG_ERR_CHECK(  sg_sqlite__exec(pCtx, pDb->psql, "ROLLBACK TRANSACTION")  );
	pDb->txCount--;

	// WARNING: 2011/10/13 Currently the timestamp cache is still
	// WARNING:            an rbtreedb beside the WC DB.  So the
	// WARNING:            above ROLLBACK to the WC DB *does not*
	// WARNING:            save the TSC -- we have to do that in
	// WARNING:            a separate step.
	// WARNING:
	// WARNING:            *BUT* if we put the TSC inside the same
	// WARNING:            WC DB, then we may need 2 parallel TXs
	// WARNING:            so that the TSC changes always get written
	// WARNING:            regardless of whether the WC data gets
	// WARNING:            committed or rolledback.
	
	SG_ERR_CHECK(  sg_wc_db__timestamp_cache__save(pCtx, pDb)  );

fail:
	return;
}
	
