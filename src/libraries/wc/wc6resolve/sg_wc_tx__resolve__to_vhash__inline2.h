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

#ifndef H_SG_WC_TX__RESOLVE__TO_VHASH__INLINE2_H
#define H_SG_WC_TX__RESOLVE__TO_VHASH__INLINE2_H

BEGIN_EXTERN_C;

//////////////////////////////////////////////////////////////////

static void _sg_resolve__value__to_vhash(SG_context * pCtx,
										 const SG_resolve__value * pValue,
										 SG_vhash ** ppvhValue)
{
	SG_vhash * pvhValue = NULL;

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhValue)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhValue, "label", pValue->szLabel)  );

	switch (pValue->pChoice->eState)
	{
	case SG_RESOLVE__STATE__EXISTENCE:
		// TODO ... fill in these details for pVariant, pVariantFile, uVariantSize
	case SG_RESOLVE__STATE__NAME:
	case SG_RESOLVE__STATE__LOCATION:
	case SG_RESOLVE__STATE__ATTRIBUTES:
	case SG_RESOLVE__STATE__CONTENTS:
	default:
		break;
	}
		
	if (pValue->szVariantSummary)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhValue, "summary", pValue->szVariantSummary)  );
	if (pValue->pszMnemonic)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhValue, "mnemonic", pValue->pszMnemonic)  );

	SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvhValue, "automerged", pValue->bAutoMerge)  );
	if (pValue->szMergeTool)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhValue, "automerge_mergetool", pValue->szMergeTool)  );

	// TODO 2012/06/04 .... ? iMergeTime
	// TODO 2012/06/04 .... ? iMergeToolResult
	// TODO 2012/06/04 .... ? pOtherParent
	// TODO 2012/06/04 .... ? pBaselineParent
	// TODO 2012/06/04 .... ? uChildren
		
	*ppvhValue = pvhValue;
	pvhValue = NULL;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhValue);
}

static void _sg_resolve__choice__to_vhash(SG_context * pCtx,
										  const SG_resolve__choice * pChoice,
										  SG_vhash ** ppvhChoice)
{
	SG_vhash * pvhChoice = NULL;
	SG_vhash * pvhValue_j = NULL;
	SG_vhash * pvhValue_Result = NULL;
	SG_varray * pvaValues = NULL;	// we don't own this
	SG_varray * pvaLeafValues = NULL;	// we don't own this
	SG_uint32 j, nrValues;

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhChoice)  );

	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhChoice, "state", gaSerializedStateValues[pChoice->eState])  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhChoice, "conflict_flags", pChoice->eConflictCauses)  );
	SG_ERR_CHECK(  SG_vhash__add__bool(pCtx, pvhChoice, "collision_flags", pChoice->bCollisionCause)  );
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhChoice, "portability_flags", pChoice->ePortCauses)  );
	
	// TODO 2012/06/04 other stuff: ? pLeafValues
	// TODO 2012/06/04 other stuff: ? pCollisionItems
	// TODO 2012/08/22 other stuff: ? pPortabilityCollisionItems
	// TODO 2012/06/04 other stuff: ? pCycleItems
	// TODO 2012/06/04 other stuff: ? szCycleHint

	if (pChoice->pTempPath)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhChoice, "repopath_tempdir", SG_pathname__sz(pChoice->pTempPath))  );

	SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvhChoice, "values", &pvaValues)  );
	SG_ERR_CHECK(  SG_vector__length(pCtx, pChoice->pValues, &nrValues)  );
	for (j=0; j < nrValues; j++)
	{
		SG_resolve__value * pValue_j = NULL;	// we do not own this

		SG_ERR_CHECK(  SG_vector__get(pCtx, pChoice->pValues, j, (void **)&pValue_j)  );
		SG_ERR_CHECK(  _sg_resolve__value__to_vhash(pCtx, pValue_j, &pvhValue_j)  );
		SG_ERR_CHECK(  SG_varray__append__vhash(pCtx, pvaValues, &pvhValue_j)  );
	}

	if (pChoice->pLeafValues)
	{
		SG_ERR_CHECK(  SG_vhash__addnew__varray(pCtx, pvhChoice, "leaf_values", &pvaLeafValues)  );
		SG_ERR_CHECK(  SG_vector__length(pCtx, pChoice->pLeafValues, &nrValues)  );
		for (j=0; j < nrValues; j++)
		{
			SG_resolve__value * pValue_j = NULL;	// we do not own this

			SG_ERR_CHECK(  SG_vector__get(pCtx, pChoice->pLeafValues, j, (void **)&pValue_j)  );
			SG_ERR_CHECK(  _sg_resolve__value__to_vhash(pCtx, pValue_j, &pvhValue_j)  );
			SG_ERR_CHECK(  SG_varray__append__vhash(pCtx, pvaLeafValues, &pvhValue_j)  );
		}
	}

	if (pChoice->pMergeableResult)
	{
		SG_ERR_CHECK(  _sg_resolve__value__to_vhash(pCtx, pChoice->pMergeableResult, &pvhValue_Result)  );
		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhChoice, "result", &pvhValue_Result)  );
	}

	*ppvhChoice = pvhChoice;
	pvhChoice = NULL;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhChoice);
	SG_VHASH_NULLFREE(pCtx, pvhValue_j);
	SG_VHASH_NULLFREE(pCtx, pvhValue_Result);
}

static void _sg_resolve__item__to_vhash(SG_context * pCtx,
										const SG_resolve__item * pItem,
										SG_vhash ** ppvhItem)
{
	SG_vhash * pvhItem = NULL;
	SG_vhash * pvhChoices = NULL;		// we do not own this
	SG_vhash * pvhChoice_k = NULL;
	SG_string * pStringRepoPathWorking = NULL;
	SG_resolve__state state_k;
	SG_bool bShowInParens;

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhItem)  );
	
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhItem, "gid", pItem->szGid)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhItem, "gid_path", SG_string__sz(pItem->pStringGidDomainRepoPath))  );

	SG_ERR_CHECK(  SG_resolve__item__get_repo_path__working(pCtx, pItem, &pStringRepoPathWorking, &bShowInParens)  );
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhItem, "path", SG_string__sz(pStringRepoPathWorking))  );
	
	SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhItem, "merged_name", pItem->szMergedName)  );
//	if (pItem->szMergedContentHid)
//		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhItem, "merged_hid", pItem->szMergedContentHid)  );
//	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhItem, "merged_attrbits", pItem->iMergedAttributes)  );
	if (pItem->szRestoreOriginalName)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhItem, "restore_original_name", pItem->szRestoreOriginalName)  );
	if (pItem->szReferenceOriginalName)
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhItem, "reference_original_name", pItem->szReferenceOriginalName)  );

	SG_ERR_CHECK(  SG_vhash__addnew__vhash(pCtx, pvhItem, "choices", &pvhChoices)  );
	for (state_k=SG_RESOLVE__STATE__EXISTENCE; state_k < SG_RESOLVE__STATE__COUNT; state_k++)
		if (pItem->aChoices[state_k])
		{
			SG_ERR_CHECK(  _sg_resolve__choice__to_vhash(pCtx, pItem->aChoices[state_k], &pvhChoice_k)  );
			SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhChoices, gaSerializedStateValues[state_k], &pvhChoice_k)  );
		}

	// Technically the issue isn't part of the resolve data, but this is all for debug so let add it too.
	SG_ERR_CHECK(  SG_vhash__addcopy__vhash(pCtx, pvhItem, "issues", pItem->pLVI->pvhIssue)  );
	// Likewise, x_xr_xu data isn't part of issue nor resolve data, but this is for debug.
	SG_ERR_CHECK(  SG_vhash__add__int64(pCtx, pvhItem, "x_xr_xu", pItem->pLVI->statusFlags_x_xr_xu)  );

	*ppvhItem = pvhItem;
	pvhItem = NULL;

fail:
	SG_STRING_NULLFREE(pCtx, pStringRepoPathWorking);
	SG_VHASH_NULLFREE(pCtx, pvhChoice_k);
	SG_VHASH_NULLFREE(pCtx, pvhItem);
}

static SG_vector_foreach_callback _sg_resolve__item__to_vhash__cb;
static void _sg_resolve__item__to_vhash__cb(SG_context * pCtx, SG_uint32 ndx, void * pVoidAssoc, void * pVoidData)
{
	SG_resolve__item * pItem = (SG_resolve__item *)pVoidAssoc;
	SG_vhash * pvhResolve = (SG_vhash *)pVoidData;
	SG_vhash * pvhItem = NULL;

	SG_UNUSED( ndx );

	SG_ERR_CHECK(  _sg_resolve__item__to_vhash(pCtx, pItem, &pvhItem)  );
	SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhResolve, pItem->szGid, &pvhItem)  );

fail:
	SG_VHASH_NULLFREE(pCtx, pvhItem);
}

/**
 * This routine creates a VHASH representing the
 * current contents of pResolve.
 *
 * This is currently intended as a debug routine.
 *
 *
 */
void SG_resolve__to_vhash(SG_context * pCtx,
						  const SG_resolve * pResolve,
						  SG_vhash ** ppvhResolve)
{
	SG_vhash * pvhResolve = NULL;

	SG_NULLARGCHECK_RETURN( pResolve );
	SG_NULLARGCHECK_RETURN( ppvhResolve );

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhResolve)  );
	SG_ERR_CHECK(  SG_vector__foreach(pCtx, pResolve->pItems, _sg_resolve__item__to_vhash__cb, pvhResolve)  );

	*ppvhResolve = pvhResolve;
	pvhResolve = NULL;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhResolve);
}

//////////////////////////////////////////////////////////////////

#if defined(DEBUG)
void SG_resolve_debug__dump_to_console(SG_context * pCtx,
									   const SG_resolve * pResolve)
{
	SG_vhash * pvhResolve = NULL;
	SG_string * pStrOut = NULL;

	SG_NULLARGCHECK_RETURN( pResolve );
	
	SG_ERR_CHECK(  SG_resolve__to_vhash(pCtx, pResolve, &pvhResolve)  );

	SG_ERR_CHECK(  SG_STRING__ALLOC(pCtx, &pStrOut)  );
	SG_ERR_CHECK(  SG_vhash__to_json__pretty_print_NOT_for_storage(pCtx, pvhResolve, pStrOut)  );
	SG_ERR_CHECK(  SG_console(pCtx, SG_CS_STDERR, "Resolve:\n%s\n", SG_string__sz(pStrOut))  );
	SG_ERR_CHECK(  SG_console__flush(pCtx, SG_CS_STDERR)  );

	// fall thru to common cleanup

fail:
	SG_VHASH_NULLFREE(pCtx, pvhResolve);
	SG_STRING_NULLFREE(pCtx, pStrOut);
}
#endif

//////////////////////////////////////////////////////////////////

END_EXTERN_C;

#endif//H_SG_WC_TX__RESOLVE__TO_VHASH__INLINE2_H
