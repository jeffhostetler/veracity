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
 * @file sg_wc_db__tsc.c
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

/**
 * Open the timestamp cache (TSC) and associate it with the
 * given DB handle.
 *
 * Currently, the TSC is in a separate rbtreedb file and NOT
 * inside the WC DB.  We *MAY* or *MAY NOT* change that.  I
 * have defined this sg_wc_db__timestamp_cache API as a gateway
 * to let me change it later if we want.
 *
 * Currently, the original TSC (associated with the original
 * PendingTree), slurped the entire TSC from a 'rbtreedb'
 * into an in-memory 'rbtree' when the file was first opened.
 * And all subsequent gets/sets manipulated the in-memory
 * rbtree.
 *
 * TODO 2011/10/13 With the WC re-write of PendingTree where
 * TODO            we have better fine-grained control, I'd
 * TODO            like to change this to either: [1] keep the
 * TODO            table on disk in the rbtreedb and SELECT/
 * TODO            INSERT on a row-at-a-time-as-needed basis
 * TODO            or [2] move the TSC to be a regular table
 * TODO            within the WC DB and use it normally as
 * TODO            we do the other tables in it.
 * TODO
 * TODO            So I'm going to try to hide those differences
 * TODO            in this thin sg_wc_db__timestamp_cache__()
 * TODO            wrapper.
 *
 */
void sg_wc_db__timestamp_cache__open(SG_context * pCtx, sg_wc_db * pDb)
{
	if (pDb->pTSC)
		return;

#if TRACE_WC_TSC
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TSC:Open '%s'\n",
							   SG_pathname__sz(pDb->pPathWorkingDirectoryTop))  );
#endif

	SG_ERR_CHECK_RETURN(  SG_timestamp_cache__allocate_and_load(pCtx,
																&pDb->pTSC,
																pDb->pPathWorkingDirectoryTop)  );
}
		
/**
 * Remove all of the rows from the timestamp cache.
 * This will delete all entries from the cache and/or
 * create a new empty one.
 *
 * Generally, you shouldn't need to use this, but
 * given that the contents of any row in the timestamp
 * cache is a guess (and there are lots of ways to
 * defeat it), it is nice to have way (probably an
 * *advanced* command) to let the user flush the
 * cache if something doesn't look right.
 *
 * WARNING: Currently the TSC is in it's own SQL database
 *          so it is not affected by any TX that we have
 *          going on in the main SQL database.  If we
 *          decide to import the TSC as a table inside the
 *          main database, we'll need to decide if we want
 *          the TSC table edits in the main TX or in a
 *          separate TX or something.
 *
 */
void sg_wc_db__timestamp_cache__remove_all(SG_context * pCtx, sg_wc_db * pDb)
{
	if (!pDb->pTSC)
		SG_ERR_CHECK_RETURN(  sg_wc_db__timestamp_cache__open(pCtx, pDb)  );

#if TRACE_WC_TSC
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TSC:RemoveAll '%s'\n",
							   SG_pathname__sz(pDb->pPathWorkingDirectoryTop))  );
#endif

	SG_ERR_CHECK_RETURN(  SG_timestamp_cache__remove_all(pCtx, pDb->pTSC)  );
}

//////////////////////////////////////////////////////////////////

/**
 * See if the info we have on this item in the TSC is valid.
 * (That the file still has the same (mtime,size) currently
 * in the working directory.)
 *
 * If so, we return the TSData for this item.  That is, we
 * are going to ***ASSUME*** that if the (mtime,size) matches
 * that the file hasn't changed and that the also-stored
 * content-HID is still valid.
 *
 * TODO 2011/10/13 If/When we move the TSC into the WC DB,
 * TODO            switch the pszGid arg to a uiAliasGid arg.
 *
 */
void sg_wc_db__timestamp_cache__is_valid(SG_context * pCtx,
										 sg_wc_db * pDb,
										 const char * pszGid,
										 const SG_fsobj_stat * pfsStat,
										 SG_bool * pbValid,
										 const SG_timestamp_data ** ppTSData)
{
	if (!pDb->pTSC)
		SG_ERR_CHECK_RETURN(  sg_wc_db__timestamp_cache__open(pCtx, pDb)  );

	SG_ERR_CHECK_RETURN(  SG_timestamp_cache__is_valid(pCtx, pDb->pTSC,
													   pszGid,
													   pfsStat->mtime_ms,
													   pfsStat->size,
													   pbValid, ppTSData)  );

#if TRACE_WC_TSC
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TSC:IsValid '%s' %s Yields %d\n",
							   SG_pathname__sz(pDb->pPathWorkingDirectoryTop),
							   pszGid, (*pbValid))  );
#endif
}

/**
 * Remove the TSC entry for this item.
 * Use this ANYTIME that we alter the
 * contents of a file.
 *
 */
void sg_wc_db__timestamp_cache__remove(SG_context * pCtx,
									   sg_wc_db * pDb,
									   const char * pszGid)
{
	if (!pDb->pTSC)
		SG_ERR_CHECK_RETURN(  sg_wc_db__timestamp_cache__open(pCtx, pDb)  );

#if TRACE_WC_TSC
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TSC:Remove '%s' %s\n",
							   SG_pathname__sz(pDb->pPathWorkingDirectoryTop), pszGid)  );
#endif

	SG_ERR_CHECK_RETURN(  SG_timestamp_cache__remove(pCtx, pDb->pTSC, pszGid)  );
}

/**
 * Set/Update the info we have on this item in the TSC to
 * be the (content-hid, mtime, size).
 *
 * This routine may either:
 * [] just update the in-memory table.
 * [] update the in-memory table and update the db on disk.
 * we're not telling.  Therefore, eventually the caller should
 * call cause SG_timestamp_cache__save() to be called to
 * ensure that it gets to disk.
 *
 * TODO 2011/10/13 If/When we move the TSC into the WC DB,
 * TODO            switch the pszGid arg to a uiAliasGid arg.
 *
 */
void sg_wc_db__timestamp_cache__set(SG_context * pCtx,
									sg_wc_db * pDb,
									const char * pszGid,
									const SG_fsobj_stat * pfsStat,
									const char * pszHid)
{
	if (!pDb->pTSC)
		SG_ERR_CHECK_RETURN(  sg_wc_db__timestamp_cache__open(pCtx, pDb)  );

#if TRACE_WC_TSC
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TSC:Set '%s' %s\n",
							   SG_pathname__sz(pDb->pPathWorkingDirectoryTop), pszGid)  );
#endif

	SG_ERR_CHECK_RETURN(  SG_timestamp_cache__add(pCtx, pDb->pTSC,
												  pszGid,
												  pfsStat->mtime_ms,
												  pfsStat->size,
												  pszHid)  );
}

void sg_wc_db__timestamp_cache__save(SG_context * pCtx, sg_wc_db * pDb)
{
	if (!pDb->pTSC)
		return;

#if TRACE_WC_TSC
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR, "TSC:Save '%s'\n",
							   SG_pathname__sz(pDb->pPathWorkingDirectoryTop))  );
#endif

	SG_ERR_CHECK_RETURN(  SG_timestamp_cache__save(pCtx, pDb->pTSC)  );
}
