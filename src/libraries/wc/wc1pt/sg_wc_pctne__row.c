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
 * @file sg_wc_pctne__row.c
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

void sg_wc_pctne__row__free(SG_context * pCtx, sg_wc_pctne__row * pPT)
{
	if (!pPT)
		return;

	SG_WC_DB__PC_ROW__NULLFREE(pCtx, pPT->pPcRow);
	SG_WC_DB__TNE_ROW__NULLFREE(pCtx, pPT->pTneRow);

	SG_NULLFREE(pCtx, pPT);
}

void sg_wc_pctne__row__alloc(SG_context * pCtx, sg_wc_pctne__row ** ppPT)
{
	sg_wc_pctne__row * pPT = NULL;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pPT)  );

	*ppPT = pPT;
	return;

fail:
	SG_WC_PCTNE__ROW__NULLFREE(pCtx, pPT);
}

//////////////////////////////////////////////////////////////////

#if TRACE_WC_DB
void sg_wc_pctne__row__debug_print(SG_context * pCtx, const sg_wc_pctne__row * pPT)
{
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "sg_wc_pctne__row:\n")  );
	if (pPT)
	{
		if (pPT->pTneRow)
			SG_ERR_IGNORE(  sg_wc_db__debug__tne_row__print(pCtx, pPT->pTneRow)  );
		if (pPT->pPcRow)
			SG_ERR_IGNORE(  sg_wc_db__debug__pc_row__print(pCtx, pPT->pPcRow)  );
	}
}
#endif

//////////////////////////////////////////////////////////////////

/**
 * Does this pctne__row represent a PENDED DELETE ?
 *
 * Note that this is *ONLY* concerned with the __NET__
 * status flags.  We don't care how the delete was
 * requested.
 *
 */
void sg_wc_pctne__row__is_delete_pended(SG_context * pCtx,
										const sg_wc_pctne__row * pPT,
										SG_bool * pbResult)
{
	SG_UNUSED( pCtx );

	*pbResult = ( (pPT->pPcRow) && (pPT->pPcRow->flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__DELETED) );
}

//////////////////////////////////////////////////////////////////

void sg_wc_pctne__row__get_alias(SG_context * pCtx,
								 const sg_wc_pctne__row * pPT,
								 SG_uint64 * puiAliasGid)
{
	if (pPT->pPcRow)
		*puiAliasGid = pPT->pPcRow->p_s->uiAliasGid;
	else if (pPT->pTneRow)
		*puiAliasGid = pPT->pTneRow->p_s->uiAliasGid;
	else // This should not happen.
		SG_ERR_THROW_RETURN(  SG_ERR_NOTIMPLEMENTED  );
}

//////////////////////////////////////////////////////////////////

/**
 * Get the alias-gid of the *CURRENT* parent directory.
 * If an item has been MOVED or ADDED, this will be the
 * new parent directory.  Otherwise, this will be the
 * parent from the baseline.
 *
 * TODO 2011/08/12 Should this throw if a delete is pended ?
 *
 */
void sg_wc_pctne__row__get_current_parent_alias(SG_context * pCtx,
												const sg_wc_pctne__row * pPT,
												SG_uint64 * puiAliasGidParent)
{
	if (pPT->pPcRow)
		*puiAliasGidParent = pPT->pPcRow->p_s->uiAliasGidParent;
	else if (pPT->pTneRow)
		*puiAliasGidParent = pPT->pTneRow->p_s->uiAliasGidParent;
	else // This should not happen.
		SG_ERR_THROW_RETURN(  SG_ERR_NOTIMPLEMENTED  );
}

/**
 * Get the alias-gid of the *ORIGINAL* parent directory.
 * This is where the item was in the baseline.
 * For ADDED items, we fallback to the current parent.
 *
 */
void sg_wc_pctne__row__get_original_parent_alias(SG_context * pCtx,
												 const sg_wc_pctne__row * pPT,
												 SG_uint64 * puiAliasGidParent)
{
	if (pPT->pTneRow)
		*puiAliasGidParent = pPT->pTneRow->p_s->uiAliasGidParent;
	else if (pPT->pPcRow)
		*puiAliasGidParent = pPT->pPcRow->p_s->uiAliasGidParent;
	else // This should not happen.
		SG_ERR_THROW_RETURN(  SG_ERR_NOTIMPLEMENTED  );
}

//////////////////////////////////////////////////////////////////

/**
 * Get the current entryname of this item.  If the item
 * has been RENAMED, this will be the new name.  Otherwise,
 * this will be the name from the baseline.
 *
 * TODO 2011/08/12 Should this throw if a delete is pended ?
 * 
 */
void sg_wc_pctne__row__get_current_entryname(SG_context * pCtx,
											 const sg_wc_pctne__row * pPT,
											 const char ** ppszEntryname)
{
	if (pPT->pPcRow)
		*ppszEntryname = pPT->pPcRow->p_s->pszEntryname;
	else if (pPT->pTneRow)
		*ppszEntryname = pPT->pTneRow->p_s->pszEntryname;
	else // This should not happen.
		SG_ERR_THROW_RETURN(  SG_ERR_NOTIMPLEMENTED  );
}

/**
 * Get the original entryname of this item.  This is the
 * name that it had in the baseline.
 * For ADDED items, we fallback to the current value.
 *
 */
void sg_wc_pctne__row__get_original_entryname(SG_context * pCtx,
											  const sg_wc_pctne__row * pPT,
											  const char ** ppszEntryname)
{
	if (pPT->pTneRow)
		*ppszEntryname = pPT->pTneRow->p_s->pszEntryname;
	else if (pPT->pPcRow)
		*ppszEntryname = pPT->pPcRow->p_s->pszEntryname;
	else // This should not happen.
		SG_ERR_THROW_RETURN(  SG_ERR_NOTIMPLEMENTED  );
}

//////////////////////////////////////////////////////////////////

/**
 * Get the TNE-TYPE of this item.  This is a constant
 * for the life of an item/gid.
 */
void sg_wc_pctne__row__get_tne_type(SG_context * pCtx,
									const sg_wc_pctne__row * pPT,
									SG_treenode_entry_type * pTneType)
{
	if (pPT->pPcRow)
		*pTneType = pPT->pPcRow->p_s->tneType;
	else if (pPT->pTneRow)
		*pTneType = pPT->pTneRow->p_s->tneType;
	else // This should not happen.
		SG_ERR_THROW_RETURN(  SG_ERR_NOTIMPLEMENTED  );
}
