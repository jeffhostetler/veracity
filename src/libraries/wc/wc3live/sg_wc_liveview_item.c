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

//////////////////////////////////////////////////////////////////

#include <sg.h>

#include "sg_wc__public_typedefs.h"
#include "sg_wc__public_prototypes.h"
#include "sg_wc__private.h"

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_item__free(SG_context * pCtx,
							   sg_wc_liveview_item * pLVI)
{
	if (!pLVI)
		return;

	// we do not own pLVI->pLiveViewDir
	// we do not own pLVI->pPrescanRow
	// we do not own pLVI->queuedOverwrite.[...]

	SG_STRING_NULLFREE(pCtx, pLVI->pStringEntryname);
	SG_WC_DB__PC_ROW__NULLFREE(pCtx, pLVI->pPcRow_PC);
	SG_VHASH_NULLFREE(pCtx, pLVI->pvhIssue);
	SG_VHASH_NULLFREE(pCtx, pLVI->pvhSavedResolutions);
	SG_NULLFREE(pCtx, pLVI->pszHid_dir_content_commit);
	SG_STRING_NULLFREE(pCtx, pLVI->pStringBaselineRepoPath);
	SG_WC_DB__TNE_ROW__NULLFREE(pCtx, pLVI->pTneRow_AlternateBaseline);
	SG_STRING_NULLFREE(pCtx, pLVI->pStringAlternateBaselineRepoPath);

	SG_NULLFREE(pCtx, pLVI);
}

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_item__alloc__clone_from_prescan(SG_context * pCtx,
													sg_wc_liveview_item ** ppLVI,
													SG_wc_tx * pWcTx,
													const sg_wc_prescan_row * pPrescanRow)
{
	sg_wc_liveview_item * pLVI = NULL;
	SG_bool bFoundIssue = SG_FALSE;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pLVI)  );

	// caller needs to set the backptr if appropriate
	// if/when it adds this LVI to the LVD's vector.
	pLVI->pLiveViewDir = NULL;

	pLVI->uiAliasGid = pPrescanRow->uiAliasGid;

	SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx,
										  &pLVI->pStringEntryname,
										  pPrescanRow->pStringEntryname)  );

	pLVI->pPrescanRow = pPrescanRow;	// borrow a reference rather than cloning the scanrow.

	// because a liveview_item must start as an
	// exact clone of a scanrow, there cannot be
	// any in-tx changes yet for it.
	pLVI->pPcRow_PC = NULL;

	pLVI->scan_flags_Live = pPrescanRow->scan_flags_Ref;
	pLVI->tneType = pPrescanRow->tneType;

	SG_ERR_CHECK(  sg_wc_db__issue__get_issue(pCtx, pWcTx->pDb,
											  pLVI->uiAliasGid,
											  &bFoundIssue,
											  &pLVI->statusFlags_x_xr_xu,
											  &pLVI->pvhIssue,
											  &pLVI->pvhSavedResolutions)  );

	*ppLVI = pLVI;
	return;

fail:
	SG_WC_LIVEVIEW_ITEM__NULLFREE(pCtx, pLVI);
}

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_item__alloc__add_special(SG_context * pCtx,
											 sg_wc_liveview_item ** ppLVI,
											 SG_wc_tx * pWcTx,
											 SG_uint64 uiAliasGid,
											 SG_uint64 uiAliasGidParent,
											 const char * pszEntryname,
											 SG_treenode_entry_type tneType,
											 const char * pszHidMerge,
											 SG_int64 attrbits,
											 SG_wc_status_flags statusFlagsAddSpecialReason)
{
	sg_wc_liveview_item * pLVI = NULL;
	SG_bool bFoundIssue = SG_FALSE;

	SG_ERR_CHECK(  SG_alloc1(pCtx, pLVI)  );

	// caller needs to set the backptr if appropriate
	// if/when it adds this LVI to the LVD's vector.
	pLVI->pLiveViewDir = NULL;

	pLVI->uiAliasGid = uiAliasGid;

	SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx,
										&pLVI->pStringEntryname,
										pszEntryname)  );

	// During the QUEUE phase where we are doing this
	// special-add, we have not yet actually created
	// the item on disk.  So we should indicate that
	// scandir/readdir didn't know anything about it.
	//
	// TODO 2012/01/31 During the APPLY phase, we need to
	// TODO            fix-up this field.  Because all of
	// TODO            the __get_original_ and __get_current_
	// TODO            routines below assume that this field
	// TODO            is set.  That is, if you want to do
	// TODO            additional operations (like status)
	// TODO            after a merge, for example.
	// TODO
	// TODO 2012/04/12 Think about using pLVI->queuedOverwrite
	// TODO            fields for this.
	// NOTE
	// NOTE 2012/05/16 Setting this field to NULL caused a problem
	// NOTE            in _deal_with_moved_out_list() and
	// NOTE            sg_wc_liveview_item__alter_structure__move_rename()
	// NOTE            during UPDATE when an ADD-SPECIAL item was initially
	// NOTE            PARKED because of a transient collision (because the
	// NOTE            final UNPARK step uses move/rename to do the work).

	pLVI->pPrescanRow = NULL;

	// because a liveview_item must start as an
	// exact clone of a scanrow, there cannot be
	// any in-tx changes yet for it.
	SG_ERR_CHECK(  SG_WC_DB__PC_ROW__ALLOC(pCtx, &pLVI->pPcRow_PC)  );

	if (statusFlagsAddSpecialReason & SG_WC_STATUS_FLAGS__S__MERGE_CREATED)
		pLVI->pPcRow_PC->flags_net = SG_WC_DB__PC_ROW__FLAGS_NET__ADD_SPECIAL_M;
	else if (statusFlagsAddSpecialReason & SG_WC_STATUS_FLAGS__S__UPDATE_CREATED)
		pLVI->pPcRow_PC->flags_net = SG_WC_DB__PC_ROW__FLAGS_NET__ADD_SPECIAL_U;
	else
		SG_ERR_THROW2(  SG_ERR_INVALIDARG,
						(pCtx, "Invalid statusFlagsAddSpecialReason for '%s'", pszEntryname)  );

	pLVI->pPcRow_PC->p_s->uiAliasGid = uiAliasGid;
	pLVI->pPcRow_PC->p_s->uiAliasGidParent = uiAliasGidParent;
	SG_ERR_CHECK(  SG_STRDUP(pCtx, pszEntryname, &pLVI->pPcRow_PC->p_s->pszEntryname)  );
	pLVI->pPcRow_PC->p_s->tneType = tneType;
	if (pszHidMerge)
		SG_ERR_CHECK(  SG_STRDUP(pCtx, pszHidMerge, &pLVI->pPcRow_PC->pszHidMerge)  );

	pLVI->tneType = tneType;
	pLVI->scan_flags_Live = SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_POSTSCAN;

	if (statusFlagsAddSpecialReason & SG_WC_STATUS_FLAGS__A__SPARSE)
	{
		pLVI->pPcRow_PC->flags_net |= SG_WC_DB__PC_ROW__FLAGS_NET__SPARSE;
		SG_ERR_CHECK(  sg_wc_db__state_dynamic__alloc(pCtx, &pLVI->pPcRow_PC->p_d_sparse)  );
		pLVI->pPcRow_PC->p_d_sparse->attrbits = attrbits;
		if (tneType != SG_TREENODEENTRY_TYPE_DIRECTORY)
		{
			SG_ASSERT( pszHidMerge && *pszHidMerge );
			SG_ERR_CHECK(  SG_STRDUP(pCtx, pszHidMerge, &pLVI->pPcRow_PC->p_d_sparse->pszHid)  );
		}
		pLVI->scan_flags_Live = SG_WC_PRESCAN_FLAGS__CONTROLLED_ACTIVE_SPARSE;	// not |=
	}

	pLVI->pPcRow_PC->ref_attrbits = attrbits;

	SG_ERR_CHECK(  sg_wc_db__issue__get_issue(pCtx, pWcTx->pDb,
											  pLVI->uiAliasGid,
											  &bFoundIssue,
											  &pLVI->statusFlags_x_xr_xu,
											  &pLVI->pvhIssue,
											  &pLVI->pvhSavedResolutions)  );

	*ppLVI = pLVI;
	return;

fail:
	SG_WC_LIVEVIEW_ITEM__NULLFREE(pCtx, pLVI);
}

//////////////////////////////////////////////////////////////////

/**
 * Get the original entryname of this item.  This is the
 * name that it had in the baseline.
 * For ADDED/FOUND items, we fallback to the current value.
 *
 * Note that we DO NOT care where it was at the beginning
 * of the TX.
 * 
 */
void sg_wc_liveview_item__get_original_entryname(SG_context * pCtx,
												 const sg_wc_liveview_item * pLVI,
												 const char ** ppszEntryname)
{
	SG_ASSERT_RELEASE_RETURN( (pLVI->pPrescanRow) );

	if (pLVI->pTneRow_AlternateBaseline)
		*ppszEntryname = pLVI->pTneRow_AlternateBaseline->p_s->pszEntryname;
	else if (pLVI->pPrescanRow->pTneRow)
		*ppszEntryname = pLVI->pPrescanRow->pTneRow->p_s->pszEntryname;
	else
		*ppszEntryname = SG_string__sz(pLVI->pStringEntryname);
}

/**
 * Get the original parent of this item.  This is the parent
 * it had in the baseline.
 * For ADDED/FOUND items, we fallback to the current value.
 *
 * Note that we DO NOT care where it was at the beginning
 * of the TX.
 *
 */
void sg_wc_liveview_item__get_original_parent_alias(SG_context * pCtx,
													const sg_wc_liveview_item * pLVI,
													SG_uint64 * puiAliasGidParent)
{
	SG_ASSERT_RELEASE_RETURN( (pLVI->pPrescanRow) );

	if (pLVI->pTneRow_AlternateBaseline)
		*puiAliasGidParent = pLVI->pTneRow_AlternateBaseline->p_s->uiAliasGidParent;
	else if (pLVI->pPrescanRow->pTneRow)
		*puiAliasGidParent = pLVI->pPrescanRow->pTneRow->p_s->uiAliasGidParent;
	else
		*puiAliasGidParent = pLVI->pPcRow_PC->p_s->uiAliasGidParent;
}

//////////////////////////////////////////////////////////////////

/**
 * Get the original attrbits on this item as it was in the baseline.
 * For ADDED/FOUND items, we fallback to the current value.
 *
 */
void sg_wc_liveview_item__get_original_attrbits(SG_context * pCtx,
												sg_wc_liveview_item * pLVI,
												SG_wc_tx * pWcTx,
												SG_uint64 * pAttrbits)
{
	SG_ASSERT_RELEASE_RETURN( (pLVI->pPrescanRow) );

	if (pLVI->pTneRow_AlternateBaseline)
	{
		*pAttrbits = pLVI->pTneRow_AlternateBaseline->p_d->attrbits;
	}
	else if (pLVI->pPrescanRow->pTneRow)
	{
		*pAttrbits = pLVI->pPrescanRow->pTneRow->p_d->attrbits;
	}
	else
	{
		SG_ERR_CHECK_RETURN(  sg_wc_readdir__row__get_attrbits(pCtx, pWcTx,
															   pLVI->pPrescanRow->pRD)  );
		*pAttrbits = *pLVI->pPrescanRow->pRD->pAttrbits;
	}
}

/**
 * Get the current attrbits observed on the item *UNIONED*
 * with the baseline, if present.
 *
 * If the item is LOST/DELETED (not present in the working
 * directory), fall back to the baseline value.
 *
 */
void sg_wc_liveview_item__get_current_attrbits(SG_context * pCtx,
											   sg_wc_liveview_item * pLVI,
											   SG_wc_tx * pWcTx,
											   SG_uint64 * pAttrbits)
{
	if (pLVI->queuedOverwrites.pvhAttrbits)
	{
		// We have a QUEUED operation on this item that set the attrbits.
		// get the value from the journal.

		SG_ERR_CHECK_RETURN(  SG_vhash__get__int64(pCtx, pLVI->queuedOverwrites.pvhAttrbits,
												   "attrbits",
												   (SG_int64 *)pAttrbits)  );
#if TRACE_WC_LIE
		{
			SG_int_to_string_buffer bufui64;
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "GetCurrentAttrbits: using journal: %s\n",
									   SG_uint64_to_sz__hex((*pAttrbits), bufui64))  );
		}
#endif
		return;
	}

#if 1 && TRACE_WC_ATTRBITS
	{
	sg_wc_db__pc_row__flags_net flags_net = SG_WC_DB__PC_ROW__FLAGS_NET__ZERO;
	SG_ERR_IGNORE(  sg_wc_liveview_item__get_flags_net(pCtx, pLVI, &flags_net)  );
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "GetCurrentAttrbits: [flagsLive %d][flagsNet 0x%02x][pPcRow_PC %04x][pPcRow_Ref %04x][pRD %04x][pTneRow %04x] %s\n",
							   ((SG_uint32)pLVI->scan_flags_Live),
							   ((SG_uint32)flags_net),
							   (SG_uint32)((pLVI->pPcRow_PC) ? (pLVI->pPcRow_PC->ref_attrbits) : 0xffff),
							   (SG_uint32)((pLVI->pPrescanRow->pPcRow_Ref) ? (pLVI->pPrescanRow->pPcRow_Ref->ref_attrbits) : 0xffff),
							   (SG_uint32)((pLVI->pPrescanRow->pRD && pLVI->pPrescanRow->pRD->pAttrbits) ? (*pLVI->pPrescanRow->pRD->pAttrbits) : 0xffff),
							   (SG_uint32)((pLVI->pPrescanRow->pTneRow && pLVI->pPrescanRow->pTneRow->p_d) ? (pLVI->pPrescanRow->pTneRow->p_d->attrbits) : 0xffff),
							   SG_string__sz(pLVI->pStringEntryname))  );
	}
#endif

	SG_ASSERT_RELEASE_RETURN( (pLVI->pPrescanRow) );

	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live))
	{
		if (pLVI->pPcRow_PC)
		{
			SG_ASSERT_RELEASE_RETURN( (pLVI->pPcRow_PC->p_d_sparse) );
			*pAttrbits = pLVI->pPcRow_PC->p_d_sparse->attrbits;
		}
		else if (pLVI->pPrescanRow->pPcRow_Ref)
		{
			SG_ASSERT_RELEASE_RETURN( (pLVI->pPrescanRow->pPcRow_Ref->p_d_sparse) );
			*pAttrbits = pLVI->pPrescanRow->pPcRow_Ref->p_d_sparse->attrbits;
		}
		else
		{
			SG_ERR_THROW2_RETURN(  SG_ERR_NOTIMPLEMENTED,
								   (pCtx, "GetCurrentAttrbits: unhandled case when sparse for '%s'.",
									SG_string__sz(pLVI->pStringEntryname))  );
		}
	}
	else if (pLVI->pPrescanRow->pRD)
	{
		SG_ERR_CHECK_RETURN(  sg_wc_readdir__row__get_attrbits(pCtx, pWcTx,
															   pLVI->pPrescanRow->pRD)  );
		if (pLVI->pPcRow_PC)
			SG_ERR_CHECK_RETURN(  sg_wc_attrbits__compute_effective_attrbits(pCtx,
																			 pWcTx->pDb->pAttrbitsData,
																			 pLVI->pPcRow_PC->ref_attrbits,
																			 *pLVI->pPrescanRow->pRD->pAttrbits,
																			 pAttrbits)  );
		else if (pLVI->pPrescanRow->pPcRow_Ref)
			SG_ERR_CHECK_RETURN(  sg_wc_attrbits__compute_effective_attrbits(pCtx,
																			 pWcTx->pDb->pAttrbitsData,
																			 pLVI->pPrescanRow->pPcRow_Ref->ref_attrbits,
																			 *pLVI->pPrescanRow->pRD->pAttrbits,
																			 pAttrbits)  );
		else if (pLVI->pPrescanRow->pTneRow)
			SG_ERR_CHECK_RETURN(  sg_wc_attrbits__compute_effective_attrbits(pCtx,
																			 pWcTx->pDb->pAttrbitsData,
																			 pLVI->pPrescanRow->pTneRow->p_d->attrbits,
																			 *pLVI->pPrescanRow->pRD->pAttrbits,
																			 pAttrbits)  );
		else
			*pAttrbits = *pLVI->pPrescanRow->pRD->pAttrbits;
	}
	else if (pLVI->pPrescanRow->pTneRow)
	{
		*pAttrbits = pLVI->pPrescanRow->pTneRow->p_d->attrbits;
	}
	else
	{
		*pAttrbits = SG_WC_ATTRBITS__ZERO;
	}

#if 1 && TRACE_WC_ATTRBITS
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "GetCurrentAttrbits: yielded %x: %s\n",
							   (SG_uint32)(*pAttrbits),
							   SG_string__sz(pLVI->pStringEntryname))  );
#endif
}

//////////////////////////////////////////////////////////////////

/**
 * Get the baseline value of the content HID.
 *
 * We DO NOT attempt to substitute the merge-created-hid.
 * But we do for created-by-update items.
 *
 * You DO NOT own the returned result.
 * 
 */
void sg_wc_liveview_item__get_original_content_hid(SG_context * pCtx,
												   sg_wc_liveview_item * pLVI,
												   SG_wc_tx * pWcTx,
												   SG_bool bNoTSC,
												   const char ** ppszHidContent)
{
	sg_wc_db__pc_row__flags_net flags_net = SG_WC_DB__PC_ROW__FLAGS_NET__ZERO;

	if (SG_WC_PRESCAN_FLAGS__IS_RESERVED(pLVI->scan_flags_Live))
	{
		// We should not ever be called on a reserved item
		// because our caller shouldn't be trying to compare
		// HIDs (or whatever).
		//
		// TODO 2012/10/05 This should probably be an assert.
		SG_ERR_THROW2_RETURN(  SG_ERR_WC_RESERVED_ENTRYNAME,
							   (pCtx, "%s", SG_string__sz(pLVI->pStringEntryname))  );
	}

	SG_ASSERT_RELEASE_RETURN( (pLVI->pPrescanRow) );

	// try to get baseline value if present.
	// fallback to current value if not.

	if (pLVI->pTneRow_AlternateBaseline)
	{
		*ppszHidContent = pLVI->pTneRow_AlternateBaseline->p_d->pszHid;
		return;
	}

	if (pLVI->pPrescanRow->pTneRow)
	{
		*ppszHidContent = pLVI->pPrescanRow->pTneRow->p_d->pszHid;
		return;
	}

	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live))
	{
		// TODO 2012/12/06 I think this only kicks in for __SPARSE + __ADD_SPECIAL_U
		// TODO            (otherwise, the above pTneRow would be present).

		if (pLVI->pPcRow_PC)
		{
			*ppszHidContent = pLVI->pPcRow_PC->p_d_sparse->pszHid;
			return;
		}
		else if (pLVI->pPrescanRow->pPcRow_Ref)
		{
			*ppszHidContent = pLVI->pPrescanRow->pPcRow_Ref->p_d_sparse->pszHid;
			return;
		}
		else
		{
			SG_ERR_THROW2_RETURN(  SG_ERR_NOTIMPLEMENTED,
								   (pCtx, "GetOriginalContentHid: %s",
									SG_string__sz(pLVI->pStringEntryname))  );
		}
	}

	// TODO 2012/12/06 I think this can be simplified.
	// TODO            We only set pszHidMerge for plain files.
	if (pLVI->tneType != SG_TREENODEENTRY_TYPE_DIRECTORY)
	{
		// If this item was an added-by-update, then we can give them the
		// HID of the file that UPDATE created.
		SG_ERR_CHECK_RETURN(  sg_wc_liveview_item__get_flags_net(pCtx, pLVI, &flags_net)  );
		if (flags_net & SG_WC_DB__PC_ROW__FLAGS_NET__ADD_SPECIAL_U)
		{
			if (pLVI->pPcRow_PC)
			{
				if (pLVI->pPcRow_PC->pszHidMerge)
				{
					*ppszHidContent = pLVI->pPcRow_PC->pszHidMerge;
					return;
				}
			}
			else if (pLVI->pPrescanRow->pPcRow_Ref)
			{
				if (pLVI->pPrescanRow->pPcRow_Ref->pszHidMerge)
				{
					*ppszHidContent = pLVI->pPrescanRow->pPcRow_Ref->pszHidMerge;
					return;
				}
			}
		}
	}


#if TRACE_WC_TSC
	if (!bNoTSC)
		SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
								   "GetOriginalContentHid: looking up '%s'\n",
								   SG_string__sz(pLVI->pStringEntryname))  );
#endif

	if (!pLVI->pPrescanRow->pRD)
	{
		// Not likely to happen, but we can give a better(?) error message
		// than sg_wc_readdir__row__get_content_hid.
		SG_ERR_THROW2_RETURN(  SG_ERR_NOT_FOUND,
							   (pCtx, "No pRD for item '%s' in GetOriginalContentHid().",
								SG_string__sz(pLVI->pStringEntryname))  );
	}
	SG_ERR_CHECK_RETURN(  sg_wc_readdir__row__get_content_hid(pCtx, pWcTx,
															  pLVI->pPrescanRow->pRD,
															  bNoTSC,
															  ppszHidContent,
															  NULL)  );
}

//////////////////////////////////////////////////////////////////

static void _fetch_size_of_blob(SG_context * pCtx,
								SG_wc_tx * pWcTx,
								const char * pszHidContent,
								SG_uint64 * pSize)
{
    SG_uint64 len = 0;

	SG_ERR_CHECK_RETURN(  SG_repo__fetch_blob__begin(pCtx, pWcTx->pDb->pRepo, pszHidContent,
													 SG_TRUE, NULL, NULL, NULL, &len, NULL)  );
	*pSize = len;
}

//////////////////////////////////////////////////////////////////

/**
 * Get the current content HID and optionally the size.
 * (Note that the current HID is not usually defined for a directory.
 * And therefore the content size of a directory is not usually
 * defined either.)
 *
 */
void sg_wc_liveview_item__get_current_content_hid(SG_context * pCtx,
												  sg_wc_liveview_item * pLVI,
												  SG_wc_tx * pWcTx,
												  SG_bool bNoTSC,
												  char ** ppszHidContent,
												  SG_uint64 * pSize)
{
	if (pLVI->queuedOverwrites.pvhContent)
	{
		// We have a QUEUED operation on this item that changed the
		// contents.  Get the 'current' value from the journal.

		const char * psz = NULL;
		
		SG_ERR_CHECK_RETURN(  SG_vhash__check__sz(pCtx, pLVI->queuedOverwrites.pvhContent, "hid", &psz)  );
		if (psz)
		{
			// last overwrite-type operation used an HID.
#if TRACE_WC_LIE
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "GetCurrentContentHid: using journal %s for: %s\n",
									   psz, SG_string__sz(pLVI->pStringEntryname))  );
#endif
			if (pSize)
				SG_ERR_CHECK_RETURN(  _fetch_size_of_blob(pCtx, pWcTx, psz, pSize)  );

			SG_ERR_CHECK_RETURN(  SG_strdup(pCtx, psz, ppszHidContent)  );
			return;
		}

		SG_ERR_CHECK_RETURN(  SG_vhash__check__sz(pCtx, pLVI->queuedOverwrites.pvhContent, "file", &psz)  );
		if (psz)
		{
			// last overwrite-type operation used a TEMP file.

			SG_ERR_CHECK_RETURN(  sg_wc_compute_file_hid__sz(pCtx, pWcTx, psz, ppszHidContent, pSize)  );
			return;
		}

		SG_ERR_CHECK_RETURN(  SG_vhash__check__sz(pCtx, pLVI->queuedOverwrites.pvhContent, "target", &psz)  );
		if (psz)
		{
			// last overwrite-type operation gave us a SYMLINK-TARGET.
			// it is no problem to compute this, i'm just being
			// lazy since i'm not sure we need this.  
			SG_ERR_THROW2_RETURN(  SG_ERR_NOTIMPLEMENTED,
								   (pCtx,
									"GetCurrentContentHid: using journal: TODO compute HID of symlink target '%s' for: %s",
									psz, SG_string__sz(pLVI->pStringEntryname))  );
			// TODO also return size
		}

		SG_ERR_THROW2_RETURN(  SG_ERR_NOTIMPLEMENTED,
							   (pCtx,
								"GetCurrentContentHid: required field missing from vhash for: %s",
								SG_string__sz(pLVI->pStringEntryname))  );
	}

	SG_ASSERT_RELEASE_RETURN( (pLVI->pPrescanRow) );

	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live))
	{
		if (pLVI->pPcRow_PC)
		{
			SG_ASSERT_RELEASE_RETURN( (pLVI->pPcRow_PC->p_d_sparse) );
			SG_ERR_CHECK_RETURN(  SG_STRDUP(pCtx,
											pLVI->pPcRow_PC->p_d_sparse->pszHid,
											ppszHidContent)  );
			if (pSize)
				SG_ERR_CHECK_RETURN(  _fetch_size_of_blob(pCtx, pWcTx,
														  pLVI->pPcRow_PC->p_d_sparse->pszHid,
														  pSize)  );
		}
		else if (pLVI->pPrescanRow->pPcRow_Ref)
		{
			SG_ASSERT_RELEASE_RETURN( (pLVI->pPrescanRow->pPcRow_Ref->p_d_sparse) );
			SG_ERR_CHECK_RETURN(  SG_STRDUP(pCtx,
											pLVI->pPrescanRow->pPcRow_Ref->p_d_sparse->pszHid,
											ppszHidContent)  );
			if (pSize)
				SG_ERR_CHECK_RETURN(  _fetch_size_of_blob(pCtx, pWcTx,
														  pLVI->pPrescanRow->pPcRow_Ref->p_d_sparse->pszHid,
														  pSize)  );
		}
		else
		{
			// With the addition of {sparse_hid,sparse_attrbits} to tbl_PC,
			// we should not get here.
			SG_ERR_THROW2_RETURN(  SG_ERR_NOTIMPLEMENTED,
								   (pCtx, "GetCurrentHid: unhandled case when sparse for '%s'.",
									SG_string__sz(pLVI->pStringEntryname))  );
		}
	}
	else if (pLVI->pPrescanRow->pRD)
	{
#if TRACE_WC_TSC
		if (!bNoTSC)
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "GetCurrentContentHid: looking up '%s'\n",
									   SG_string__sz(pLVI->pStringEntryname))  );
#endif
		SG_ERR_CHECK_RETURN(  sg_wc_readdir__row__get_content_hid__owned(pCtx, pWcTx,
																		 pLVI->pPrescanRow->pRD,
																		 bNoTSC,
																		 ppszHidContent,
																		 pSize)  );
	}
	else if (pLVI->pPrescanRow->pTneRow)
	{
		SG_ERR_CHECK_RETURN(  SG_STRDUP(pCtx, pLVI->pPrescanRow->pTneRow->p_d->pszHid, ppszHidContent)  );
		if (pSize)
			SG_ERR_CHECK_RETURN(  _fetch_size_of_blob(pCtx, pWcTx, pLVI->pPrescanRow->pTneRow->p_d->pszHid, pSize)  );
	}
	else
	{
		// perhaps an ADD-SPECIAL + DELETE
		// or an ADDED+LOST in an UPDATE ?
		SG_ERR_THROW2_RETURN(  SG_ERR_NOTIMPLEMENTED,
							   (pCtx, "GetCurrentHid: unhandled case for '%s'.",
								SG_string__sz(pLVI->pStringEntryname))  );
	}
}

//////////////////////////////////////////////////////////////////

/**
 * This is a special case of __get_original_content_hid() that
 * basically doest that and then fetches the blob and returns
 * the actual symlink target.
 *
 * This is a convenience for STATUS that can report 'target'
 * fields rather than (or in addition to) 'hid' for symlinks.
 *
 */
void sg_wc_liveview_item__get_original_symlink_target(SG_context * pCtx,
													  sg_wc_liveview_item * pLVI,
													  SG_wc_tx * pWcTx,
													  SG_string ** ppStringTarget)
{
	const char * pszHidContent;
	SG_string * pStringTarget = NULL;
	SG_byte*    pBuffer   = NULL;
	SG_uint64   uSize     = 0u;

	if (pLVI->tneType != SG_TREENODEENTRY_TYPE_SYMLINK)
		SG_ERR_THROW2_RETURN(  SG_ERR_INVALIDARG,
							   (pCtx, "GetOriginalSymlinkTarget: '%s' is not a symlink.",
								SG_string__sz(pLVI->pStringEntryname))  );

	SG_ERR_CHECK(  sg_wc_liveview_item__get_original_content_hid(pCtx, pLVI, pWcTx,
																 SG_FALSE,
																 &pszHidContent)  );
	SG_ERR_CHECK(  SG_repo__fetch_blob_into_memory(pCtx, pWcTx->pDb->pRepo,
												   pszHidContent,
												   &pBuffer, &uSize)  );
	SG_ERR_CHECK(  SG_STRING__ALLOC__BUF_LEN(pCtx, &pStringTarget, pBuffer, (SG_uint32)uSize)  );

	*ppStringTarget = pStringTarget;
	pStringTarget = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, pStringTarget);
	SG_NULLFREE(pCtx, pBuffer);
}

/**
 * This is a special case of __get_current_content_hid() that
 * basically does that and then fetches the blob and returns
 * the actual target string contained within.
 *
 * Normally, we wouldn't need this specialization, but if we
 * have a QUEUED overwrite-symlink-target, THEN WE NEED TO LIE
 * and return the new/queued value rather than the actual
 * target value that the existing symlink has in the WD.
 *
 */
void sg_wc_liveview_item__get_current_symlink_target(SG_context * pCtx,
													 sg_wc_liveview_item * pLVI,
													 SG_wc_tx * pWcTx,
													 SG_string ** ppStringTarget)
{
	SG_byte*    pBuffer   = NULL;
	SG_uint64   uSize     = 0u;

	if (pLVI->tneType != SG_TREENODEENTRY_TYPE_SYMLINK)
		SG_ERR_THROW2_RETURN(  SG_ERR_INVALIDARG,
							   (pCtx, "GetCurrentSymlinkTarget: '%s' is not a symlink.",
								SG_string__sz(pLVI->pStringEntryname))  );

	if (pLVI->queuedOverwrites.pvhContent)
	{
		// We have a QUEUED operation on this item that changed the
		// contents.  Get the 'current' value from the journal.

		const char * psz = NULL;

		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pLVI->queuedOverwrites.pvhContent, "target", &psz)  );
		if (psz)
		{
			// last overwrite-type operation gave us a SYMLINK-TARGET.

#if TRACE_WC_LIE
			SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
									   "GetCurrentSymlinkTarget: using journal '%s' for: %s\n",
									   psz, SG_string__sz(pLVI->pStringEntryname))  );
#endif
			SG_ERR_CHECK(  SG_STRING__ALLOC__SZ(pCtx, ppStringTarget, psz)  );
			return;
		}

		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pLVI->queuedOverwrites.pvhContent, "file", &psz)  );
		if (psz)
		{
			// last overwrite-type operation used a TEMP file.
			// this cannot/should not happen.
			SG_ERR_THROW2(  SG_ERR_INVALIDARG,
							(pCtx,
							 "GetCurrentSymlinkTarget: journal contains temp file '%s' for: %s",
							 psz, SG_string__sz(pLVI->pStringEntryname))  );
		}

		SG_ERR_CHECK(  SG_vhash__check__sz(pCtx, pLVI->queuedOverwrites.pvhContent, "hid", &psz)  );
		if (psz)
		{
			// last overwrite-type operation used an HID.
			SG_ERR_CHECK(  SG_repo__fetch_blob_into_memory(pCtx, pWcTx->pDb->pRepo, psz, &pBuffer, &uSize)  );
			SG_ERR_CHECK(  SG_STRING__ALLOC__BUF_LEN(pCtx, ppStringTarget, pBuffer, (SG_uint32)uSize)  );
			SG_NULLFREE(pCtx, pBuffer);
			return;
		}
		
		SG_ERR_THROW2(  SG_ERR_NOTIMPLEMENTED,
						(pCtx,
						 "GetCurrentSymlinkTarget: required field missing from vhash for: %s",
						 SG_string__sz(pLVI->pStringEntryname))  );
	}

	// Otherwise, return the current pre-tx value.

	SG_ASSERT_RELEASE_RETURN( (pLVI->pPrescanRow) );

	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED_SPARSE(pLVI->scan_flags_Live))
	{
		if (pLVI->pPcRow_PC)
		{
			SG_ASSERT_RELEASE_RETURN( (pLVI->pPcRow_PC->p_d_sparse) );
			SG_ERR_CHECK(  SG_repo__fetch_blob_into_memory(pCtx, pWcTx->pDb->pRepo,
														   pLVI->pPcRow_PC->p_d_sparse->pszHid,
														   &pBuffer, &uSize)  );
			SG_ERR_CHECK(  SG_STRING__ALLOC__BUF_LEN(pCtx, ppStringTarget, pBuffer, (SG_uint32)uSize)  );
			SG_NULLFREE(pCtx, pBuffer);
			return;
		}
		else if (pLVI->pPrescanRow->pPcRow_Ref)
		{
			SG_ASSERT_RELEASE_RETURN( (pLVI->pPrescanRow->pPcRow_Ref->p_d_sparse) );
			SG_ERR_CHECK(  SG_repo__fetch_blob_into_memory(pCtx, pWcTx->pDb->pRepo,
														   pLVI->pPrescanRow->pPcRow_Ref->p_d_sparse->pszHid,
														   &pBuffer, &uSize)  );
			SG_ERR_CHECK(  SG_STRING__ALLOC__BUF_LEN(pCtx, ppStringTarget, pBuffer, (SG_uint32)uSize)  );
			SG_NULLFREE(pCtx, pBuffer);
			return;
		}
		else
		{
			// With the addition of {sparse_hid,sparse_attrbits} to tbl_PC,
			// we should not get here.
			SG_ERR_THROW2_RETURN(  SG_ERR_NOTIMPLEMENTED,
								   (pCtx, "GetCurrentSymlinkTarget: unhandled case when sparse for '%s'.",
									SG_string__sz(pLVI->pStringEntryname))  );
		}
	}
	else if (pLVI->pPrescanRow->pRD)
	{
		const SG_string * pStringTargetRef;
		SG_ERR_CHECK(  sg_wc_readdir__row__get_content_symlink_target(pCtx, pWcTx, pLVI->pPrescanRow->pRD,
																	  &pStringTargetRef)  );
		SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx, ppStringTarget, pStringTargetRef)  );
		return;
	}
	else if (pLVI->pPrescanRow->pTneRow)
	{
		SG_ERR_CHECK(  SG_repo__fetch_blob_into_memory(pCtx, pWcTx->pDb->pRepo,
													   pLVI->pPrescanRow->pTneRow->p_d->pszHid,
													   &pBuffer, &uSize)  );
		SG_ERR_CHECK(  SG_STRING__ALLOC__BUF_LEN(pCtx, ppStringTarget, pBuffer, (SG_uint32)uSize)  );
		SG_NULLFREE(pCtx, pBuffer);
		return;
	}
	else
	{
		// perhaps an ADD-SPECIAL + DELETE
		// or an ADDED+LOST in an UPDATE ?
		SG_ERR_THROW2_RETURN(  SG_ERR_NOTIMPLEMENTED,
							   (pCtx, "GetCurrentSymlinkTarget: unhandled case for '%s'.",
								SG_string__sz(pLVI->pStringEntryname))  );
	}


fail:
	SG_NULLFREE(pCtx, pBuffer);
}

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_item__get_flags_net(SG_context * pCtx,
										const sg_wc_liveview_item * pLVI,
										sg_wc_db__pc_row__flags_net * pFlagsNet)
{
	sg_wc_db__pc_row__flags_net flags_net = SG_WC_DB__PC_ROW__FLAGS_NET__ZERO;

	SG_UNUSED( pCtx );

	// If the item has been changed during this TX, then we will
	// have a complete set of flags pPcRow_PC (which superceeds
	// anything that was set at the beginning of the TX).  If there
	// were no changes during this TX, then see if there were any
	// changes as of the start of the TX.

	if (SG_WC_PRESCAN_FLAGS__IS_CONTROLLED(pLVI->scan_flags_Live))
	{
		if (pLVI->pPcRow_PC)
			flags_net = pLVI->pPcRow_PC->flags_net;
		else if (pLVI->pPrescanRow->pPcRow_Ref)
			flags_net = pLVI->pPrescanRow->pPcRow_Ref->flags_net;
	}

	*pFlagsNet = flags_net;
}

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_item__remember_last_overwrite(SG_context * pCtx,
												  sg_wc_liveview_item * pLVI,
												  SG_vhash * pvhContent,
												  SG_vhash * pvhAttrbits)
{
#if TRACE_WC_LIE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "RememberOverwrite: [%c,%c] for '%s'\n",
							   ((pvhContent) ? 'c' : ' '),
							   ((pvhAttrbits) ? 'a' : ' '),
							   SG_string__sz(pLVI->pStringEntryname))  );
#else
	SG_UNUSED( pCtx );
#endif

	if (pvhContent)
		pLVI->queuedOverwrites.pvhContent = pvhContent;
	if (pvhAttrbits)
		pLVI->queuedOverwrites.pvhAttrbits = pvhAttrbits;
}

/**
 * Return a pathname (live or temp) to a file that contains
 * the CURRENTLY QUEUED content that this item **SHOULD** have
 * at this point in the TX.
 *
 * That is, the caller could be in the middle of a TX and have
 * overwritten the file once or twice and then may now be
 * requesting the path to show a diff.  Or the file content may
 * be unchanged, but we have queued one or more moves/renames to
 * it or parent directories.
 *
 * As a good player INSIDE THE TX, we need to give them a path
 * to a CURRENT IN-TX COPY OF THE ***CONTENT*** (wherever it
 * may be).
 *
 * So the path we return may be to a temp file that was created
 * as a source for a QUEUED overwrite.  Or it may be a path to
 * the unmodified content in the WD -- WHERE IT WAS BEFORE THE
 * TX -- because until APPLY is called, the WD hasn't been
 * changed yet.
 *
 * Regardless of whether the result is a temp file or not, the
 * caller should be careful to not let the user modify the file
 * without participating in the TX.  That is, if we return the
 * actual non-temp working copy of a file and they use it in a
 * DIFF and the user's difftool is interactive and they alter
 * it and then we cancel the TX, what should the WD version of
 * the file contain?
 * 
 * See also:
 * __overwrite_file_from_file()
 * __overwrite_file_from_repo()
 * __add_special()
 * __undo_delete()
 *
 *
 * We return an indication of whether the file is a TEMP file
 * and shouldn't be written to.  It DOES NOT indicate that you
 * can delete it -- it indicates that you should not edit it because
 * *WE* will probably delete the file if the TX is rolled-back and so
 * the user would lose their edits.
 * 
 */
void sg_wc_liveview_item__get_proxy_file_path(SG_context * pCtx,
											  sg_wc_liveview_item * pLVI,
											  SG_wc_tx * pWcTx,
											  SG_pathname ** ppPath,
											  SG_bool * pbIsTmp)
{
	SG_string * pStringRepoPath = NULL;
	SG_pathname * pPathAbsolute = NULL;
	char * pszGid = NULL;
	const char * psz;
	SG_bool bIsTmp = SG_TRUE;

	if (pLVI->tneType != SG_TREENODEENTRY_TYPE_REGULAR_FILE)
		SG_ERR_THROW2_RETURN(  SG_ERR_INVALIDARG,
							   (pCtx, "GetProxyFilePath: '%s' is not a file.",
								SG_string__sz(pLVI->pStringEntryname))  );

	if (pLVI->queuedOverwrites.pvhContent == NULL)
	{
		// No changes to the content yet in this TX.  Return the PRE-TX
		// pathname of this file.  (We may have QUEUED moves/renames on
		// the file or a parent directory, but they haven't been applied
		// yet.)

		SG_ASSERT(  pLVI->pPrescanRow  );
		SG_ASSERT(  pLVI->pPrescanRow->pStringEntryname  );
		SG_ASSERT(  pLVI->pPrescanRow->pPrescanDir_Ref  );
		SG_ASSERT(  pLVI->pPrescanRow->pPrescanDir_Ref->pStringRefRepoPath  );

		SG_ERR_CHECK(  SG_STRING__ALLOC__COPY(pCtx,
											  &pStringRepoPath,
											  pLVI->pPrescanRow->pPrescanDir_Ref->pStringRefRepoPath)  );
		SG_ERR_CHECK(  SG_repopath__append_entryname(pCtx,
													 pStringRepoPath,
													 SG_string__sz(pLVI->pPrescanRow->pStringEntryname),
													 SG_FALSE)  );
		SG_ERR_CHECK(  sg_wc_db__path__repopath_to_absolute(pCtx, pWcTx->pDb,
															pStringRepoPath,
															&pPathAbsolute)  );
		bIsTmp = SG_FALSE;	// path is to actual WC file
		goto done;
	}

	SG_ERR_CHECK_RETURN(  SG_vhash__check__sz(pCtx, pLVI->queuedOverwrites.pvhContent, "file", &psz)  );
	if (psz)
	{
		// return path to existing TEMP file.  someone else owns the file.

		SG_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathAbsolute, psz)  );
		bIsTmp = SG_TRUE;	// path is to a TEMP file (for which an overwrite-from-file has already been scheduled).
		goto done;
	}

	SG_ERR_CHECK_RETURN(  SG_vhash__check__sz(pCtx, pLVI->queuedOverwrites.pvhContent, "hid", &psz)  );
	if (psz)
	{
		// synthesize a TEMP file for this.  caller owns the new temp file.

		SG_ERR_CHECK(  sg_wc_db__gid__get_gid_from_alias(pCtx, pWcTx->pDb, pLVI->uiAliasGid, &pszGid)  );
		SG_ERR_CHECK(  sg_wc_diff_utils__export_to_temp_file(pCtx, pWcTx, "ref", pszGid, psz,
															 SG_string__sz(pLVI->pStringEntryname),	// for suffix only
															 &pPathAbsolute)  );
		bIsTmp = SG_TRUE;	// path is to a TEMP file that we just created.
		goto done;
	}

	SG_ERR_THROW2_RETURN(  SG_ERR_NOTIMPLEMENTED,
						   (pCtx,
							"GetProxyFilePath: required field missing from vhash for: %s",
							SG_string__sz(pLVI->pStringEntryname))  );

done:
#if TRACE_WC_LIE
	SG_ERR_IGNORE(  SG_console(pCtx, SG_CS_STDERR,
							   "GetProxyFilePath: '%s' ==> '%s' [bIsTmp %d]\n",
							   SG_string__sz(pLVI->pStringEntryname),
							   SG_pathname__sz(pPathAbsolute),
							   bIsTmp)  );
#endif
	*ppPath = pPathAbsolute;
	pPathAbsolute = NULL;
	*pbIsTmp = bIsTmp;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathAbsolute);
	SG_STRING_NULLFREE(pCtx, pStringRepoPath);
	SG_NULLFREE(pCtx, pszGid);
}

//////////////////////////////////////////////////////////////////

void sg_wc_liveview_item__set_alternate_baseline_during_update(SG_context * pCtx,
															   SG_wc_tx * pWcTx,
															   sg_wc_liveview_item * pLVI)
{
	SG_bool bFound;

	SG_NULLARGCHECK_RETURN( pWcTx );
	SG_NULLARGCHECK_RETURN( pLVI );

	// we should probably assert that these are null.
	SG_WC_DB__TNE_ROW__NULLFREE(pCtx, pLVI->pTneRow_AlternateBaseline);
	SG_STRING_NULLFREE(pCtx, pLVI->pStringAlternateBaselineRepoPath);

	SG_ERR_CHECK(  sg_wc_db__tne__get_row_by_alias(pCtx, pWcTx->pDb, pWcTx->pCSetRow_Other,
												   pLVI->uiAliasGid, &bFound, &pLVI->pTneRow_AlternateBaseline)  );
	if (pLVI->pTneRow_AlternateBaseline)
		SG_ERR_CHECK(  sg_wc_db__tne__get_extended_repo_path_from_alias(pCtx, pWcTx->pDb, pWcTx->pCSetRow_Other,
																		pLVI->uiAliasGid, SG_WC__REPO_PATH_DOMAIN__B,
																		&pLVI->pStringAlternateBaselineRepoPath)  );

fail:
	return;
}
