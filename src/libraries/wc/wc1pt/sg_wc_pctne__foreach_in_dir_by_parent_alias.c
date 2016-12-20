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
 * @file sg_wc_pctne__foreach_in_dir_by_parent_alias.c
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

struct _level1_data
{
	sg_wc_db *						pDb;
	const sg_wc_db__cset_row *		pCSetRow;
	sg_wc_pctne__foreach_cb *		pfn_cb_level2;
	void *							pVoidLevel2Data;
	SG_rbtree_ui64 *				prb64AliasesSeen;
};

/**
 * Maintain a list of the gid-aliases that we have visited.
 *
 */
static void _seen_alias(SG_context * pCtx,
						struct _level1_data * pLevel1Data,
						SG_uint64 uiAliasGid,
						SG_bool * pbAlreadySeen)
{
	SG_rbtree_ui64__add__with_assoc(pCtx,
									pLevel1Data->prb64AliasesSeen,
									uiAliasGid,
									NULL);
	if (!SG_CONTEXT__HAS_ERR(pCtx))
	{
		*pbAlreadySeen = SG_FALSE;
		return;
	}
	if (SG_context__err_equals(pCtx, SG_ERR_RBTREE_DUPLICATEKEY))
	{
		SG_context__err_reset(pCtx);
		*pbAlreadySeen = SG_TRUE;
		return;
	}

	SG_ERR_RETHROW_RETURN;
}

/**
 * Part A of level-1 of the 2-level callback.
 * We are called once for each pTneRow in the tne_L0 table
 * for items in that directory in the baseline.
 *
 * We lookup any peer pPcRow (by gid-alias) in the tbl_pc,
 * if present.
 *
 * We build a pPT pair and send IT to the caller's real callback.
 *
 * Note that by doing it this way we will broadcast all
 * rows present in this directory in the baseline -- both
 * (structurally) UNCHANGED rows and rows that have PENDED
 * STRUCTURAL CHANGES.  (Including MOVED-AWAYs)
 */
static sg_wc_db__tne__foreach_cb _tne_cb_level1;

static void _tne_cb_level1(SG_context * pCtx, void * pVoidLevel1Data, sg_wc_db__tne_row ** ppTneRow)
{
	struct _level1_data * pLevel1Data = (struct _level1_data *)pVoidLevel1Data;
	sg_wc_pctne__row * pPT = NULL;
	SG_bool bFoundInPc;
	SG_bool bAlreadySeen = SG_FALSE;

	// record that we have seen this alias-gid.
	// this will let Part B avoid iterating over duplicates.
	SG_ERR_CHECK(  _seen_alias(pCtx, pLevel1Data, (*ppTneRow)->p_s->uiAliasGid, &bAlreadySeen)  );
	SG_ASSERT( (bAlreadySeen == SG_FALSE) );

	SG_ERR_CHECK(  SG_WC_PCTNE__ROW__ALLOC(pCtx, &pPT)  );

	pPT->pTneRow = *ppTneRow;		// we steal it
	*ppTneRow = NULL;

	// lookup any peer pPcRow, if it exists.
	SG_ERR_CHECK(  sg_wc_db__pc__get_row_by_alias(pCtx, pLevel1Data->pDb, pLevel1Data->pCSetRow,
												  pPT->pTneRow->p_s->uiAliasGid,
												  &bFoundInPc, &pPT->pPcRow)  );

	// we pass pPT by address so that the cb can steal it.
	SG_ERR_CHECK(  (*pLevel1Data->pfn_cb_level2)(pCtx,
												 pLevel1Data->pVoidLevel2Data,
												 &pPT)  );

fail:
	SG_WC_PCTNE__ROW__NULLFREE(pCtx, pPT);
}

/**
 * Part B of level-1 of the 2-level callback.
 * We are called once for each pPcRow in the tbl_pc table
 * for items that have had any kind of structural change
 * (ADD/DELETE/MOVE/RENAME) **AND** are CURRENTLY in the
 * requested directory.
 *
 * These rows are effectively unioned with the ones from
 * Part A into the result set that we pass to the caller's
 * real callback.
 *
 * We check for duplicates using the 'already-seen' table.
 * So in effect what we contribute are the ADDED and MOVED-TOs
 * rows.
 *
 */
static sg_wc_db__pc__foreach_cb _pc_cb_level1;

static void _pc_cb_level1(SG_context * pCtx, void * pVoidLevel1Data, sg_wc_db__pc_row ** ppPcRow)
{
	struct _level1_data * pLevel1Data = (struct _level1_data *)pVoidLevel1Data;
	sg_wc_pctne__row * pPT = NULL;
	SG_bool bAlreadySeen = SG_FALSE;
	SG_bool bFoundInTne;

	// look-up whether and/or record that we have seen this alias-gid.
	// this will let Part B avoid iterating over duplicates.
	SG_ERR_CHECK(  _seen_alias(pCtx, pLevel1Data, (*ppPcRow)->p_s->uiAliasGid, &bAlreadySeen)  );
	if (bAlreadySeen)
		return;

	SG_ERR_CHECK(  SG_WC_PCTNE__ROW__ALLOC(pCtx, &pPT)  );

	pPT->pPcRow = *ppPcRow;		// we steal it
	*ppPcRow = NULL;

	// lookup any peer pTneRow, if it exists.
	SG_ERR_CHECK(  sg_wc_db__tne__get_row_by_alias(pCtx, pLevel1Data->pDb, pLevel1Data->pCSetRow,
												   pPT->pPcRow->p_s->uiAliasGid,
												   &bFoundInTne, &pPT->pTneRow)  );

	// we pass pPT by address so that the cb can steal it.
	SG_ERR_CHECK(  (*pLevel1Data->pfn_cb_level2)(pCtx,
												 pLevel1Data->pVoidLevel2Data,
												 &pPT)  );

fail:
	SG_WC_PCTNE__ROW__NULLFREE(pCtx, pPT);
}

/**
 * Lookup the directory with the given gid-alias and
 * iterate over all entries that ARE OR WERE in it.
 * This requires us to internally iterate over BOTH
 * the tne_L0 and tbl_pc/temp_refpc tables.
 *
 * (And use a 2-level callback.)
 *
 * This is conceptually a:
 * 
 * SELECT * FROM
 *     tne_L0 LEFT OUTER JOIN tbl_pc
 *     ON tne_L0.alias_gid=tbl_pc.alias_gid
 *     WHERE tne_L0.alias_gid_parent = ?
 *   UNION ALL
 *     SELECT * FROM
 *       tbl_pc LEFT OUTER JOIN tne_L0
 *       ON tbl_pc.alias_gid=tne_L0.alias_gid
 *       WHERE tbl_pc.alias_gid_parent = ?
 *
 * with the addition of a lookup in tnrp_L0
 * for the rows that are directories.
 */
void sg_wc_pctne__foreach_in_dir_by_parent_alias(SG_context * pCtx,
												 sg_wc_db * pDb,
												 const sg_wc_db__cset_row * pCSetRow,
												 SG_uint64 uiAliasGidParent,
												 sg_wc_pctne__foreach_cb * pfn_cb_level2,
												 void * pVoidLevel2Data)
{
	struct _level1_data level1data;
#if TRACE_WC_DB
	SG_int_to_string_buffer bufui64;
#endif

	memset(&level1data, 0, sizeof(level1data));
	level1data.pDb = pDb;
	level1data.pCSetRow = pCSetRow;
	level1data.pfn_cb_level2 = pfn_cb_level2;
	level1data.pVoidLevel2Data = pVoidLevel2Data;
	SG_ERR_CHECK(  SG_RBTREE_UI64__ALLOC(pCtx, &level1data.prb64AliasesSeen)  );

#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_pctne__foreach_in_dir_by_parent_alias: %s part 1:\n",
							   SG_uint64_to_sz(uiAliasGidParent, bufui64))  );
#endif

	SG_ERR_CHECK(  sg_wc_db__tne__foreach_in_dir_by_parent_alias(pCtx, pDb, pCSetRow,
																 uiAliasGidParent,
																 _tne_cb_level1, &level1data)  );

#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_pctne__foreach_in_dir_by_parent_alias: %s part 2:\n",
							   SG_uint64_to_sz(uiAliasGidParent, bufui64))  );
#endif

	SG_ERR_CHECK(  sg_wc_db__pc__foreach_in_dir_by_parent_alias(pCtx, pDb, pCSetRow,
																uiAliasGidParent,
																_pc_cb_level1, &level1data)  );

#if TRACE_WC_DB
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_pctne__foreach_in_dir_by_parent_alias: %s finished:\n",
							   SG_uint64_to_sz(uiAliasGidParent, bufui64))  );
#endif

fail:
	SG_RBTREE_UI64_NULLFREE(pCtx, level1data.prb64AliasesSeen);
}
