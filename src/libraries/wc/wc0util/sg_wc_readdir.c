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
 * @file sg_wc_readdir.c
 *
 * @details Routines to read the contents of a directory within
 * the working directory.
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

struct _cb_data
{
	SG_rbtree * prb;
	const SG_pathname * pPathDir;	// we do not own this
};

/**
 * We get called once for each item observed in the
 * directory on disk.
 *
 * Create a ROW in the RBTREE for this item.
 *
 */
static SG_dir_foreach_callback _sg_wc_readdir__scan_cb;

static void _sg_wc_readdir__scan_cb(SG_context * pCtx,
									const SG_string * pStringEntryname,
									/*const*/ SG_fsobj_stat * pfsStat,
									void * pVoidData)
{
	struct _cb_data * pData = (struct _cb_data *)pVoidData;
	sg_wc_readdir__row * pRow = NULL;

#if TRACE_WC_READDIR
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_readdir__alloc__from_scan: received: ['%s','%s']\n",
							   SG_pathname__sz(pData->pPathDir),
							   SG_string__sz(pStringEntryname))  );
#endif

	SG_ERR_CHECK(  sg_wc_readdir__row__alloc(pCtx, &pRow,
											 pData->pPathDir,
											 pStringEntryname,
											 pfsStat)  );
	SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx,
											  pData->prb,
											  SG_string__sz(pStringEntryname),
											  pRow)  );
	return;

fail:
	SG_WC_READDIR__ROW__NULLFREE(pCtx, pRow);
}

/**
 * Read the contents of the directory and build a
 * RBTREE of the entrynames.
 *
 * rbEntrynames : map[<entryname> ==> <sg_wc_readdir__row *>]
 *
 * We let the caller choose which special entries
 * to SKIP, but we force a STAT (because I need
 * the file-type).
 */
void sg_wc_readdir__alloc__from_scan(SG_context * pCtx,
									 const SG_pathname * pPathDir,
									 SG_dir_foreach_flags flagsReaddir,
									 SG_rbtree ** pprb_readdir)
{
	struct _cb_data data;

#if TRACE_WC_READDIR
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_readdir__alloc__from_scan: %s\n",
							   SG_pathname__sz(pPathDir))  );
#endif

	memset(&data, 0, sizeof(data));
	data.pPathDir = pPathDir;

	SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &data.prb)  );

	SG_ERR_CHECK(  SG_dir__foreach(pCtx, pPathDir,
								   (flagsReaddir | SG_DIR__FOREACH__STAT),
								   _sg_wc_readdir__scan_cb,
								   (void *)&data)  );
	*pprb_readdir = data.prb;
	return;

fail:
	SG_RBTREE_NULLFREE(pCtx, data.prb);
}
