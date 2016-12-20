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

struct _load_cset_data
{
	SG_mrg *		pMrg;

	SG_uint32		kSPCA;
	SG_uint32		kLeaves;
};

typedef struct _load_cset_data _load_cset_data;

//////////////////////////////////////////////////////////////////

static void _load_A__using_repo(SG_context * pCtx,
								SG_mrg * pMrg,
								const char * pszAcceptLabel,
								const char * pszHid_CSet)
{
	const char * pszCSetLabel = "A";
	const char * pszMnemonicName = SG_MRG_MNEMONIC__A;

	// insert "A" into tbl_csets
	// do not populate tne_A

	SG_ERR_CHECK(  sg_wc_db__load_named_cset(pCtx, pMrg->pWcTx->pDb, pszCSetLabel, pszHid_CSet,
											 SG_FALSE, SG_FALSE, SG_FALSE)  );

	// use the repo version of the Ancestor CSET to build the in-memory
	// version of the CSET in pMrg->pMrgCSet_LCA.

	SG_ERR_CHECK(  SG_MRG_CSET__ALLOC(pCtx, pszMnemonicName, pszCSetLabel, pszAcceptLabel,
									  SG_MRG_CSET_ORIGIN__LOADED, SG_DAGLCA_NODE_TYPE__LCA,
									  &pMrg->pMrgCSet_LCA)  );
	SG_ERR_CHECK(  SG_mrg_cset__load_entire_cset__using_repo(pCtx, pMrg, pMrg->pMrgCSet_LCA, pszHid_CSet)  );

fail:
	return;
}

static void _load_B__using_wc(SG_context * pCtx,
							  SG_mrg * pMrg,
							  const char * pszAcceptLabel)
{
	const char * pszCSetLabel = "L0";
	const char * pszMnemonicName = SG_MRG_MNEMONIC__B;

	// L0 is already present in tbl_csets
	// L0 is already populated in tne_L0
	//
	// So we don't need to call sg_wc_db__load_named_cset()

	// Use the WC version (tne_L0+tbl_PC) of the CSET to build
	// the in-memory view of the CSET in pMrg->pMrgCSet_Baseline.
	// This may contain dirt.

	SG_ERR_CHECK_RETURN(  SG_MRG_CSET__ALLOC(pCtx, pszMnemonicName, pszCSetLabel, pszAcceptLabel,
											 SG_MRG_CSET_ORIGIN__LOADED, SG_DAGLCA_NODE_TYPE__LEAF,
											 &pMrg->pMrgCSet_Baseline)  );
	SG_ERR_CHECK(  SG_mrg_cset__load_entire_cset__using_wc(pCtx, pMrg,
														   pMrg->pMrgCSet_Baseline,
														   pMrg->pWcTx->pCSetRow_Baseline->psz_hid_cset)  );

fail:
	return;
}

static void _load_C__using_repo(SG_context * pCtx,
								SG_mrg * pMrg,
								const char * pszAcceptLabel,
								const char * pszHid_CSet,
								SG_bool bCreateTneIndex,
								SG_bool bCreatePC)
{
	const char * pszCSetLabel = "L1";
	const char * pszMnemonicName = SG_MRG_MNEMONIC__C;
	SG_bool bPopulateTne = SG_TRUE;

	// insert "L1" into tbl_csets
	// populate "tne_L1"

	SG_ERR_CHECK(  sg_wc_db__load_named_cset(pCtx, pMrg->pWcTx->pDb, pszCSetLabel, pszHid_CSet,
											 bPopulateTne, bCreateTneIndex, bCreatePC)  );
	SG_ERR_CHECK(  sg_wc_db__csets__get_row_by_label(pCtx, pMrg->pWcTx->pDb, pszCSetLabel, NULL,
													 &pMrg->pWcTx->pCSetRow_Other)  );

	// Use the repo version of the other CSET to build the in-memory
	// version of the CSET in pMrg->pMrgCSet_Other.

	SG_ERR_CHECK_RETURN(  SG_MRG_CSET__ALLOC(pCtx, pszMnemonicName, pszCSetLabel, pszAcceptLabel,
											 SG_MRG_CSET_ORIGIN__LOADED, SG_DAGLCA_NODE_TYPE__LEAF,
											 &pMrg->pMrgCSet_Other)  );
	SG_ERR_CHECK(  SG_mrg_cset__load_entire_cset__using_repo(pCtx, pMrg, pMrg->pMrgCSet_Other, pszHid_CSet)  );

fail:
	return;
}

/**
 * Force load a specific CSet into the AUX table.  These
 * are used when we need an alternate per-item ancestor.
 *
 * We cache the result, so you DO NOT own it.
 *
 */
void sg_mrg_cset__load_aux__using_repo(SG_context * pCtx,
									   SG_mrg * pMrg,
									   const char * pszHid_CSet,
									   SG_mrg_cset **ppMrgCSet)
{
	// I'm going to lie and say that the mnemonic for this AUX CSet is __A
	// so that it will look right in any merge issues generated using it.
	// That is, this CSet is NOT the actual LCA ancestor, BUT FOR AN
	// INDIVIDUAL FILE, it may contain the TNE/pMrgCSetEntry info that
	// we used when we merge this individual file.  So the __A section
	// ON THAT INDIVIDUAL FILE'S ISSUE should refer to this CSet.

	const char * pszMnemonicName = SG_MRG_MNEMONIC__A;
	SG_mrg_cset * pMrgCSet_Allocated = NULL;
	char bufCSetLabel[10];
	SG_uint32 nrAux = 0;

	if (pMrg->prbCSets_Aux)
	{
		SG_bool bFound;
		SG_ERR_CHECK(  SG_rbtree__find(pCtx, pMrg->prbCSets_Aux, pszHid_CSet,
									   &bFound, (void **)ppMrgCSet)  );
		if (bFound)
			return;

		SG_ERR_CHECK(  SG_rbtree__count(pCtx, pMrg->prbCSets_Aux, &nrAux)  );
	}
	else
		SG_ERR_CHECK(  SG_RBTREE__ALLOC(pCtx, &pMrg->prbCSets_Aux)  );

	SG_ERR_CHECK(  SG_sprintf(pCtx, bufCSetLabel, SG_NrElements(bufCSetLabel), "x%d", nrAux++)  );

	// do not insert "S*" into tbl_csets
	// do not populate tne_S*

	// Use the repo version of the CSET to build the in-memory
	// version of the CSET in pMrg->prbCSets_Aux[*].
  
	SG_ERR_CHECK_RETURN(  SG_MRG_CSET__ALLOC(pCtx, pszMnemonicName, bufCSetLabel,
											 SG_MRG_ACCEPT_LABEL__A__MERGE,
											 SG_MRG_CSET_ORIGIN__LOADED,
											 SG_DAGLCA_NODE_TYPE__NOBODY,
											 &pMrgCSet_Allocated)  );
	SG_ERR_CHECK(  SG_mrg_cset__load_entire_cset__using_repo(pCtx, pMrg, pMrgCSet_Allocated,
															 pszHid_CSet)  );
	SG_ERR_CHECK(  SG_rbtree__add__with_assoc(pCtx, pMrg->prbCSets_Aux, pszHid_CSet,
											  (void *)pMrgCSet_Allocated)  );
	*ppMrgCSet = pMrgCSet_Allocated;
	pMrgCSet_Allocated = NULL;		// rbtree owns it now

fail:
	SG_MRG_CSET_NULLFREE(pCtx, pMrgCSet_Allocated);
}

//////////////////////////////////////////////////////////////////

static SG_daglca__foreach_callback _sg_mrg__daglca_callback__load_cset;

static void _sg_mrg__daglca_callback__load_cset(SG_context * pCtx,
												const char * pszHid_CSet,
												SG_daglca_node_type nodeType,
												SG_int32 generation,
												SG_rbtree * prbImmediateDescendants,
												void * pVoidCallerData)
{
	// Each CSET in the DAGLCA graph needs to be loaded into memory.
	// This allows us to do arbitrary compares between the different
	// versions of the tree (guided by the DAGLCA graph).
	//
	// We have a little impedance mis-match here
	// betweeen the new WC (aka PendingTree replacement)
	// code and the core algorithms in the original
	// (PendingTree-based) MERGE code.
	// 
	// The new WC code builds the "tbl_L0" table which is a cached
	// copy of the baseline CSET in a FLATTENED form and indexed
	// by GID/alias.  The WC code also maintains the "tbl_PC"
	// table which contains PENDED STRUCTURAL changes in
	// the WC (relative to the baseline/L0).  These are used
	// by the LVI/LVD code to give rapid/random access to
	// anything in the tree without regard for location and
	// without the need for a tree-walk to find.  (And they
	// are sync'd with the SCANDIR code.)
	// 
	// As of 2012/03/12 we now use these 2 tables to ***LIE***
	// to the MERGE code.  The __using_wc() routines build
	// (what MERGE calls) the "baseline" version so that we
	// secretly support a dirty merge -- or rather we can *TRY*
	// to let MERGE start with the dirty WD.
	//
	//
	// TODO 2012/03/13 The WC code also builds the "tbl_L1"
	// TODO            table which is a read-only view of
	// TODO            the other CSET.  This is currently
	// TODO            only used to help
	// TODO            sg_wc_tx__liveview__compute_*_repo_path()
	// TODO            for merge_created+deleted items.  And it
	// TODO            lets users type an extended-prefix
	// TODO            repo-path using a domain key for the other
	// TODO            CSET.
	// TODO
	// TODO            See W3250.
	// TODO
	// TODO            Currently, we DO NOT use the "tbl_L1" table
	// TODO            to load stuff into pMrgCSet_Other.  We use
	// TODO            the __using_repo() routines.  So we are
	// TODO            currently loading the L1 changeset *twice*:
	// TODO            once to build the tne_L* tables and once to
	// TODO            build the merge structures.
	// TODO
	// TODO            Think about having another set of __using_*
	// TODO            routines that would let us avoid this.

	_load_cset_data * pLoadCSetData = (_load_cset_data *)pVoidCallerData;
	SG_bool bIsBaseline;

	SG_UNUSED(generation);

	// "Mnemonic" names are used to indicate the STRUCTURAL ROLE that
	// a CSET/Version had (in the ISSUE data we save for RESOLVE).
	// RESOLVE needs this to know which branch is which when re-merging
	// and etc.  (This is perhaps an abuse of the word "mnemonic", but
	// it's easy to grep for.)
	//
	// "CSetLabel" names are used *internally* in tbl_csets and (when
	// present) to name individual (tne_*, pc_*) tables associated
	// with a cset.  We don't need to expose them to RESOLVE nor the user.
	//
	// Part of the distinction is just baggage left in the code, but part
	// is necessary.
	// Historically, I've been using A, L0, L1, ... terms to name the
	// merge csets and name the sql tables, where I need a tight binding
	// during a single operation such as MERGE (and now UPDATE) where these
	// refer to specific items.  But as we allow dirty-UPDATE to do chaining
	// and carry-forward unresolved issues, we'll be implicitly carrying-
	// forward info for a *series* of L0's -- but on an item-by-item basis
	// each item/issue will have a specific L0 -- that can be named by the
	// generic 'B' mnemonic.

	switch (nodeType)
	{
	case SG_DAGLCA_NODE_TYPE__LEAF:
		bIsBaseline = (strcmp(pszHid_CSet, pLoadCSetData->pMrg->pszHid_StartingBaseline) == 0);
		if (bIsBaseline)
		{
			SG_ERR_CHECK(  _load_B__using_wc(pCtx, pLoadCSetData->pMrg,
											 SG_MRG_ACCEPT_LABEL__B__MERGE)  );
		}
		else
		{
			// When MERGE is doing a normal 2-legged merge, we do not need an index on tne_L1.
			// And L1 is just for reference and therefore is constant and won't
			// have any changes, so it doesn't need a pc_L1 table.

			SG_ERR_CHECK(  _load_C__using_repo(pCtx, pLoadCSetData->pMrg,
											   SG_MRG_ACCEPT_LABEL__C__MERGE, pszHid_CSet,
											   SG_FALSE, SG_FALSE)  );
		}
		break;

	case SG_DAGLCA_NODE_TYPE__SPCA:
		break;

	case SG_DAGLCA_NODE_TYPE__LCA:
		SG_ERR_CHECK(  _load_A__using_repo(pCtx, pLoadCSetData->pMrg,
										   SG_MRG_ACCEPT_LABEL__A__MERGE, pszHid_CSet)  );
		break;

	default:
		SG_ASSERT( (0) );
		break;
	}

fail:
	SG_RBTREE_NULLFREE(pCtx, prbImmediateDescendants);
}

/**
 * Load the CSETs for each of the (ancestor, baseline, other).
 *
 */
void sg_wc_tx__merge__load_csets(SG_context * pCtx,
								 SG_mrg * pMrg)
{
	_load_cset_data load_cset_data;

	memset(&load_cset_data,0,sizeof(load_cset_data));
	load_cset_data.pMrg = pMrg;
	load_cset_data.kLeaves = 1;	// force required baseline to be L0 and label others starting with L1.

	// load the complete version control tree of all of the CSETs (leaves, SPCAs, and the LCA)
	// into the CSET-RBTREE.  This converts the treenodes and treenode-entries into sg_mrg_cset_entry's
	// and builds the flat entry-list in each CSET.

	SG_ERR_CHECK(  SG_daglca__foreach(pCtx,pMrg->pDagLca,SG_TRUE,_sg_mrg__daglca_callback__load_cset,&load_cset_data)  );

	SG_ASSERT(  (pMrg->pMrgCSet_LCA)  );
	SG_ASSERT(  (pMrg->pMrgCSet_Baseline)  );
	SG_ASSERT(  (pMrg->pMrgCSet_Other)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * This routine is used by UPDATE to fake a MERGE; that is, to do
 * a one-legged merge.  This routine must do set the same fields
 * in pMrg that the above routine does.
 *
 * I've isolated the effects here because UPDATE has already done
 * most of the homework/validation and we don't need to step thru
 * the stuff again here.
 *
 * 
 *
 * Given the Baseline cset (B),
 * the (possibly dirty) Working Directory (WD),
 * and the Goal/Target cset (T),
 * set things up to do:
 *
 *        B                   L0
 *       / \.     aka        /  \.
 *     WD   T           L0+PC    L1
 *       \ /                 \  /
 *        WD'               L0+PC' === L1+PC"
 *
 * That is, use the rules of MERGE to make the (possibly dirty)
 * WD a little more dirty by applying the changes made in L1
 * (relative to L0) to the WD.  We will later do a "change of basis"
 * (using DB tricks) to re-express the final state of the WD 
 * relative to L1 rather than L0.
 * 
 *
 * By construction there cannot be any SPCAs.   
 *
 *
 * That is, we are going to LIE to MERGE and:
 * [1] claim that the virgin baseline is the ancestor and
 *     load it __using_repo() into pMrg->pMrgCSet_LCA.
 * [2] use the same baseline+dirt (B+WC) that we use in
 *     the normal dirty merge case and load it __using_wc()
 *     into pMrg->pMrgCSet_Baseline.
 * [3] claim that the target changes as the other and load
 *     it __using_repo() into pMrg->pMrgCSet_Other.
 * 
 * Confused yet?
 *
 * 
 * We then trick MERGE to doing the normal merge.
 *
 */
void sg_wc_tx__fake_merge__update__load_csets(SG_context * pCtx,
											  SG_mrg * pMrg)
{
	// When MERGE is doing a normal 2-legged merge, we do not need an index on tne_L1.
	// When UPDATE is tricking us, the tne_L1 will become the baseline afterwards, so
	// we will want an index after the switch over; so we go ahead and create now.
	//
	// Likewise, in a normal merge, L1 is just for reference and is constant and won't
	// have any changes, so it doesn't need a pc_L1 table.
	// When UPDATE is tricking us, we will eventually switch the baseline to L1 and
	// we need a pc_L1 when it becomes the new baseline; so we create it too.

	SG_bool bCreateTneIndex = SG_TRUE;
	SG_bool bCreatePC = SG_TRUE;
	
	SG_ERR_CHECK(  _load_A__using_repo(pCtx, pMrg, SG_MRG_ACCEPT_LABEL__A__UPDATE, pMrg->pszHid_StartingBaseline)  );
	SG_ERR_CHECK(  _load_B__using_wc(  pCtx, pMrg, SG_MRG_ACCEPT_LABEL__B__UPDATE)  );
	SG_ERR_CHECK(  _load_C__using_repo(pCtx, pMrg, SG_MRG_ACCEPT_LABEL__C__UPDATE, pMrg->pszHidTarget, bCreateTneIndex, bCreatePC)  );

fail:
	return;
}

//////////////////////////////////////////////////////////////////

/**
 * This routine is used by REVERT-ALL to do a fake MERGE.
 *
 * Given the Baseline cset (B), and the (possibly dirty) Working Directory (WD),
 * set things up to do:
 *
 *      WD              L0+PC
 *     /  \            /     \
 *   WD    B      L0+PC       L0
 *     \  /            \     /
 *      B'              L0+PC'
 *
 * The result is B' -- which should equal B -- but may be slightly
 * different if there are conflicts/collisions with uncontrolled
 * items.
 *
 */
void sg_wc_tx__fake_merge__revert_all__load_csets(SG_context * pCtx,
												  SG_mrg * pMrg)
{
	// Do some contrived setup to build the above diamond.
	// 
	// Since the baseline (with or without dirt) appears in all 4 spots
	// in the diamond graph, we don't need to actually populate tne_L1 in SQL
	// and we don't actually want SQL to have A and L1 rows in tbl_csets after
	// we're done with the REVERT.
	//
	// But the MERGE code still needs to reference them as 3 different
	// inputs.  So we will just play some games with indirection.
	//////////////////////////////////////////////////////////////////
	//
	// L0 is already present in tbl_csets.
	// 
	// no need to add row A to tbl_csets.
	// L0 is already populated in {tne_L0,pc_L0}.
	// no need to add row L1 to tbl_csets.
	//
	// NOTE This means that pWcTX->pCSetRow_Other won't be set.
	//
	//////////////////////////////////////////////////////////////////
	//
	// TODO 2012/07/23 We should be able to share the graphs/rbtrees/vhashes
	// TODO            referenced *within* these 3 inputs so we don't have
	// TODO            to build them multiple times in memory.
	//
	//////////////////////////////////////////////////////////////////
	//
	// Tell MERGE that A is "L0+PC".
	// Tell MERGE that B is "L0+PC"

	SG_ERR_CHECK(  SG_MRG_CSET__ALLOC(pCtx, SG_MRG_MNEMONIC__A, "A", SG_MRG_ACCEPT_LABEL__A__REVERT,
									  SG_MRG_CSET_ORIGIN__LOADED, SG_DAGLCA_NODE_TYPE__LCA,
									  &pMrg->pMrgCSet_LCA)  );
	SG_ERR_CHECK(  SG_mrg_cset__load_entire_cset__using_wc(pCtx, pMrg,
														   pMrg->pMrgCSet_LCA,
														   pMrg->pszHid_StartingBaseline)  );

	SG_ERR_CHECK(  SG_MRG_CSET__ALLOC(pCtx, SG_MRG_MNEMONIC__B, "L0", SG_MRG_ACCEPT_LABEL__B__REVERT,
									  SG_MRG_CSET_ORIGIN__LOADED, SG_DAGLCA_NODE_TYPE__LEAF,
									  &pMrg->pMrgCSet_Baseline)  );
	SG_ERR_CHECK(  SG_mrg_cset__load_entire_cset__using_wc(pCtx, pMrg,
														   pMrg->pMrgCSet_Baseline,
														   pMrg->pszHid_StartingBaseline)  );
	
	SG_ERR_CHECK(  SG_MRG_CSET__ALLOC(pCtx, SG_MRG_MNEMONIC__C, "L1", SG_MRG_ACCEPT_LABEL__C__REVERT,
									  SG_MRG_CSET_ORIGIN__LOADED, SG_DAGLCA_NODE_TYPE__LEAF,
									  &pMrg->pMrgCSet_Other)  );
	SG_ERR_CHECK(  SG_mrg_cset__load_entire_cset__using_repo(pCtx, pMrg,
															 pMrg->pMrgCSet_Other,
															 pMrg->pszHid_StartingBaseline)  );

	// None of the static code above should have set this field.
	SG_ASSERT(  (pMrg->pWcTx->pCSetRow_Other == NULL)  );

fail:
	return;
}

